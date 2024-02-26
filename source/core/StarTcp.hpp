#pragma once

#include "StarIODevice.hpp"
#include "StarSocket.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(TcpSocket);
STAR_CLASS(TcpServer);

class TcpSocket : public Socket {
public:
  static TcpSocketPtr connectTo(HostAddressWithPort const& address);
  static TcpSocketPtr listen(HostAddressWithPort const& address);

  TcpSocketPtr accept();

  // Must be called after connect.  Sets TCP_NODELAY option.
  void setNoDelay(bool noDelay);

  size_t receive(char* data, size_t len);
  size_t send(char const* data, size_t len);

  HostAddressWithPort localAddress() const;
  HostAddressWithPort remoteAddress() const;

private:
  TcpSocket(NetworkMode networkMode);
  TcpSocket(NetworkMode networkMode, SocketImplPtr impl);

  void connect(HostAddressWithPort const& address);

  HostAddressWithPort m_remoteAddress;
};

// Simple class to listen for and open TcpSocket instances.
class TcpServer {
public:
  typedef function<void(TcpSocketPtr socket)> AcceptCallback;

  TcpServer(HostAddressWithPort const& address);
  // Listens to all interfaces.
  TcpServer(uint16_t port);
  ~TcpServer();

  void stop();
  bool isListening() const;

  // Blocks until next connection available for the given timeout.  Throws
  // ServerClosed if close() is called.  Cannot be called if AcceptCallback is
  // set.
  TcpSocketPtr accept(unsigned timeout);

  // Rather than calling and blocking on accept(), if an AcceptCallback is set
  // here, it will be called whenever a new connection is available.
  // Exceptions thrown from the callback function will be caught and logged,
  // and will cause the server to close.  The timeout here is the timeout that
  // is passed to accept in the loop, the longer the timeout the slower it will
  // shutdown on a call to close.
  void setAcceptCallback(AcceptCallback callback, unsigned timeout = 20);

private:
  mutable Mutex m_mutex;

  AcceptCallback m_callback;
  ThreadFunction<void> m_callbackThread;
  HostAddressWithPort m_hostAddress;
  TcpSocketPtr m_listenSocket;
};

}
