#ifndef TCPING_ERROR_H
#define TCPING_ERROR_H

#include <string>
#include "statistics.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#endif

ConnectionState getConnectionState(int errorCode);
std::string getDetailedErrorDescription(int errorCode);
std::string getConnectionStateString(ConnectionState state);

#ifdef _WIN32
std::string getLastErrorString();
#endif

#endif // TCPING_ERROR_H
