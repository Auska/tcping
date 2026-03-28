#ifndef TCPING_ERROR_H
#define TCPING_ERROR_H

#include <string>
#include "statistics.h"

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <cerrno>
#endif

auto getConnectionState(int error_code) -> ConnectionState;
auto getDetailedErrorDescription(int error_code) -> std::string;
auto getConnectionStateString(ConnectionState state) -> std::string;

#ifdef _WIN32
std::string getLastErrorString();
#endif

#endif // TCPING_ERROR_H
