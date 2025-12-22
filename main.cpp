#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>

// Version and author information
const std::string VERSION = "1.0.0";
const std::string AUTHOR = "Luodan <luodan0709@live.cn>";

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <netdb.h>
#endif

std::atomic<bool> keep_running(true);

#ifdef _WIN32
class WinSockInitializer {
public:
    WinSockInitializer() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            exit(1);
        }
    }
    ~WinSockInitializer() {
        WSACleanup();
    }
};
static WinSockInitializer winSockInit;
#endif

void signalHandler(int /* signal */) {
    keep_running = false;
}

#ifdef _WIN32
#define CLOSE_SOCKET(s) closesocket(s)
#define SOCKET_ERROR_CODE WSAGetLastError()
#define GET_ERROR_MSG() getLastErrorString()
#else
#define CLOSE_SOCKET(s) close(s)
#define SOCKET_ERROR_CODE errno
#define GET_ERROR_MSG() strerror(errno)
#endif

#ifdef _WIN32
std::string getLastErrorString() {
    int errorCode = WSAGetLastError();
    char* msgBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgBuffer, 0, NULL);
    
    std::string message(msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}
#endif

std::string getDetailedErrorDescription(int errorCode) {
#ifdef _WIN32
    switch (errorCode) {
        case WSAECONNREFUSED:
            return "Connection refused (目标端口主动拒绝连接)";
        case WSAETIMEDOUT:
            return "Connection timed out (连接超时)";
        case WSAEHOSTUNREACH:
            return "Host unreachable (主机不可达)";
        case WSAENETUNREACH:
            return "Network unreachable (网络不可达)";
        case WSAENETDOWN:
            return "Network is down (网络已关闭)";
        case WSAECONNABORTED:
            return "Connection aborted (连接被中止)";
        case WSAECONNRESET:
            return "Connection reset (连接被重置)";
        case WSAEADDRINUSE:
            return "Address already in use (地址已被使用)";
        case WSAEACCES:
            return "Access denied (访问被拒绝)";
        default:
            return GET_ERROR_MSG();
    }
#else
    switch (errorCode) {
        case ECONNREFUSED:
            return "Connection refused (目标端口主动拒绝连接)";
        case ETIMEDOUT:
            return "Connection timed out (连接超时)";
        case EHOSTUNREACH:
            return "Host unreachable (主机不可达)";
        case ENETUNREACH:
            return "Network unreachable (网络不可达)";
        case ENETDOWN:
            return "Network is down (网络已关闭)";
        case ECONNABORTED:
            return "Connection aborted (连接被中止)";
        case ECONNRESET:
            return "Connection reset (连接被重置)";
        case EADDRINUSE:
            return "Address already in use (地址已被使用)";
        case EACCES:
            return "Access denied (访问被拒绝)";
        case EPERM:
            return "Operation not permitted (操作不被允许)";
        default:
            return strerror(errorCode);
    }
#endif
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
#endif
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

class Tcping {
public:
    Tcping(const std::string& host, int port) : host_(host), port_(port) {}

    bool checkConnection(int timeout_ms = 3000, double* connection_time_ms = nullptr, std::string* error_msg = nullptr) {
        // RAII wrapper for socket
        struct SocketGuard {
#ifdef _WIN32
            SOCKET sockfd;
            SocketGuard() : sockfd(INVALID_SOCKET) {}
            ~SocketGuard() { if (sockfd != INVALID_SOCKET) CLOSE_SOCKET(sockfd); }
            operator SOCKET() const { return sockfd; }
            SOCKET* operator&() { return &sockfd; }
#else
            int sockfd;
            SocketGuard() : sockfd(-1) {}
            ~SocketGuard() { if (sockfd >= 0) CLOSE_SOCKET(sockfd); }
            operator int() const { return sockfd; }
            int* operator&() { return &sockfd; }
#endif
        } socket_guard;

        // Create socket
#ifdef _WIN32
        socket_guard.sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_guard.sockfd == INVALID_SOCKET) {
#else
        socket_guard.sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_guard.sockfd < 0) {
#endif
            int errorCode = SOCKET_ERROR_CODE;
            std::cerr << "Socket creation failed: " << getDetailedErrorDescription(errorCode) << " (Error " << errorCode << ")" << std::endl;
            if (connection_time_ms) *connection_time_ms = -1;
            return false;
        }

        // Set socket to non-blocking mode
#ifdef _WIN32
        u_long mode = 1;
        if (ioctlsocket(socket_guard.sockfd, FIONBIO, &mode) != 0) {
#else
        int flags = fcntl(socket_guard.sockfd, F_GETFL, 0);
        if (flags == -1 || fcntl(socket_guard.sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
#endif
            int errorCode = SOCKET_ERROR_CODE;
            std::cerr << "Failed to set non-blocking mode: " << getDetailedErrorDescription(errorCode) << " (Error " << errorCode << ")" << std::endl;
            if (connection_time_ms) *connection_time_ms = -1;
            return false;
        }

        // Prepare server address
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<u_short>(port_));
        
        // First try to parse as IP address
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) > 0) {
            // Successfully parsed as IP address, continue with connection
        } else {
            // Try to resolve as hostname
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            
            int resolve_result = getaddrinfo(host_.c_str(), nullptr, &hints, &result);
            if (resolve_result != 0) {
                if (connection_time_ms) *connection_time_ms = -1;
                if (error_msg) {
#ifdef _WIN32
                    *error_msg = "DNS resolution failed for '" + host_ + "': " + gai_strerrorA(resolve_result) + " (请检查域名是否正确或网络连接)";
#else
                    *error_msg = "DNS resolution failed for '" + host_ + "': " + gai_strerror(resolve_result) + " (请检查域名是否正确或网络连接)";
#endif
                }
                return false;
            }
            
            // Copy the resolved IP address
            struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
            server_addr.sin_addr = addr_in->sin_addr;
            
            // Store the resolved IP for display
            std::string resolved_ip = inet_ntoa(server_addr.sin_addr);
            if (resolved_ip != host_) {
                resolved_host_ = resolved_ip;
            }
            
            freeaddrinfo(result);
        }

        // Start connection attempt
        auto start_time = std::chrono::high_resolution_clock::now();
        int result = connect(socket_guard.sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        if (result == 0) {
            // Connection succeeded immediately
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            if (connection_time_ms) *connection_time_ms = duration_us.count() / 1000.0; // 转换为毫秒
            return true;
        }

        // Check if connection is in progress
        int errorCode = SOCKET_ERROR_CODE;
#ifdef _WIN32
        if (errorCode != WSAEWOULDBLOCK) {
#else
        if (errorCode != EINPROGRESS) {
#endif
            std::cout << "Connection to " << host_ << ":" << port_ << " failed: " 
                      << getDetailedErrorDescription(errorCode) << " (Error " << errorCode << ")" << std::endl;
            if (connection_time_ms) *connection_time_ms = -1;
            return false;
        }

        // Use select to wait for connection with timeout
        fd_set write_fds, error_fds;
        FD_ZERO(&write_fds);
        FD_ZERO(&error_fds);
        FD_SET(socket_guard.sockfd, &write_fds);
        FD_SET(socket_guard.sockfd, &error_fds);

        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        result = select(static_cast<int>(socket_guard.sockfd) + 1, nullptr, &write_fds, &error_fds, &timeout);
        
        if (result <= 0) {
            if (result == 0) {
                if (error_msg) {
                    *error_msg = "Connection timed out after " + std::to_string(timeout_ms) + "ms";
                }
            } else {
                errorCode = SOCKET_ERROR_CODE;
                if (error_msg) {
                    *error_msg = getDetailedErrorDescription(errorCode) + " (Error " + std::to_string(errorCode) + ")";
                }
            }
            if (connection_time_ms) *connection_time_ms = -1;
            return false;
        }

        // Check if socket is ready for writing or has error
        if (FD_ISSET(socket_guard.sockfd, &error_fds)) {
            int sock_error = 0;
            socklen_t len = sizeof(sock_error);
            if (getsockopt(socket_guard.sockfd, SOL_SOCKET, SO_ERROR, (char*)&sock_error, &len) == 0) {
                if (error_msg) {
                    *error_msg = getDetailedErrorDescription(sock_error) + " (Error " + std::to_string(sock_error) + ")";
                }
            }
            if (connection_time_ms) *connection_time_ms = -1;
            return false;
        }

        if (FD_ISSET(socket_guard.sockfd, &write_fds)) {
            // Check if connection actually succeeded by getting socket error
            int sock_error = 0;
            socklen_t len = sizeof(sock_error);
            if (getsockopt(socket_guard.sockfd, SOL_SOCKET, SO_ERROR, (char*)&sock_error, &len) == 0) {
                if (sock_error == 0) {
                    // Connection succeeded
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                    if (connection_time_ms) *connection_time_ms = duration_us.count() / 1000.0; // 转换为毫秒
                    return true;
                } else {
                    // Connection failed
                    if (error_msg) {
                        *error_msg = getDetailedErrorDescription(sock_error) + " (Error " + std::to_string(sock_error) + ")";
                    }
                    if (connection_time_ms) *connection_time_ms = -1;
                    return false;
                }
            } else {
                // getsockopt failed
                if (error_msg) {
                    *error_msg = "Failed to get socket error status";
                }
                if (connection_time_ms) *connection_time_ms = -1;
                return false;
            }
        }

        // Should not reach here
        if (connection_time_ms) *connection_time_ms = -1;
        return false;
    }

public:
    std::string getResolvedHost() const {
        return resolved_host_.empty() ? host_ : resolved_host_;
    }

private:
    std::string host_;
    int port_;
    std::string resolved_host_;
};

struct Config {
    std::string host;
    int port = 0;
    int interval = 1;
    int timeout = 3000;
    bool verbose = false;
};

class ArgumentParser {
public:
    static bool parseArguments(int argc, char* argv[], Config& config) {
        if (argc < 3 || argc > 7) {
            printUsage(argv[0]);
            return false;
        }

        config.host = argv[1];
        
        if (!parsePort(argv[2], config.port)) {
            return false;
        }

        for (int i = 3; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-i" && i + 1 < argc) {
                if (!parseInterval(argv[++i], config.interval)) {
                    return false;
                }
            } else if (arg == "-t" && i + 1 < argc) {
                if (!parseTimeout(argv[++i], config.timeout)) {
                    return false;
                }
            } else if (arg == "-v") {
                config.verbose = true;
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                printUsage(argv[0]);
                return false;
            }
        }

        return true;
    }

private:
    static void printUsage(const char* programName) {
        std::cout << "TCPing v" << VERSION << " by " << AUTHOR << std::endl;
        std::cerr << "Usage: " << programName << " <host> <port> [options]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Arguments:" << std::endl;
        std::cerr << "  <host>        Target host (IP address or domain name)" << std::endl;
        std::cerr << "  <port>        Target port (1-65535)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -i <seconds>  Check interval in seconds (default: 1)" << std::endl;
        std::cerr << "  -t <ms>       Connection timeout in milliseconds (default: 3000)" << std::endl;
        std::cerr << "  -v            Verbose mode (show detailed error messages)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << programName << " 192.168.1.1 80" << std::endl;
        std::cerr << "  " << programName << " google.com 443 -i 2 -t 5000" << std::endl;
    }

    static bool parsePort(const char* portStr, int& port) {
        try {
            port = std::stoi(portStr);
            if (port <= 0 || port > 65535) {
                std::cerr << "Port must be between 1 and 65535" << std::endl;
                return false;
            }
        } catch (const std::exception& /* e */) {
            std::cerr << "Invalid port number: " << portStr << std::endl;
            return false;
        }
        return true;
    }

    static bool parseInterval(const char* intervalStr, int& interval) {
        try {
            interval = std::stoi(intervalStr);
            if (interval <= 0) {
                std::cerr << "Interval must be positive" << std::endl;
                return false;
            }
        } catch (const std::exception& /* e */) {
            std::cerr << "Invalid interval value: " << intervalStr << std::endl;
            return false;
        }
        return true;
    }

    static bool parseTimeout(const char* timeoutStr, int& timeout) {
        try {
            timeout = std::stoi(timeoutStr);
            if (timeout <= 0) {
                std::cerr << "Timeout must be positive" << std::endl;
                return false;
            }
        } catch (const std::exception& /* e */) {
            std::cerr << "Invalid timeout value: " << timeoutStr << std::endl;
            return false;
        }
        return true;
    }
};

void printStartupInfo(const Config& config) {
    std::cout << "TCPing v" << VERSION << " by " << AUTHOR << std::endl;
    std::cout << "Starting continuous monitoring of " << config.host << ":" << config.port 
              << " with " << config.interval << "s interval and " << config.timeout 
              << "ms timeout. Press Ctrl+C to stop." << std::endl;
}

void printConnectionResult(const std::string& timestamp, const Config& config, 
                          bool success, double connectionTime, const std::string& errorMsg = "", 
                          const std::string& resolvedHost = "") {
    std::string displayHost = resolvedHost.empty() ? config.host : config.host + " (" + resolvedHost + ")";
    
    if (success) {
        std::cout << "[" << timestamp << "] " << displayHost << ":" << config.port 
                  << " - Connected (time=";
        if (connectionTime < 1.0) {
            std::cout << std::fixed << std::setprecision(2) << connectionTime << "ms)";
        } else {
            std::cout << static_cast<int>(connectionTime + 0.5) << "ms)";
        }
        std::cout << std::endl;
    } else {
        if (!errorMsg.empty()) {
            std::cout << "[" << timestamp << "] " << displayHost << ":" << config.port 
                      << " - Connection failed: " << errorMsg << std::endl;
        } else {
            std::cout << "[" << timestamp << "] " << displayHost << ":" << config.port 
                      << " - Connection failed" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    Config config;
    
    if (!ArgumentParser::parseArguments(argc, argv, config)) {
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Tcping tcping(config.host, config.port);
    
    printStartupInfo(config);
    
    while (keep_running) {
        double connection_time = -1.0;
        std::string error_msg;
        bool success = tcping.checkConnection(config.timeout, &connection_time, &error_msg);
        
        std::string timestamp = getCurrentTimestamp();
        std::string resolved_host = tcping.getResolvedHost();
        printConnectionResult(timestamp, config, success, connection_time, error_msg, resolved_host);
        
        if (keep_running) {
            std::this_thread::sleep_for(std::chrono::seconds(config.interval));
        }
    }
    std::cout << "\nMonitoring stopped." << std::endl;
    
    return 0;
}