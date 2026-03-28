#include <string>
#include <vector>
#include "catch_amalgamated.hpp"

// Forward declarations from args.cpp
namespace {
bool parsePorts(const char* portStr, std::vector<int>& ports);
bool parseInterval(const char* intervalStr, int& interval);
bool parseTimeout(const char* timeoutStr, int& timeout);
bool parseCount(const char* countStr, int& count);
bool parseConcurrency(const char* concurrencyStr, int& concurrency);
bool parseHost(const char* hostStr, std::vector<std::string>& hosts);
} // namespace

// We need to test the internal functions, so we include the args.cpp
// But since they are in anonymous namespace, we test through ArgumentParser

#include "args.h"
#include "config.h"

TEST_CASE("ArgumentParser - basic argument parsing", "[args]") {
  SECTION("Minimum required arguments") {
    const char* argv[] = {"tcping", "127.0.0.1", "80"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
    REQUIRE(config.host == "127.0.0.1");
    REQUIRE(config.ports.size() == 1);
    REQUIRE(config.ports[0] == 80);
    REQUIRE(config.interval == 1);
    REQUIRE(config.timeout == 3000);
    REQUIRE(!config.verbose);
    REQUIRE(!config.ipv6);
    REQUIRE(config.concurrency == 50);
  }

  SECTION("Missing arguments") {
    const char* argv[] = {"tcping", "127.0.0.1"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(2, const_cast<char**>(argv), config));
  }

  SECTION("Too few arguments") {
    const char* argv[] = {"tcping"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(1, const_cast<char**>(argv), config));
  }
}

TEST_CASE("ArgumentParser - optional arguments", "[args]") {
  SECTION("Verbose flag") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-v"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(4, const_cast<char**>(argv), config));
    REQUIRE(config.verbose);
  }

  SECTION("IPv6 flag") {
    const char* argv[] = {"tcping", "::1", "80", "-6"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(4, const_cast<char**>(argv), config));
    REQUIRE(config.ipv6);
  }

  SECTION("IPv4 flag (default)") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-4"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(4, const_cast<char**>(argv), config));
    REQUIRE(!config.ipv6);
  }

  SECTION("Show all flag") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-a"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(4, const_cast<char**>(argv), config));
    REQUIRE(config.show_all);
  }

  SECTION("Interval option") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-i", "5"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
    REQUIRE(config.interval == 5);
  }

  SECTION("Timeout option") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-t", "5000"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
    REQUIRE(config.timeout == 5000);
  }

  SECTION("Count option") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-c", "10"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
    REQUIRE(config.count == 10);
  }

  SECTION("Concurrency option") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-j", "100"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
    REQUIRE(config.concurrency == 100);
  }
}

TEST_CASE("ArgumentParser - port parsing", "[args]") {
  SECTION("Single port") {
    const char* argv[] = {"tcping", "127.0.0.1", "80"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
    REQUIRE(config.ports.size() == 1);
    REQUIRE(config.ports[0] == 80);
  }

  SECTION("Multiple ports") {
    const char* argv[] = {"tcping", "127.0.0.1", "22,80,443"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
    REQUIRE(config.ports.size() == 3);
    REQUIRE(config.ports[0] == 22);
    REQUIRE(config.ports[1] == 80);
    REQUIRE(config.ports[2] == 443);
  }

  SECTION("Port range") {
    const char* argv[] = {"tcping", "127.0.0.1", "100-105"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
    REQUIRE(config.ports.size() == 6);
    REQUIRE(config.ports[0] == 100);
    REQUIRE(config.ports[5] == 105);
  }

  SECTION("Mixed ports and ranges") {
    const char* argv[] = {"tcping", "127.0.0.1", "22,80-82,443"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
    REQUIRE(config.ports.size() == 5);
    REQUIRE(config.ports[0] == 22);
    REQUIRE(config.ports[1] == 80);
    REQUIRE(config.ports[2] == 81);
    REQUIRE(config.ports[3] == 82);
    REQUIRE(config.ports[4] == 443);
  }

  SECTION("Invalid port - zero") {
    const char* argv[] = {"tcping", "127.0.0.1", "0"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
  }

  SECTION("Invalid port - too large") {
    const char* argv[] = {"tcping", "127.0.0.1", "65536"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
  }

  SECTION("Invalid port range - start > end") {
    const char* argv[] = {"tcping", "127.0.0.1", "100-50"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(3, const_cast<char**>(argv), config));
  }
}

TEST_CASE("ArgumentParser - invalid values", "[args]") {
  SECTION("Invalid interval - zero") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-i", "0"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
  }

  SECTION("Invalid interval - negative") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-i", "-1"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
  }

  SECTION("Invalid timeout - zero") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-t", "0"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
  }

  SECTION("Invalid count - negative") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-c", "-1"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
  }

  SECTION("Invalid concurrency - zero") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-j", "0"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(5, const_cast<char**>(argv), config));
  }

  SECTION("Unknown option") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-x"};
    Config config;
    REQUIRE(
        !ArgumentParser::parseArguments(4, const_cast<char**>(argv), config));
  }
}

TEST_CASE("ArgumentParser - combined options", "[args]") {
  SECTION("Multiple options together") {
    const char* argv[] = {"tcping", "127.0.0.1", "80", "-i", "2", "-t",
                          "1000",   "-c",        "5",  "-v", "-a"};
    Config config;
    REQUIRE(
        ArgumentParser::parseArguments(11, const_cast<char**>(argv), config));
    REQUIRE(config.interval == 2);
    REQUIRE(config.timeout == 1000);
    REQUIRE(config.count == 5);
    REQUIRE(config.verbose);
    REQUIRE(config.show_all);
  }
}
