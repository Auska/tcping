# TCPing - TCP端口持续监控工具

## 项目概述
TCPing是一个用C++23开发的TCP端口持续监控工具，用于检查远程主机端口是否开放并提供实时监控功能。

## 技术特性
- 使用C++23标准开发
- 基于CMake构建系统
- 跨平台支持（Linux/Unix/Windows）
- 非阻塞socket连接
- 精确的连接时间测量（毫秒级）
- 优雅的信号处理（Ctrl+C停止）

## 核心功能
- **持续监控**：默认持续监控指定端口，无需额外参数
- **时间戳日志**：每次检查都带有精确时间戳
- **自定义间隔**：支持自定义检查间隔（默认1秒）
- **连接时间显示**：显示每次连接的响应时间
- **错误处理**：完善的错误处理和状态报告

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
./tcping <host> <port> [-i <interval_seconds>]

# Windows
.\tcping.exe <host> <port> [-i <interval_seconds>]
```

### 参数说明
- `<host>`：目标主机IP地址
- `<port>`：目标端口号（1-65535）
- `-i <interval_seconds>`：检查间隔秒数（可选，默认1秒）

### 使用示例

#### 基本监控（默认1秒间隔）
```bash
./tcping 192.168.88.254 22
```

#### 自定义间隔监控
```bash
./tcping 127.0.0.1 22 -i 3
```

#### 本地端口监控
```bash
./tcping 127.0.0.1 8080 -i 2
```

## 输出格式

### 成功连接
```
[2025-12-21 18:33:28.209] 192.168.88.254:22 - Connected (time=2ms)
```

### 连接失败
```
[2025-12-21 18:33:30.210] 127.0.0.1:9999 - Connection failed
```

### 启动信息
```
Starting continuous monitoring of 192.168.88.254:22 with 1s interval. Press Ctrl+C to stop.
```

## 项目结构
```
tcping/
├── main.cpp          # 主程序源码
├── CMakeLists.txt    # CMake构建配置
├── IFLOW.md          # 项目文档
└── build/           # 构建输出目录
```

## 平台支持

### Linux/Unix
- 使用POSIX socket API
- 支持信号处理（SIGINT, SIGTERM）
- 标准文件描述符操作

### Windows
- 使用Winsock2 API
- 自动WSA初始化和清理
- Windows特定的错误处理
- 兼容Windows 10/11和Windows Server

## 开发特性
- 遵循现代C++最佳实践
- 完整的编译器警告（-Wall -Wextra -Wpedantic /W4）
- 原子操作确保线程安全
- RAII资源管理
- 标准库优先，最小化平台依赖
- 条件编译确保跨平台兼容性

## 许可证
本项目采用开源许可证，可自由使用和修改。
