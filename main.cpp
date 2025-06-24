#include "websocket_server.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sstream>

// 全局服务器实例，用于信号处理
WebSocketServer *g_server = nullptr;

void signalHandler(int signal)
{
    if (g_server)
    {
        std::cout << "\nReceived signal " << signal << ", shutting down server..." << std::endl;
        g_server->stop();
    }
    exit(0);
}

int main()
{
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 创建WebSocket服务器，监听8080端口，使用4个工作线程
    WebSocketServer server(8080, 4);
    g_server = &server;

    // 设置消息处理回调
    server.setMessageHandler([&server](int client_id, const std::string &message)
                             {
        std::cout << "Received message from client " << client_id << ": " << message << std::endl;

        // 回显消息给发送者
        server.sendMessageToClient(client_id, "Echo: " + message);

        // 如果消息是"broadcast"，则广播给所有客户端
        if (message == "broadcast")
        {
            server.broadcastMessage("Broadcast message from client " + std::to_string(client_id));
        }

        // 如果消息是"time"，发送当前时间
        if (message == "time")
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string time_str = std::ctime(&time_t);
            time_str.pop_back(); // 移除换行符
            server.sendMessageToClient(client_id, "Current time: " + time_str);
        } });

    // 设置连接处理回调
    server.setConnectionHandler([](int client_id, const std::string &client_ip)
                                { std::cout << "Client " << client_id << " connected from " << client_ip << std::endl; });

    // 设置断开连接处理回调
    server.setDisconnectionHandler([](int client_id)
                                   { std::cout << "Client " << client_id << " disconnected" << std::endl; });

    // 启动服务器
    if (!server.start())
    {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Type 'help' for available commands" << std::endl;
    std::cout << std::endl;

    // 主循环 - 保持服务器运行
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "quit" || input == "exit")
        {
            break;
        }
        else if (input == "status")
        {
            std::cout << "\n=== Server Status ===" << std::endl;
            std::cout << "Port: 8080" << std::endl;
            std::cout << "Connected clients: " << server.getClientCount() << std::endl;
            std::cout << "Thread pool size: " << server.getThreadPoolSize() << std::endl;
            std::cout << "Available threads: " << server.getAvailableThreads() << std::endl;
            std::cout << "Server running: " << (server.isRunning() ? "Yes" : "No") << std::endl;
            std::cout << "====================" << std::endl;
        }
        else if (input.substr(0, 9) == "broadcast")
        {
            std::string message = input.length() > 10 ? input.substr(10) : "Server broadcast message";
            server.broadcastMessage(message);
            std::cout << "Broadcasted: " << message << std::endl;
        }
        else if (input == "time")
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string time_str = std::ctime(&time_t);
            time_str.pop_back(); // 移除换行符
            std::cout << "Current time: " << time_str << std::endl;
        }
        else if (input.substr(0, 4) == "send")
        {
            // 解析命令格式: send <client_id> <message>
            std::istringstream iss(input);
            std::string command, client_id_str, message;

            if (!(iss >> command >> client_id_str))
            {
                std::cout << "Error: Invalid command format. Usage: send <client_id> <message>" << std::endl;
                continue;
            }

            // 获取剩余的消息内容（可能包含空格）
            std::getline(iss, message);
            if (!message.empty() && message[0] == ' ')
            {
                message = message.substr(1); // 移除开头的空格
            }

            if (message.empty())
            {
                std::cout << "Error: Message cannot be empty" << std::endl;
                continue;
            }

            // 验证client_id是否为有效数字
            try
            {
                int client_id = std::stoi(client_id_str);
                if (client_id <= 0)
                {
                    std::cout << "Error: Client ID must be a positive number" << std::endl;
                    continue;
                }

                // 检查client_id是否在服务器中
                if (!server.isClientExists(client_id))
                {
                    std::cout << "Error: Client " << client_id << " does not exist" << std::endl;
                    continue;
                }

                std::cout << "Sending message to client " << client_id << ": " << message << std::endl;
                server.sendMessageToClient(client_id, message);
            }
            catch (const std::invalid_argument &e)
            {
                std::cout << "Error: Invalid client ID '" << client_id_str << "'. Must be a number." << std::endl;
            }
            catch (const std::out_of_range &e)
            {
                std::cout << "Error: Client ID '" << client_id_str << "' is out of range." << std::endl;
            }
        }
        else if (input == "help")
        {
            std::cout << "\n=== WebSocket Server Commands ===" << std::endl;
            std::cout << "  broadcast <message>        - Send message to all connected clients" << std::endl;
            std::cout << "  send <client_id> <message> - Send message to specific client" << std::endl;
            std::cout << "  list                       - List all connected clients" << std::endl;
            std::cout << "  status                     - Show server status" << std::endl;
            std::cout << "  time                       - Show current server time" << std::endl;
            std::cout << "  help                       - Show this help message" << std::endl;
            std::cout << "  quit/exit                  - Stop the server and exit" << std::endl;
            std::cout << "=================================" << std::endl;
        }
        else if (input == "list")
        {
            std::cout << "Connected clients: " << server.getClientCount() << std::endl;
            auto clients = server.getConnectedClients();
            if (clients.empty())
            {
                std::cout << "  No clients connected" << std::endl;
            }
            else
            {
                for (const auto &client : clients)
                {
                    std::cout << "  Client " << client.first << " (" << client.second << ")" << std::endl;
                }
            }
        }
        else if (!input.empty())
        {
            std::cout << "Unknown command: " << input << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }

    // 停止服务器
    server.stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}
