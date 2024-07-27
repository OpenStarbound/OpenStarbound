#include "StarUniverseConnection.hpp"
#include "StarLogging.hpp"

namespace Star {

static const int PacketSocketPollSleep = 1;

UniverseConnection::UniverseConnection(PacketSocketUPtr packetSocket)
  : m_packetSocket(std::move(packetSocket)) {}

UniverseConnection::UniverseConnection(UniverseConnection&& rhs) {
  operator=(std::move(rhs));
}

UniverseConnection::~UniverseConnection() {
  if (m_packetSocket)
    m_packetSocket->close();
}

UniverseConnection& UniverseConnection::operator=(UniverseConnection&& rhs) {
  MutexLocker locker(m_mutex);
  m_sendQueue = take(rhs.m_sendQueue);
  m_receiveQueue = take(rhs.m_receiveQueue);
  m_packetSocket = take(rhs.m_packetSocket);
  return *this;
}

bool UniverseConnection::isOpen() const {
  MutexLocker locker(m_mutex);
  return m_packetSocket->isOpen();
}

void UniverseConnection::close() {
  MutexLocker locker(m_mutex);
  m_packetSocket->close();
}

void UniverseConnection::push(List<PacketPtr> packets) {
  MutexLocker locker(m_mutex);
  m_sendQueue.appendAll(std::move(packets));
}

void UniverseConnection::pushSingle(PacketPtr packet) {
  MutexLocker locker(m_mutex);
  m_sendQueue.append(std::move(packet));
}

List<PacketPtr> UniverseConnection::pull() {
  MutexLocker locker(m_mutex);
  return List<PacketPtr>::from(take(m_receiveQueue));
}

PacketPtr UniverseConnection::pullSingle() {
  MutexLocker locker(m_mutex);
  if (m_receiveQueue.empty())
    return {};
  return m_receiveQueue.takeFirst();
}

bool UniverseConnection::send() {
  MutexLocker locker(m_mutex);
  m_packetSocket->sendPackets(take(m_sendQueue));
  return m_packetSocket->writeData();
}

bool UniverseConnection::sendAll(unsigned timeout) {
  MutexLocker locker(m_mutex);

  m_packetSocket->sendPackets(take(m_sendQueue));

  auto timer = Timer::withMilliseconds(timeout);
  while (true) {
    m_packetSocket->writeData();
    if (!m_packetSocket->sentPacketsPending())
      return true;

    if (timer.timeUp() || !m_packetSocket->isOpen())
      return false;

    Thread::sleep(PacketSocketPollSleep);
  }
}

bool UniverseConnection::receive() {
  MutexLocker locker(m_mutex);
  bool received = m_packetSocket->readData();
  m_receiveQueue.appendAll(m_packetSocket->receivePackets());
  return received;
}

bool UniverseConnection::receiveAny(unsigned timeout) {
  MutexLocker locker(m_mutex);
  if (!m_receiveQueue.empty())
    return true;

  auto timer = Timer::withMilliseconds(timeout);
  while (true) {
    m_packetSocket->readData();
    m_receiveQueue.appendAll(m_packetSocket->receivePackets());
    if (!m_receiveQueue.empty())
      return true;

    if (timer.timeUp() || !m_packetSocket->isOpen())
      return false;

    Thread::sleep(PacketSocketPollSleep);
  }
}

PacketSocket& UniverseConnection::packetSocket() {
  return *m_packetSocket;
}

Maybe<PacketStats> UniverseConnection::incomingStats() const {
  MutexLocker locker(m_mutex);
  return m_packetSocket->incomingStats();
}

Maybe<PacketStats> UniverseConnection::outgoingStats() const {
  MutexLocker locker(m_mutex);
  return m_packetSocket->outgoingStats();
}

UniverseConnectionServer::UniverseConnectionServer(PacketReceiveCallback packetReceiver)
  : m_packetReceiver(std::move(packetReceiver)), m_shutdown(false) {
  m_processingLoop = Thread::invoke("UniverseConnectionServer::processingLoop", [this]() {
      RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
      try {
        while (!m_shutdown) {
          connectionsLocker.lock();
          auto connections = m_connections.pairs();
          connectionsLocker.unlock();

          bool dataTransmitted = false;
          for (auto& p : connections) {
            MutexLocker connectionLocker(p.second->mutex);
            if (!p.second->packetSocket || !p.second->packetSocket->isOpen())
              continue;

            p.second->packetSocket->sendPackets(take(p.second->sendQueue));
            dataTransmitted |= p.second->packetSocket->writeData();

            dataTransmitted |= p.second->packetSocket->readData();
            List<PacketPtr> receivePackets = p.second->packetSocket->receivePackets();
            if (!receivePackets.empty()) {
              p.second->lastActivityTime = Time::monotonicMilliseconds();
              p.second->receiveQueue.appendAll(take(receivePackets));
            }

            if (!p.second->receiveQueue.empty()) {
              List<PacketPtr> toReceive = List<PacketPtr>::from(take(p.second->receiveQueue));
              connectionLocker.unlock();

              try {
                m_packetReceiver(this, p.first, std::move(toReceive));
              } catch (std::exception const& e) {
                Logger::error("Exception caught handling incoming server packets, disconnecting client '{}' {}", p.first, outputException(e, true));

                connectionLocker.lock();
                p.second->packetSocket->close();
              }
            }
          }

          if (!dataTransmitted)
            Thread::sleep(PacketSocketPollSleep);
        }
      } catch (std::exception const& e) {
        Logger::error("Exception caught in UniverseConnectionServer::remoteProcessLoop, closing all remote connections: {}", e.what());
        connectionsLocker.lock();
        for (auto& p : m_connections)
          p.second->packetSocket->close();
      }
    });
}

UniverseConnectionServer::~UniverseConnectionServer() {
  m_shutdown = true;
  m_processingLoop.finish();
  removeAllConnections();
}

bool UniverseConnectionServer::hasConnection(ConnectionId clientId) const {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  return m_connections.contains(clientId);
}

List<ConnectionId> UniverseConnectionServer::allConnections() const {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  return m_connections.keys();
}

bool UniverseConnectionServer::connectionIsOpen(ConnectionId clientId) const {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  if (auto conn = m_connections.value(clientId)) {
    MutexLocker connectionLocker(conn->mutex);
    return conn->packetSocket->isOpen();
  }

  throw UniverseConnectionException::format("No such client '{}' in UniverseConnectionServer::connectionIsOpen", clientId);
}

int64_t UniverseConnectionServer::lastActivityTime(ConnectionId clientId) const {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  if (auto conn = m_connections.value(clientId)) {
    MutexLocker connectionLocker(conn->mutex);
    return conn->lastActivityTime;
  }
  throw UniverseConnectionException::format("No such client '{}' in UniverseConnectionServer::lastRemoteActivityTime", clientId);
}

void UniverseConnectionServer::addConnection(ConnectionId clientId, UniverseConnection uc) {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  if (m_connections.contains(clientId))
    throw UniverseConnectionException::format("Client '{}' already exists in UniverseConnectionServer::addConnection", clientId);

  auto connection = make_shared<Connection>();
  connection->packetSocket = std::move(uc.m_packetSocket);
  connection->sendQueue = std::move(uc.m_sendQueue);
  connection->receiveQueue = std::move(uc.m_receiveQueue);
  connection->lastActivityTime = Time::monotonicMilliseconds();
  m_connections.add(clientId, std::move(connection));
}

UniverseConnection UniverseConnectionServer::removeConnection(ConnectionId clientId) {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  if (!m_connections.contains(clientId))
    throw UniverseConnectionException::format("Client '{}' does not exist in UniverseConnectionServer::removeConnection", clientId);

  auto conn = m_connections.take(clientId);
  MutexLocker connectionLocker(conn->mutex);

  UniverseConnection uc;
  uc.m_packetSocket = take(conn->packetSocket);
  uc.m_sendQueue = std::move(conn->sendQueue);
  uc.m_receiveQueue = std::move(conn->receiveQueue);
  return uc;
}

List<UniverseConnection> UniverseConnectionServer::removeAllConnections() {
  List<UniverseConnection> removedConnections;
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  for (auto connectionId : m_connections.keys())
    removedConnections.append(removeConnection(connectionId));
  return removedConnections;
}

void UniverseConnectionServer::sendPackets(ConnectionId clientId, List<PacketPtr> packets) {
  RecursiveMutexLocker connectionsLocker(m_connectionsMutex);
  if (auto conn = m_connections.value(clientId)) {
    MutexLocker connectionLocker(conn->mutex);
    conn->sendQueue.appendAll(std::move(packets));

    if (conn->packetSocket->isOpen()) {
      conn->packetSocket->sendPackets(take(conn->sendQueue));
      conn->packetSocket->writeData();
    }
  } else {
    throw UniverseConnectionException::format("No such client '{}' in UniverseConnectionServer::sendPackets", clientId);
  }
}

}
