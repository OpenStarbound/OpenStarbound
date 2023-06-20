#include "StarServerQueryThread.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarVersion.hpp"
#include "StarUniverseServer.hpp"
#include "StarIterator.hpp"

namespace Star {

ServerQueryThread::ServerQueryThread(UniverseServer* universe, HostAddressWithPort const& bindAddress)
  : Thread("QueryServer"),
    m_universe(universe),
    m_queryServer(bindAddress),
    m_stop(true),
    m_lastChallengeCheck(Time::monotonicMilliseconds()) {
  m_playersResponse.resize(A2S_PACKET_SIZE);
  m_playersResponse.setByteOrder(ByteOrder::LittleEndian);
  m_playersResponse.setNullTerminatedStrings(true);

  m_rulesResponse.resize(A2S_PACKET_SIZE);
  m_rulesResponse.setByteOrder(ByteOrder::LittleEndian);
  m_rulesResponse.setNullTerminatedStrings(true);

  m_generalResponse.resize(A2S_PACKET_SIZE);
  m_generalResponse.setByteOrder(ByteOrder::LittleEndian);
  m_generalResponse.setNullTerminatedStrings(true);

  m_serverPort = 0;
  m_lastActiveTime = 0;

  auto& root = Root::singleton();
  auto cfg = root.configuration();

  m_maxPlayers = cfg->get("maxPlayers").toUInt();
  m_serverName = cfg->get("serverName").toString();

  m_lastPlayersResponse = 0;
  m_lastRulesResponse = 0;
}

ServerQueryThread::~ServerQueryThread() {
  stop();
  join();
}

void ServerQueryThread::start() {
  m_stop = false;
  Thread::start();
  m_lastActiveTime = Time::monotonicMilliseconds();
}

void ServerQueryThread::stop() {
  m_stop = true;
  m_queryServer.close();
}

void ServerQueryThread::sendTo(HostAddressWithPort const& address, DataStreamBuffer* ds) {
  m_queryServer.send(address, ds->ptr(), ds->size());
}

uint8_t ServerQueryThread::serverPlayerCount() {
  return m_universe->numberOfClients();
}

bool ServerQueryThread::serverPassworded() {
  // TODO: implement
  return false;
}

String ServerQueryThread::serverWorldNames() {
  auto activeWorlds = m_universe->activeWorlds();
  if (activeWorlds.empty())
    return String("Unknown");

  return StringList(activeWorlds.transformed(printWorldId)).join(",");
}

const char* ServerQueryThread::serverPlugins() {
  // TODO: implement
  return "none";
}

bool ServerQueryThread::processPacket(HostAddressWithPort const& address, char const* data, size_t length) {
  uint8_t* buf = (uint8_t*)data;
  if (length < 5 || buf[0] != 0xff || buf[1] != 0xff || buf[2] != 0xff || buf[3] != 0xff) {
    // short packet or missing header
    return false;
  }

  // Process packet
  switch (buf[4]) {
    case A2S_INFO_REQUEST: {
      // We use -6 and not -5 as the string should be NULL terminated
      // but instead of the std::string constructor stopping at the NULL
      // it includes it :(
      std::string str((const char*)(buf + 5), length - 6);
      if (str.compare(A2S_INFO_REQUEST_STRING) != 0) {
        // Invalid request
        return false;
      }

      m_generalResponse.clear();
      m_generalResponse << A2S_HEAD_INT << A2S_INFO_REPLY << A2S_VERSION << m_serverName << serverWorldNames()
                        << GAME_DIR << GAME_DESC << A2S_APPID // Should be SteamAppId but this isn't a short :(
                        << serverPlayerCount() << m_maxPlayers << (uint8_t)0x00 // bots
                        << A2S_TYPE_DEDICATED // dedicated
#ifdef STAR_SYSTEM_FAMILY_WINDOWS
                        << A2S_ENV_WINDOWS // os
#else
                        << A2S_ENV_LINUX // os
#endif
                        << serverPassworded() << A2S_VAC_OFF // secure
                        << StarVersionString << A2S_EDF_PORT // EDF
                        << m_serverPort;

      sendTo(address, &m_generalResponse);
      return true;
    }
    case A2S_CHALLENGE_REQUEST:
      sendChallenge(address);
      return true;

    case A2S_PLAYER_REQUEST:
      if (challengeRequest(address, data, length))
        return true;

      if (!validChallenge(address, data, length))
        return false;

      buildPlayerResponse();
      sendTo(address, &m_playersResponse);
      return true;

    case A2S_RULES_REQUEST:
      if (challengeRequest(address, data, length))
        return true;

      if (!validChallenge(address, data, length))
        return false;

      buildRuleResponse();
      sendTo(address, &m_rulesResponse);
      return true;
  }

  return false;
}

void ServerQueryThread::buildPlayerResponse() {
  int64_t now = Time::monotonicMilliseconds();
  if (now < m_lastPlayersResponse + responseCacheTime) {
    return;
  }

  auto clientIds = m_universe->clientIds();
  uint8_t cnt = (uint8_t)clientIds.count();
  int32_t kills = 0; // Not currently supported
  float timeConnected = 60; // Not supported defaults to 1min

  m_playersResponse.clear();
  m_playersResponse << A2S_HEAD_INT << A2S_PLAYER_REPLY << cnt;

  uint8_t i = 0;
  for (auto clientId : clientIds) {
    m_playersResponse << i++ << m_universe->clientNick(clientId) << kills << timeConnected;
  }

  m_lastPlayersResponse = now;
}

void ServerQueryThread::buildRuleResponse() {
  int64_t now = Time::monotonicMilliseconds();
  if (now < m_lastRulesResponse + responseCacheTime) {
    return;
  }

  uint16_t cnt = 1;
  m_rulesResponse.clear();
  m_rulesResponse << A2S_HEAD_INT << A2S_RULES_REPLY << cnt << "plugins" << serverPlugins();

  m_lastRulesResponse = now;
}

void ServerQueryThread::sendChallenge(HostAddressWithPort const& address) {
  auto challenge = make_shared<RequestChallenge>();

  m_validChallenges[address.address()] = challenge;
  m_generalResponse.clear();
  m_generalResponse << A2S_HEAD_INT << A2S_CHALLENGE_RESPONSE << challenge->getChallenge();

  sendTo(address, &m_generalResponse);
}

void ServerQueryThread::pruneChallenges() {
  int64_t now = Time::monotonicMilliseconds();
  if (now < m_lastChallengeCheck + challengeCheckInterval) {
    return;
  }

  auto expire = now - challengeCheckInterval;
  auto it = makeSMutableMapIterator(m_validChallenges);
  while (it.hasNext()) {
    auto const& pair = it.next();
    if (pair.second->before(expire)) {
      it.remove();
    }
  }
  m_lastChallengeCheck = now;
}

void ServerQueryThread::run() {
  HostAddressWithPort udpAddress;
  char udpData[MaxUdpData];
  while (!m_stop) {
    try {
      auto len = m_queryServer.receive(&udpAddress, udpData, MaxUdpData, 100);
      pruneChallenges();
      if (len != 0)
        processPacket(udpAddress, udpData, len);
    } catch (SocketClosedException const&) {
    } catch (std::exception const& e) {
      Logger::error("ServerQueryThread exception caught: %s", outputException(e, true));
    }
  }
}

ServerQueryThread::RequestChallenge::RequestChallenge()
  : m_time(Time::monotonicMilliseconds()), m_challenge(Random::randi32()) {}

bool ServerQueryThread::RequestChallenge::before(uint64_t time) {
  return m_time < time;
}

int ServerQueryThread::RequestChallenge::getChallenge() {
  return m_challenge;
}

bool ServerQueryThread::validChallenge(HostAddressWithPort const& address, char const* data, size_t len) {
  if (len != 9) {
    // too much or too little data
    return false;
  }

  if (m_validChallenges.count(address.address()) == 0) {
    // Don't know this source address ignore
    return false;
  }

  uint8_t const* b = (uint8_t const*)data;
  int32_t challenge = ((int32_t)b[8] & 0xff) << 24 | ((int32_t)b[7] & 0xff) << 16 | ((int32_t)b[6] & 0xff) << 8
      | ((int32_t)b[5] & 0xff);
  // Note: No byte order swapping needed as protcol performs no conversion
  if (m_validChallenges.get(address.address())->getChallenge() != challenge) {
    // Challenges didnt match ignore
    return false;
  }

  // All good
  return true;
}

bool ServerQueryThread::challengeRequest(HostAddressWithPort const& address, char const* data, size_t len) {
  if (len != 9) {
    // too much or too little data
    return false;
  }

  uint8_t const* buf = (uint8_t const*)data;
  if ((buf[5] == 0xff) && (buf[6] == 0xff) && (buf[7] == 0xff) && (buf[8] == 0xff)) {
    sendChallenge(address);
    return true;
  }

  return false;
}

}
