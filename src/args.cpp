#include "args.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "common.h"
#include "version.h"

// Forward declarations for helper functions
namespace {
auto parsePorts(const char* port_str, std::vector<int>& ports) -> bool;
auto parseInterval(const char* interval_str, int& interval) -> bool;
auto parseTimeout(const char* timeout_str, int& timeout) -> bool;
auto parseCount(const char* count_str, int& count) -> bool;
auto parseConcurrency(const char* concurrency_str, int& concurrency) -> bool;
auto parseHost(const char* host_str, std::vector<std::string>& hosts) -> bool;
} // namespace

auto ArgumentParser::parseArguments(int argc, char* argv[], Config& config) -> bool {
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
      std::cerr << "Unknown option: " << arg << '\n';
      printUsage(argv[0]);
      return false;
    }
  }

  return true;
}

void ArgumentParser::printUsage(const char* program_name) {
  std::cout << "TCPing v" << VERSION << " by " << AUTHOR << '\n';
  std::cerr << "Usage: " << program_name << " <host> <port> [options]"
            << '\n';
  std::cerr << '\n';
  std::cerr << "Arguments:" << '\n';
  std::cerr << "  <host>        Target host (IP address or domain name)"
            << '\n';
  std::cerr << "  <port>        Target port(s): single port (80), multiple "
               "(22,80), or range (100-200)"
            << '\n';
  std::cerr << '\n';
  std::cerr << "Options:" << '\n';
  std::cerr
      << "  -i <seconds>  Wait interval between checks in seconds (default: 1)"
      << '\n';
  std::cerr
      << "  -t <ms>       Connection timeout in milliseconds (default: 3000)"
      << '\n';
  std::cerr << "  -v            Verbose mode (show detailed error messages)"
            << '\n';
  std::cerr
      << "  -c <count>    Number of connection attempts (default: unlimited)"
      << '\n';
  std::cerr
      << "  -a            Show all statistics (default: only show open ports)"
      << '\n';
  std::cerr << "  -4            Force IPv4 mode (default)" << '\n';
  std::cerr << "  -6            Force IPv6 mode" << '\n';
  std::cerr << "  -j <num>      Max concurrent connections (default: 50)"
            << '\n';
  std::cerr << '\n';
  std::cerr << "Notes:" << '\n';
  std::cerr
      << "  Statistics are automatically shown when scanning multiple hosts "
         "or ports"
      << '\n';
  std::cerr << '\n';
  std::cerr << "Examples:" << '\n';
  std::cerr << "  " << program_name << " google.com 443 -i 2 -t 5000"
            << '\n';
  std::cerr << "  " << program_name << " 127.0.0.1 22 -c 10" << '\n';
  std::cerr << "  " << program_name << " ipv6.google.com 443 -6" << '\n';
  std::cerr << "  " << program_name << " ::1 22 -6" << '\n';
}

namespace {
auto parsePorts(const char* port_str, std::vector<int>& ports) -> bool {
  std::string str(port_str);
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, ',')) {
    // Check if it's a range (e.g., "100-200")
    size_t dash_pos = token.find('-');
    if (dash_pos != std::string::npos) {
      std::string start_str = token.substr(0, dash_pos);
      std::string end_str = token.substr(dash_pos + 1);

      try {
        int start_port = std::stoi(start_str);
        int end_port = std::stoi(end_str);

        if (start_port <= 0 || start_port > 65535 || end_port <= 0 ||
            end_port > 65535) {
          std::cerr << "Port must be between 1 and 65535" << '\n';
          return false;
        }

        if (start_port > end_port) {
          std::cerr << "Invalid port range: " << start_port << "-" << end_port
                    << '\n';
          return false;
        }

        for (int p = start_port; p <= end_port; p++) {
          ports.push_back(p);
        }
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid port range: " << token << '\n';
        return false;
      }
    } else {
      // Single port
      try {
        int port = std::stoi(token);
        if (port <= 0 || port > 65535) {
          std::cerr << "Port must be between 1 and 65535" << '\n';
          return false;
        }
        ports.push_back(port);
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid port number: " << token << '\n';
        return false;
      }
    }
  }

  if (ports.empty()) {
    std::cerr << "At least one port must be specified" << '\n';
    return false;
  }

  return true;
}

auto parseInterval(const char* interval_str, int& interval) -> bool {
  try {
    interval = std::stoi(interval_str);
    if (interval <= 0) {
      std::cerr << "Interval must be positive" << '\n';
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid interval value: " << interval_str << '\n';
    return false;
  }
  return true;
}

auto parseTimeout(const char* timeout_str, int& timeout) -> bool {
  try {
    timeout = std::stoi(timeout_str);
    if (timeout <= 0) {
      std::cerr << "Timeout must be positive" << '\n';
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid timeout value: " << timeout_str << '\n';
    return false;
  }
  return true;
}

auto parseCount(const char* count_str, int& count) -> bool {
  try {
    count = std::stoi(count_str);
    if (count < 0) {
      std::cerr << "Count must be non-negative" << '\n';
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid count value: " << count_str << '\n';
    return false;
  }
  return true;
}

auto parseConcurrency(const char* concurrency_str, int& concurrency) -> bool {
  try {
    concurrency = std::stoi(concurrency_str);
    if (concurrency <= 0) {
      std::cerr << "Concurrency must be positive" << '\n';
      return false;
    }
  } catch (const std::exception& /* e */) {
    std::cerr << "Invalid concurrency value: " << concurrency_str << '\n';
    return false;
  }
  return true;
}

auto parseHost(const char* host_str, std::vector<std::string>& hosts) -> bool {
  std::string str(host_str);

  // Check if it's a CIDR notation (e.g., 192.168.88.0/24)
  size_t cidr_pos = str.find('/');
  if (cidr_pos != std::string::npos) {
    std::string ip_part = str.substr(0, cidr_pos);
    std::string prefix_len_str = str.substr(cidr_pos + 1);

    // Validate IP address
    struct in_addr addr{};
    if (inet_pton(AF_INET, ip_part.c_str(), &addr) != 1) {
      std::cerr << "Invalid IP address in CIDR: " << ip_part << '\n';
      return false;
    }

    // Parse prefix length
    int prefix_len = 0;
    try {
      prefix_len = std::stoi(prefix_len_str);
      if (prefix_len < 0 || prefix_len > 32) {
        std::cerr << "CIDR prefix must be between 0 and 32" << '\n';
        return false;
      }
    } catch (const std::exception& /* e */) {
      std::cerr << "Invalid CIDR prefix: " << prefix_len_str << '\n';
      return false;
    }

    // Calculate IP range from CIDR
    uint32_t ip = ntohl(addr.s_addr);
    uint32_t mask = prefix_len == 0 ? 0 : 0xFFFFFFFF << (32 - prefix_len);
    uint32_t network = ip & mask;
    uint32_t broadcast = network | (~mask);

    // Limit to reasonable range (max 65536 IPs)
    if (broadcast - network + 1 > 65536) {
      std::cerr << "CIDR range too large (max 65536 IPs)" << '\n';
      return false;
    }

    for (uint32_t i = network; i <= broadcast; i++) {
      struct in_addr tmp{};
      tmp.s_addr = htonl(i);
      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &tmp, ip_str, sizeof(ip_str));
      hosts.emplace_back(ip_str);
    }

    return true;
  }

  // Check if it's a range (e.g., 192.168.88.100-200)
  size_t dash_pos = str.find('-');
  if (dash_pos != std::string::npos) {
    std::string ip_part = str.substr(0, dash_pos);
    std::string end_part = str.substr(dash_pos + 1);

    // Validate base IP address
    struct in_addr addr{};
    if (inet_pton(AF_INET, ip_part.c_str(), &addr) != 1) {
      std::cerr << "Invalid IP address: " << ip_part << '\n';
      return false;
    }

    uint32_t base_ip = ntohl(addr.s_addr);
    // Get first 3 octets as base prefix
    uint32_t base_prefix = (base_ip >> 8) & 0xFFFFFF; // First 3 octets

    // Parse end IP (could be full IP or just last octet)
    uint32_t end_ip = 0;
    if (end_part.find('.') != std::string::npos) {
      // Full IP address
      struct in_addr end_addr{};
      if (inet_pton(AF_INET, end_part.c_str(), &end_addr) != 1) {
        std::cerr << "Invalid end IP address: " << end_part << '\n';
        return false;
      }
      end_ip = ntohl(end_addr.s_addr);
    } else {
      // Just last octet
      try {
        int last_octet = std::stoi(end_part);
        if (last_octet < 0 || last_octet > 255) {
          std::cerr << "Invalid last octet: " << last_octet << '\n';
          return false;
        }
        end_ip = (base_prefix << 8) | (last_octet & 0xFF);
      } catch (const std::exception& /* e */) {
        std::cerr << "Invalid IP range: " << str << '\n';
        return false;
      }
    }

    uint32_t start_ip = base_ip;

    // Validate range
    if (end_ip < start_ip) {
      std::cerr << "Invalid IP range: start > end" << '\n';
      return false;
    }

    // Limit to reasonable range (max 65536 IPs)
    if (end_ip - start_ip + 1 > 65536) {
      std::cerr << "IP range too large (max 65536 IPs)" << '\n';
      return false;
    }

    for (uint32_t i = start_ip; i <= end_ip; i++) {
      struct in_addr tmp{};
      tmp.s_addr = htonl(i);
      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &tmp, ip_str, sizeof(ip_str));
      hosts.emplace_back(ip_str);
    }

    return true;
  }

  // Plain IP address or domain name - just pass through
  hosts.push_back(str);
  return true;
}
} // namespace
