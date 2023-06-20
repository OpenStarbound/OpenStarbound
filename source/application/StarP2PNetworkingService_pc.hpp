#ifndef STAR_P2P_NETWORKING_SERVICE_PC_HPP
#define STAR_P2P_NETWORKING_SERVICE_PC_HPP

#include "StarPlatformServices_pc.hpp"
#include "StarAlgorithm.hpp"
#include "StarThread.hpp"
#include "StarStrongTypedef.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(PcP2PNetworkingService);

class PcP2PNetworkingService : public P2PNetworkingService {
public:
  PcP2PNetworkingService(PcPlatformServicesStatePtr state);
  ~PcP2PNetworkingService();

  void setJoinUnavailable() override;
  void setJoinLocal(uint32_t capacity) override;
  void setJoinRemote(HostAddressWithPort location) override;
  void setActivityData(String const& title, Maybe<pair<uint16_t, uint16_t>>) override;

  MVariant<P2PNetworkingPeerId, HostAddressWithPort> pullPendingJoin() override;
  Maybe<pair<String, RpcPromiseKeeper<P2PJoinRequestReply>>> pullJoinRequest() override;

  void setAcceptingP2PConnections(bool acceptingP2PConnections) override;
  List<P2PSocketUPtr> acceptP2PConnections() override;
  void update() override;
  Either<String, P2PSocketUPtr> connectToPeer(P2PNetworkingPeerId peerId) override;

  void addPendingJoin(String connectionString);

private:
  strong_typedef(Empty, JoinUnavailable);
  struct JoinLocal {
    bool operator==(JoinLocal const& rhs) const { return capacity == rhs.capacity; };
    uint32_t capacity;
  };
  strong_typedef(HostAddressWithPort, JoinRemote);
  typedef Variant<JoinUnavailable, JoinLocal, JoinRemote> JoinLocation;

#ifdef STAR_ENABLE_STEAM_INTEGRATION

  struct SteamP2PSocket : P2PSocket {
    SteamP2PSocket() = default;
    ~SteamP2PSocket();

    bool isOpen() override;
    bool sendMessage(ByteArray const& message) override;
    Maybe<ByteArray> receiveMessage() override;

    Mutex mutex;
    PcP2PNetworkingService* parent = nullptr;
    CSteamID steamId = CSteamID();
    Deque<ByteArray> incoming;
    bool connected = false;
  };

  unique_ptr<SteamP2PSocket> createSteamP2PSocket(CSteamID steamId);

  STEAM_CALLBACK(PcP2PNetworkingService, steamOnConnectionFailure, P2PSessionConnectFail_t, m_callbackConnectionFailure);
  STEAM_CALLBACK(PcP2PNetworkingService, steamOnJoinRequested, GameRichPresenceJoinRequested_t, m_callbackJoinRequested);
  STEAM_CALLBACK(PcP2PNetworkingService, steamOnSessionRequest, P2PSessionRequest_t, m_callbackSessionRequest);

  void steamCloseSocket(SteamP2PSocket* socket);
  void steamReceiveAll();

#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION

  enum class DiscordSocketMode {
    Startup,
    Connected,
    Disconnected
  };

  struct DiscordP2PSocket : P2PSocket {
    DiscordP2PSocket() = default;
    ~DiscordP2PSocket();

    bool isOpen() override;
    bool sendMessage(ByteArray const& message) override;
    Maybe<ByteArray> receiveMessage() override;

    Mutex mutex;
    PcP2PNetworkingService* parent = nullptr;
    DiscordSocketMode mode = DiscordSocketMode::Disconnected;
    discord::LobbyId lobbyId = {};
    discord::UserId remoteUserId;
    Deque<ByteArray> incoming;
  };

  P2PSocketUPtr discordConnectRemote(discord::UserId remoteUserId, discord::LobbyId lobbyId, String const& lobbySecret);
  void discordCloseSocket(DiscordP2PSocket* socket);

  void discordOnReceiveMessage(discord::LobbyId lobbyId, discord::UserId userId, discord::NetworkChannelId channel, uint8_t* data, uint32_t size);
  void discordOnLobbyMemberConnect(discord::LobbyId lobbyId, discord::UserId userId);
  void discordOnLobbyMemberUpdate(discord::LobbyId lobbyId, discord::UserId userId);
  void discordOnLobbyMemberDisconnect(discord::LobbyId lobbyId, discord::UserId userId);

#endif
  
  void setJoinLocation(JoinLocation joinLocation);

  PcPlatformServicesStatePtr m_state;

  Mutex m_mutex;
  JoinLocation m_joinLocation;
  bool m_acceptingP2PConnections = false;
  List<P2PSocketUPtr> m_pendingIncomingConnections;
  MVariant<P2PNetworkingPeerId, HostAddressWithPort> m_pendingJoin;

#ifdef STAR_ENABLE_STEAM_INTEGRATION

  HashMap<uint64, SteamP2PSocket*> m_steamOpenSockets;

#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION

  
  List<pair<discord::UserId, String>> m_discordJoinRequests;
  List<pair<discord::UserId, RpcPromise<P2PJoinRequestReply>>> m_pendingDiscordJoinRequests;

  HashMap<discord::UserId, DiscordP2PSocket*> m_discordOpenSockets;
  String m_discordActivityTitle;
  Maybe<pair<uint16_t, uint16_t>> m_discordPartySize;
  bool m_discordForceUpdateActivity = false;
  bool m_discordUpdatingActivity = false;
  Maybe<pair<discord::LobbyId, String>> m_discordServerLobby = {};

  int m_discordOnActivityJoinToken = 0;
  int m_discordOnActivityRequestToken = 0;
  int m_discordOnReceiveMessage = 0;
  int m_discordOnLobbyMemberConnect = 0;
  int m_discordOnLobbyMemberUpdate = 0;
  int m_discordOnLobbyMemberDisconnect = 0;

#endif
};

}

#endif
