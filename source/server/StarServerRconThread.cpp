#include "StarServerRconThread.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarUniverseServer.hpp"
#include "StarServerRconClient.hpp"
#include "StarIterator.hpp"

namespace Star {

ServerRconThread::ServerRconThread(UniverseServer* universe, HostAddressWithPort const& address)
  : Thread("RconServer"), m_universe(universe), m_rconServer(address), m_stop(true) {
  if (Root::singleton().configuration()->get("rconServerPassword").toString().empty())
    Logger::warn("rconServerPassword is not configured requests will NOT be processed");
}

ServerRconThread::~ServerRconThread() {
  stop();
  join();
}

void ServerRconThread::clearClients(bool all) {
  auto it = makeSMutableMapIterator(m_clients);
  while (it.hasNext()) {
    auto const& pair = it.next();
    auto client = pair.second;
    if (all)
      client->stop();
    else if (!client->isRunning())
      it.remove();
  }
}

void ServerRconThread::start() {
  m_stop = false;
  Thread::start();
}

void ServerRconThread::stop() {
  m_stop = true;
  m_rconServer.stop();
  clearClients(true);
}

void ServerRconThread::run() {
  try {
    auto timeout = Root::singleton().configuration()->get("rconServerTimeout").toInt();
    while (!m_stop) {
      if (auto client = m_rconServer.accept(100)) {
        client->setTimeout(timeout);
        auto rconClient = make_shared<ServerRconClient>(m_universe, client);
        rconClient->start();
        m_clients[client->remoteAddress().address()] = rconClient;
        clearClients();
      }
    }
  } catch (std::exception const& e) {
    Logger::error("ServerRconThread exception caught: {}", e.what());
  }
}

}
