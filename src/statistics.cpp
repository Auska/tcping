#include "statistics.h"
#include <iomanip>
#include <iostream>
#include <string>

namespace {
void printStatsEntry(const std::string& label, const Statistics& stats,
                     bool show_all) {
  std::cout << label << ": " << stats.successful_connections << "/"
            << stats.total_attempts << " (" << std::fixed
            << std::setprecision(1) << stats.getSuccessRate() << "%)";

  if (stats.successful_connections > 0) {
    std::cout << " min=" << std::setprecision(2) << stats.min_time
              << "ms avg=" << stats.getAverageTime() << "ms max="
              << std::setprecision(2) << stats.max_time << "ms";
  }

  if (stats.failed_connections > 0 && show_all) {
    std::cout << " [";
    bool first = true;
    if (stats.timeout_count > 0) {
      std::cout << "timeout=" << stats.timeout_count;
      first = false;
    }
    if (stats.refused_count > 0) {
      if (!first)
        std::cout << ", ";
      std::cout << "refused=" << stats.refused_count;
      first = false;
    }
    if (stats.unreachable_count > 0) {
      if (!first)
        std::cout << ", ";
      std::cout << "unreachable=" << stats.unreachable_count;
      first = false;
    }
    if (stats.dns_failure_count > 0) {
      if (!first)
        std::cout << ", ";
      std::cout << "dns=" << stats.dns_failure_count;
    }
    std::cout << "]";
  }

  std::cout << std::endl;
}
} // namespace

void Statistics::recordAttempt(bool success, double time_ms,
                               ConnectionState state) {
  total_attempts++;
  if (success) {
    successful_connections++;
    total_time += time_ms;
    if (time_ms < min_time)
      min_time = time_ms;
    if (time_ms > max_time)
      max_time = time_ms;
  } else {
    failed_connections++;
    switch (state) {
      case ConnectionState::Timeout:
        timeout_count++;
        break;
      case ConnectionState::Refused:
        refused_count++;
        break;
      case ConnectionState::Unreachable:
        unreachable_count++;
        break;
      case ConnectionState::DnsFailure:
        dns_failure_count++;
        break;
      default:
        break;
    }
  }
}

double Statistics::getAverageTime() const {
  return successful_connections > 0 ? total_time / successful_connections : 0;
}

double Statistics::getSuccessRate() const {
  return total_attempts > 0 ? (successful_connections * 100.0) / total_attempts
                            : 0;
}

void PortStatistics::recordHostAttempt(const std::string& host, int port,
                                       bool success, double time_ms,
                                       ConnectionState state) {
  port_stats[port].recordAttempt(success, time_ms, state);
  host_stats[host].recordAttempt(success, time_ms, state);
}

void PortStatistics::printSummary(bool show_all) const {
  bool has_successful = false;
  bool has_any = false;
  for (const auto& [host, stats] : host_stats) {
    (void)host;
    if (stats.successful_connections > 0) {
      has_successful = true;
    }
    if (stats.total_attempts > 0) {
      has_any = true;
    }
  }
  for (const auto& [port, stats] : port_stats) {
    (void)port;
    if (stats.successful_connections > 0) {
      has_successful = true;
    }
    if (stats.total_attempts > 0) {
      has_any = true;
    }
  }

  if (!has_successful && !show_all) {
    return;
  }
  if (!has_any) {
    return;
  }

  if (!host_stats.empty()) {
    std::cout << "\n--- Host Statistics ---" << std::endl;
    for (const auto& [host, stats] : host_stats) {
      if (!show_all && stats.successful_connections == 0) {
        continue;
      }
      printStatsEntry("host " + host, stats, show_all);
    }
    std::cout << "-----------------------\n" << std::endl;
  }

  if (!port_stats.empty()) {
    std::cout << "\n--- Port Statistics ---" << std::endl;
    for (const auto& [port, stats] : port_stats) {
      if (!show_all && stats.successful_connections == 0) {
        continue;
      }
      printStatsEntry("port " + std::to_string(port), stats, show_all);
    }
    std::cout << "-----------------------\n" << std::endl;
  }
}
