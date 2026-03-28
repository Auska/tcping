#ifndef TCPING_ARGS_H
#define TCPING_ARGS_H

#include <string>
#include "config.h"

class ArgumentParser {
public:
  static auto parseArguments(int argc, char* argv[], Config& config) -> bool;
  static void printUsage(const char* program_name);
};

#endif // TCPING_ARGS_H
