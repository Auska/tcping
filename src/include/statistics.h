#ifndef TCPING_STATISTICS_H
#define TCPING_STATISTICS_H

#include <string>

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
    void printSummary() const;
};

#endif // TCPING_STATISTICS_H
