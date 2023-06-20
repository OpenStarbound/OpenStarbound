#include "StarUniverseConnection.hpp"
#include "StarTcp.hpp"

#include "gtest/gtest.h"

using namespace Star;

unsigned const PacketCount = 20;
uint16_t const ServerPort = 55555;

unsigned const NumLocalASyncConnections = 5;
unsigned const NumRemoteASyncConnections = 5;
unsigned const ASyncSleepMillis = 5;

unsigned const NumLocalSyncConnections = 5;
unsigned const NumRemoteSyncConnections = 5;
unsigned const SyncWaitMillis = 10000;

class ASyncClientThread : public Thread {
public:
  ASyncClientThread(UniverseConnection conn)
    : Thread("UniverseConnectionTestClientThread"), m_connection(move(conn)) {
    start();
  }

  virtual void run() {
    try {
      unsigned read = 0;
      unsigned written = 0;
      while (read < PacketCount || written < PacketCount) {
        m_connection.receive();
        if (read < PacketCount) {
          if (auto packet = m_connection.pullSingle()) {
            EXPECT_TRUE(convert<ProtocolRequestPacket>(packet)->requestProtocolVersion == read);
            ++read;
          }
        }

        if (written < PacketCount) {
          m_connection.push({make_shared<ProtocolRequestPacket>(written)});
          ++written;
        }
        m_connection.send();

        Thread::sleep(ASyncSleepMillis);

        if (!m_connection.isOpen())
          break;
      }

      EXPECT_EQ(PacketCount, read);
      EXPECT_EQ(PacketCount, written);
      m_connection.close();
      EXPECT_TRUE(m_connection.pull().empty());
    } catch (std::exception const& e) {
      ADD_FAILURE() << "Exception: " << outputException(e, true);
    } catch (...) {
      ADD_FAILURE();
    }
  }

private:
  UniverseConnection m_connection;
};

class SyncClientThread : public Thread {
public:
  SyncClientThread(UniverseConnection conn)
    : Thread("UniverseConnectionTestClientThread"), m_connection(move(conn)) {
    start();
  }

  virtual void run() {
    try {
      for (unsigned i = 0; i < PacketCount; ++i) {
        m_connection.pushSingle(make_shared<ProtocolRequestPacket>(i));
        EXPECT_TRUE(m_connection.sendAll(SyncWaitMillis));
        EXPECT_TRUE(m_connection.receiveAny(SyncWaitMillis));
        EXPECT_EQ(convert<ProtocolRequestPacket>(m_connection.pullSingle())->requestProtocolVersion, i);

        if (!m_connection.isOpen())
          break;
      }

      m_connection.close();
      EXPECT_TRUE(m_connection.pull().empty());
    } catch (std::exception const& e) {
      ADD_FAILURE() << "Exception: " << outputException(e, true);
    } catch (...) {
      ADD_FAILURE();
    }
  }

private:
  UniverseConnection m_connection;
};

TEST(UniverseConnections, All) {
  UniverseConnectionServer server([](UniverseConnectionServer* server, ConnectionId clientId, List<PacketPtr> packets) {
      server->sendPackets(clientId, packets);
    });

  ConnectionId clientId = ServerConnectionId;
  TcpServer tcpServer(HostAddressWithPort(HostAddress::localhost(), ServerPort));
  tcpServer.setAcceptCallback([&server, &clientId](TcpSocketPtr socket) {
      socket->setNonBlocking(true);
      auto conn = UniverseConnection(TcpPacketSocket::open(move(socket)));
      server.addConnection(++clientId, move(conn));
    });

  LinkedList<ASyncClientThread> localASyncClients;
  for (unsigned i = 0; i < NumLocalASyncConnections; ++i) {
    auto pair = LocalPacketSocket::openPair();
    server.addConnection(++clientId, UniverseConnection(move(pair.first)));
    localASyncClients.emplaceAppend(UniverseConnection(move(pair.second)));
  }

  LinkedList<SyncClientThread> localSyncClients;
  for (unsigned i = 0; i < NumLocalSyncConnections; ++i) {
    auto pair = LocalPacketSocket::openPair();
    server.addConnection(++clientId, UniverseConnection(move(pair.first)));
    localSyncClients.emplaceAppend(UniverseConnection(move(pair.second)));
  }

  LinkedList<ASyncClientThread> remoteASyncClients;
  for (unsigned i = 0; i < NumRemoteASyncConnections; ++i) {
    auto socket = TcpSocket::connectTo({HostAddress::localhost(), ServerPort});
    socket->setNonBlocking(true);
    remoteASyncClients.emplaceAppend(UniverseConnection(TcpPacketSocket::open(move(socket))));
  }

  LinkedList<SyncClientThread> remoteSyncClients;
  for (unsigned i = 0; i < NumRemoteSyncConnections; ++i) {
    auto socket = TcpSocket::connectTo({HostAddress::localhost(), ServerPort});
    socket->setNonBlocking(true);
    remoteSyncClients.emplaceAppend(UniverseConnection(TcpPacketSocket::open(move(socket))));
  }

  for (auto& c : localASyncClients)
    c.join();

  for (auto& c : remoteASyncClients)
    c.join();

  for (auto& c : localSyncClients)
    c.join();

  for (auto& c : remoteSyncClients)
    c.join();

  server.removeAllConnections();
}
