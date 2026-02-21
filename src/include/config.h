#ifndef TCPING_CONFIG_H
#define TCPING_CONFIG_H

#include <string>
#include <vector>

struct Config {
  std::string host; // Original host input (could be IP, CIDR, range, or domain)
  std::vector<std::string> hosts; // Expanded IP addresses from CIDR/range
  std::vector<int> ports;
  std::string ports_str; // Original port string (e.g., "80-100")
  int interval = 1;
  int timeout = 3000;
  bool verbose = false;
  bool show_all = false; // Show all statistics (including failed connections)
  int count = 0;
  bool ipv6 = false;    // Force IPv6 mode
  int concurrency = 50; // Max concurrent connections
};

#endif // TCPING_CONFIG_H