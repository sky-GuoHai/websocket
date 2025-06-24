#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#else
// Windows compatibility headers would go here
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include "thread_pool.h"

class WebSocketConnection
{
public:
    WebSocketConnection(int socket_fd, const std::string &client_ip);
    ~WebSocketConnection();

    bool sendMessage(const std::string &message);
    std::string receiveMessage();
    bool isConnected() const { return connected; }
    int getSocketFd() const { return socket_fd; }
    std::string getClientIP() const { return client_ip; }
    void close();

private:
    int socket_fd;
    std::string client_ip;
    std::atomic<bool> connected;
    std::mutex send_mutex;

    std::string encodeFrame(const std::string &payload);
    std::string decodeFrame(const std::vector<uint8_t> &frame);
    bool performHandshake();
    std::string generateAcceptKey(const std::string &key);
    std::string base64Encode(const std::vector<uint8_t> &input);
};

class WebSocketServer
{
public:
    WebSocketServer(int port, size_t thread_pool_size = 4);
    ~WebSocketServer();

    bool start();
    void stop();
    void broadcastMessage(const std::string &message);
    void sendMessageToClient(int client_id, const std::string &message);

    // 服务器状态查询
    bool isRunning() const { return running; }
    size_t getClientCount() const;
    size_t getThreadPoolSize() const;
    size_t getAvailableThreads() const;
    std::vector<std::pair<int, std::string>> getConnectedClients() const;
    bool disconnectClient(int client_id);
    bool isClientExists(int client_id) const;

    // 设置消息处理回调
    void setMessageHandler(std::function<void(int, const std::string &)> handler);
    void setConnectionHandler(std::function<void(int, const std::string &)> handler);
    void setDisconnectionHandler(std::function<void(int)> handler);

private:
    int port;
    int server_socket;
    std::atomic<bool> running;
    std::unique_ptr<ThreadPool> thread_pool;
    size_t thread_pool_size;

    // 客户端连接管理
    std::map<int, std::shared_ptr<WebSocketConnection>> clients;
    mutable std::mutex clients_mutex;
    std::atomic<int> next_client_id;

    // 事件处理回调
    std::function<void(int, const std::string &)> message_handler;
    std::function<void(int, const std::string &)> connection_handler;
    std::function<void(int)> disconnection_handler;

    std::shared_ptr<WebSocketConnection> acceptConnections();
    void handleClient(std::shared_ptr<WebSocketConnection> connection, int client_id);
    void removeClient(int client_id);
    bool setupSocket();
};

#endif
