#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>
#include <limits>
#include <algorithm>

// Version and author information
const std::string VERSION = "1.0.0";
const std::string AUTHOR = "Luodan <luodan0709@live.cn>";

// Connection state enumeration for better error handling
enum class ConnectionState {
    Success,
    Timeout,
    Refused,
    Unreachable,
    NetworkDown,
    DnsFailure,
    ConnectionReset,
    ConnectionAborted,
    AddressInUse,
    AccessDenied,
    Unknown
};

// Statistics structure for tracking connection results
struct Statistics {
    int total_attempts = 0;
    int successful_connections = 0;
    int failed_connections = 0;
    double min_time = std::numeric_limits<double>::max();
    double max_time = 0;
    double total_time = 0;
    int timeout_count = 0;
    int refused_count = 0;
    int unreachable_count = 0;
    int dns_failure_count = 0;

    void recordAttempt(bool success, double time_ms, ConnectionState state) {
        total_attempts++;
        if (success) {
            successful_connections++;
            total_time += time_ms;
            if (time_ms < min_time) min_time = time_ms;
            if (time_ms > max_time) max_time = time_ms;
        } else {
            failed_connections++;
            switch (state) {
                case ConnectionState::Timeout:
                    timeout_count++;
                    break;
                case ConnectionState::Refused:
                    refused_count++;
                    break;
                case ConnectionState::Unreachable:
                    unreachable_count++;
                    break;
                case ConnectionState::DnsFailure:
                    dns_failure_count++;
                    break;
                default:
                    break;
            }
        }
    }

    double getAverageTime() const {
        return successful_connections > 0 ? total_time / successful_connections : 0;
    }

    double getSuccessRate() const {
        return total_attempts > 0 ? (successful_connections * 100.0) / total_attempts : 0;
    }

    void printSummary() const {
        std::cout << "\n=== Connection Statistics ===" << std::endl;
        std::cout << "Total attempts:     " << total_attempts << std::endl;
        std::cout << "Successful:         " << successful_connections 
                  << " (" << std::fixed << std::setprecision(1) << getSuccessRate() << "%)" << std::endl;
        std::cout << "Failed:             " << failed_connections << std::endl;
        if (failed_connections > 0) {
            std::cout << "  - Timeouts:       " << timeout_count << std::endl;
            std::cout << "  - Refused:        " << refused_count << std::endl;
            std::cout << "  - Unreachable:    " << unreachable_count << std::endl;
            std::cout << "  - DNS failures:   " << dns_failure_count << std::endl;
        }
        if (successful_connections > 0) {
            std::cout << "\nResponse times (ms):" << std::endl;
            std::cout << "  Min:               " << std::fixed << std::setprecision(2) << min_time << std::endl;
            std::cout << "  Max:               " << std::fixed << std::setprecision(2) << max_time << std::endl;
            std::cout << "  Average:           " << std::fixed << std::setprecision(2) << getAverageTime() << std::endl;
        }
        std::cout << "===========================\n" << std::endl;
    }
};

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

ConnectionState getConnectionState(int errorCode) {
#ifdef _WIN32
    switch (errorCode) {
        case WSAECONNREFUSED:
            return ConnectionState::Refused;
        case WSAETIMEDOUT:
            return ConnectionState::Timeout;
        case WSAEHOSTUNREACH:
            return ConnectionState::Unreachable;
        case WSAENETUNREACH:
            return ConnectionState::Unreachable;
        case WSAENETDOWN:
            return ConnectionState::NetworkDown;
        case WSAECONNABORTED:
            return ConnectionState::ConnectionAborted;
        case WSAECONNRESET:
            return ConnectionState::ConnectionReset;
        case WSAEADDRINUSE:
            return ConnectionState::AddressInUse;
        case WSAEACCES:
            return ConnectionState::AccessDenied;
        default:
            return ConnectionState::Unknown;
    }
#else
    switch (errorCode) {
        case ECONNREFUSED:
            return ConnectionState::Refused;
        case ETIMEDOUT:
            return ConnectionState::Timeout;
        case EHOSTUNREACH:
            return ConnectionState::Unreachable;
        case ENETUNREACH:
            return ConnectionState::Unreachable;
        case ENETDOWN:
            return ConnectionState::NetworkDown;
        case ECONNABORTED:
            return ConnectionState::ConnectionAborted;
        case ECONNRESET:
            return ConnectionState::ConnectionReset;
        case EADDRINUSE:
            return ConnectionState::AddressInUse;
        case EACCES:
            return ConnectionState::AccessDenied;
        case EPERM:
            return ConnectionState::AccessDenied;
        default:
            return ConnectionState::Unknown;
    }
#endif
}

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

std::string getConnectionStateString(ConnectionState state) {
    switch (state) {
        case ConnectionState::Success:
            return "Success";
        case ConnectionState::Timeout:
            return "Timeout";
        case ConnectionState::Refused:
            return "Connection refused";
        case ConnectionState::Unreachable:
            return "Host unreachable";
        case ConnectionState::NetworkDown:
            return "Network down";
        case ConnectionState::DnsFailure:
            return "DNS resolution failed";
        case ConnectionState::ConnectionReset:
            return "Connection reset";
        case ConnectionState::ConnectionAborted:
            return "Connection aborted";
        case ConnectionState::AddressInUse:
            return "Address in use";
        case ConnectionState::AccessDenied:
            return "Access denied";
        default:
            return "Unknown error";
    }
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

    bool checkConnection(int timeout_ms = 3000, double* connection_time_ms = nullptr, std::string* error_msg = nullptr, ConnectionState* connection_state = nullptr) {
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
                if (connection_state) *connection_state = ConnectionState::DnsFailure;
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
            if (connection_state) *connection_state = getConnectionState(errorCode);
            if (connection_time_ms) *connection_time_ms = -1;
            if (error_msg) {
                *error_msg = getDetailedErrorDescription(errorCode) + " (Error " + std::to_string(errorCode) + ")";
            }
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
                if (connection_state) *connection_state = ConnectionState::Timeout;
                if (error_msg) {
                    *error_msg = "Connection timed out after " + std::to_string(timeout_ms) + "ms";
                }
            } else {
                errorCode = SOCKET_ERROR_CODE;
                if (connection_state) *connection_state = getConnectionState(errorCode);
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
                if (connection_state) *connection_state = getConnectionState(sock_error);
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
                    if (connection_state) *connection_state = ConnectionState::Success;
                    return true;
                } else {
                    // Connection failed
                    if (connection_state) *connection_state = getConnectionState(sock_error);
                    if (error_msg) {
                        *error_msg = getDetailedErrorDescription(sock_error) + " (Error " + std::to_string(sock_error) + ")";
                    }
                    if (connection_time_ms) *connection_time_ms = -1;
                    return false;
                }
            } else {
                // getsockopt failed
                if (connection_state) *connection_state = ConnectionState::Unknown;
                if (error_msg) {
                    *error_msg = "Failed to get socket error status";
                }
                if (connection_time_ms) *connection_time_ms = -1;
                return false;
            }
        }

        // Should not reach here
        if (connection_state) *connection_state = ConnectionState::Unknown;
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
    bool show_statistics = true;  // 默认显示统计信息
    int count = 0;  // 0 表示无限次，>0 表示连接次数限制
};

class ArgumentParser {
public:
    static bool parseArguments(int argc, char* argv[], Config& config) {
        if (argc < 3) {
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
            } else if (arg == "-s") {
                config.show_statistics = true;
            } else if (arg == "-S") {
                config.show_statistics = false;
            } else if (arg == "-c" && i + 1 < argc) {
                if (!parseCount(argv[++i], config.count)) {
                    return false;
                }
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
        std::cerr << "  -c <count>    Number of connection attempts (default: unlimited)" << std::endl;
        std::cerr << "  -s            Show statistics summary when stopped (default: enabled)" << std::endl;
        std::cerr << "  -S            Hide statistics summary" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << programName << " 192.168.1.1 80" << std::endl;
        std::cerr << "  " << programName << " google.com 443 -i 2 -t 5000" << std::endl;
        std::cerr << "  " << programName << " 127.0.0.1 22 -c 10" << std::endl;
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

    static bool parseCount(const char* countStr, int& count) {
        try {
            count = std::stoi(countStr);
            if (count < 0) {
                std::cerr << "Count must be non-negative" << std::endl;
                return false;
            }
        } catch (const std::exception& /* e */) {
            std::cerr << "Invalid count value: " << countStr << std::endl;
            return false;
        }
        return true;
    }
};

void printStartupInfo(const Config& config) {
    std::cout << "TCPing v" << VERSION << " by " << AUTHOR << std::endl;
    if (config.count > 0) {
        std::cout << "Starting " << config.count << " connection attempts to " << config.host << ":" << config.port 
                  << " with " << config.interval << "s interval and " << config.timeout 
                  << "ms timeout. Press Ctrl+C to stop." << std::endl;
    } else {
        std::cout << "Starting continuous monitoring of " << config.host << ":" << config.port 
                  << " with " << config.interval << "s interval and " << config.timeout 
                  << "ms timeout. Press Ctrl+C to stop." << std::endl;
    }
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
    Statistics stats;
    
    printStartupInfo(config);
    
    int attempt_count = 0;
    while (keep_running) {
        // 检查是否达到指定的连接次数
        if (config.count > 0 && attempt_count >= config.count) {
            break;
        }
        
        double connection_time = -1.0;
        std::string error_msg;
        ConnectionState connection_state = ConnectionState::Unknown;
        bool success = tcping.checkConnection(config.timeout, &connection_time, &error_msg, &connection_state);
        
        // 记录统计信息
        stats.recordAttempt(success, connection_time, connection_state);
        attempt_count++;
        
        std::string timestamp = getCurrentTimestamp();
        std::string resolved_host = tcping.getResolvedHost();
        printConnectionResult(timestamp, config, success, connection_time, error_msg, resolved_host);
        
        if (keep_running && (config.count == 0 || attempt_count < config.count)) {
            std::this_thread::sleep_for(std::chrono::seconds(config.interval));
        }
    }
    
    std::cout << "\nMonitoring stopped." << std::endl;
    
    // 显示统计信息
    if (config.show_statistics && stats.total_attempts > 0) {
        stats.printSummary();
    }
    
    return 0;
}