# Makefile for WebSocket Server

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
LDFLAGS = -lssl -lcrypto -lpthread

# 目标文件
TARGET = websocket_server
SOURCES = main.cpp websocket_server.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = websocket_server.h thread_pool.h

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# 编译源文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译文件
clean:
	rm -f $(OBJECTS) $(TARGET)

# 安装依赖（Ubuntu/Debian）
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libssl-dev

# 运行服务器
run: $(TARGET)
	./$(TARGET)

# 调试版本
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all          - Build the WebSocket server (default)"
	@echo "  clean        - Remove compiled files"
	@echo "  install-deps - Install required dependencies (Ubuntu/Debian)"
	@echo "  run          - Build and run the server"
	@echo "  debug        - Build debug version"
	@echo "  help         - Show this help message"

.PHONY: all clean install-deps run debug help
