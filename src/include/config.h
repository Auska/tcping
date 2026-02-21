#ifndef TCPING_CONFIG_H
#define TCPING_CONFIG_H

#include <string>
#include <vector>

struct Config {
  std::string host;
  std::vector<int> ports;
  int interval = 1;
  int timeout = 3000;
  bool verbose = false;
  bool show_statistics = true;
  int count = 0;
  bool ipv6 = false; // Force IPv6 mode
};

#endif // TCPING_CONFIG_H