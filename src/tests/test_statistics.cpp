#include "catch_amalgamated.hpp"
#include "statistics.h"

TEST_CASE("Statistics - initial state", "[statistics]") {
  Statistics stats;

  SECTION("Default values") {
    REQUIRE(stats.total_attempts == 0);
    REQUIRE(stats.successful_connections == 0);
    REQUIRE(stats.failed_connections == 0);
    REQUIRE(stats.min_time > 1e18); // std::numeric_limits<double>::max()
    REQUIRE(stats.max_time == 0);
    REQUIRE(stats.total_time == 0);
    REQUIRE(stats.timeout_count == 0);
    REQUIRE(stats.refused_count == 0);
    REQUIRE(stats.unreachable_count == 0);
    REQUIRE(stats.dns_failure_count == 0);
  }
}

TEST_CASE("Statistics - recordAttempt", "[statistics]") {
  Statistics stats;

  SECTION("Record successful attempt") {
    stats.recordAttempt(true, 10.5, ConnectionState::Success);
    REQUIRE(stats.total_attempts == 1);
    REQUIRE(stats.successful_connections == 1);
    REQUIRE(stats.failed_connections == 0);
    REQUIRE(stats.min_time == 10.5);
    REQUIRE(stats.max_time == 10.5);
    REQUIRE(stats.total_time == 10.5);
  }

  SECTION("Record multiple successful attempts") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(true, 20.0, ConnectionState::Success);
    stats.recordAttempt(true, 5.0, ConnectionState::Success);

    REQUIRE(stats.total_attempts == 3);
    REQUIRE(stats.successful_connections == 3);
    REQUIRE(stats.min_time == 5.0);
    REQUIRE(stats.max_time == 20.0);
    REQUIRE(stats.total_time == 35.0);
  }

  SECTION("Record timeout failure") {
    stats.recordAttempt(false, -1.0, ConnectionState::Timeout);
    REQUIRE(stats.total_attempts == 1);
    REQUIRE(stats.successful_connections == 0);
    REQUIRE(stats.failed_connections == 1);
    REQUIRE(stats.timeout_count == 1);
  }

  SECTION("Record refused failure") {
    stats.recordAttempt(false, -1.0, ConnectionState::Refused);
    REQUIRE(stats.failed_connections == 1);
    REQUIRE(stats.refused_count == 1);
  }

  SECTION("Record unreachable failure") {
    stats.recordAttempt(false, -1.0, ConnectionState::Unreachable);
    REQUIRE(stats.failed_connections == 1);
    REQUIRE(stats.unreachable_count == 1);
  }

  SECTION("Record DNS failure") {
    stats.recordAttempt(false, -1.0, ConnectionState::DnsFailure);
    REQUIRE(stats.failed_connections == 1);
    REQUIRE(stats.dns_failure_count == 1);
  }

  SECTION("Record unknown failure") {
    stats.recordAttempt(false, -1.0, ConnectionState::Unknown);
    REQUIRE(stats.failed_connections == 1);
    REQUIRE(stats.timeout_count == 0);
    REQUIRE(stats.refused_count == 0);
    REQUIRE(stats.unreachable_count == 0);
    REQUIRE(stats.dns_failure_count == 0);
  }

  SECTION("Mixed successes and failures") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(false, -1.0, ConnectionState::Timeout);
    stats.recordAttempt(true, 15.0, ConnectionState::Success);
    stats.recordAttempt(false, -1.0, ConnectionState::Refused);

    REQUIRE(stats.total_attempts == 4);
    REQUIRE(stats.successful_connections == 2);
    REQUIRE(stats.failed_connections == 2);
    REQUIRE(stats.timeout_count == 1);
    REQUIRE(stats.refused_count == 1);
  }
}

TEST_CASE("Statistics - getAverageTime", "[statistics]") {
  Statistics stats;

  SECTION("No attempts - average is 0") {
    REQUIRE(stats.getAverageTime() == 0.0);
  }

  SECTION("Single successful attempt") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    REQUIRE(stats.getAverageTime() == 10.0);
  }

  SECTION("Multiple successful attempts") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(true, 20.0, ConnectionState::Success);
    stats.recordAttempt(true, 30.0, ConnectionState::Success);
    REQUIRE(stats.getAverageTime() == 20.0);
  }

  SECTION("Failures don't affect average") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(false, -1.0, ConnectionState::Timeout);
    stats.recordAttempt(true, 20.0, ConnectionState::Success);
    // Average of successful only: (10 + 20) / 2 = 15
    REQUIRE(stats.getAverageTime() == 15.0);
  }
}

TEST_CASE("Statistics - getSuccessRate", "[statistics]") {
  Statistics stats;

  SECTION("No attempts - rate is 0") {
    REQUIRE(stats.getSuccessRate() == 0.0);
  }

  SECTION("All successful") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(true, 20.0, ConnectionState::Success);
    REQUIRE(stats.getSuccessRate() == 100.0);
  }

  SECTION("All failed") {
    stats.recordAttempt(false, -1.0, ConnectionState::Timeout);
    stats.recordAttempt(false, -1.0, ConnectionState::Refused);
    REQUIRE(stats.getSuccessRate() == 0.0);
  }

  SECTION("Mixed results") {
    stats.recordAttempt(true, 10.0, ConnectionState::Success);
    stats.recordAttempt(false, -1.0, ConnectionState::Timeout);
    stats.recordAttempt(true, 20.0, ConnectionState::Success);
    stats.recordAttempt(false, -1.0, ConnectionState::Refused);
    // 2 out of 4 = 50%
    REQUIRE(stats.getSuccessRate() == 50.0);
  }
}

TEST_CASE("PortStatistics - recordHostAttempt", "[statistics]") {
  PortStatistics port_stats;

  SECTION("Record single attempt") {
    port_stats.recordHostAttempt("127.0.0.1", 80, true, 10.0,
                                ConnectionState::Success);

    REQUIRE(port_stats.host_stats.size() == 1);
    REQUIRE(port_stats.port_stats.size() == 1);
    REQUIRE(port_stats.host_stats["127.0.0.1"].successful_connections == 1);
    REQUIRE(port_stats.port_stats[80].successful_connections == 1);
  }

  SECTION("Record multiple hosts same port") {
    port_stats.recordHostAttempt("192.168.1.1", 80, true, 10.0,
                                ConnectionState::Success);
    port_stats.recordHostAttempt("192.168.1.2", 80, true, 15.0,
                                ConnectionState::Success);
    port_stats.recordHostAttempt("192.168.1.3", 80, false, -1.0,
                                ConnectionState::Timeout);

    REQUIRE(port_stats.host_stats.size() == 3);
    REQUIRE(port_stats.port_stats.size() == 1);
    REQUIRE(port_stats.port_stats[80].total_attempts == 3);
    REQUIRE(port_stats.port_stats[80].successful_connections == 2);
  }

  SECTION("Record same host multiple ports") {
    port_stats.recordHostAttempt("127.0.0.1", 80, true, 10.0,
                                ConnectionState::Success);
    port_stats.recordHostAttempt("127.0.0.1", 443, true, 15.0,
                                ConnectionState::Success);
    port_stats.recordHostAttempt("127.0.0.1", 22, false, -1.0,
                                ConnectionState::Refused);

    REQUIRE(port_stats.host_stats.size() == 1);
    REQUIRE(port_stats.port_stats.size() == 3);
    REQUIRE(port_stats.host_stats["127.0.0.1"].total_attempts == 3);
    REQUIRE(port_stats.host_stats["127.0.0.1"].successful_connections == 2);
  }
}

TEST_CASE("ConnectionState enum", "[statistics]") {
  SECTION("Enum values exist") {
    REQUIRE(static_cast<int>(ConnectionState::Success) !=
            static_cast<int>(ConnectionState::Timeout));
    REQUIRE(static_cast<int>(ConnectionState::Timeout) !=
            static_cast<int>(ConnectionState::Refused));
    REQUIRE(static_cast<int>(ConnectionState::Refused) !=
            static_cast<int>(ConnectionState::Unreachable));
    REQUIRE(static_cast<int>(ConnectionState::Unreachable) !=
            static_cast<int>(ConnectionState::DnsFailure));
  }
}
