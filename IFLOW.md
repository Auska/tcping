# TCPing - TCP端口持续监控工具

## 项目概述
TCPing是一个用C++23开发的TCP端口持续监控工具，用于检查远程主机端口是否开放并提供实时监控功能。经过优化后，现在支持真正的非阻塞连接和精确的超时控制。

## 技术特性
- 使用C++23标准开发
- 基于CMake构建系统
- 跨平台支持（Linux/Unix/Windows）
- **非阻塞socket连接**：使用select()实现真正的超时控制
- **RAII资源管理**：SocketGuard类自动管理socket资源
- 精确的连接时间测量（毫秒级）
- 优雅的信号处理（Ctrl+C停止）
- 模块化代码结构：Config结构体和ArgumentParser类分离配置逻辑

## 核心功能
- **持续监控**：默认持续监控指定端口，无需额外参数
- **时间戳日志**：每次检查都带有精确时间戳
- **自定义间隔**：支持自定义检查间隔（默认1秒）
- **连接超时控制**：支持自定义连接超时时间（默认3000ms）
- **连接时间显示**：显示每次连接的响应时间
- **详细错误处理**：完善的错误处理和状态报告
- **详细模式**：可选的详细输出模式

## 构建说明

### 依赖要求
- CMake 3.20或更高版本
- 支持C++23的编译器（GCC 14+推荐，MSVC 19.35+）

### Linux/Unix构建步骤
```bash
mkdir build
cd build
cmake ..
make
```

### Windows构建步骤
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

或使用Visual Studio：
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### 安装
```bash
# Linux/Unix
make install  # 可选，安装到系统bin目录

# Windows
cmake --install . --config Release  # 可选，安装到系统目录
```

## 使用方法

### 基本语法
```bash
# Linux/Unix
./tcping <host> <port> [options]

# Windows
.\tcping.exe <host> <port> [options]
```

### 参数说明
- `<host>`：目标主机IP地址
- `<port>`：目标端口号（1-65535）
- `-i <seconds>`：检查间隔秒数（可选，默认1秒）
- `-t <ms>`：连接超时时间，单位毫秒（可选，默认3000ms）
- `-v`：详细模式（显示详细错误信息）

### 使用示例

#### 基本监控（默认设置）
```bash
./tcping 192.168.88.254 22
```

#### 自定义间隔和超时
```bash
./tcping 127.0.0.1 22 -i 3 -t 5000
```

#### 详细模式监控
```bash
./tcping 127.0.0.1 8080 -i 2 -v
```

#### 快速连接测试
```bash
./tcping 192.168.1.1 80 -i 1 -t 1000
```

## 输出格式

### 成功连接
```
[2025-12-21 19:28:01.412] 127.0.0.1:22 - Connected (time=0ms)
```

### 连接失败
```
[2025-12-21 18:33:30.210] 127.0.0.1:9999 - Connection failed
```

### 连接超时
```
Connection to 192.168.1.1:80 timed out after 1000ms
```

### 启动信息
```
TCPing v1.0.0 by Luodan <luodan0709@live.cn>
Starting continuous monitoring of 127.0.0.1:22 with 2s interval and 1000ms timeout. Press Ctrl+C to stop.
```

## 项目结构
```
tcping/
├── main.cpp          # 主程序源码（已优化）
├── CMakeLists.txt    # CMake构建配置
├── IFLOW.md          # 项目文档
├── .gitignore        # Git忽略文件
└── build/           # 构建输出目录
```

## 架构改进

### 代码结构优化
- **Config结构体**：集中管理配置参数
- **ArgumentParser类**：模块化参数解析逻辑
- **SocketGuard类**：RAII模式管理socket资源
- **功能分离**：将显示逻辑与核心功能分离

### 网络连接优化
- **非阻塞连接**：使用非阻塞socket避免长时间阻塞
- **select()超时控制**：精确的连接超时控制
- **错误检测改进**：更准确的连接状态检测
- **资源管理**：自动资源清理，防止资源泄露

## 平台支持

### Linux/Unix
- 使用POSIX socket API
- 支持信号处理（SIGINT, SIGTERM）
- 非阻塞I/O和select()系统调用
- 标准文件描述符操作

### Windows
- 使用Winsock2 API
- 自动WSA初始化和清理
- Windows特定的错误处理
- ioctlsocket设置非阻塞模式
- 兼容Windows 10/11和Windows Server

## 开发特性
- 遵循现代C++最佳实践
- 完整的编译器警告（-Wall -Wextra -Wpedantic /W4）
- 原子操作确保线程安全
- RAII资源管理
- 标准库优先，最小化平台依赖
- 条件编译确保跨平台兼容性
- 模块化设计提高代码可维护性

## 许可证
本项目采用开源许可证，可自由使用和修改。
