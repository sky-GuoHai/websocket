#include "simple_websocket_server.h"

// SimpleSHA1 实现
std::string SimpleSHA1::hash(const std::string& input) {
    // SHA1 初始值
    uint32_t h[5] = {
        0x67452301,
        0xEFCDAB89,
        0x98BADCFE,
        0x10325476,
        0xC3D2E1F0
    };
    
    // 准备消息
    std::vector<uint8_t> message(input.begin(), input.end());
    uint64_t original_length = message.size() * 8;
    
    // 添加填充
    message.push_back(0x80);
    while ((message.size() % 64) != 56) {
        message.push_back(0x00);
    }
    
    // 添加长度
    for (int i = 7; i >= 0; i--) {
        message.push_back((original_length >> (i * 8)) & 0xFF);
    }
    
    // 处理每个512位块
    for (size_t i = 0; i < message.size(); i += 64) {
        processChunk(&message[i], h);
    }
    
    // 生成最终哈希
    std::vector<uint8_t> hash_bytes;
    for (int i = 0; i < 5; i++) {
        for (int j = 3; j >= 0; j--) {
            hash_bytes.push_back((h[i] >> (j * 8)) & 0xFF);
        }
    }
    
    return base64Encode(hash_bytes);
}

uint32_t SimpleSHA1::leftRotate(uint32_t value, int amount) {
    return (value << amount) | (value >> (32 - amount));
}

void SimpleSHA1::processChunk(const uint8_t* chunk, uint32_t* h) {
    uint32_t w[80];
    
    // 复制块到w[0..15]
    for (int i = 0; i < 16; i++) {
        w[i] = (chunk[i*4] << 24) | (chunk[i*4+1] << 16) | (chunk[i*4+2] << 8) | chunk[i*4+3];
    }
    
    // 扩展到w[16..79]
    for (int i = 16; i < 80; i++) {
        w[i] = leftRotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    // 初始化哈希值
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    
    // 主循环
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        
        uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = leftRotate(b, 30);
        b = a;
        a = temp;
    }
    
    // 添加到哈希值
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

std::string SimpleSHA1::base64Encode(const std::vector<uint8_t>& input) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t value = 0;
        int count = 0;
        
        for (int j = 0; j < 3 && i + j < input.size(); j++) {
            value = (value << 8) | input[i + j];
            count++;
        }
        
        value <<= (3 - count) * 8;
        
        for (int j = 0; j < 4; j++) {
            if (j <= count) {
                result += chars[(value >> (18 - j * 6)) & 0x3F];
            } else {
                result += '=';
            }
        }
    }
    
    return result;
}

// SimpleWebSocketConnection 实现
SimpleWebSocketConnection::SimpleWebSocketConnection(int socket_fd, const std::string& client_ip)
    : socket_fd(socket_fd), client_ip(client_ip), connected(false) {
    if (performHandshake()) {
        connected = true;
        std::cout << "WebSocket handshake successful with " << client_ip << std::endl;
    } else {
        std::cout << "WebSocket handshake failed with " << client_ip << std::endl;
#ifdef __linux__
        ::close(socket_fd);
#else
        closesocket(socket_fd);
#endif
    }
}

SimpleWebSocketConnection::~SimpleWebSocketConnection() {
    close();
}

void SimpleWebSocketConnection::close() {
    if (connected.exchange(false)) {
#ifdef __linux__
        ::close(socket_fd);
#else
        closesocket(socket_fd);
#endif
    }
}

bool SimpleWebSocketConnection::performHandshake() {
    char buffer[4096];
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        return false;
    }
    
    buffer[bytes_received] = '\0';
    std::string request(buffer);
    
    // 解析WebSocket握手请求
    std::regex key_regex("Sec-WebSocket-Key: ([^\r\n]+)");
    std::smatch match;
    
    if (!std::regex_search(request, match, key_regex)) {
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

std::string SimpleWebSocketConnection::generateAcceptKey(const std::string& key) {
    std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic_string;
    return SimpleSHA1::hash(combined);
}

bool SimpleWebSocketConnection::sendMessage(const std::string& message) {
    if (!connected) return false;

    std::lock_guard<std::mutex> lock(send_mutex);
    std::string frame = encodeFrame(message);

    int bytes_sent = send(socket_fd, frame.c_str(), frame.length(), 0);
    return bytes_sent > 0;
}

std::string SimpleWebSocketConnection::receiveMessage() {
    if (!connected) return "";

    std::vector<uint8_t> buffer(4096);
    int bytes_received = recv(socket_fd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);

    if (bytes_received <= 0) {
        connected = false;
        return "";
    }

    buffer.resize(bytes_received);
    return decodeFrame(buffer);
}

std::string SimpleWebSocketConnection::encodeFrame(const std::string& payload) {
    std::vector<uint8_t> frame;

    // FIN=1, RSV=000, Opcode=0001 (text frame)
    frame.push_back(0x81);

    size_t payload_length = payload.length();
    if (payload_length < 126) {
        frame.push_back(static_cast<uint8_t>(payload_length));
    } else if (payload_length < 65536) {
        frame.push_back(126);
        frame.push_back((payload_length >> 8) & 0xFF);
        frame.push_back(payload_length & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((payload_length >> (i * 8)) & 0xFF);
        }
    }

    // 添加payload
    frame.insert(frame.end(), payload.begin(), payload.end());

    return std::string(frame.begin(), frame.end());
}

std::string SimpleWebSocketConnection::decodeFrame(const std::vector<uint8_t>& frame) {
    if (frame.size() < 2) return "";

    uint8_t opcode = frame[0] & 0x0F;
    if (opcode == 0x8) { // Close frame
        connected = false;
        return "";
    }

    bool masked = (frame[1] & 0x80) != 0;
    uint64_t payload_length = frame[1] & 0x7F;

    size_t header_size = 2;
    if (payload_length == 126) {
        if (frame.size() < 4) return "";
        payload_length = (frame[2] << 8) | frame[3];
        header_size = 4;
    } else if (payload_length == 127) {
        if (frame.size() < 10) return "";
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | frame[2 + i];
        }
        header_size = 10;
    }

    if (masked) {
        header_size += 4; // mask key
    }

    if (frame.size() < header_size + payload_length) return "";

    std::string payload;
    if (masked) {
        uint8_t mask[4] = {frame[header_size-4], frame[header_size-3], frame[header_size-2], frame[header_size-1]};
        for (uint64_t i = 0; i < payload_length; i++) {
            payload += static_cast<char>(frame[header_size + i] ^ mask[i % 4]);
        }
    } else {
        payload = std::string(frame.begin() + header_size, frame.begin() + header_size + payload_length);
    }

    return payload;
}

// SimpleWebSocketServer 实现
SimpleWebSocketServer::SimpleWebSocketServer(int port, size_t thread_pool_size)
    : port(port), server_socket(-1), running(false), next_client_id(1) {
    thread_pool = new ThreadPool(thread_pool_size);
}

SimpleWebSocketServer::~SimpleWebSocketServer() {
    stop();
    delete thread_pool;
}

bool SimpleWebSocketServer::start() {
    if (running) return false;

    if (!setupSocket()) {
        std::cerr << "Failed to setup socket" << std::endl;
        return false;
    }

    running = true;

    // 在线程池中启动接受连接的任务
    thread_pool->enqueue([this] { acceptConnections(); });

    std::cout << "WebSocket server started on port " << port << std::endl;
    return true;
}

void SimpleWebSocketServer::stop() {
    if (!running) return;

    running = false;

    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto& pair : clients) {
            pair.second->close();
        }
        clients.clear();
    }

    // 关闭服务器socket
    if (server_socket != -1) {
#ifdef __linux__
        ::close(server_socket);
#else
        closesocket(server_socket);
#endif
        server_socket = -1;
    }

    std::cout << "WebSocket server stopped" << std::endl;
}

bool SimpleWebSocketServer::setupSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
#ifdef __linux__
        ::close(server_socket);
#else
        closesocket(server_socket);
#endif
        return false;
    }

    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
#ifdef __linux__
        ::close(server_socket);
#else
        closesocket(server_socket);
#endif
        return false;
    }

    // 开始监听
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
#ifdef __linux__
        ::close(server_socket);
#else
        closesocket(server_socket);
#endif
        return false;
    }

    return true;
}

void SimpleWebSocketServer::acceptConnections() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_id = next_client_id++;

        // 创建WebSocket连接
        auto connection = std::shared_ptr<SimpleWebSocketConnection>(new SimpleWebSocketConnection(client_socket, client_ip));

        if (connection->isConnected()) {
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients[client_id] = connection;
            }

            // 触发连接事件
            if (connection_handler) {
                connection_handler(client_id, client_ip);
            }

            // 在线程池中处理客户端
            thread_pool->enqueue([this, connection, client_id] {
                handleClient(connection, client_id);
            });
        }
    }
}

void SimpleWebSocketServer::handleClient(std::shared_ptr<SimpleWebSocketConnection> connection, int client_id) {
    while (connection->isConnected() && running) {
        std::string message = connection->receiveMessage();

        if (!message.empty()) {
            // 触发消息处理事件
            if (message_handler) {
                message_handler(client_id, message);
            }
        } else {
            // 连接断开
            break;
        }
    }

    // 清理客户端连接
    removeClient(client_id);

    // 触发断开连接事件
    if (disconnection_handler) {
        disconnection_handler(client_id);
    }
}

void SimpleWebSocketServer::removeClient(int client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end()) {
        it->second->close();
        clients.erase(it);
    }
}

void SimpleWebSocketServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& pair : clients) {
        if (pair.second->isConnected()) {
            pair.second->sendMessage(message);
        }
    }
}

void SimpleWebSocketServer::sendMessageToClient(int client_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end() && it->second->isConnected()) {
        it->second->sendMessage(message);
    }
}

void SimpleWebSocketServer::setMessageHandler(std::function<void(int, const std::string&)> handler) {
    message_handler = handler;
}

void SimpleWebSocketServer::setConnectionHandler(std::function<void(int, const std::string&)> handler) {
    connection_handler = handler;
}

void SimpleWebSocketServer::setDisconnectionHandler(std::function<void(int)> handler) {
    disconnection_handler = handler;
}
