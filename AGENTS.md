# TCPing - TCP端口持续监控工具

## 项目概述
TCPing是一个用C++23开发的TCP端口持续监控工具，用于检查远程主机端口是否开放并提供实时监控功能。经过优化后，现在支持真正的非阻塞连接、精确的超时控制、域名解析以及IPv6支持。

## 技术特性
- 使用C++23标准开发
- 基于CMake构建系统
- 跨平台支持（Linux/Unix/Windows）
- **非阻塞socket连接**：使用select()实现真正的超时控制
- **RAII资源管理**：SocketGuard类自动管理socket资源
- **DNS域名解析**：支持域名和IP地址输入，自动解析域名
- **IPv6支持**：支持IPv4和IPv6双协议栈
- 精确的连接时间测量（毫秒级）
- 优雅的信号处理（Ctrl+C停止）
- 模块化代码结构：Config结构体和ArgumentParser类分离配置逻辑
- **双语错误提示**：中英文双语错误信息，便于故障排查
- **详细统计功能**：提供连接成功率、平均响应时间等统计信息

## 核心功能
- **持续监控**：默认持续监控指定端口，无需额外参数
- **时间戳日志**：每次检查都带有精确时间戳
- **自定义间隔**：支持自定义检查间隔（默认1秒）
- **连接超时控制**：支持自定义连接超时时间（默认3000ms）
- **连接时间显示**：显示每次连接的响应时间
- **DNS域名解析**：支持域名输入，自动解析并显示IP地址
- **IPv6支持**：支持IPv6地址和域名解析
- **连接计数**：支持指定连接尝试次数
- **详细统计**：提供成功率、最小/最大/平均响应时间等统计信息
- **详细错误处理**：完善的错误处理和状态报告（中英文双语）
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

### Linux上交叉编译Windows版本
如果需要在Linux上编译Windows可执行文件，可以使用MinGW交叉编译工具链：

```bash
# 安装MinGW交叉编译器（Debian/Ubuntu）
sudo apt-get install mingw-w64

# 交叉编译Windows版本
mkdir build-win
cd build-win
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake
make
```

这将生成可在Windows系统上运行的 `tcping.exe` 文件。

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
- `<host>`：目标主机（支持IP地址或域名）
- `<port>`：目标端口号（1-65535）
- `-i <seconds>`：检查间隔秒数（可选，默认1秒）
- `-t <ms>`：连接超时时间，单位毫秒（可选，默认3000ms）
- `-c <count>`：连接尝试次数（可选，默认无限）
- `-v`：详细模式（显示详细错误信息）
- `-s`：显示统计摘要（默认开启）
- `-S`：隐藏统计摘要
- `-4`：强制使用IPv4模式（默认）
- `-6`：强制使用IPv6模式

### 使用示例

#### 基本监控（默认设置）
```bash
./tcping 192.168.88.254 22
```

#### 域名监控（支持DNS解析）
```bash
./tcping google.com 443
./tcping www.baidu.com 80
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

#### 指定连接次数
```bash
./tcping 127.0.0.1 22 -c 10    # 只进行10次连接尝试
```

#### IPv6连接
```bash
./tcping ipv6.google.com 443 -6
./tcping ::1 22 -6
```

## 输出格式

### 成功连接
```
[2025-12-21 19:28:01.412] 127.0.0.1:22 - Connected (time=0ms)
```

### 域名解析成功
```
[2025-12-21 19:28:01.412] google.com (142.250.185.238):443 - Connected (time=15ms)
```

### IPv6连接成功
```
[2025-12-21 19:28:01.412] ipv6.google.com (2001:4860:4860::8888):443 - Connected (time=15ms)
```

### 连接失败
```
[2025-12-21 18:33:30.210] 127.0.0.1:9999 - Connection failed
```

### DNS解析失败
```
[2025-12-21 18:33:30.210] invalid.domain.com:80 - Connection failed: DNS resolution failed for 'invalid.domain.com': Name or service not known (请检查域名是否正确或网络连接)
```

### 连接超时
```
[2025-12-21 18:33:30.210] 192.168.1.1:80 - Connection failed: Connection timed out after 1000ms
```

### 启动信息
```
TCPing v1.0.0 by Luodan <luodan0709@live.cn>
Starting continuous monitoring of google.com:443 with 2s interval and 1000ms timeout. Press Ctrl+C to stop.
```

### 统计摘要（Ctrl+C停止后显示）
```
--- Statistics ---
Total attempts: 10
Successful: 8 (80.0%)
Failed: 2
Timeout: 1
Connection refused: 1
Min time: 5ms
Max time: 25ms
Average time: 12ms
```

## 项目结构
```
tcping/
├── CMakeLists.txt           # CMake构建配置
├── mingw-toolchain.cmake    # MinGW交叉编译工具链配置
├── AGENTS.md                # 项目文档
├── .gitignore               # Git忽略文件
├── build/                   # 构建输出目录
└── src/
    ├── main.cpp             # 主程序入口
    ├── tcping.cpp           # TCP连接实现
    ├── args/
    │   ├── args.cpp         # 命令行参数解析
    │   └── args.h           # 参数解析头文件
    ├── error/
    │   └── error.cpp        # 错误处理实现
    ├── statistics/
    │   └── statistics.cpp   # 统计功能实现
    └── include/
        ├── common.h         # 公共头文件
        ├── version.h        # 版本信息
        ├── config.h         # 配置结构体
        ├── tcping.h         # TCP连接类
        ├── args.h           # 参数解析类
        ├── error.h          # 错误处理
        └── statistics.h     # 统计结构体
```

## 架构改进

### 代码结构优化
- **Config结构体**：集中管理配置参数
- **ArgumentParser类**：模块化参数解析逻辑
- **Tcping类**：封装网络连接和DNS解析功能
- **SocketGuard类**：RAII模式管理socket资源
- **Statistics类**：独立的统计管理和输出功能
- **功能分离**：将显示逻辑与核心功能分离

### 网络连接优化
- **非阻塞连接**：使用非阻塞socket避免长时间阻塞
- **select()超时控制**：精确的连接超时控制
- **DNS域名解析**：使用getaddrinfo实现域名到IP的转换
- **IP地址回显**：显示解析后的IP地址，便于确认目标
- **IPv6支持**：完整的IPv6协议栈支持
- **错误检测改进**：更准确的连接状态检测
- **资源管理**：自动资源清理，防止资源泄露

### 代码优化
- **公共头文件**：创建common.h集中管理公共头文件，减少重复包含
- **const正确性**：确保成员函数正确使用const修饰符

### 跨平台兼容性
- **条件编译**：使用预处理指令区分平台特定代码
- **统一接口**：定义平台无关的宏（CLOSE_SOCKET, SOCKET_ERROR_CODE）
- **错误信息本地化**：Windows和Unix使用不同的错误获取机制
- **MinGW支持**：提供交叉编译工具链配置文件

## 平台支持

### Linux/Unix
- 使用POSIX socket API
- 支持信号处理（SIGINT, SIGTERM）
- 非阻塞I/O和select()系统调用
- 标准文件描述符操作

### Windows
- 使用Winsock2 API
- 自动WSA初始化和清理（WinSockInitializer类）
- Windows特定的错误处理（FormatMessageA）
- ioctlsocket设置非阻塞模式
- 兼容Windows 10/11和Windows Server
- 支持MinGW交叉编译

## 开发特性
- 遵循现代C++23最佳实践
- 完整的编译器警告（-Wall -Wextra -Wpedantic /W4）
- 原子操作确保线程安全（std::atomic）
- RAII资源管理（SocketGuard, WinSockInitializer）
- 标准库优先，最小化平台依赖
- 条件编译确保跨平台兼容性
- 模块化设计提高代码可维护性
- **智能DNS解析**：支持域名和IP地址混合使用
- **双语错误信息**：中英文双语错误描述，提升用户体验
- **高精度计时**：使用chrono实现微秒级计时精度

## 版本历史

### v1.0.0
- 初始版本发布
- 跨平台支持（Linux/Unix/Windows）
- 非阻塞socket连接和精确超时控制
- 持续监控功能
- 时间戳日志
- 自定义检查间隔和超时时间
- 详细错误处理

### 最近更新
- **代码优化**：创建公共头文件common.h，集中管理公共头文件减少重复代码
- **IPv6支持**：添加IPv6协议支持（-6/-4参数）
- **连接计数**：添加-c参数支持指定连接尝试次数
- **详细统计**：添加详细的连接统计功能（成功率、超时、拒绝等）
- **统计控制**：添加-s/-S参数控制统计摘要显示
- **代码重构**：模块化代码结构，分离args、error、statistics子模块
- **域名支持**：添加DNS解析功能，支持域名输入
- **错误信息优化**：中英文双语错误描述
- **IP地址显示**：显示解析后的IP地址
- **MinGW支持**：添加Linux交叉编译Windows版本的配置
- **跨平台改进**：修复Windows编译问题（netdb.h）
- **代码优化**：修复编译器警告，提高代码质量