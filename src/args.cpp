#include "args.h"
#include <arpa/inet.h>
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
bool parseHost(const char* hostStr, std::vector<std::string>& hosts);
} // namespace

bool ArgumentParser::parseArguments(int argc, char* argv[], Config& config) {
  if (argc < 3) {
    printUsage(argv[0]);
    return false;
  }

  config.host = argv[1];
  if (!parseHost(argv[1], config.hosts)) {
    return false;
  }

  config.ports_str = argv[2];
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
    } else if (arg == "-c" && i + 1 < argc) {
      if (!parseCount(argv[++i], config.count)) {
        return false;
      }
    } else if (arg == "-6") {
      config.ipv6 = true;
    } else if (arg == "-4") {
      config.ipv6 = false;
    } else if (arg == "-a") {
      config.show_all = true;
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
  std::cerr
      << "  -i <seconds>  Wait interval between checks in seconds (default: 1)"
      << std::endl;
  std::cerr
      << "  -t <ms>       Connection timeout in milliseconds (default: 3000)"
      << std::endl;
  std::cerr << "  -v            Verbose mode (show detailed error messages)"
            << std::endl;
  std::cerr
      << "  -c <count>    Number of connection attempts (default: unlimited)"
      << std::endl;
  std::cerr
      << "  -a            Show all statistics (default: only show open ports)"
      << std::endl;
  std::cerr << "  -4            Force IPv4 mode (default)" << std::endl;
  std::cerr << "  -6            Force IPv6 mode" << std::endl;
  std::cerr << "  -j <num>      Max concurrent connections (default: 50)"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Notes:" << std::endl;
  std::cerr
      << "  Statistics are automatically shown when scanning multiple hosts "
         "or ports"
      << std::endl;
  std::cerr << std::endl;
  std::cerr << "Examples:" << std::endl;
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

bool parseHost(const char* hostStr, std::vector<std::string>& hosts) {
  std::string str(hostStr);

  // Check if it's a CIDR notation (e.g., 192.168.88.0/24)
  size_t cidrPos = str.find('/');
  if (cidrPos != std::string::npos) {
    std::string ipPart = str.substr(0, cidrPos);
    std::string prefixLenStr = str.substr(cidrPos + 1);

    // Validate IP address
    struct in_addr addr;
    if (inet_pton(AF_INET, ipPart.c_str(), &addr) != 1) {
      std::cerr << "Invalid IP address in CIDR: " << ipPart << std::endl;
      return false;
    }

    // Parse prefix length
    int prefixLen;
    try {
      prefixLen = std::stoi(prefixLenStr);
      if (prefixLen < 0 || prefixLen > 32) {
        std::cerr << "CIDR prefix must be between 0 and 32" << std::endl;
        return false;
      }
    } catch (const std::exception& /* e */) {
      std::cerr << "Invalid CIDR prefix: " << prefixLenStr << std::endl;
      return false;
    }

    // Calculate IP range from CIDR
    uint32_t ip = ntohl(addr.s_addr);
    uint32_t mask = prefixLen == 0 ? 0 : 0xFFFFFFFF << (32 - prefixLen);
    uint32_t network = ip & mask;
    uint32_t broadcast = network | (~mask);

    // Limit to reasonable range (max 65536 IPs)
    if (broadcast - network + 1 > 65536) {
      std::cerr << "CIDR range too large (max 65536 IPs)" << std::endl;
      return false;
    }

    for (uint32_t i = network; i <= broadcast; i++) {
      struct in_addr tmp;
      tmp.s_addr = htonl(i);
      char ipStr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &tmp, ipStr, sizeof(ipStr));
      hosts.push_back(std::string(ipStr));
    }

    return true;
  }

  // Check if it's a range (e.g., 192.168.88.100-200)
  size_t dashPos = str.find('-');
  if (dashPos != std::string::npos) {
    std::string ipPart = str.substr(0, dashPos);
    std::string endPart = str.substr(dashPos + 1);

    // Validate base IP address
    struct in_addr addr;
    if (inet_pton(AF_INET, ipPart.c_str(), &addr) != 1) {
      std::cerr << "Invalid IP address: " << ipPart << std::endl;
      return false;
    }

    uint32_t baseIp = ntohl(addr.s_addr);
    // Get first 3 octets as base prefix
    uint32_t basePrefix = (baseIp >> 8) & 0xFFFFFF; // First 3 octets

    // Parse end IP (could be full IP or just last octet)
    uint32_t endIp;
    if (endPart.find('.') != std::string::npos) {
      // Full IP address
      struct in_addr endAddr;
      if (inet_pton(AF_INET, endPart.c_str(), &endAddr) != 1) {
        std::cerr << "Invalid end IP address: " << endPart << std::endl;
        return false;
      }
      endIp = ntohl(endAddr.s_addr);
    } else {
      // Just last octet
      try {
        int lastOctet = std::stoi(endPart);
        if (lastOctet < 0 || lastOctet > 255) {
          std::cerr << "Invalid last octet: " << lastOctet << std::endl;
          return false;
        }
        endIp = (basePrefix << 8) | (lastOctet & 0xFF);
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid IP range: " << str << std::endl;
        return false;
      }
    }

    uint32_t startIp = baseIp;

    // Validate range
    if (endIp < startIp) {
      std::cerr << "Invalid IP range: start > end" << std::endl;
      return false;
    }

    // Limit to reasonable range (max 65536 IPs)
    if (endIp - startIp + 1 > 65536) {
      std::cerr << "IP range too large (max 65536 IPs)" << std::endl;
      return false;
    }

    for (uint32_t i = startIp; i <= endIp; i++) {
      struct in_addr tmp;
      tmp.s_addr = htonl(i);
      char ipStr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &tmp, ipStr, sizeof(ipStr));
      hosts.push_back(std::string(ipStr));
    }

    return true;
  }

  // Plain IP address or domain name - just pass through
  hosts.push_back(str);
  return true;
}
} // namespace
