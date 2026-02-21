#ifndef TCPING_ARGS_H
#define TCPING_ARGS_H

#include <string>
#include "config.h"

class ArgumentParser {
public:
  static bool parseArguments(int argc, char* argv[], Config& config);
  static void printUsage(const char* programName);
};

#endif // TCPING_ARGS_H
