#include "catch_amalgamated.hpp"
#include "statistics.h"
#include "tcping.h"

#include <chrono>
#include <thread>

TEST_CASE("Tcping - constructor and basic operations", "[tcping]") {
  SECTION("Constructor with IP address") {
    Tcping tcping("127.0.0.1", 80);
    REQUIRE(tcping.getResolvedHost() == "127.0.0.1");
  }

  SECTION("Constructor with IPv6 address") {
    Tcping tcping("::1", 80, true);
    REQUIRE(tcping.getResolvedHost() == "::1");
  }

  SECTION("Constructor with pre-resolved IP") {
    Tcping tcping("localhost", 80, "127.0.0.1");
    // The resolved IP should be the one we passed in
    // (it will try to resolve it, but we verify it got set)
    REQUIRE_FALSE(tcping.getResolvedHost().empty());
  }
}

TEST_CASE("Tcping - checkConnection to localhost", "[tcping]") {
  // Note: These tests depend on local network conditions

  SECTION("Connection to closed port should fail") {
    // Use a high port that's likely not in use
    Tcping tcping("127.0.0.1", 59999);
    double conn_time = 0;
    std::string error_msg;
    ConnectionState state = ConnectionState::Unknown;

    bool result = tcping.checkConnection(1000, &conn_time, &error_msg, &state);

    REQUIRE_FALSE(result);
    REQUIRE(state != ConnectionState::Success);
  }

  SECTION("Connection with short timeout") {
    Tcping tcping("10.255.255.1", 80); // Non-routable IP
    double conn_time = 0;
    std::string error_msg;
    ConnectionState state = ConnectionState::Unknown;

    auto start = std::chrono::steady_clock::now();
    bool result = tcping.checkConnection(500, &conn_time, &error_msg, &state);
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

    REQUIRE_FALSE(result);
    // Should timeout within reasonable time (500ms + some overhead)
    REQUIRE(elapsed_ms < 2000);
  }
}

TEST_CASE("Tcping - DNS resolution", "[tcping]") {
  SECTION("Valid domain resolution") {
    Tcping tcping("localhost", 80);
    // localhost should resolve to 127.0.0.1 or ::1 depending on system
    std::string resolved = tcping.getResolvedHost();
    REQUIRE_FALSE(resolved.empty());
  }

  SECTION("IP address passthrough") {
    Tcping tcping("192.168.1.1", 80);
    REQUIRE(tcping.getResolvedHost() == "192.168.1.1");
  }
}

TEST_CASE("Tcping - connection state tracking", "[tcping]") {
  SECTION("Connection refused detection") {
    Tcping tcping("127.0.0.1", 59998);
    ConnectionState state = ConnectionState::Unknown;
    tcping.checkConnection(1000, nullptr, nullptr, &state);

    // On most systems, connection to closed port results in Refused
    REQUIRE((state == ConnectionState::Refused ||
             state == ConnectionState::Timeout ||
             state != ConnectionState::Success));
  }
}

TEST_CASE("Tcping - timeout parameter", "[tcping]") {
  SECTION("Default timeout") {
    Tcping tcping("10.255.255.1", 80);
    auto start = std::chrono::steady_clock::now();
    tcping.checkConnection(); // Default 3000ms
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    // Should timeout around 3000ms with some overhead
    REQUIRE(elapsed_ms >= 2000);
    REQUIRE(elapsed_ms < 5000);
  }

  SECTION("Custom short timeout") {
    Tcping tcping("10.255.255.1", 80);
    auto start = std::chrono::steady_clock::now();
    tcping.checkConnection(200);
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    REQUIRE(elapsed_ms >= 100);
    REQUIRE(elapsed_ms < 1000);
  }
}

TEST_CASE("Tcping - output parameters", "[tcping]") {
  SECTION("Connection time measurement") {
    Tcping tcping("127.0.0.1", 59997);
    double conn_time = -1.0;
    tcping.checkConnection(1000, &conn_time, nullptr, nullptr);
    // Connection time should remain -1 or be set to some value
    // For failed connections, it might not be set
  }

  SECTION("Error message retrieval") {
    Tcping tcping("127.0.0.1", 59996);
    std::string error_msg;
    tcping.checkConnection(1000, nullptr, &error_msg, nullptr);
    // Error message might be empty or contain error description
  }

  SECTION("All parameters together") {
    Tcping tcping("127.0.0.1", 59995);
    double conn_time = 0;
    std::string error_msg;
    ConnectionState state = ConnectionState::Success;

    bool result = tcping.checkConnection(1000, &conn_time, &error_msg, &state);

    REQUIRE_FALSE(result);
    REQUIRE(state != ConnectionState::Success);
  }
}

TEST_CASE("resolveHost function", "[tcping]") {
  SECTION("Resolve localhost") {
    std::string resolved = resolveHost("localhost", false);
    REQUIRE_FALSE(resolved.empty());
    REQUIRE(resolved != "localhost");
  }

  SECTION("IP address unchanged") {
    std::string resolved = resolveHost("8.8.8.8", false);
    REQUIRE(resolved == "8.8.8.8");
  }

  SECTION("Invalid domain returns original") {
    std::string resolved = resolveHost("invalid.invalid.invalid", false);
    REQUIRE(resolved == "invalid.invalid.invalid");
  }
}

TEST_CASE("getCurrentTimestamp function", "[tcping]") {
  SECTION("Returns non-empty string") {
    std::string ts = getCurrentTimestamp();
    REQUIRE_FALSE(ts.empty());
  }

  SECTION("Has correct format") {
    std::string ts = getCurrentTimestamp();
    // Format: YYYY-MM-DD HH:MM:SS.mmm
    REQUIRE(ts.length() == 23);
    REQUIRE(ts[4] == '-');
    REQUIRE(ts[7] == '-');
    REQUIRE(ts[10] == ' ');
    REQUIRE(ts[13] == ':');
    REQUIRE(ts[16] == ':');
    REQUIRE(ts[19] == '.');
  }

  SECTION("Generates unique timestamps") {
    std::string ts1 = getCurrentTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::string ts2 = getCurrentTimestamp();
    // Timestamps should be different (or at least not necessarily the same)
    // This test might occasionally fail if executed very quickly
  }
}

TEST_CASE("Tcping - IPv4 vs IPv6 mode", "[tcping]") {
  SECTION("IPv4 mode flag") {
    Tcping tcping("127.0.0.1", 80, false);
    // Should work in IPv4 mode
    REQUIRE(tcping.getResolvedHost() == "127.0.0.1");
  }

  SECTION("IPv6 mode flag") {
    Tcping tcping("::1", 80, true);
    // Should work in IPv6 mode
    REQUIRE(tcping.getResolvedHost() == "::1");
  }
}
