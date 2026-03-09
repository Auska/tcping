#ifndef TCPING_TCPING_H
#define TCPING_TCPING_H

#include <string>
#include "config.h"
#include "statistics.h"

class Tcping {
public:
  Tcping(const std::string& host, int port, bool ipv6 = false);
  // 接受已解析的IP地址，避免重复DNS解析
  Tcping(const std::string& host, int port, const std::string& resolved_ip,
         bool ipv6 = false);
  bool checkConnection(int timeout_ms = 3000,
                       double* connection_time_ms = nullptr,
                       std::string* error_msg = nullptr,
                       ConnectionState* connection_state = nullptr);
  std::string getResolvedHost() const;

private:
  std::string host_;
  int port_;
  std::string resolved_host_;
  bool ipv6_ = false;
};

void printStartupInfo(const Config& config);
void printConnectionResult(const std::string& timestamp,
                           const std::string& host, int port, bool success,
                           double connectionTime, const std::string& errorMsg,
                           const std::string& resolvedHost, bool verbose);
std::string getCurrentTimestamp();

// Resolve host to IP address (DNS lookup)
std::string resolveHost(const std::string& host, bool ipv6);

#endif // TCPING_TCPING_H
