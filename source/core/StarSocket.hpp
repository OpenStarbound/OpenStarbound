#pragma once

#include "StarHostAddress.hpp"
#include "StarThread.hpp"

namespace Star {

// Thrown when some call on a socket failed because the socket is *either*
// closed or shutdown, for other errors sockets will throw NetworkException
STAR_EXCEPTION(SocketClosedException, NetworkException);

STAR_STRUCT(SocketImpl);
STAR_CLASS(Socket);

enum class SocketMode {
  Closed,
  Shutdown,
  Bound,
  Connected
};

struct SocketPollQueryEntry {
  // Query whether the tcp socket is readable
  bool readable;
  // Query whether the tcp socket is writable
  bool writable;
};

struct SocketPollResultEntry {
  // The tcp socket can be read without blocking
  bool readable;
  // The tcp socket can be written without blocking
  bool writable;
  // The tcp socket has had an error condition, or it has been closed.
  bool exception;
};

typedef Map<SocketPtr, SocketPollQueryEntry> SocketPollQuery;
typedef Map<SocketPtr, SocketPollResultEntry> SocketPollResult;

class Socket {
public:
  // Waits for sockets that are readable, writiable, or have pending error
  // conditions within the given timeout.  Returns result if any sockets are
  // ready for I/O or have had error events occur on them within the timeout,
  // nothing otherwise.  If socket hangup occurs during this call, this will
  // automatically shut down the socket.
  static Maybe<SocketPollResult> poll(SocketPollQuery const& query, unsigned timeout);

  ~Socket();

  void bind(HostAddressWithPort const& address);
  void listen(int backlog);

  // Sockets default to blocking mode
  void setNonBlocking(bool nonBlocking);
  // Sockets default to 60 second timeout
  void setTimeout(unsigned millis);

  NetworkMode networkMode() const;
  SocketMode socketMode() const;

  // Is the socketMode either Bound or Connected?
  bool isActive() const;

  // Is the socketMode not closed?
  bool isOpen() const;

  // Shuts down the underlying socket only.
  void shutdown();

  // Shuts down and closes the underlying socket.
  void close();

protected:
  enum class SocketType {
    Tcp,
    Udp
  };

  Socket(SocketType type, NetworkMode networkMode);
  Socket(NetworkMode networkMode, SocketImplPtr impl, SocketMode socketMode);

  void checkOpen(char const* methodName) const;
  void doShutdown();
  void doClose();

  mutable ReadersWriterMutex m_mutex;
  NetworkMode m_networkMode;
  SocketImplPtr m_impl;
  atomic<SocketMode> m_socketMode;
  HostAddressWithPort m_localAddress;
};

}
