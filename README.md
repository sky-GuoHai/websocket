# C++ WebSocket Server with Thread Pool

这是一个基于C++实现的WebSocket服务器，支持多客户端连接、双向通信和线程池任务执行。

## 功能特性

- ✅ WebSocket协议支持（RFC 6455）
- ✅ 多客户端并发连接
- ✅ 线程池任务执行
- ✅ 双向消息通信
- ✅ 连接状态管理
- ✅ 事件回调机制
- ✅ 广播消息功能
- ✅ 单点消息发送

## 系统要求

- Linux操作系统
- GCC 4.8+ (支持C++11)
- OpenSSL开发库

## 安装依赖

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

### CentOS/RHEL
```bash
sudo yum install gcc-c++ openssl-devel
```

### 或者使用Makefile安装依赖
```bash
make install-deps
```

## 编译和运行

### 方式一：使用简化版本（推荐，无OpenSSL依赖）
```bash
# 使用简化版Makefile
make -f Makefile.simple

# 或者使用编译脚本
chmod +x compile_simple.sh
./compile_simple.sh

# 运行简化版服务器
./simple_websocket_server
```

### 方式二：使用完整版本（需要OpenSSL）
```bash
# 编译完整版
make

# 运行完整版服务器
make run
# 或者直接运行
./websocket_server
```

### 清理编译文件
```bash
make clean
# 或者清理简化版
make -f Makefile.simple clean
```

### 编译调试版本
```bash
make debug
# 或者简化版调试
make -f Makefile.simple debug
```

## 使用方法

### 1. 启动服务器
```bash
./websocket_server
```
服务器将在端口8080上启动并等待客户端连接。

### 2. 测试连接
打开 `test_client.html` 文件在浏览器中测试WebSocket连接。

### 3. 服务器命令
服务器运行时支持以下命令：
- `status` - 显示服务器状态
- `broadcast <message>` - 向所有客户端广播消息
- `quit` 或 `exit` - 停止服务器

### 4. 客户端测试命令
客户端可以发送以下特殊命令：
- `time` - 获取服务器当前时间
- `broadcast` - 触发服务器广播消息

## 代码结构

```
├── thread_pool.h          # 线程池头文件
├── websocket_server.h     # WebSocket服务器头文件
├── websocket_server.cpp   # WebSocket服务器实现
├── main.cpp              # 主程序和示例
├── test_client.html      # HTML测试客户端
├── Makefile             # 编译脚本
└── README.md            # 说明文档
```

## API 使用示例

### 基本服务器设置
```cpp
#include "websocket_server.h"

// 创建服务器实例
WebSocketServer server(8080, 4); // 端口8080，4个工作线程

// 设置消息处理回调
server.setMessageHandler([&server](int client_id, const std::string& message) {
    std::cout << "Client " << client_id << ": " << message << std::endl;
    // 回显消息
    server.sendMessageToClient(client_id, "Echo: " + message);
});

// 设置连接事件回调
server.setConnectionHandler([](int client_id, const std::string& client_ip) {
    std::cout << "Client " << client_id << " connected from " << client_ip << std::endl;
});

// 设置断开连接事件回调
server.setDisconnectionHandler([](int client_id) {
    std::cout << "Client " << client_id << " disconnected" << std::endl;
});

// 启动服务器
server.start();
```

### 发送消息
```cpp
// 发送消息给特定客户端
server.sendMessageToClient(client_id, "Hello Client!");

// 广播消息给所有客户端
server.broadcastMessage("Hello Everyone!");
```

## 线程池配置

线程池大小可以在创建服务器时配置：
```cpp
WebSocketServer server(8080, 8); // 使用8个工作线程
```

建议线程数量设置为CPU核心数的1-2倍。

## 安全注意事项

1. 当前实现仅支持非加密的WebSocket连接（ws://）
2. 生产环境建议添加SSL/TLS支持（wss://）
3. 建议添加输入验证和速率限制
4. 考虑添加身份验证机制

## 故障排除

### 编译错误
- 确保安装了OpenSSL开发库
- 检查GCC版本是否支持C++11

### 连接失败
- 检查防火墙设置
- 确认端口8080未被占用
- 验证客户端WebSocket URL格式

### 性能优化
- 调整线程池大小
- 优化消息缓冲区大小
- 考虑使用epoll等高性能I/O模型

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。
