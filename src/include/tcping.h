#ifndef TCPING_TCPING_H
#define TCPING_TCPING_H

#include "common.h"
#include "config.h"
#include "statistics.h"
#include <string>

class Tcping {
public:
  Tcping(const std::string& host, int port, bool ipv6 = false);
  Tcping(const std::string& host, int port, const std::string& resolved_ip,
         bool ipv6 = false);
  bool checkConnection(int timeout_ms = 3000,
                       double* connection_time_ms = nullptr,
                       std::string* error_msg = nullptr,
                       ConnectionState* connection_state = nullptr);
  std::string getResolvedHost() const;

private:
  bool resolveAndCache(const std::string& target);
  std::string host_;
  int port_;
  std::string resolved_host_;
  bool ipv6_ = false;
  bool cached_ = false;
  struct sockaddr_storage cached_addr_;
  socklen_t cached_addrlen_ = 0;
  int cached_family_ = 0;
};

void printStartupInfo(const Config& config);
void printConnectionResult(const std::string& timestamp,
                           const std::string& host, int port, bool success,
                           double connectionTime, const std::string& errorMsg,
                           const std::string& resolvedHost, bool verbose);
std::string getCurrentTimestamp();
std::string resolveHost(const std::string& host, bool ipv6);

#endif // TCPING_TCPING_H
