#ifndef TCPING_STATISTICS_H
#define TCPING_STATISTICS_H

#include <limits>
#include <map>
#include <string>

#ifdef _WIN32
#  undef max
#  undef min
#endif

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

  void recordAttempt(bool success, double time_ms, ConnectionState state);
  double getAverageTime() const;
  double getSuccessRate() const;
};

struct PortStatistics {
  std::map<int, Statistics> port_stats;
  std::map<std::string, Statistics> host_stats;

  void recordHostAttempt(const std::string& host, int port, bool success,
                         double time_ms, ConnectionState state);
  void printSummary(bool show_all = false) const;
};

#endif // TCPING_STATISTICS_H
