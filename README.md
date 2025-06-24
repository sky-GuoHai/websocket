# C++ WebSocket Server with Thread Pool

这是一个基于C++实现的高性能WebSocket服务器，支持多客户端连接、双向通信和线程池任务执行。项目提供两个版本：完整版（支持OpenSSL）和简化版（无OpenSSL依赖）。

## 功能特性

- ✅ WebSocket协议支持（RFC 6455）
- ✅ 多客户端并发连接（使用epoll高性能I/O）
- ✅ 线程池任务执行
- ✅ 双向消息通信
- ✅ 连接状态管理
- ✅ 事件回调机制
- ✅ 广播消息功能
- ✅ 单点消息发送
- ✅ 服务器状态监控
- ✅ 客户端管理（连接/断开/查询）
- ✅ 信号处理（优雅关闭）
- ✅ 多种测试客户端（HTML、Python）

## 系统要求

- Linux操作系统
- GCC 4.8+ (支持C++11)
- OpenSSL开发库（仅完整版需要）
- Python 3.6+ 和 websockets库（用于Python测试客户端）

## 安装依赖

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev pkg-config
```

### CentOS/RHEL
```bash
sudo yum install gcc-c++ openssl-devel pkgconfig
```

### Fedora
```bash
sudo dnf install gcc-c++ openssl-devel pkgconfig
```

### 或者使用自动化脚本安装依赖
```bash
# 使用Makefile
make install-deps

# 或者使用构建脚本
chmod +x build.sh
./build.sh install-deps
```

### Python测试客户端依赖
```bash
pip install websockets
```

## 编译和运行

### 方式一：使用简化版本（无OpenSSL依赖）
```bash
# 进入简化版目录
cd simple_websocket

# 使用Makefile编译
make

# 或者使用编译脚本
chmod +x compile_simple.sh
./compile_simple.sh

# 运行简化版服务器
./simple_websocket_server
```

### 方式二：使用完整版本（需要OpenSSL）
```bash
# 在项目根目录编译完整版
make

# 运行完整版服务器
make run
# 或者直接运行
./websocket_server
```

### 方式三：使用自动化构建脚本
```bash
# 使用构建脚本（自动检查依赖）
chmod +x build.sh
./build.sh

# 编译调试版本
./build.sh debug

# 清理并重新编译
./build.sh clean build

# 编译并运行
./build.sh build run
```

### 清理编译文件
```bash
# 清理完整版
make clean

# 清理简化版
cd simple_websocket && make clean

# 使用构建脚本清理
./build.sh clean
```

### 编译调试版本
```bash
# 完整版调试
make debug

# 简化版调试
cd simple_websocket && make debug

# 使用构建脚本编译调试版
./build.sh debug
```

## 使用方法

### 1. 启动服务器
```bash
# 启动完整版服务器
./websocket_server

# 或启动简化版服务器
cd simple_websocket
./simple_websocket_server
```
服务器将在端口8080上启动并等待客户端连接。

### 2. 测试连接

#### 使用HTML客户端
打开 `test_client.html` 文件在浏览器中测试WebSocket连接。

#### 使用Python客户端
```bash
# 确保安装了websockets库
pip install websockets

# 运行Python测试客户端
python test_client.py

# 或指定服务器地址
python test_client.py ws://localhost:8080
```

### 3. 服务器命令
服务器运行时支持以下交互命令：
- `status` - 显示服务器状态（连接数、线程池状态等）
- `broadcast <message>` - 向所有客户端广播消息
- `list` - 列出所有连接的客户端
- `help` - 显示帮助信息
- `time` - 显示服务器当前时间
- `send <client_id> <message>` - 向特定客户端发送消息
- `quit` 或 `exit` - 优雅停止服务器

### 4. 客户端测试命令
客户端可以发送以下特殊命令：
- `time` - 获取服务器当前时间
- `broadcast` - 触发服务器广播消息
- `hello` - 获取欢迎消息（仅简化版）

## 项目结构

```
websocket/
├── 📁 simple_websocket/           # 简化版WebSocket服务器（无OpenSSL依赖）
│   ├── simple_websocket_server.h  # 简化版服务器头文件
│   ├── simple_websocket_server.cpp # 简化版服务器实现
│   ├── simple_main.cpp            # 简化版主程序
│   ├── thread_pool.h              # 线程池头文件
│   ├── Makefile                   # 简化版编译脚本
│   └── compile_simple.sh          # 简化版编译脚本
├── 📄 websocket_server.h          # 完整版WebSocket服务器头文件
├── 📄 websocket_server.cpp        # 完整版WebSocket服务器实现
├── 📄 main.cpp                    # 完整版主程序和示例
├── 📄 thread_pool.h               # 线程池头文件
├── 📄 test_client.html            # HTML测试客户端
├── 📄 test_client.py              # Python测试客户端
├── 📄 Makefile                    # 完整版编译脚本
├── 📄 build.sh                    # 自动化构建脚本
└── 📄 README.md                   # 项目说明文档
```

### 版本对比

| 特性 | 完整版 | 简化版 |
|------|--------|--------|
| OpenSSL依赖 | ✅ 需要 | ❌ 不需要 |
| 编译复杂度 | 中等 | 简单 |
| 功能完整性 | 完整 | 基础功能 |
| 性能优化 | epoll + 线程池 | 基础实现 |
| 适用场景 | 生产环境 | 学习/测试 |

## API 使用示例

### 完整版服务器设置
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

### 简化版服务器设置
```cpp
#include "simple_websocket_server.h"

// 创建简化版服务器实例
SimpleWebSocketServer server(8080, 4);

// 设置相同的回调函数
server.setMessageHandler([&server](int client_id, const std::string& message) {
    // 处理消息逻辑
});

server.setConnectionHandler([](int client_id, const std::string& client_ip) {
    // 处理连接事件
});

server.setDisconnectionHandler([](int client_id) {
    // 处理断开连接事件
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

### 服务器状态查询（完整版）
```cpp
// 获取服务器状态
bool running = server.isRunning();
size_t client_count = server.getClientCount();
size_t thread_pool_size = server.getThreadPoolSize();
size_t available_threads = server.getAvailableThreads();

// 获取所有连接的客户端
auto clients = server.getConnectedClients();
for (const auto& client : clients) {
    int client_id = client.first;
    std::string client_ip = client.second;
    std::cout << "Client " << client_id << " from " << client_ip << std::endl;
}

// 断开特定客户端
server.disconnectClient(client_id);

// 检查客户端是否存在
bool exists = server.isClientExists(client_id);
```

## 配置选项

### 线程池配置
线程池大小可以在创建服务器时配置：
```cpp
WebSocketServer server(8080, 8); // 使用8个工作线程
SimpleWebSocketServer server(8080, 8); // 简化版也支持
```

建议线程数量设置为CPU核心数的1-2倍。

### 端口配置
默认端口为8080，可以修改：
```cpp
WebSocketServer server(9090, 4); // 使用端口9090
```

## 测试和调试

### 使用构建脚本进行测试
```bash
# 检查依赖
./build.sh check

# 编译测试
./build.sh build

# 运行服务器
./build.sh run
```

### 使用Python客户端测试
```bash
# 交互模式测试
python test_client.py

# 自动化测试
python test_client.py # 选择模式2
```

### 调试模式
```bash
# 编译调试版本
make debug
# 或
./build.sh debug

# 使用gdb调试
gdb ./websocket_server
```

## 性能特性

### 完整版性能特性
- **epoll I/O多路复用**：支持大量并发连接
- **线程池**：高效的任务处理
- **连接管理**：智能的客户端生命周期管理
- **内存管理**：使用智能指针避免内存泄漏

### 简化版特性
- **轻量级实现**：无外部依赖
- **易于理解**：适合学习WebSocket协议
- **快速部署**：编译简单，部署方便

## 安全注意事项

1. **加密连接**：当前实现仅支持非加密的WebSocket连接（ws://）
2. **生产环境**：建议添加SSL/TLS支持（wss://）
3. **输入验证**：建议添加消息内容验证和长度限制
4. **速率限制**：考虑添加连接频率和消息频率限制
5. **身份验证**：生产环境应添加客户端身份验证机制
6. **防火墙**：确保适当的网络安全配置

## 故障排除

### 编译错误
```bash
# 检查编译器
g++ --version

# 检查依赖（完整版）
pkg-config --exists openssl && echo "OpenSSL found" || echo "OpenSSL not found"

# 使用简化版避免OpenSSL依赖
cd simple_websocket && make
```

### 连接失败
```bash
# 检查端口占用
netstat -tlnp | grep 8080

# 检查防火墙（Ubuntu）
sudo ufw status

# 测试本地连接
telnet localhost 8080
```

### 性能优化
- **线程池大小**：根据CPU核心数调整
- **连接数限制**：监控系统资源使用
- **消息缓冲区**：根据应用需求调整
- **epoll配置**：完整版已使用epoll优化

## 快速开始

### 1分钟快速体验
```bash
# 克隆项目
git clone <repository-url>
cd websocket

# 快速启动简化版
cd simple_websocket
make && ./simple_websocket_server

# 在另一个终端测试
python ../test_client.py # 选择模式2
```

### 生产环境部署
```bash
# 安装依赖
./build.sh install-deps

# 编译优化版本
./build.sh

# 启动服务器
./websocket_server
```

## 常见问题 (FAQ)

### Q: 两个版本有什么区别？
A: 完整版使用OpenSSL和epoll，性能更好，适合生产环境；简化版无外部依赖，编译简单，适合学习和测试。

### Q: 如何修改端口？
A: 修改main.cpp或simple_main.cpp中的端口号，重新编译即可。

### Q: 支持SSL/TLS吗？
A: 当前版本不支持，但完整版已包含OpenSSL依赖，可以扩展支持。

### Q: 如何处理大量并发连接？
A: 使用完整版，它采用epoll和线程池，可以处理大量并发连接。

### Q: Python客户端连接失败怎么办？
A: 确保安装了websockets库：`pip install websockets`，并检查服务器是否正在运行。

## 扩展开发

### 添加新的消息处理
```cpp
server.setMessageHandler([&server](int client_id, const std::string& message) {
    if (message.substr(0, 5) == "echo:") {
        server.sendMessageToClient(client_id, message.substr(5));
    } else if (message == "users") {
        auto clients = server.getConnectedClients();
        server.sendMessageToClient(client_id, "Connected users: " + std::to_string(clients.size()));
    }
    // 添加更多自定义命令...
});
```

### 集成到现有项目
```cpp
// 作为库使用
#include "websocket_server.h"

class MyApplication {
private:
    WebSocketServer ws_server;

public:
    MyApplication() : ws_server(8080, 4) {
        setupWebSocketHandlers();
    }

    void setupWebSocketHandlers() {
        ws_server.setMessageHandler([this](int client_id, const std::string& message) {
            handleWebSocketMessage(client_id, message);
        });
    }
};
```

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！

### 贡献指南
1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 开发环境设置
```bash
# 克隆项目
git clone <repository-url>
cd websocket

# 安装开发依赖
./build.sh install-deps

# 编译调试版本
./build.sh debug

# 运行测试
python test_client.py
```

## 更新日志

### v1.0.0
- ✅ 基础WebSocket服务器实现
- ✅ 线程池支持
- ✅ 简化版本（无OpenSSL依赖）
- ✅ Python测试客户端
- ✅ 自动化构建脚本
- ✅ epoll高性能I/O支持
