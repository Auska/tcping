#include "statistics.h"
#include <iomanip>
#include <iostream>
#include <limits>

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

void Statistics::printSummary() const {
  std::cout << "\n=== Connection Statistics ===" << std::endl;
  std::cout << "Total attempts:     " << total_attempts << std::endl;
  std::cout << "Successful:         " << successful_connections << " ("
            << std::fixed << std::setprecision(1) << getSuccessRate() << "%)"
            << std::endl;
  std::cout << "Failed:             " << failed_connections << std::endl;
  if (failed_connections > 0) {
    std::cout << "  - Timeouts:       " << timeout_count << std::endl;
    std::cout << "  - Refused:        " << refused_count << std::endl;
    std::cout << "  - Unreachable:    " << unreachable_count << std::endl;
    std::cout << "  - DNS failures:   " << dns_failure_count << std::endl;
  }
  if (successful_connections > 0) {
    std::cout << "\nResponse times (ms):" << std::endl;
    std::cout << "  Min:               " << std::fixed << std::setprecision(2)
              << min_time << std::endl;
    std::cout << "  Max:               " << std::fixed << std::setprecision(2)
              << max_time << std::endl;
    std::cout << "  Average:           " << std::fixed << std::setprecision(2)
              << getAverageTime() << std::endl;
  }
  std::cout << "===========================\n" << std::endl;
}

void PortStatistics::recordAttempt(int port, bool success, double time_ms,
                                   ConnectionState state) {
  port_stats[port].recordAttempt(success, time_ms, state);
}

void PortStatistics::printSummary() const {
  if (port_stats.empty()) {
    return;
  }

  std::cout << "\n--- Statistics ---" << std::endl;

  for (const auto& [port, stats] : port_stats) {
    std::cout << "port " << port << ": " << stats.successful_connections << "/"
              << stats.total_attempts << " (" << std::fixed
              << std::setprecision(1) << stats.getSuccessRate() << "%)";

    if (stats.successful_connections > 0) {
      std::cout << " min=" << std::setprecision(2) << stats.min_time
                << "ms avg=" << stats.getAverageTime()
                << "ms max=" << std::setprecision(2) << stats.max_time << "ms";
    }

    if (stats.failed_connections > 0) {
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

  std::cout << "------------------\n" << std::endl;
}
