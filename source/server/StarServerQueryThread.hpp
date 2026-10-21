#pragma once

#include "StarThread.hpp"
#include "StarHostAddress.hpp"
#include "StarUdp.hpp"
#include "StarMap.hpp"
#include "StarDataStreamDevices.hpp"

#include <random>

namespace Star {

STAR_CLASS(UniverseServer);
STAR_CLASS(ServerQueryThread);

class ServerQueryThread : public Thread {
public:
  ServerQueryThread(UniverseServer* universe, HostAddressWithPort const& bindAddress);
  ~ServerQueryThread();

  void start();
  void stop();

protected:
  virtual void run();

private:
  static const uint8_t A2A_PING_REQUEST = 0x69;
  static const uint8_t A2A_PING_REPLY = 0x6a;
  static const uint8_t A2S_CHALLENGE_REQUEST = 0x57;
  static const uint8_t A2S_CHALLENGE_RESPONSE = 0x41;
  static const uint8_t A2S_INFO_REQUEST = 0x54;
  static const uint8_t A2S_INFO_REPLY = 0x49;
  static const uint8_t A2S_PLAYER_REQUEST = 0x55;
  static const uint8_t A2S_PLAYER_REPLY = 0x44;
  static const uint8_t A2S_RULES_REQUEST = 0x56;
  static const uint8_t A2S_RULES_REPLY = 0x45;
  static const uint8_t A2S_VERSION = 0x07;
  static const uint8_t A2S_STR_TERM = 0x00;
  static const uint8_t A2S_EDF_GID = 0x01;
  static const uint8_t A2S_EDF_SID = 0x10;
  static const uint8_t A2S_EDF_TAGS = 0x20;
  static const uint8_t A2S_EDF_STV = 0x40;
  static const uint8_t A2S_EDF_PORT = 0x80;
  static const uint8_t A2S_ENV_WINDOWS = 'w';
  static const uint8_t A2S_ENV_LINUX = 'l';
  static const uint8_t A2S_ENV_MAC = 'm';
  static const uint8_t A2S_TYPE_DEDICATED = 'd';
  static const uint8_t A2S_TYPE_LISTEN = 'l';
  static const uint8_t A2S_TYPE_TV = 'p';
  static const uint8_t A2S_VAC_OFF = 0x00;
  static const uint8_t A2S_VAC_ON = 0x01;
  static constexpr const char* A2S_INFO_REQUEST_STRING = "Source Engine Query";
  static const uint16_t A2S_APPID = (uint16_t)0xfffe;
  static const uint16_t A2S_PACKET_SIZE = (uint16_t)0x4e0;
  static const uint32_t A2S_HEAD_INT = 0xffffffff;
  static constexpr const char* GAME_DIR = "starbound";
  static constexpr const char* GAME_DESC = "Starbound";
  static constexpr const char* GAME_TYPE = "SMP";
  static const int32_t challengeCheckInterval = 30000;
  static const int32_t responseCacheTime = 5000;

  void sendTo(HostAddressWithPort const& address, DataStreamBuffer* ds);
  bool processPacket(HostAddressWithPort const& address, char const* data, size_t length);
  void buildPlayerResponse();
  void buildRuleResponse();
  bool validChallenge(HostAddressWithPort const& address, char const* data, size_t length);
  void sendChallenge(HostAddressWithPort const& address);
  void pruneChallenges();
  bool challengeRequest(HostAddressWithPort const& address, char const* data, size_t length);

  // Server API
  uint8_t serverPlayerCount();
  bool serverPassworded();
  const char* serverPlugins();
  String serverWorldNames();

  UniverseServer* m_universe;
  UdpServer m_queryServer;
  bool m_stop;
  DataStreamBuffer m_playersResponse;
  DataStreamBuffer m_rulesResponse;
  DataStreamBuffer m_generalResponse;

  class RequestChallenge {
  public:
    RequestChallenge();
    bool before(uint64_t time);
    int32_t getChallenge();

  private:
    uint64_t m_time;
    int32_t m_challenge;
  };

  uint16_t m_serverPort;
  uint8_t m_maxPlayers;
  String m_serverName;
  HashMap<HostAddress, shared_ptr<RequestChallenge>> m_validChallenges;
  int64_t m_lastChallengeCheck;
  int64_t m_lastPlayersResponse;
  int64_t m_lastRulesResponse;
  int64_t m_lastActiveTime;
};

}
