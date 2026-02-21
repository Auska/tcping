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
