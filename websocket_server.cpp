#include <openssl/sha.h>
#include "websocket_server.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <cstring>
#include <cerrno>

// WebSocketConnection 实现
WebSocketConnection::WebSocketConnection(int socket_fd, const std::string &client_ip)
    : socket_fd(socket_fd), client_ip(client_ip), connected(false)
{
    if (performHandshake())
    {
        connected = true;
        std::cout << "WebSocket handshake successful with " << client_ip << std::endl;
    }
    else
    {
        std::cout << "WebSocket handshake failed with " << client_ip << std::endl;
        ::close(socket_fd);
    }
}

WebSocketConnection::~WebSocketConnection()
{
    close();
}

void WebSocketConnection::close()
{
    if (connected.exchange(false))
    {
        ::close(socket_fd);
    }
}

bool WebSocketConnection::performHandshake()
{
    char buffer[4096];
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        return false;
    }

    buffer[bytes_received] = '\0';
    std::string request(buffer);

    // 解析WebSocket握手请求
    std::regex key_regex("Sec-WebSocket-Key: ([^\r\n]+)");
    std::smatch match;

    if (!std::regex_search(request, match, key_regex))
    {
        return false;
    }

    std::string websocket_key = match[1].str();
    std::string accept_key = generateAcceptKey(websocket_key);

    // 构建响应
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << accept_key << "\r\n"
             << "\r\n";

    std::string response_str = response.str();
    return send(socket_fd, response_str.c_str(), response_str.length(), 0) > 0;
}

std::string WebSocketConnection::generateAcceptKey(const std::string &key)
{
    std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic_string;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(combined.c_str()), combined.length(), hash);

    std::vector<uint8_t> hash_vec(hash, hash + SHA_DIGEST_LENGTH);
    return base64Encode(hash_vec);
}

std::string WebSocketConnection::base64Encode(const std::vector<uint8_t> &input)
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), input.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return result;
}

bool WebSocketConnection::sendMessage(const std::string &message)
{
    if (!connected)
        return false;

    std::lock_guard<std::mutex> lock(send_mutex);
    std::string frame = encodeFrame(message);

    int bytes_sent = send(socket_fd, frame.c_str(), frame.length(), 0);
    return bytes_sent > 0;
}

std::string WebSocketConnection::receiveMessage()
{
    if (!connected)
        return "";

    std::vector<uint8_t> buffer(4096);
    int bytes_received = recv(socket_fd, buffer.data(), buffer.size(), 0);

    if (bytes_received <= 0)
    {
        connected = false;
        return "";
    }

    buffer.resize(bytes_received);
    return decodeFrame(buffer);
}

std::string WebSocketConnection::encodeFrame(const std::string &payload)
{
    std::vector<uint8_t> frame;

    // FIN=1, RSV=000, Opcode=0001 (text frame)
    frame.push_back(0x81);

    size_t payload_length = payload.length();
    if (payload_length < 126)
    {
        frame.push_back(static_cast<uint8_t>(payload_length));
    }
    else if (payload_length < 65536)
    {
        frame.push_back(126);
        frame.push_back((payload_length >> 8) & 0xFF);
        frame.push_back(payload_length & 0xFF);
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--)
        {
            frame.push_back((payload_length >> (i * 8)) & 0xFF);
        }
    }

    // 添加payload
    frame.insert(frame.end(), payload.begin(), payload.end());

    return std::string(frame.begin(), frame.end());
}

std::string WebSocketConnection::decodeFrame(const std::vector<uint8_t> &frame)
{
    if (frame.size() < 2)
        return "";

    uint8_t opcode = frame[0] & 0x0F;
    if (opcode == 0x8)
    { // Close frame
        connected = false;
        return "";
    }

    bool masked = (frame[1] & 0x80) != 0;
    uint64_t payload_length = frame[1] & 0x7F;

    size_t header_size = 2;
    if (payload_length == 126)
    {
        if (frame.size() < 4)
            return "";
        payload_length = (frame[2] << 8) | frame[3];
        header_size = 4;
    }
    else if (payload_length == 127)
    {
        if (frame.size() < 10)
            return "";
        payload_length = 0;
        for (int i = 0; i < 8; i++)
        {
            payload_length = (payload_length << 8) | frame[2 + i];
        }
        header_size = 10;
    }

    if (masked)
    {
        header_size += 4; // mask key
    }

    if (frame.size() < header_size + payload_length)
        return "";

    std::string payload;
    if (masked)
    {
        uint8_t mask[4] = {frame[header_size - 4], frame[header_size - 3], frame[header_size - 2], frame[header_size - 1]};
        for (uint64_t i = 0; i < payload_length; i++)
        {
            payload += static_cast<char>(frame[header_size + i] ^ mask[i % 4]);
        }
    }
    else
    {
        payload = std::string(frame.begin() + header_size, frame.begin() + header_size + payload_length);
    }

    return payload;
}

// WebSocketServer 实现
WebSocketServer::WebSocketServer(int port, size_t thread_pool_size)
    : port(port), server_socket(-1), running(false), thread_pool_size(thread_pool_size), next_client_id(1)
{
    thread_pool.reset(new ThreadPool(thread_pool_size));
}

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start()
{
    if (running)
        return false;

    if (!setupSocket())
    {
        std::cerr << "Failed to setup socket" << std::endl;
        return false;
    }

    running = true;

    std::thread([this]
                {
        struct epoll_event ev, events[1024];
        int epoll_fd = epoll_create(1);
        if(epoll_fd < 0) {
            std::cerr << "Failed to create epoll instance" << std::endl;
            running = false;
            return;
        }

        ev.events = EPOLLIN;
        ev.data.fd = server_socket;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) < 0) {
            std::cerr << "Failed to add server socket to epoll" << std::endl;
            running = false;
            return;
        }

        // 用于存储socket fd到client id的映射
        std::map<int, int> socket_to_client_id;

        while(running) {
            int n = epoll_wait(epoll_fd, events, 1024, 1000); // 1秒超时
            if(n < 0) {
                if(errno == EINTR) continue; // 被信号中断，继续
                std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
                break;
            }

            // 定期检查并清理断开的连接
            if(n == 0) { // 超时，进行清理检查
                std::vector<int> disconnected_sockets;
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    for(const auto& pair : socket_to_client_id) {
                        int sock_fd = pair.first;
                        int client_id = pair.second;
                        auto client_it = clients.find(client_id);
                        if(client_it != clients.end() && !client_it->second->isConnected()) {
                            disconnected_sockets.push_back(sock_fd);
                        }
                    }
                }

                // 清理断开的连接
                for(int sock_fd : disconnected_sockets) {
                    auto it = socket_to_client_id.find(sock_fd);
                    if(it != socket_to_client_id.end()) {
                        int client_id = it->second;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
                        socket_to_client_id.erase(it);
                        removeClient(client_id);
                        if(disconnection_handler) {
                            disconnection_handler(client_id);
                        }
                        std::cout << "Cleaned up disconnected client " << client_id << std::endl;
                    }
                }
            }

            for(int i = 0; i < n; i++) {
                if(events[i].data.fd == server_socket) {
                    // 处理新连接
                    auto connection = acceptConnections();
                    if(connection && connection->isConnected()) {
                        // 将客户端socket添加到epoll监听
                        struct epoll_event client_ev;
                        client_ev.events = EPOLLIN | EPOLLET; // 边缘触发
                        client_ev.data.fd = connection->getSocketFd();

                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection->getSocketFd(), &client_ev) == 0) {
                            // 找到对应的client_id
                            int client_id = -1;
                            {
                                std::lock_guard<std::mutex> lock(clients_mutex);
                                for(const auto& pair : clients) {
                                    if(pair.second == connection) {
                                        client_id = pair.first;
                                        break;
                                    }
                                }
                            }
                            if(client_id != -1) {
                                socket_to_client_id[connection->getSocketFd()] = client_id;
                                // std::cout << "Added client socket " << connection->getSocketFd()
                                //          << " (client_id: " << client_id << ") to epoll" << std::endl;
                            }
                        } else {
                            std::cerr << "Failed to add client socket to epoll: " << strerror(errno) << std::endl;
                        }
                    }
                } else {
                    // 处理客户端消息
                    int client_socket = events[i].data.fd;
                    auto it = socket_to_client_id.find(client_socket);
                    if(it != socket_to_client_id.end()) {
                        int client_id = it->second;

                        // 获取对应的连接对象
                        std::shared_ptr<WebSocketConnection> connection;
                        {
                            std::lock_guard<std::mutex> lock(clients_mutex);
                            auto client_it = clients.find(client_id);
                            if(client_it != clients.end()) {
                                connection = client_it->second;
                            }
                        }

                        if(connection && connection->isConnected()) {
                            // 检查是否有空闲线程，如果没有则等待
                            if(thread_pool->getAvailableThreads() == 0) {
                                std::cout << "No available threads, waiting..." << std::endl;
                                thread_pool->waitForAvailableThread();
                            }

                            // 在线程池中处理消息
                            thread_pool->enqueue([this, connection, client_id, client_socket, &socket_to_client_id, epoll_fd] {
                                // std::cout << "Thread processing message for client " << client_id << std::endl;

                                std::string message = connection->receiveMessage();
                                if(!message.empty()) {
                                    if(message_handler) {
                                        message_handler(client_id, message);
                                    }
                                } else {
                                    // 连接断开，需要在主线程中处理epoll清理
                                    // 这里我们只标记连接为断开，实际清理在epoll线程中进行
                                    connection->close();
                                }

                                // std::cout << "Thread finished processing message for client " << client_id
                                //          << ", returning to pool" << std::endl;
                            });
                        } else {
                            // 连接已断开，清理
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
                            socket_to_client_id.erase(client_socket);
                            removeClient(client_id);
                            if(disconnection_handler) {
                                disconnection_handler(client_id);
                            }
                        }
                    }
                }
            }
        }

        // 清理epoll
        ::close(epoll_fd); })
        .detach();

    std::cout << "WebSocket server started on port " << port << std::endl;
    return true;
}

void WebSocketServer::stop()
{
    if (!running)
        return;

    running = false;

    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto &pair : clients)
        {
            pair.second->close();
        }
        clients.clear();
    }

    // 关闭服务器socket
    if (server_socket != -1)
    {
        ::close(server_socket);
        server_socket = -1;
    }

    std::cout << "WebSocket server stopped" << std::endl;
}

bool WebSocketServer::setupSocket()
{
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options" << std::endl;
        ::close(server_socket);
        return false;
    }

    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
        ::close(server_socket);
        return false;
    }

    // 开始监听
    if (listen(server_socket, 10) < 0)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        ::close(server_socket);
        return false;
    }

    return true;
}

std::shared_ptr<WebSocketConnection> WebSocketServer::acceptConnections()
{
    while (running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (running)
            {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_id = next_client_id++;

        // 创建WebSocket连接
        auto connection = std::make_shared<WebSocketConnection>(client_socket, client_ip);

        if (connection->isConnected())
        {
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients[client_id] = connection;
            }

            // 触发连接事件
            if (connection_handler)
            {
                connection_handler(client_id, client_ip);
            }

            // 客户端处理现在由epoll线程负责

            return connection;
        }
    }

    // 如果服务器停止运行，返回空指针
    return nullptr;
}

void WebSocketServer::handleClient(std::shared_ptr<WebSocketConnection> connection, int client_id)
{
    while (connection->isConnected() && running)
    {
        std::string message = connection->receiveMessage();

        if (!message.empty())
        {
            // 触发消息处理事件
            if (message_handler)
            {
                message_handler(client_id, message);
            }
        }
        else
        {
            // 连接断开
            break;
        }
    }

    // 清理客户端连接
    removeClient(client_id);

    // 触发断开连接事件
    if (disconnection_handler)
    {
        disconnection_handler(client_id);
    }
}

void WebSocketServer::removeClient(int client_id)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end())
    {
        it->second->close();
        clients.erase(it);
    }
}

void WebSocketServer::broadcastMessage(const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto &pair : clients)
    {
        if (pair.second->isConnected())
        {
            pair.second->sendMessage(message);
        }
    }
}

void WebSocketServer::sendMessageToClient(int client_id, const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end() && it->second->isConnected())
    {
        it->second->sendMessage(message);
    }
}

void WebSocketServer::setMessageHandler(std::function<void(int, const std::string &)> handler)
{
    message_handler = handler;
}

void WebSocketServer::setConnectionHandler(std::function<void(int, const std::string &)> handler)
{
    connection_handler = handler;
}

void WebSocketServer::setDisconnectionHandler(std::function<void(int)> handler)
{
    disconnection_handler = handler;
}

// 服务器状态查询方法实现
size_t WebSocketServer::getClientCount() const
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients.size();
}

size_t WebSocketServer::getThreadPoolSize() const
{
    return thread_pool_size;
}

size_t WebSocketServer::getAvailableThreads() const
{
    return thread_pool ? thread_pool->getAvailableThreads() : 0;
}

std::vector<std::pair<int, std::string>> WebSocketServer::getConnectedClients() const
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::vector<std::pair<int, std::string>> result;

    for (const auto &pair : clients)
    {
        if (pair.second && pair.second->isConnected())
        {
            result.emplace_back(pair.first, pair.second->getClientIP());
        }
    }

    return result;
}

bool WebSocketServer::disconnectClient(int client_id)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(client_id);

    if (it != clients.end() && it->second->isConnected())
    {
        it->second->close();
        clients.erase(it);

        // 触发断开连接事件
        if (disconnection_handler)
        {
            disconnection_handler(client_id);
        }

        return true;
    }

    return false;
}

bool WebSocketServer::isClientExists(int client_id) const
{
    return clients.find(client_id) != clients.end();
}
