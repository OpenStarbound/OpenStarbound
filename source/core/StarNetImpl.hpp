#pragma once

#ifdef STAR_SYSTEM_FAMILY_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "StarString_windows.hpp"
#else
#ifdef STAR_SYSTEM_FREEBSD
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

#include "StarHostAddress.hpp"

#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

namespace Star {

#ifdef STAR_SYSTEM_FAMILY_WINDOWS
struct WindowsSocketInitializer {
  WindowsSocketInitializer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      fatalError("WSAStartup failed", false);
  };
};
static WindowsSocketInitializer g_windowsSocketInitializer;
#endif

inline String netErrorString() {
#ifdef STAR_SYSTEM_WINDOWS
  LPWSTR lpMsgBuf = NULL;
  int error = WSAGetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
              | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR)&lpMsgBuf,
      0,
      NULL);

  String result = strf("{} - {}", error, utf16ToString(lpMsgBuf));

  if (lpMsgBuf != NULL)
    LocalFree(lpMsgBuf);

  return result;
#else
  return strf("{} - {}", errno, strerror(errno));
#endif
}

inline bool netErrorConnectionReset() {
#ifdef STAR_SYSTEM_FAMILY_WINDOWS
  return WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAENETRESET;
#else
  return errno == ECONNRESET || errno == ETIMEDOUT;
#endif
}

inline bool netErrorInterrupt() {
#ifdef STAR_SYSTEM_FAMILY_WINDOWS
  return WSAGetLastError() == WSAEINTR || WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK;
#endif
}

inline void setAddressFromNative(HostAddressWithPort& addressWithPort, NetworkMode mode, struct sockaddr_storage* sockAddr) {
  switch (mode) {
    case NetworkMode::IPv4: {
      struct sockaddr_in* addr4 = (struct sockaddr_in*)sockAddr;
      addressWithPort = HostAddressWithPort(mode, (uint8_t*)&(addr4->sin_addr.s_addr), ntohs(addr4->sin_port));
      break;
    }
    case NetworkMode::IPv6: {
      struct sockaddr_in6* addr6 = (struct sockaddr_in6*)sockAddr;
      addressWithPort = HostAddressWithPort(mode, (uint8_t*)&addr6->sin6_addr.s6_addr, ntohs(addr6->sin6_port));
      break;
    }
    default:
      throw NetworkException("Invalid network mode for setAddressFromNative");
  }
}

inline void setNativeFromAddress(HostAddressWithPort const& addressWithPort, struct sockaddr_storage* sockAddr, socklen_t* sockAddrLen) {
  switch (addressWithPort.address().mode()) {
    case NetworkMode::IPv4: {
      struct sockaddr_in* addr4 = (struct sockaddr_in*)sockAddr;
      *sockAddrLen = sizeof(*addr4);

      memset(addr4, 0, *sockAddrLen);
      addr4->sin_family = AF_INET;
      addr4->sin_port = htons(addressWithPort.port());
      memcpy(((char*)&addr4->sin_addr.s_addr), addressWithPort.address().bytes(), addressWithPort.address().size());

      break;
    }
    case NetworkMode::IPv6: {
      struct sockaddr_in6* addr6 = (struct sockaddr_in6*)sockAddr;
      *sockAddrLen = sizeof(*addr6);

      memset(addr6, 0, *sockAddrLen);
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port = htons(addressWithPort.port());
      memcpy(((char*)&addr6->sin6_addr.s6_addr), addressWithPort.address().bytes(), addressWithPort.address().size());
      break;
    }
    default:
      throw NetworkException("Invalid network mode for setNativeFromAddress");
  }
}

#ifdef STAR_SYSTEM_FAMILY_WINDOWS
inline bool invalidSocketDescriptor(SOCKET socket) {
  return socket == INVALID_SOCKET;
}
#else
inline bool invalidSocketDescriptor(int socket) {
  return socket < 0;
}
#endif

struct SocketImpl {
  SocketImpl() {
    socketDesc = 0;
  }

  void setSockOpt(int level, int optname, const void* optval, socklen_t len) {
#ifdef STAR_SYSTEM_FAMILY_WINDOWS
    int ret = ::setsockopt(socketDesc, level, optname, (const char*)optval, len);
#else
    int ret = ::setsockopt(socketDesc, level, optname, optval, len);
#endif
    if (ret < 0)
      throw NetworkException(strf("setSockOpt failed to set {}, {}: {}", level, optname, netErrorString()));
  }

#ifdef STAR_SYSTEM_FAMILY_WINDOWS
  SOCKET socketDesc;
#else
  int socketDesc;
#endif
};

}
