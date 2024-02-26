#pragma once

#include "StarSocket.hpp"

namespace Star {

STAR_CLASS(UdpSocket);
STAR_CLASS(UdpServer);

// A Good default assumption for a maximum size of a UDP datagram without
// fragmentation
unsigned const MaxUdpData = 1460;

class UdpSocket : public Socket {
public:
  UdpSocket(NetworkMode networkMode);

  size_t receive(HostAddressWithPort* address, char* data, size_t size);
  size_t send(HostAddressWithPort const& address, char const* data, size_t size);
};

class UdpServer {
public:
  UdpServer(HostAddressWithPort const& address);
  ~UdpServer();

  void close();
  bool isListening() const;

  size_t receive(HostAddressWithPort* address, char* data, size_t size, unsigned timeout);
  size_t send(HostAddressWithPort const& address, char const* data, size_t size);

private:
  HostAddressWithPort const m_hostAddress;
  UdpSocketPtr m_listenSocket;
};

}
