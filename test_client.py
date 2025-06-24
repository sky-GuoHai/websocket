#!/usr/bin/env python3
"""
WebSocket客户端测试脚本
用于测试C++ WebSocket服务器
"""

import asyncio
import websockets
import json
import sys
from datetime import datetime

class WebSocketTestClient:
    def __init__(self, uri="ws://localhost:8080"):
        self.uri = uri
        self.websocket = None
        self.running = False

    async def connect(self):
        """连接到WebSocket服务器"""
        try:
            self.websocket = await websockets.connect(self.uri)
            self.running = True
            print(f"✅ Connected to {self.uri}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False

    async def disconnect(self):
        """断开连接"""
        if self.websocket:
            await self.websocket.close()
            self.running = False
            print("🔌 Disconnected")

    async def send_message(self, message):
        """发送消息"""
        if self.websocket and self.running:
            try:
                await self.websocket.send(message)
                print(f"📤 Sent: {message}")
                return True
            except Exception as e:
                print(f"❌ Failed to send message: {e}")
                return False
        else:
            print("❌ Not connected")
            return False

    async def receive_messages(self):
        """接收消息的协程"""
        try:
            while self.running:
                message = await self.websocket.recv()
                timestamp = datetime.now().strftime("%H:%M:%S")
                print(f"📥 [{timestamp}] Received: {message}")
        except websockets.exceptions.ConnectionClosed:
            print("🔌 Connection closed by server")
            self.running = False
        except Exception as e:
            print(f"❌ Error receiving message: {e}")
            self.running = False

    async def interactive_mode(self):
        """交互模式"""
        print("\n🎮 Interactive mode started")
        print("Commands:")
        print("  - Type any message to send")
        print("  - 'time' - Get server time")
        print("  - 'broadcast' - Trigger broadcast")
        print("  - 'quit' or 'exit' - Disconnect and exit")
        print("  - 'help' - Show this help")
        print()

        # 启动接收消息的任务
        receive_task = asyncio.create_task(self.receive_messages())

        try:
            while self.running:
                try:
                    # 使用asyncio读取用户输入
                    message = await asyncio.get_event_loop().run_in_executor(
                        None, input, "💬 Enter message: "
                    )
                    
                    message = message.strip()
                    
                    if message.lower() in ['quit', 'exit']:
                        break
                    elif message.lower() == 'help':
                        print("Commands: time, broadcast, quit, exit, help")
                        continue
                    elif message:
                        await self.send_message(message)
                        
                except KeyboardInterrupt:
                    print("\n🛑 Interrupted by user")
                    break
                except Exception as e:
                    print(f"❌ Input error: {e}")
                    break
                    
        finally:
            self.running = False
            receive_task.cancel()
            try:
                await receive_task
            except asyncio.CancelledError:
                pass

    async def run_tests(self):
        """运行自动化测试"""
        print("🧪 Running automated tests...")
        
        test_messages = [
            "Hello Server!",
            "time",
            "This is a test message",
            "broadcast",
            "Another test message"
        ]
        
        for i, message in enumerate(test_messages, 1):
            print(f"\n📋 Test {i}/{len(test_messages)}")
            await self.send_message(message)
            
            # 等待响应
            try:
                response = await asyncio.wait_for(self.websocket.recv(), timeout=2.0)
                timestamp = datetime.now().strftime("%H:%M:%S")
                print(f"📥 [{timestamp}] Response: {response}")
            except asyncio.TimeoutError:
                print("⏰ No response received (timeout)")
            except Exception as e:
                print(f"❌ Error receiving response: {e}")
            
            # 测试间隔
            await asyncio.sleep(1)
        
        print("\n✅ Automated tests completed")

async def main():
    """主函数"""
    if len(sys.argv) > 1:
        uri = sys.argv[1]
    else:
        uri = "ws://localhost:8080"
    
    client = WebSocketTestClient(uri)
    
    print("🚀 WebSocket Test Client")
    print(f"🔗 Target: {uri}")
    print()
    
    # 连接到服务器
    if not await client.connect():
        return
    
    try:
        # 询问用户选择模式
        print("Select mode:")
        print("1. Interactive mode")
        print("2. Automated tests")
        
        while True:
            try:
                choice = input("Enter choice (1 or 2): ").strip()
                if choice == '1':
                    await client.interactive_mode()
                    break
                elif choice == '2':
                    await client.run_tests()
                    break
                else:
                    print("Invalid choice. Please enter 1 or 2.")
            except KeyboardInterrupt:
                print("\n🛑 Interrupted by user")
                break
    
    finally:
        await client.disconnect()

if __name__ == "__main__":
    try:
        # 检查websockets库是否安装
        import websockets
    except ImportError:
        print("❌ websockets library not found")
        print("📦 Install it with: pip install websockets")
        sys.exit(1)
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        sys.exit(1)
