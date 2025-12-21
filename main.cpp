#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>

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

    bool checkConnection(int timeout_ms = 3000, int* connection_time_ms = nullptr) {
#ifdef _WIN32
        SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == INVALID_SOCKET) {
            std::cerr << "Error creating socket: " << GET_ERROR_MSG() << std::endl;
            return false;
        }

        // Set socket to non-blocking mode
        u_long mode = 1;
        if (ioctlsocket(sockfd, FIONBIO, &mode) != 0) {
            std::cerr << "Error setting non-blocking mode: " << GET_ERROR_MSG() << std::endl;
            CLOSE_SOCKET(sockfd);
            return false;
        }
#else
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket: " << GET_ERROR_MSG() << std::endl;
            return false;
        }

        // Set socket to non-blocking mode
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<u_short>(port_));
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid IP address format: " << host_ << std::endl;
            CLOSE_SOCKET(sockfd);
            return false;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        
        int result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        if (result == 0) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            if (connection_time_ms) *connection_time_ms = static_cast<int>(duration.count());
            CLOSE_SOCKET(sockfd);
            return true;
        } else if (SOCKET_ERROR_CODE == 
#ifdef _WIN32
                   WSAEWOULDBLOCK
#else
                   EINPROGRESS
#endif
                  ) {
            fd_set write_fds;
            struct timeval timeout;
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;

            FD_ZERO(&write_fds);
            FD_SET(sockfd, &write_fds);

            result = select(
#ifdef _WIN32
                    0
#else
                    sockfd + 1
#endif
                    , NULL, &write_fds, NULL, &timeout);
            
            if (result > 0) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
                
                if (so_error == 0) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    if (connection_time_ms) *connection_time_ms = static_cast<int>(duration.count());
                    CLOSE_SOCKET(sockfd);
                    return true;
                } else {
                    #ifdef _WIN32
                    char errorBuf[256];
                    strerror_s(errorBuf, sizeof(errorBuf), so_error);
                    std::cout << "Connection to " << host_ << ":" << port_ << " failed: " << errorBuf << std::endl;
#else
                    std::cout << "Connection to " << host_ << ":" << port_ << " failed: " << strerror(so_error) << std::endl;
#endif
                    CLOSE_SOCKET(sockfd);
                    return false;
                }
            } else if (result == 0) {
                std::cout << "Connection to " << host_ << ":" << port_ << " timed out" << std::endl;
                CLOSE_SOCKET(sockfd);
                return false;
            } else {
                std::cerr << "Select error: " << GET_ERROR_MSG() << std::endl;
                CLOSE_SOCKET(sockfd);
                return false;
            }
        } else {
            std::cout << "Connection to " << host_ << ":" << port_ << " failed: " << GET_ERROR_MSG() << std::endl;
            CLOSE_SOCKET(sockfd);
            return false;
        }
    }

private:
    std::string host_;
    int port_;
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> [-i <interval_seconds>]" << std::endl;
        std::cerr << "  -i: Check interval in seconds (default: 1)" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port;
    int interval = 1;
    
    try {
        port = std::stoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Port must be between 1 and 65535" << std::endl;
            return 1;
        }
    } catch (const std::exception& /* e */) {
        std::cerr << "Invalid port number: " << argv[2] << std::endl;
        return 1;
    }

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            try {
                interval = std::stoi(argv[++i]);
                if (interval <= 0) {
                    std::cerr << "Interval must be positive" << std::endl;
                    return 1;
                }
            } catch (const std::exception& /* e */) {
                std::cerr << "Invalid interval value: " << argv[i] << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Tcping tcping(host, port);
    
    std::cout << "Starting continuous monitoring of " << host << ":" << port 
              << " with " << interval << "s interval. Press Ctrl+C to stop." << std::endl;
    
    while (keep_running) {
        int connection_time = -1;
        bool success = tcping.checkConnection(3000, &connection_time);
        
        std::string timestamp = getCurrentTimestamp();
        if (success) {
            std::cout << "[" << timestamp << "] " << host << ":" << port 
                      << " - Connected (time=" << connection_time << "ms)" << std::endl;
        } else {
            std::cout << "[" << timestamp << "] " << host << ":" << port 
                      << " - Connection failed" << std::endl;
        }
        
        if (keep_running) {
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
    }
    std::cout << "\nMonitoring stopped." << std::endl;
    
    return 0;
}