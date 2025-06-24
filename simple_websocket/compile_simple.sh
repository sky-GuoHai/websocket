#!/bin/bash

# 简单的编译脚本，用于编译不依赖OpenSSL的WebSocket服务器

echo "编译简化版WebSocket服务器..."

# 编译参数
CXX="g++"
CXXFLAGS="-std=c++11 -Wall -Wextra -O2 -pthread"
LDFLAGS="-lpthread"

# 清理旧文件
echo "清理旧文件..."
rm -f simple_main.o simple_websocket_server.o simple_websocket_server

# 编译源文件
echo "编译 simple_websocket_server.cpp..."
$CXX $CXXFLAGS -c simple_websocket_server.cpp -o simple_websocket_server.o

if [ $? -ne 0 ]; then
    echo "编译 simple_websocket_server.cpp 失败"
    exit 1
fi

echo "编译 simple_main.cpp..."
$CXX $CXXFLAGS -c simple_main.cpp -o simple_main.o

if [ $? -ne 0 ]; then
    echo "编译 simple_main.cpp 失败"
    exit 1
fi

# 链接
echo "链接可执行文件..."
$CXX simple_websocket_server.o simple_main.o -o simple_websocket_server $LDFLAGS

if [ $? -ne 0 ]; then
    echo "链接失败"
    exit 1
fi

echo "编译成功！可执行文件: simple_websocket_server"
echo "运行服务器: ./simple_websocket_server"
