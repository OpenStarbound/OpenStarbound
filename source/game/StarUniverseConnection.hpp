#pragma once

#include "StarNetPacketSocket.hpp"

namespace Star {

STAR_CLASS(UniverseConnection);
STAR_CLASS(UniverseConnectionServer);

STAR_EXCEPTION(UniverseConnectionException, StarException);

// Symmetric NetPacket based connection between the UniverseServer and the
// UniverseClient.
class UniverseConnection {
public:
  explicit UniverseConnection(PacketSocketUPtr packetSocket);
  UniverseConnection(UniverseConnection&& rhs);
  ~UniverseConnection();

  UniverseConnection& operator=(UniverseConnection&& rhs);

  bool isOpen() const;
  void close();

  // Push packets onto the send queue.
  void push(List<PacketPtr> packets);
  void pushSingle(PacketPtr packet);

  // Pull packets from the receive queue.
  List<PacketPtr> pull();
  PacketPtr pullSingle();

  // Send all data that we can without blocking, returns true if any data was
  // sent.
  bool send();

  // Block, trying to send the entire send queue before the given timeout.
  // Returns true if the entire send queue was sent before the timeout, false
  // otherwise.
  bool sendAll(unsigned timeout);

  // Receive all the data that we can without blocking, returns true if any
  // data was received.
  bool receive();

  // Block, trying to read at least one packet into the receive queue before
  // the timeout.  Returns true once any packets are on the receive queue,
  // false if the timeout was reached with no packets receivable.
  bool receiveAny(unsigned timeout);

  // Returns a reference to the packet socket.
  PacketSocket& packetSocket();

  // Packet stats for the most recent one second window of activity incoming
  // and outgoing.  Will only return valid stats if the underlying PacketSocket
  // implements stat collection.
  Maybe<PacketStats> incomingStats() const;
  Maybe<PacketStats> outgoingStats() const;

private:
  friend class UniverseConnectionServer;

  UniverseConnection() = default;

  mutable Mutex m_mutex;
  PacketSocketUPtr m_packetSocket;
  List<PacketPtr> m_sendQueue;
  Deque<PacketPtr> m_receiveQueue;
};

// Manage a set of UniverseConnections cheaply and in an asynchronous way.
// Uses multiple background threads to handle remote sending and receiving.
class UniverseConnectionServer {
public:
  // The packet receive callback is called asynchronously on every packet group
  // received.  It will be called such that it is safe to recursively call any
  // method on the UniverseConnectionServer without deadlocking.  The receive
  // callback will not be called for any client until the previous callback for
  // that client is complete.
  typedef function<void(UniverseConnectionServer*, ConnectionId, List<PacketPtr>)> PacketReceiveCallback;

  UniverseConnectionServer(PacketReceiveCallback packetReceiver, size_t numWorkerThreads = 0);
  ~UniverseConnectionServer();

  bool hasConnection(ConnectionId clientId) const;
  List<ConnectionId> allConnections() const;
  bool connectionIsOpen(ConnectionId clientId) const;
  int64_t lastActivityTime(ConnectionId clientId) const;

  void addConnection(ConnectionId clientId, UniverseConnection connection);
  UniverseConnection removeConnection(ConnectionId clientId);
  List<UniverseConnection> removeAllConnections();

  void sendPackets(ConnectionId clientId, List<PacketPtr> packets);

  // Get total packets processed across all worker threads
  uint64_t totalPacketsProcessed() const;
  // Get number of worker threads
  size_t numWorkerThreads() const;

private:
  struct Connection {
    Mutex mutex;
    PacketSocketUPtr packetSocket;
    List<PacketPtr> sendQueue;
    Deque<PacketPtr> receiveQueue;
    int64_t lastActivityTime;
    size_t workerIndex;
  };

  struct WorkerStats {
    atomice<uint64_t> packetsProcessed{0};
    atomic<uint64_t> bytesReceived{0};
    atomic<uint64_t> bytesSent{0};
    atomic<uint64_t> connectionsHandled{0};

    WorkerStats() = default;
    WorkerStats(WorkerStats&& other) noexcept : packetsProcessed(other.packetsProcessed.load()), bytesReceived(other.bytesReceived.load()), bytesSent(other.bytesSent.load()), connectionsHandled(other.connectionsHandled.load()) {
                                                };
    WorkerStats(const WorkerStats&) = delete;
    WorkerStats& operator=(WorkerStats&& other) noexcept {
      if (this != &other) {
        packetsProcessed = other.packetsProcessed.load();
        bytesReceived = other.bytesReceived.load();
        bytesSent = other.bytesSent.load();
        connectionsHandled = other.connectionsHandled.load();
      }
      return *this;
    };
    WorkerStats& operator=(const WorkerStats&) = delete;
  };

  PacketReceiveCallback const m_packetReceiver;

  mutable RecursiveMutex m_connectionsMutex;
  HashMap<ConnectionId, shared_ptr<Connection>> m_connections;

  List<ThreadFunction<void>> m_processingThreads;
  List<WorkerStats> m_workerStats;
  atomic<bool> m_shutdown;
  size_t m_numWorkerThreads;
};

}// namespace Star
