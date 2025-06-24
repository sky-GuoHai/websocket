# Makefile for Simple WebSocket Server (No OpenSSL dependency)

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
LDFLAGS = -lpthread

# 目标文件
TARGET = simple_websocket_server
SOURCES = simple_main.cpp simple_websocket_server.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = simple_websocket_server.h thread_pool.h

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

# 运行服务器
run: $(TARGET)
	./$(TARGET)

# 调试版本
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# 测试编译（不运行）
test-compile: $(TARGET)
	@echo "Compilation successful!"

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all          - Build the simple WebSocket server (default)"
	@echo "  clean        - Remove compiled files"
	@echo "  run          - Build and run the server"
	@echo "  debug        - Build debug version"
	@echo "  test-compile - Test compilation without running"
	@echo "  help         - Show this help message"

.PHONY: all clean run debug test-compile help
