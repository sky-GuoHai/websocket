#!/bin/bash

# WebSocket服务器构建脚本

set -e  # 遇到错误时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        print_error "g++ 编译器未找到"
        print_info "在Ubuntu/Debian上安装: sudo apt-get install build-essential"
        print_info "在CentOS/RHEL上安装: sudo yum install gcc-c++"
        exit 1
    fi
    
    # 检查OpenSSL
    if ! pkg-config --exists openssl; then
        print_warning "OpenSSL开发库未找到"
        print_info "在Ubuntu/Debian上安装: sudo apt-get install libssl-dev"
        print_info "在CentOS/RHEL上安装: sudo yum install openssl-devel"
        
        # 尝试继续编译，可能系统有OpenSSL但没有pkg-config
        print_warning "尝试继续编译..."
    fi
    
    print_success "依赖检查完成"
}

# 清理旧文件
clean() {
    print_info "清理旧的编译文件..."
    rm -f *.o websocket_server
    print_success "清理完成"
}

# 编译项目
build() {
    print_info "开始编译WebSocket服务器..."
    
    # 编译参数
    CXX="g++"
    CXXFLAGS="-std=c++11 -Wall -Wextra -O2 -pthread"
    LDFLAGS="-lssl -lcrypto -lpthread"
    
    # 如果是调试模式
    if [ "$1" = "debug" ]; then
        CXXFLAGS="$CXXFLAGS -g -DDEBUG"
        print_info "编译调试版本..."
    fi
    
    # 编译源文件
    print_info "编译 websocket_server.cpp..."
    $CXX $CXXFLAGS -c websocket_server.cpp -o websocket_server.o
    
    print_info "编译 main.cpp..."
    $CXX $CXXFLAGS -c main.cpp -o main.o
    
    # 链接
    print_info "链接可执行文件..."
    $CXX websocket_server.o main.o -o websocket_server $LDFLAGS
    
    print_success "编译完成！可执行文件: websocket_server"
}

# 运行服务器
run() {
    if [ ! -f "websocket_server" ]; then
        print_error "可执行文件不存在，请先编译"
        exit 1
    fi
    
    print_info "启动WebSocket服务器..."
    ./websocket_server
}

# 安装依赖
install_deps() {
    print_info "安装依赖..."
    
    if command -v apt-get &> /dev/null; then
        # Ubuntu/Debian
        sudo apt-get update
        sudo apt-get install -y build-essential libssl-dev pkg-config
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL
        sudo yum install -y gcc-c++ openssl-devel pkgconfig
    elif command -v dnf &> /dev/null; then
        # Fedora
        sudo dnf install -y gcc-c++ openssl-devel pkgconfig
    else
        print_error "不支持的包管理器，请手动安装依赖"
        exit 1
    fi
    
    print_success "依赖安装完成"
}

# 显示帮助
show_help() {
    echo "WebSocket服务器构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  build         编译项目 (默认)"
    echo "  debug         编译调试版本"
    echo "  clean         清理编译文件"
    echo "  run           运行服务器"
    echo "  install-deps  安装依赖"
    echo "  check         检查依赖"
    echo "  help          显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0              # 编译项目"
    echo "  $0 debug        # 编译调试版本"
    echo "  $0 clean build  # 清理后编译"
    echo "  $0 run          # 运行服务器"
}

# 主函数
main() {
    if [ $# -eq 0 ]; then
        # 默认行为：检查依赖并编译
        check_dependencies
        build
        return
    fi
    
    while [ $# -gt 0 ]; do
        case $1 in
            build)
                check_dependencies
                build
                ;;
            debug)
                check_dependencies
                build debug
                ;;
            clean)
                clean
                ;;
            run)
                run
                ;;
            install-deps)
                install_deps
                ;;
            check)
                check_dependencies
                ;;
            help|--help|-h)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
        shift
    done
}

# 运行主函数
main "$@"
