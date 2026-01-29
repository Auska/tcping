#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/types.h>

#include "tcping.h"
#include "error.h"
#include "version.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <netdb.h>
#include <sys/select.h>
#endif

#ifdef _WIN32
class SocketGuard {
public:
    explicit SocketGuard(SOCKET sock) : sock_(sock) {}
    ~SocketGuard() {
        if (sock_ != INVALID_SOCKET) {
            closesocket(sock_);
        }
    }
    SOCKET get() const { return sock_; }
    SOCKET release() {
        SOCKET s = sock_;
        sock_ = INVALID_SOCKET;
        return s;
    }
private:
    SOCKET sock_;
};
#else
class SocketGuard {
public:
    explicit SocketGuard(int sock) : sock_(sock) {}
    ~SocketGuard() {
        if (sock_ >= 0) {
            close(sock_);
        }
    }
    int get() const { return sock_; }
    int release() {
        int s = sock_;
        sock_ = -1;
        return s;
    }
private:
    int sock_;
};
#endif

Tcping::Tcping(const std::string& host, int port, bool ipv6)
    : host_(host), port_(port), ipv6_(ipv6) {
    
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (status != 0) {
        resolved_host_ = host_;
        return;
    }
    
    char ipstr[INET6_ADDRSTRLEN];
    void* addr;
    
    if (result->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)result->ai_addr;
        addr = &(ipv4->sin_addr);
    } else {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)result->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    
    inet_ntop(result->ai_family, addr, ipstr, sizeof(ipstr));
    resolved_host_ = std::string(ipstr);
    
    freeaddrinfo(result);
}

bool Tcping::checkConnection(int timeout_ms, double* connection_time_ms,
                             std::string* error_msg, ConnectionState* connection_state) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv6_ ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &result);
    if (status != 0) {
        if (error_msg) *error_msg = "DNS resolution failed";
        if (connection_state) *connection_state = ConnectionState::DnsFailure;
        return false;
    }
    
#ifdef _WIN32
    SOCKET sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd == INVALID_SOCKET) {
        freeaddrinfo(result);
        if (error_msg) *error_msg = "Failed to create socket";
        if (connection_state) *connection_state = ConnectionState::Unknown;
        return false;
    }
#else
    int sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(result);
        if (error_msg) *error_msg = "Failed to create socket";
        if (connection_state) *connection_state = ConnectionState::Unknown;
        return false;
    }
#endif
    
    SocketGuard guard(sockfd);
    
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
    
    status = connect(sockfd, result->ai_addr, result->ai_addrlen);
    
    if (status == 0) {
        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        if (connection_time_ms) *connection_time_ms = elapsed;
        if (connection_state) *connection_state = ConnectionState::Success;
        freeaddrinfo(result);
        return true;
    }
    
#ifdef _WIN32
    int error_code = WSAGetLastError();
    if (error_code != WSAEWOULDBLOCK) {
        freeaddrinfo(result);
        if (error_msg) *error_msg = getDetailedErrorDescription(error_code);
        if (connection_state) *connection_state = getConnectionState(error_code);
        return false;
    }
#else
    if (errno != EINPROGRESS) {
        int error_code = errno;
        freeaddrinfo(result);
        if (error_msg) *error_msg = getDetailedErrorDescription(error_code);
        if (connection_state) *connection_state = getConnectionState(error_code);
        return false;
    }
#endif
    
    fd_set write_fds, except_fds;
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    
#ifdef _WIN32
    FD_SET(sockfd, &write_fds);
    FD_SET(sockfd, &except_fds);
#else
    FD_SET(sockfd, &write_fds);
    FD_SET(sockfd, &except_fds);
#endif
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    status = select(
#ifdef _WIN32
        0,
#else
        sockfd + 1,
#endif
        nullptr, &write_fds, &except_fds, &tv);
    
    if (status <= 0) {
        freeaddrinfo(result);
        if (error_msg) *error_msg = "Connection timed out";
        if (connection_state) *connection_state = ConnectionState::Timeout;
        return false;
    }
    
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
        freeaddrinfo(result);
        if (error_msg) *error_msg = getDetailedErrorDescription(error);
        if (connection_state) *connection_state = getConnectionState(error);
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    if (connection_time_ms) *connection_time_ms = elapsed;
    if (connection_state) *connection_state = ConnectionState::Success;
    
    freeaddrinfo(result);
    return true;
}

std::string Tcping::getResolvedHost() const {
    return resolved_host_;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void printStartupInfo(const Config& config) {
    std::cout << "TCPing v" << VERSION << " by " << AUTHOR << std::endl;
    std::cout << "Starting continuous monitoring of " << config.host << ":" << config.port
              << " with " << config.interval << "s interval and " << config.timeout << "ms timeout";
    if (config.ipv6) {
        std::cout << " (IPv6)";
    }
    std::cout << ". Press Ctrl+C to stop." << std::endl;
    std::cout.flush();
}

void printConnectionResult(const std::string& timestamp, const Config& config,
                          bool success, double connectionTime, const std::string& errorMsg,
                          const std::string& resolvedHost) {
    std::cout << "[" << timestamp << "] " << config.host << ":" << config.port;
    
    if (resolvedHost != config.host) {
        std::cout << " (" << resolvedHost << ")";
    }
    
    if (success) {
        std::cout << " - Connected (time=" << std::fixed << std::setprecision(0) << connectionTime << "ms)";
    } else {
        std::cout << " - Connection failed";
        if (config.verbose && !errorMsg.empty()) {
            std::cout << ": " << errorMsg;
        }
    }
    
    std::cout << std::endl;
    std::cout.flush();
}