#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

#include "include/version.h"
#include "include/config.h"
#include "include/statistics.h"
#include "include/args.h"
#include "include/tcping.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif
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
        if (config.count > 0 && attempt_count >= config.count) {
            break;
        }
        
        double connection_time = -1.0;
        std::string error_msg;
        ConnectionState connection_state = ConnectionState::Unknown;
        bool success = tcping.checkConnection(config.timeout, &connection_time, &error_msg, &connection_state);
        
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
    std::cout.flush();
    
    if (config.show_statistics && stats.total_attempts > 0) {
        stats.printSummary();
    }
    
    return 0;
}
