#include "simple_websocket_server.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <signal.h>

// 全局服务器实例，用于信号处理
SimpleWebSocketServer* g_server = nullptr;

void signalHandler(int signal) {
    if (g_server) {
        std::cout << "\nReceived signal " << signal << ", shutting down server..." << std::endl;
        g_server->stop();
    }
    exit(0);
}

int main() {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 创建WebSocket服务器，监听8080端口，使用4个工作线程
    SimpleWebSocketServer server(8080, 4);
    g_server = &server;
    
    // 设置消息处理回调
    server.setMessageHandler([&server](int client_id, const std::string& message) {
        std::cout << "Received message from client " << client_id << ": " << message << std::endl;
        
        // 回显消息给发送者
        server.sendMessageToClient(client_id, "Echo: " + message);
        
        // 如果消息是"broadcast"，则广播给所有客户端
        if (message == "broadcast") {
            server.broadcastMessage("Broadcast message from client " + std::to_string(client_id));
        }
        
        // 如果消息是"time"，发送当前时间
        if (message == "time") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string time_str = std::ctime(&time_t);
            time_str.pop_back(); // 移除换行符
            server.sendMessageToClient(client_id, "Current time: " + time_str);
        }
        
        // 如果消息是"hello"，发送欢迎消息
        if (message == "hello") {
            server.sendMessageToClient(client_id, "Hello! Welcome to the WebSocket server!");
        }
    });
    
    // 设置连接处理回调
    server.setConnectionHandler([&server](int client_id, const std::string& client_ip) {
        std::cout << "Client " << client_id << " connected from " << client_ip << std::endl;
        // 发送欢迎消息
        server.sendMessageToClient(client_id, "Welcome to WebSocket Server! Your client ID is " + std::to_string(client_id));
    });
    
    // 设置断开连接处理回调
    server.setDisconnectionHandler([](int client_id) {
        std::cout << "Client " << client_id << " disconnected" << std::endl;
    });
    
    // 启动服务器
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Simple WebSocket server is running on port 8080" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  - Send 'hello' to get a welcome message" << std::endl;
    std::cout << "  - Send 'broadcast' to broadcast a message to all clients" << std::endl;
    std::cout << "  - Send 'time' to get current server time" << std::endl;
    std::cout << "  - Press Ctrl+C to stop the server" << std::endl;
    std::cout << std::endl;
    std::cout << "Server commands:" << std::endl;
    std::cout << "  - Type 'status' to show server status" << std::endl;
    std::cout << "  - Type 'broadcast <message>' to broadcast to all clients" << std::endl;
    std::cout << "  - Type 'quit' or 'exit' to stop the server" << std::endl;
    std::cout << std::endl;
    
    // 主循环 - 保持服务器运行
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit" || input == "exit") {
            break;
        } else if (input == "status") {
            std::cout << "Server is running on port 8080" << std::endl;
        } else if (input.substr(0, 9) == "broadcast") {
            std::string message = input.length() > 10 ? input.substr(10) : "Server broadcast message";
            server.broadcastMessage(message);
            std::cout << "Broadcasted: " << message << std::endl;
        } else if (input == "help") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  status - Show server status" << std::endl;
            std::cout << "  broadcast <message> - Broadcast message to all clients" << std::endl;
            std::cout << "  quit/exit - Stop the server" << std::endl;
            std::cout << "  help - Show this help" << std::endl;
        } else if (!input.empty()) {
            std::cout << "Unknown command: " << input << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }
    
    // 停止服务器
    server.stop();
    std::cout << "Server stopped." << std::endl;
    
    return 0;
}
