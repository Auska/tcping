#include "error.h"
#include "common.h"

ConnectionState getConnectionState(int errorCode) {
#ifdef _WIN32
  switch (errorCode) {
    case WSAECONNREFUSED:
      return ConnectionState::Refused;
    case WSAETIMEDOUT:
      return ConnectionState::Timeout;
    case WSAEHOSTUNREACH:
      return ConnectionState::Unreachable;
    case WSAENETUNREACH:
      return ConnectionState::Unreachable;
    case WSAENETDOWN:
      return ConnectionState::NetworkDown;
    case WSAECONNABORTED:
      return ConnectionState::ConnectionAborted;
    case WSAECONNRESET:
      return ConnectionState::ConnectionReset;
    case WSAEADDRINUSE:
      return ConnectionState::AddressInUse;
    case WSAEACCES:
      return ConnectionState::AccessDenied;
    default:
      return ConnectionState::Unknown;
  }
#else
  switch (errorCode) {
    case ECONNREFUSED:
      return ConnectionState::Refused;
    case ETIMEDOUT:
      return ConnectionState::Timeout;
    case EHOSTUNREACH:
      return ConnectionState::Unreachable;
    case ENETUNREACH:
      return ConnectionState::Unreachable;
    case ENETDOWN:
      return ConnectionState::NetworkDown;
    case ECONNABORTED:
      return ConnectionState::ConnectionAborted;
    case ECONNRESET:
      return ConnectionState::ConnectionReset;
    case EADDRINUSE:
      return ConnectionState::AddressInUse;
    case EACCES:
      return ConnectionState::AccessDenied;
    case EPERM:
      return ConnectionState::AccessDenied;
    default:
      return ConnectionState::Unknown;
  }
#endif
}

std::string getDetailedErrorDescription(int errorCode) {
#ifdef _WIN32
  switch (errorCode) {
    case WSAECONNREFUSED:
      return "Connection refused (目标端口主动拒绝连接)";
    case WSAETIMEDOUT:
      return "Connection timed out (连接超时)";
    case WSAEHOSTUNREACH:
      return "Host unreachable (主机不可达)";
    case WSAENETUNREACH:
      return "Network unreachable (网络不可达)";
    case WSAENETDOWN:
      return "Network is down (网络已关闭)";
    case WSAECONNABORTED:
      return "Connection aborted (连接被中止)";
    case WSAECONNRESET:
      return "Connection reset (连接被重置)";
    case WSAEADDRINUSE:
      return "Address already in use (地址已被使用)";
    case WSAEACCES:
      return "Access denied (访问被拒绝)";
    default:
      return getLastErrorString();
  }
#else
  switch (errorCode) {
    case ECONNREFUSED:
      return "Connection refused (目标端口主动拒绝连接)";
    case ETIMEDOUT:
      return "Connection timed out (连接超时)";
    case EHOSTUNREACH:
      return "Host unreachable (主机不可达)";
    case ENETUNREACH:
      return "Network unreachable (网络不可达)";
    case ENETDOWN:
      return "Network is down (网络已关闭)";
    case ECONNABORTED:
      return "Connection aborted (连接被中止)";
    case ECONNRESET:
      return "Connection reset (连接被重置)";
    case EADDRINUSE:
      return "Address already in use (地址已被使用)";
    case EACCES:
      return "Access denied (访问被拒绝)";
    case EPERM:
      return "Operation not permitted (操作不被允许)";
    default:
      return strerror(errorCode);
  }
#endif
}

std::string getConnectionStateString(ConnectionState state) {
  switch (state) {
    case ConnectionState::Success:
      return "Success";
    case ConnectionState::Timeout:
      return "Timeout";
    case ConnectionState::Refused:
      return "Connection refused";
    case ConnectionState::Unreachable:
      return "Host unreachable";
    case ConnectionState::NetworkDown:
      return "Network down";
    case ConnectionState::DnsFailure:
      return "DNS resolution failed";
    case ConnectionState::ConnectionReset:
      return "Connection reset";
    case ConnectionState::ConnectionAborted:
      return "Connection aborted";
    case ConnectionState::AddressInUse:
      return "Address in use";
    case ConnectionState::AccessDenied:
      return "Access denied";
    default:
      return "Unknown error";
  }
}

#ifdef _WIN32
std::string getLastErrorString() {
  int errorCode = WSAGetLastError();
  char* msgBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&msgBuffer, 0, NULL);

  std::string message(msgBuffer, size);
  LocalFree(msgBuffer);
  return message;
}
#endif
