#ifndef SIMPLE_WEBSOCKET_SERVER_H
#define SIMPLE_WEBSOCKET_SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <sstream>
#include <iomanip>
#include <regex>
#include <cstring>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "thread_pool.h"

// 简单的SHA1实现（不依赖OpenSSL）
class SimpleSHA1 {
public:
    static std::string hash(const std::string& input);
    static std::string base64Encode(const std::vector<uint8_t>& input);

private:
    static uint32_t leftRotate(uint32_t value, int amount);
    static void processChunk(const uint8_t* chunk, uint32_t* h);
};

class SimpleWebSocketConnection {
public:
    SimpleWebSocketConnection(int socket_fd, const std::string& client_ip);
    ~SimpleWebSocketConnection();
    
    bool sendMessage(const std::string& message);
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
    
    std::string encodeFrame(const std::string& payload);
    std::string decodeFrame(const std::vector<uint8_t>& frame);
    bool performHandshake();
    std::string generateAcceptKey(const std::string& key);
};

class SimpleWebSocketServer {
public:
    SimpleWebSocketServer(int port, size_t thread_pool_size = 4);
    ~SimpleWebSocketServer();
    
    bool start();
    void stop();
    void broadcastMessage(const std::string& message);
    void sendMessageToClient(int client_id, const std::string& message);
    
    // 设置消息处理回调
    void setMessageHandler(std::function<void(int, const std::string&)> handler);
    void setConnectionHandler(std::function<void(int, const std::string&)> handler);
    void setDisconnectionHandler(std::function<void(int)> handler);

private:
    int port;
    int server_socket;
    std::atomic<bool> running;
    ThreadPool* thread_pool;
    
    // 客户端连接管理
    std::map<int, std::shared_ptr<SimpleWebSocketConnection>> clients;
    std::mutex clients_mutex;
    std::atomic<int> next_client_id;
    
    // 事件处理回调
    std::function<void(int, const std::string&)> message_handler;
    std::function<void(int, const std::string&)> connection_handler;
    std::function<void(int)> disconnection_handler;
    
    void acceptConnections();
    void handleClient(std::shared_ptr<SimpleWebSocketConnection> connection, int client_id);
    void removeClient(int client_id);
    bool setupSocket();
};

#endif
