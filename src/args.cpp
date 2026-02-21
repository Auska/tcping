#include "args.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "version.h"

// Forward declarations for helper functions
namespace {
bool parsePorts(const char* portStr, std::vector<int>& ports);
bool parseInterval(const char* intervalStr, int& interval);
bool parseTimeout(const char* timeoutStr, int& timeout);
bool parseCount(const char* countStr, int& count);
bool parseConcurrency(const char* concurrencyStr, int& concurrency);
} // namespace

bool ArgumentParser::parseArguments(int argc, char* argv[], Config& config) {
  if (argc < 3) {
    printUsage(argv[0]);
    return false;
  }

  config.host = argv[1];

  if (!parsePorts(argv[2], config.ports)) {
    return false;
  }

  for (int i = 3; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-i" && i + 1 < argc) {
      if (!parseInterval(argv[++i], config.interval)) {
        return false;
      }
    } else if (arg == "-t" && i + 1 < argc) {
      if (!parseTimeout(argv[++i], config.timeout)) {
        return false;
      }
    } else if (arg == "-v") {
      config.verbose = true;
    } else if (arg == "-s") {
      config.show_statistics = true;
    } else if (arg == "-S") {
      config.show_statistics = false;
    } else if (arg == "-c" && i + 1 < argc) {
      if (!parseCount(argv[++i], config.count)) {
        return false;
      }
    } else if (arg == "-6") {
      config.ipv6 = true;
    } else if (arg == "-4") {
      config.ipv6 = false;
    } else if (arg == "-j" && i + 1 < argc) {
      if (!parseConcurrency(argv[++i], config.concurrency)) {
        return false;
      }
    } else {
      std::cerr << "Unknown option: " << arg << std::endl;
      printUsage(argv[0]);
      return false;
    }
  }

  return true;
}

void ArgumentParser::printUsage(const char* programName) {
  std::cout << "TCPing v" << VERSION << " by " << AUTHOR << std::endl;
  std::cerr << "Usage: " << programName << " <host> <port> [options]"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Arguments:" << std::endl;
  std::cerr << "  <host>        Target host (IP address or domain name)"
            << std::endl;
  std::cerr << "  <port>        Target port(s): single port (80), multiple "
               "(22,80), or range (100-200)"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  -i <seconds>  Check interval in seconds (default: 1)"
            << std::endl;
  std::cerr
      << "  -t <ms>       Connection timeout in milliseconds (default: 3000)"
      << std::endl;
  std::cerr << "  -v            Verbose mode (show detailed error messages)"
            << std::endl;
  std::cerr
      << "  -c <count>    Number of connection attempts (default: unlimited)"
      << std::endl;
  std::cerr << "  -s            Show statistics summary when stopped (default: "
               "enabled)"
            << std::endl;
  std::cerr << "  -S            Hide statistics summary" << std::endl;
  std::cerr << "  -4            Force IPv4 mode (default)" << std::endl;
  std::cerr << "  -6            Force IPv6 mode" << std::endl;
  std::cerr << "  -j <num>      Max concurrent connections (default: 50)"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Examples:" << std::endl;
  std::cerr << "  " << programName << " 192.168.1.1 80" << std::endl;
  std::cerr << "  " << programName << " google.com 443 -i 2 -t 5000"
            << std::endl;
  std::cerr << "  " << programName << " 127.0.0.1 22 -c 10" << std::endl;
  std::cerr << "  " << programName << " ipv6.google.com 443 -6" << std::endl;
  std::cerr << "  " << programName << " ::1 22 -6" << std::endl;
}

namespace {
bool parsePorts(const char* portStr, std::vector<int>& ports) {
  std::string str(portStr);
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, ',')) {
    // Check if it's a range (e.g., "100-200")
    size_t dashPos = token.find('-');
    if (dashPos != std::string::npos) {
      std::string startStr = token.substr(0, dashPos);
      std::string endStr = token.substr(dashPos + 1);

      try {
        int startPort = std::stoi(startStr);
        int endPort = std::stoi(endStr);

        if (startPort <= 0 || startPort > 65535 || endPort <= 0 ||
            endPort > 65535) {
          std::cerr << "Port must be between 1 and 65535" << std::endl;
          return false;
        }

        if (startPort > endPort) {
          std::cerr << "Invalid port range: " << startPort << "-" << endPort
                    << std::endl;
          return false;
        }

        for (int p = startPort; p <= endPort; p++) {
          ports.push_back(p);
        }
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid port range: " << token << std::endl;
        return false;
      }
    } else {
      // Single port
      try {
        int port = std::stoi(token);
        if (port <= 0 || port > 65535) {
          std::cerr << "Port must be between 1 and 65535" << std::endl;
          return false;
        }
        ports.push_back(port);
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid port number: " << token << std::endl;
        return false;
      }
    }
  }

  if (ports.empty()) {
    std::cerr << "At least one port must be specified" << std::endl;
    return false;
  }

  return true;
}

bool parseInterval(const char* intervalStr, int& interval) {
  try {
    interval = std::stoi(intervalStr);
    if (interval <= 0) {
      std::cerr << "Interval must be positive" << std::endl;
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid interval value: " << intervalStr << std::endl;
    return false;
  }
  return true;
}

bool parseTimeout(const char* timeoutStr, int& timeout) {
  try {
    timeout = std::stoi(timeoutStr);
    if (timeout <= 0) {
      std::cerr << "Timeout must be positive" << std::endl;
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid timeout value: " << timeoutStr << std::endl;
    return false;
  }
  return true;
}

bool parseCount(const char* countStr, int& count) {
  try {
    count = std::stoi(countStr);
    if (count < 0) {
      std::cerr << "Count must be non-negative" << std::endl;
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid count value: " << countStr << std::endl;
    return false;
  }
  return true;
}

bool parseConcurrency(const char* concurrencyStr, int& concurrency) {
  try {
    concurrency = std::stoi(concurrencyStr);
    if (concurrency <= 0) {
      std::cerr << "Concurrency must be positive" << std::endl;
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid concurrency value: " << concurrencyStr << std::endl;
    return false;
  }
  return true;
}
} // namespace
