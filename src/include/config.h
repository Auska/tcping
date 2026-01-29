#ifndef TCPING_CONFIG_H
#define TCPING_CONFIG_H

#include <string>

struct Config {
    std::string host;
    int port = 0;
    int interval = 1;
    int timeout = 3000;
    bool verbose = false;
    bool show_statistics = true;
    int count = 0;
    bool ipv6 = false;  // Force IPv6 mode
};

#endif // TCPING_CONFIG_H