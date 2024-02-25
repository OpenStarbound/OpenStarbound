#pragma once

#include "StarThread.hpp"
#include "StarTcp.hpp"
#include "StarMap.hpp"
#include "StarServerRconClient.hpp"

namespace Star {

STAR_CLASS(UniverseServer);
STAR_CLASS(ServerRconThread);

class ServerRconThread : public Thread {
public:
  ServerRconThread(UniverseServer* universe, HostAddressWithPort const& address);
  ~ServerRconThread();

  void start();
  void stop();

protected:
  virtual void run();

private:
  void clearClients(bool all = false);

  UniverseServer* m_universe;
  TcpServer m_rconServer;
  bool m_stop;
  HashMap<HostAddress, ServerRconClientPtr> m_clients;
};

}
