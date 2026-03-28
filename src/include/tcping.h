#ifndef TCPING_TCPING_H
#define TCPING_TCPING_H

#include <string>
#include "common.h"
#include "config.h"
#include "statistics.h"

class Tcping {
public:
  Tcping(const std::string& host, int port, bool ipv6 = false);
  Tcping(std::string  host, int port, const std::string& resolved_ip,
         bool ipv6 = false);
  auto checkConnection(int timeout_ms = 3000,
                       double* connection_time_ms = nullptr,
                       std::string* error_msg = nullptr,
                       ConnectionState* connection_state = nullptr) -> bool;
  [[nodiscard]] auto getResolvedHost() const -> std::string;

private:
  auto resolveAndCache(const std::string& target) -> bool;
  std::string host_;
  int port_;
  std::string resolved_host_;
  bool ipv6_ = false;
  bool cached_ = false;
  struct sockaddr_storage cached_addr_{};
  socklen_t cached_addrlen_ = 0;
  int cached_family_ = 0;
};

void printStartupInfo(const Config& config);
void printConnectionResult(const std::string& timestamp,
                           const std::string& host, int port, bool success,
                           double connection_time, const std::string& error_msg,
                           const std::string& resolved_host, bool verbose);
auto getCurrentTimestamp() -> std::string;
auto resolveHost(const std::string& host, bool ipv6) -> std::string;

#endif // TCPING_TCPING_H
