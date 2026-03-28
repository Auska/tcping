#include "catch_amalgamated.hpp"
#include "error.h"
#include "statistics.h"

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <cerrno>
#endif

TEST_CASE("getConnectionState - maps error codes correctly", "[error]") {
  SECTION("Timeout error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAETIMEDOUT) == ConnectionState::Timeout);
#else
    REQUIRE(getConnectionState(ETIMEDOUT) == ConnectionState::Timeout);
#endif
  }

  SECTION("Connection refused error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAECONNREFUSED) == ConnectionState::Refused);
#else
    REQUIRE(getConnectionState(ECONNREFUSED) == ConnectionState::Refused);
#endif
  }

  SECTION("Host unreachable error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAEHOSTUNREACH) ==
            ConnectionState::Unreachable);
#else
    REQUIRE(getConnectionState(EHOSTUNREACH) == ConnectionState::Unreachable);
#endif
  }

  SECTION("Network unreachable error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAENETUNREACH) == ConnectionState::Unreachable);
#else
    REQUIRE(getConnectionState(ENETUNREACH) == ConnectionState::Unreachable);
#endif
  }

  SECTION("Network down error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAENETDOWN) == ConnectionState::NetworkDown);
#else
    REQUIRE(getConnectionState(ENETDOWN) == ConnectionState::NetworkDown);
#endif
  }

  SECTION("Connection aborted error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAECONNABORTED) ==
            ConnectionState::ConnectionAborted);
#else
    REQUIRE(getConnectionState(ECONNABORTED) ==
            ConnectionState::ConnectionAborted);
#endif
  }

  SECTION("Connection reset error") {
#ifdef _WIN32
    REQUIRE(getConnectionState(WSAECONNRESET) ==
            ConnectionState::ConnectionReset);
#else
    REQUIRE(getConnectionState(ECONNRESET) == ConnectionState::ConnectionReset);
#endif
  }

  SECTION("Unknown error code") {
    REQUIRE(getConnectionState(99999) == ConnectionState::Unknown);
  }
}

TEST_CASE("getDetailedErrorDescription - returns non-empty strings",
          "[error]") {
  SECTION("Connection refused description") {
#ifdef _WIN32
    std::string desc = getDetailedErrorDescription(WSAECONNREFUSED);
#else
    std::string desc = getDetailedErrorDescription(ECONNREFUSED);
#endif
    REQUIRE(!desc.empty());
    // Should contain both English and Chinese text
    REQUIRE(desc.find("refused") != std::string::npos);
  }

  SECTION("Timeout description") {
#ifdef _WIN32
    std::string desc = getDetailedErrorDescription(WSAETIMEDOUT);
#else
    std::string desc = getDetailedErrorDescription(ETIMEDOUT);
#endif
    REQUIRE(!desc.empty());
    REQUIRE(desc.find("timed out") != std::string::npos);
  }

  SECTION("Unknown error returns some description") {
    std::string desc = getDetailedErrorDescription(99999);
    REQUIRE(!desc.empty());
  }
}

TEST_CASE("getConnectionStateString - returns correct strings", "[error]") {
  SECTION("Success state") {
    REQUIRE(getConnectionStateString(ConnectionState::Success) == "Success");
  }

  SECTION("Timeout state") {
    REQUIRE(getConnectionStateString(ConnectionState::Timeout) == "Timeout");
  }

  SECTION("Refused state") {
    REQUIRE(getConnectionStateString(ConnectionState::Refused) ==
            "Connection refused");
  }

  SECTION("Unreachable state") {
    REQUIRE(getConnectionStateString(ConnectionState::Unreachable) ==
            "Host unreachable");
  }

  SECTION("Network down state") {
    REQUIRE(getConnectionStateString(ConnectionState::NetworkDown) ==
            "Network down");
  }

  SECTION("DNS failure state") {
    REQUIRE(getConnectionStateString(ConnectionState::DnsFailure) ==
            "DNS resolution failed");
  }

  SECTION("Connection reset state") {
    REQUIRE(getConnectionStateString(ConnectionState::ConnectionReset) ==
            "Connection reset");
  }

  SECTION("Connection aborted state") {
    REQUIRE(getConnectionStateString(ConnectionState::ConnectionAborted) ==
            "Connection aborted");
  }

  SECTION("Address in use state") {
    REQUIRE(getConnectionStateString(ConnectionState::AddressInUse) ==
            "Address in use");
  }

  SECTION("Access denied state") {
    REQUIRE(getConnectionStateString(ConnectionState::AccessDenied) ==
            "Access denied");
  }

  SECTION("Unknown state") {
    REQUIRE(getConnectionStateString(ConnectionState::Unknown) ==
            "Unknown error");
  }
}
