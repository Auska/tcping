#ifndef TCPING_COMMON_H
#define TCPING_COMMON_H

// C++ standard library headers
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <limits>

// Platform-specific headers
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
    #include <sys/select.h>
    #include <sys/types.h>
#endif

#endif // TCPING_COMMON_H
