#ifndef TCPING_TCPING_H
#define TCPING_TCPING_H

#include <string>
#include "statistics.h"
#include "config.h"

class Tcping {
public:
    Tcping(const std::string& host, int port);
    bool checkConnection(int timeout_ms = 3000, double* connection_time_ms = nullptr, 
                         std::string* error_msg = nullptr, ConnectionState* connection_state = nullptr);
    std::string getResolvedHost() const;

private:
    std::string host_;
    int port_;
    std::string resolved_host_;
};

void printStartupInfo(const Config& config);
void printConnectionResult(const std::string& timestamp, const Config& config,
                          bool success, double connectionTime, const std::string& errorMsg,
                          const std::string& resolvedHost);
std::string getCurrentTimestamp();

#endif // TCPING_TCPING_H