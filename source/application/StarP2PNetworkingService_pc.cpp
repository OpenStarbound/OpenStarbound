#include "StarP2PNetworkingService_pc.hpp"
#include "StarLexicalCast.hpp"
#include "StarEither.hpp"
#include "StarLogging.hpp"
#include "StarRandom.hpp"
#include "StarEncode.hpp"
#include "StarUuid.hpp"

namespace Star {

#ifdef STAR_ENABLE_DISCORD_INTEGRATION

discord::NetworkChannelId const DiscordMainNetworkChannel = 0;

#endif

PcP2PNetworkingService::PcP2PNetworkingService(PcPlatformServicesStatePtr state)
#ifdef STAR_ENABLE_STEAM_INTEGRATION
  : m_callbackConnectionFailure(this, &PcP2PNetworkingService::steamOnConnectionFailure),
    m_callbackJoinRequested(this, &PcP2PNetworkingService::steamOnJoinRequested),
    m_callbackSessionRequest(this, &PcP2PNetworkingService::steamOnSessionRequest),
    m_state(std::move(state)) {
#else
  : m_state(std::move(state)) {
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (m_state->discordAvailable) {
    MutexLocker discordLocker(m_state->discordMutex);

    m_discordPartySize = {};

    m_discordOnActivityJoinToken = m_state->discordCore->ActivityManager().OnActivityJoin.Connect([this](char const* peerId) {
        MutexLocker serviceLocker(m_mutex);
        Logger::info("Joining Discord peer at '{}'", peerId);
        addPendingJoin(strf("+platform:{}", peerId));
      });
    m_discordOnActivityRequestToken = m_state->discordCore->ActivityManager().OnActivityJoinRequest.Connect([this](discord::User const& user) {
        MutexLocker serviceLocker(m_mutex);
        String userName = String(user.GetUsername());
        Logger::info("Received join request from user '{}'", userName);
        m_discordJoinRequests.emplace_back(make_pair(user.GetId(), userName));
      });
    m_discordOnReceiveMessage = m_state->discordCore->LobbyManager().OnNetworkMessage.Connect(bind(&PcP2PNetworkingService::discordOnReceiveMessage, this, _1, _2, _3, _4, _5));
    m_discordOnLobbyMemberConnect = m_state->discordCore->LobbyManager().OnMemberConnect.Connect(bind(&PcP2PNetworkingService::discordOnLobbyMemberConnect, this, _1, _2));
    m_discordOnLobbyMemberUpdate = m_state->discordCore->LobbyManager().OnMemberUpdate.Connect(bind(&PcP2PNetworkingService::discordOnLobbyMemberUpdate, this, _1, _2));
    m_discordOnLobbyMemberDisconnect = m_state->discordCore->LobbyManager().OnMemberDisconnect.Connect(bind(&PcP2PNetworkingService::discordOnLobbyMemberDisconnect, this, _1, _2));
  }
#endif
}

PcP2PNetworkingService::~PcP2PNetworkingService() {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (m_state->discordAvailable) {
    MutexLocker discordLocker(m_state->discordMutex);
    if (m_discordServerLobby) {
      Logger::info("Deleting Discord server lobby {}", m_discordServerLobby->first);
      m_state->discordCore->LobbyManager().DeleteLobby(m_discordServerLobby->first, [](discord::Result res) {
          Logger::error("Could not delete Discord server lobby (err {})", (int)res);
        });
    }

    m_state->discordCore->ActivityManager().OnActivityJoin.Disconnect(m_discordOnActivityJoinToken);
    m_state->discordCore->LobbyManager().OnNetworkMessage.Disconnect(m_discordOnReceiveMessage);
    m_state->discordCore->LobbyManager().OnMemberConnect.Disconnect(m_discordOnLobbyMemberConnect);
    m_state->discordCore->LobbyManager().OnMemberUpdate.Disconnect(m_discordOnLobbyMemberUpdate);
    m_state->discordCore->LobbyManager().OnMemberDisconnect.Disconnect(m_discordOnLobbyMemberDisconnect);
  }
#endif
}

void PcP2PNetworkingService::setJoinUnavailable() {
  setJoinLocation(JoinUnavailable());
}

void PcP2PNetworkingService::setJoinLocal(uint32_t capacity) {
  setJoinLocation(JoinLocal{capacity});
}

void PcP2PNetworkingService::setJoinRemote(HostAddressWithPort location) {
  setJoinLocation(JoinRemote(location));
}

void Star::PcP2PNetworkingService::setActivityData(
  [[maybe_unused]] const char* title,
  [[maybe_unused]] const char* details,
  [[maybe_unused]] int64_t startTime,
  [[maybe_unused]] Maybe<pair<uint16_t, uint16_t>> party) {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  MutexLocker discordLocker(m_state->discordMutex);
#endif
  MutexLocker serviceLocker(m_mutex);

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (m_state->discordAvailable && m_state->discordCurrentUser) {
    if (m_discordUpdatingActivity)
      return;

    if (title != m_discordActivityTitle
     || details != m_discordActivityDetails
     || startTime != m_discordActivityStartTime || party != m_discordPartySize || m_discordForceUpdateActivity) {
      m_discordForceUpdateActivity = false;
      m_discordPartySize = party;
      m_discordActivityTitle = title;
      m_discordActivityDetails = details;
      m_discordActivityStartTime = startTime;

      discord::Activity activity = {};
      activity.SetType(discord::ActivityType::Playing);
      activity.SetName("Starbound");
      activity.SetState(title);
      activity.SetDetails(details);
      activity.GetTimestamps().SetStart(startTime);
      if (party) {
        auto& size = activity.GetParty().GetSize();
        size.SetCurrentSize(party->first);
        size.SetMaxSize(party->second);
      }
  
      if (auto lobby = m_discordServerLobby)
        activity.GetParty().SetId(toString(lobby->first).c_str());

      if (m_joinLocation.is<JoinLocal>()) {
        if (auto lobby = m_discordServerLobby) {
          String joinSecret = strf("connect:discord_{}_{}_{}", m_state->discordCurrentUser->GetId(), lobby->first, lobby->second);
          Logger::info("Setting Discord join secret as {}", joinSecret);
          activity.GetSecrets().SetJoin(joinSecret.utf8Ptr());
        }
      } else if (m_joinLocation.is<JoinRemote>()) {
        String address = toString((HostAddressWithPort)m_joinLocation.get<JoinRemote>());
        String joinSecret = strf("connect:address_{}", address);
        Logger::info("Setting Discord join secret as {}", joinSecret);
        activity.GetSecrets().SetJoin(joinSecret.utf8Ptr());
      
        activity.GetParty().SetId(address.utf8Ptr());
      }
    
      m_discordUpdatingActivity = true;
      m_state->discordCore->ActivityManager().UpdateActivity(activity, [this](discord::Result res) {
          if (res != discord::Result::Ok)
            Logger::error("Failed to set Discord activity (err {})", (int)res);
        
          MutexLocker serviceLocker(m_mutex);
          m_discordUpdatingActivity = false;
        });
    }
  }
#endif
}

MVariant<P2PNetworkingPeerId, HostAddressWithPort> PcP2PNetworkingService::pullPendingJoin() {
  MutexLocker serviceLocker(m_mutex);
  return take(m_pendingJoin);
}

Maybe<pair<String, RpcPromiseKeeper<P2PJoinRequestReply>>> Star::PcP2PNetworkingService::pullJoinRequest() {
  MutexLocker serviceLocker(m_mutex);

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (auto request = m_discordJoinRequests.maybeTakeLast()) {
    auto promisePair = RpcPromise<P2PJoinRequestReply>::createPair();
    m_pendingDiscordJoinRequests.push_back(make_pair(request->first, promisePair.first));
    return make_pair(request->second, promisePair.second);
  }
#endif

  return {};
}

void PcP2PNetworkingService::setAcceptingP2PConnections(bool acceptingP2PConnections) {
  MutexLocker serviceLocker(m_mutex);
  m_acceptingP2PConnections = acceptingP2PConnections;
  if (!m_acceptingP2PConnections)
    m_pendingIncomingConnections.clear();
}

List<P2PSocketUPtr> PcP2PNetworkingService::acceptP2PConnections() {
  MutexLocker serviceLocker(m_mutex);
  return take(m_pendingIncomingConnections);
}

void Star::PcP2PNetworkingService::update() {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  MutexLocker discordLocker(m_state->discordMutex);
#endif
  MutexLocker serviceLocker(m_mutex);
  
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  for (auto& p : m_pendingDiscordJoinRequests) {
    if (auto res = p.second.result()) {
      auto reply = discord::ActivityJoinRequestReply::Ignore;
      switch (*res) {
        case P2PJoinRequestReply::Yes:
          reply = discord::ActivityJoinRequestReply::Yes;
          break;
        case P2PJoinRequestReply::No:
          reply = discord::ActivityJoinRequestReply::No;
          break;
        case P2PJoinRequestReply::Ignore:
          reply = discord::ActivityJoinRequestReply::Ignore;
          break;
      }

      m_state->discordCore->ActivityManager().SendRequestReply(p.first, reply, [](discord::Result res) {
          if (res != discord::Result::Ok)
            Logger::error("Could not send Discord activity join response (err {})", (int)res);
        });
    }
  }
  m_pendingDiscordJoinRequests = m_pendingDiscordJoinRequests.filtered([](pair<discord::UserId, RpcPromise<P2PJoinRequestReply>>& p) {
      return !p.second.finished();
    });
#endif
}

Either<String, P2PSocketUPtr> PcP2PNetworkingService::connectToPeer(P2PNetworkingPeerId peerId) {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  MutexLocker discordLocker(m_state->discordMutex);
#endif
  MutexLocker serviceLocker(m_mutex);
  String type = peerId.extract("_");

#ifdef STAR_ENABLE_STEAM_INTEGRATION
  if (m_state->steamAvailable) {
    if (type == "steamid") {
      CSteamID steamId(lexicalCast<uint64>(peerId));
      return makeRight(createSteamP2PSocket(steamId));
    }
  }
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (m_state->discordAvailable) {
    if (type == "discord") {
      auto remoteUserId = lexicalCast<discord::UserId>(peerId.extract("_"));
      auto lobbyId = lexicalCast<discord::LobbyId>(peerId.extract("_"));
      String lobbySecret = std::move(peerId);
      return makeRight(discordConnectRemote(remoteUserId, lobbyId, lobbySecret));
    }
  }
#endif

  return makeLeft(strf("Unsupported peer type '{}'", type));
}

void PcP2PNetworkingService::addPendingJoin(String connectionString) {
  MutexLocker serviceLocker(m_mutex);

  if (connectionString.extract(":") != "+platform")
    throw ApplicationException::format("malformed connection string '{}'", connectionString);

  if (connectionString.extract(":") != "connect")
    throw ApplicationException::format("malformed connection string '{}'", connectionString);

  String target = std::move(connectionString);
  String targetType = target.extract("_");

  if (targetType == "address")
    m_pendingJoin = HostAddressWithPort(target);
  else
    m_pendingJoin = P2PNetworkingPeerId(strf("{}_{}", targetType, target));
}

#ifdef STAR_ENABLE_STEAM_INTEGRATION

PcP2PNetworkingService::SteamP2PSocket::~SteamP2PSocket() {
  MutexLocker serviceLocker(parent->m_mutex);
  MutexLocker socketLocker(mutex);
  parent->steamCloseSocket(this);
}

bool PcP2PNetworkingService::SteamP2PSocket::isOpen() {
  MutexLocker socketLocker(mutex);
  return connected;
}

bool PcP2PNetworkingService::SteamP2PSocket::sendMessage(ByteArray const& message) {
  MutexLocker socketLocker(mutex);
  if (!connected)
    return false;

  if (!SteamNetworking()->SendP2PPacket(steamId, message.ptr(), message.size(), k_EP2PSendReliable))
    throw ApplicationException("SteamNetworking::SendP2PPacket unexpectedly returned false");
  return true;
}

Maybe<ByteArray> PcP2PNetworkingService::SteamP2PSocket::receiveMessage() {
  MutexLocker socketLocker(mutex);
  if (!incoming.empty())
    return incoming.takeFirst();

  if (connected) {
    socketLocker.unlock();
    {
      MutexLocker serviceLocker(parent->m_mutex);
      parent->steamReceiveAll();
    }

    socketLocker.lock();
    if (!incoming.empty())
      return incoming.takeFirst();
  }

  return {};
}

auto PcP2PNetworkingService::createSteamP2PSocket(CSteamID steamId) -> unique_ptr<SteamP2PSocket> {
  if (auto oldSocket = m_steamOpenSockets.value(steamId.ConvertToUint64())) {
    MutexLocker socketLocker(oldSocket->mutex);
    steamCloseSocket(oldSocket);
  }

  unique_ptr<SteamP2PSocket> socket(new SteamP2PSocket);
  socket->parent = this;
  socket->steamId = steamId;
  socket->connected = true;

  m_steamOpenSockets[steamId.ConvertToUint64()] = socket.get();

  return socket;
}

void PcP2PNetworkingService::steamOnConnectionFailure(P2PSessionConnectFail_t* callback) {
  MutexLocker serviceLocker(m_mutex);
  Logger::warn("Connection with Steam user {} failed", callback->m_steamIDRemote.ConvertToUint64());
  if (auto socket = m_steamOpenSockets.value(callback->m_steamIDRemote.ConvertToUint64())) {
    MutexLocker socketLocker(socket->mutex);
    steamCloseSocket(socket);
  }
}

void PcP2PNetworkingService::steamOnJoinRequested(GameRichPresenceJoinRequested_t* callback) {
  Logger::info("Enqueueing join request with Steam friend id {} to address {}", callback->m_steamIDFriend.ConvertToUint64(), callback->m_rgchConnect);
  addPendingJoin(callback->m_rgchConnect);
}

void PcP2PNetworkingService::steamOnSessionRequest(P2PSessionRequest_t* callback) {
  MutexLocker serviceLocker(m_mutex);
  // Not sure whether this HasFriend call is actually necessary, or whether
  // non-friends can even initiate P2P sessions.
  if (m_acceptingP2PConnections && SteamFriends()->HasFriend(callback->m_steamIDRemote, k_EFriendFlagImmediate)) {
    if (SteamNetworking()->AcceptP2PSessionWithUser(callback->m_steamIDRemote)) {
      Logger::info("Accepted Steam P2P connection with user {}", callback->m_steamIDRemote.ConvertToUint64());
      m_pendingIncomingConnections.append(createSteamP2PSocket(callback->m_steamIDRemote));
    } else {
      Logger::error("Accepting Steam P2P connection from user {} failed!", callback->m_steamIDRemote.ConvertToUint64());
    }
  } else {
    Logger::error("Ignoring Steam P2P connection from user {}", callback->m_steamIDRemote.ConvertToUint64());
  }
}

void PcP2PNetworkingService::steamCloseSocket(SteamP2PSocket* socket) {
  if (socket->connected) {
    Logger::info("Closing P2P connection with Steam user {}", socket->steamId.ConvertToUint64());
    m_steamOpenSockets.remove(socket->steamId.ConvertToUint64());
    socket->connected = false;
  }
  SteamNetworking()->CloseP2PSessionWithUser(socket->steamId);
}

void PcP2PNetworkingService::steamReceiveAll() {
  uint32_t messageSize;
  CSteamID messageRemoteUser;
  while (SteamNetworking()->IsP2PPacketAvailable(&messageSize)) {
    ByteArray data(messageSize, 0);
    SteamNetworking()->ReadP2PPacket(data.ptr(), messageSize, &messageSize, &messageRemoteUser);
    if (auto openSocket = m_steamOpenSockets.value(messageRemoteUser.ConvertToUint64())) {
      MutexLocker socketLocker(openSocket->mutex);
      openSocket->incoming.append(std::move(data));
    }
  }
}

#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION

PcP2PNetworkingService::DiscordP2PSocket::~DiscordP2PSocket() {
  MutexLocker discordLocker(parent->m_state->discordMutex);
  MutexLocker serviceLocker(parent->m_mutex);
  MutexLocker socketLocker(mutex);
  parent->discordCloseSocket(this);
}

bool PcP2PNetworkingService::DiscordP2PSocket::isOpen() {
  MutexLocker socketLocker(mutex);
  return mode != DiscordSocketMode::Disconnected;
}

bool PcP2PNetworkingService::DiscordP2PSocket::sendMessage(ByteArray const& message) {
  MutexLocker discordLocker(parent->m_state->discordMutex);
  MutexLocker socketLocker(mutex);
  if (mode != DiscordSocketMode::Connected)
    return false;

  discord::Result res = parent->m_state->discordCore->LobbyManager().SendNetworkMessage(lobbyId, remoteUserId, DiscordMainNetworkChannel, (uint8_t*)message.ptr(), message.size());
  if (res != discord::Result::Ok)
    throw ApplicationException::format("discord::Network::Send returned error (err {})", (int)res);

  return true;
}

Maybe<ByteArray> PcP2PNetworkingService::DiscordP2PSocket::receiveMessage() {
  MutexLocker socketLocker(mutex);
  if (!incoming.empty())
    return incoming.takeFirst();
  else
    return {};
}

void PcP2PNetworkingService::discordCloseSocket(DiscordP2PSocket* socket) {
  if (socket->mode != DiscordSocketMode::Disconnected) {
    m_discordOpenSockets.remove(socket->remoteUserId);

    if (socket->mode == DiscordSocketMode::Connected) {
      if (!m_joinLocation.is<JoinLocal>() && m_discordOpenSockets.empty()) {
        auto res = m_state->discordCore->LobbyManager().DisconnectNetwork(socket->lobbyId);
        if (res != discord::Result::Ok)
          Logger::error("Failed to leave network for lobby {} (err {})", socket->lobbyId, (int)res);

        m_state->discordCore->LobbyManager().DisconnectLobby(socket->lobbyId, [this, lobbyId = socket->lobbyId](discord::Result res) {
            if (res != discord::Result::Ok)
              Logger::error("Failed to leave Discord lobby {}", lobbyId);

            Logger::info("Left Discord lobby {}", lobbyId);
            MutexLocker serviceLocker(m_mutex);
            m_discordServerLobby = {};
            m_discordForceUpdateActivity = true;
          });
      }
    }

    socket->mode = DiscordSocketMode::Disconnected;
  }
}

P2PSocketUPtr PcP2PNetworkingService::discordConnectRemote(discord::UserId remoteUserId, discord::LobbyId lobbyId, String const& lobbySecret) {
  if (auto oldSocket = m_discordOpenSockets.value(remoteUserId)) {
    MutexLocker socketLocker(oldSocket->mutex);
    discordCloseSocket(oldSocket);
  }

  unique_ptr<DiscordP2PSocket> socket(new DiscordP2PSocket);
  socket->parent = this;
  socket->mode = DiscordSocketMode::Startup;
  socket->remoteUserId = remoteUserId;
  socket->lobbyId = lobbyId;
  m_discordOpenSockets[remoteUserId] = socket.get();

  Logger::info("Connecting to Discord lobby {}", lobbyId);
  m_state->discordCore->LobbyManager().ConnectLobby(lobbyId, lobbySecret.utf8Ptr(), [this, remoteUserId, lobbyId](discord::Result res, discord::Lobby const&) {
      MutexLocker serviceLocker(m_mutex);
      if (res == discord::Result::Ok) {
        if (auto socket = m_discordOpenSockets.value(remoteUserId)) {
          MutexLocker socketLocker(socket->mutex);
          
          res = m_state->discordCore->LobbyManager().ConnectNetwork(lobbyId);
          if (res != discord::Result::Ok) {
            discordCloseSocket(socket);
            return Logger::error("Could not connect to Discord lobby network (err {})", (int)res);
          }
          
          res = m_state->discordCore->LobbyManager().OpenNetworkChannel(lobbyId, DiscordMainNetworkChannel, true);
          if (res != discord::Result::Ok) {
            discordCloseSocket(socket);
            return Logger::error("Could not open Discord main network channel (err {})", (int)res);
          }

              socket->mode = DiscordSocketMode::Connected;
              Logger::info("Discord P2P connection opened to remote user {} via lobby {}", remoteUserId, lobbyId);

              m_discordServerLobby = make_pair(lobbyId, String());
              m_discordForceUpdateActivity = true;
        } else {
          Logger::error("discord::Lobbies::Connect callback no matching remoteUserId {} found", remoteUserId);
        }
      } else {
        Logger::error("Failed to connect to remote lobby (err {})", (int)res);
        if (auto socket = m_discordOpenSockets.value(remoteUserId)) {
          MutexLocker socketLocker(socket->mutex);
          discordCloseSocket(socket);
        }
      }
    });

  return unique_ptr<P2PSocket>(std::move(socket));
}

void PcP2PNetworkingService::discordOnReceiveMessage(discord::LobbyId lobbyId, discord::UserId userId, discord::NetworkChannelId channel, uint8_t* data, uint32_t size) {
  MutexLocker serviceLocker(m_mutex);

  if (lobbyId != m_discordServerLobby->first)
    return Logger::error("Received message from unexpected lobby {}", lobbyId);

  if (auto socket = m_discordOpenSockets.value(userId)) {
      if (channel == DiscordMainNetworkChannel) {
        MutexLocker socketLocker(socket->mutex);
        socket->incoming.append(ByteArray((char const*)data, size));
      } else {
        Logger::error("Received Discord message on unexpected channel {}, ignoring", channel);
      }
    } else {
    Logger::error("Could not find associated Discord socket for user id {}", userId);
    }
  }

void PcP2PNetworkingService::discordOnLobbyMemberConnect(discord::LobbyId lobbyId, discord::UserId userId) {
  MutexLocker serviceLocker(m_mutex);

  if (m_discordServerLobby && m_discordServerLobby->first == lobbyId && userId != m_state->discordCurrentUser->GetId()) {
    if (!m_discordOpenSockets.contains(userId)) {
      unique_ptr<DiscordP2PSocket> socket(new DiscordP2PSocket);
      socket->parent = this;
      socket->lobbyId = lobbyId;
      socket->remoteUserId = userId;
      socket->mode = DiscordSocketMode::Connected;

      m_discordOpenSockets[userId] = socket.get();
      m_pendingIncomingConnections.append(std::move(socket));
      Logger::info("Accepted new Discord connection from remote user {}", userId);
    }
  }
}

void PcP2PNetworkingService::discordOnLobbyMemberUpdate(discord::LobbyId lobbyId, discord::UserId userId) {
  discordOnLobbyMemberConnect(lobbyId, userId);
}

void PcP2PNetworkingService::discordOnLobbyMemberDisconnect(discord::LobbyId lobbyId, discord::UserId userId) {
  MutexLocker serviceLocker(m_mutex);

  if (m_discordServerLobby && m_discordServerLobby->first == lobbyId && userId != m_state->discordCurrentUser->GetId()) {
    if (auto socket = m_discordOpenSockets.value(userId)) {
      MutexLocker socketLocker(socket->mutex);
      discordCloseSocket(socket);
    }
  }
}

#endif

void PcP2PNetworkingService::setJoinLocation(JoinLocation location) {
#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  MutexLocker discordLocker(m_state->discordMutex);
#endif
  MutexLocker serviceLocker(m_mutex);

  if (location == m_joinLocation)
    return;
  m_joinLocation = location;

#ifdef STAR_ENABLE_STEAM_INTEGRATION
  if (m_state->steamAvailable) {
    if (m_joinLocation.is<JoinUnavailable>()) {
      Logger::info("Clearing Steam rich presence connection");
      SteamFriends()->SetRichPresence("connect", "");

    } else if (m_joinLocation.is<JoinLocal>()) {
      auto steamId = SteamUser()->GetSteamID().ConvertToUint64();
      Logger::info("Setting Steam rich presence connection as steamid_{}", steamId);
      SteamFriends()->SetRichPresence("connect", strf("+platform:connect:steamid_{}", steamId).c_str());

    } else if (m_joinLocation.is<JoinRemote>()) {
      auto address = (HostAddressWithPort)location.get<JoinRemote>();
      Logger::info("Setting Steam rich presence connection as address_{}", address);
      SteamFriends()->SetRichPresence("connect", strf("+platform:connect:address_{}", address).c_str());
    }
  }
#endif

#ifdef STAR_ENABLE_DISCORD_INTEGRATION
  if (m_state->discordAvailable && m_state->discordCurrentUser) {
    if (m_discordServerLobby) {
      Logger::info("Deleting Discord server lobby {}", m_discordServerLobby->first);
      m_state->discordCore->LobbyManager().DeleteLobby(m_discordServerLobby->first, [](discord::Result res) {
          Logger::error("Could not delete Discord server lobby (err {})", (int)res);
        });
    }

    if (auto joinLocal = m_joinLocation.maybe<JoinLocal>()) {
      discord::LobbyTransaction createLobby{};
      if (m_state->discordCore->LobbyManager().GetLobbyCreateTransaction(&createLobby) != discord::Result::Ok)
        throw ApplicationException::format("discord::Lobbies::CreateLobbyTransaction failed");

      createLobby.SetCapacity(joinLocal->capacity);
      createLobby.SetType(discord::LobbyType::Private);
      m_state->discordCore->LobbyManager().CreateLobby(createLobby, [this](discord::Result res, discord::Lobby const& lobby) {
          if (res == discord::Result::Ok) {
            MutexLocker serviceLocker(m_mutex);
            
            discord::LobbyId lobbyId = lobby.GetId();
            
            res = m_state->discordCore->LobbyManager().ConnectNetwork(lobbyId);
            if (res == discord::Result::Ok) {
              res = m_state->discordCore->LobbyManager().OpenNetworkChannel(lobbyId, DiscordMainNetworkChannel, true);
              if (res == discord::Result::Ok) {
                m_discordServerLobby = make_pair(lobbyId, String(lobby.GetSecret()));
            m_discordForceUpdateActivity = true;

                // successfully joined lobby network
                return;
              } else {
                Logger::error("Failed to open Discord main network channel (err {})", (int)res);
              }
            } else {
              Logger::error("Failed to join Discord lobby network (err {})", (int)res);
            }

            // Created lobby but failed to join the lobby network, delete lobby
            Logger::error("Failed to join Discord lobby network (err {})", (int)res);

            Logger::info("Deleting Discord lobby {}", lobbyId);
            m_state->discordCore->LobbyManager().DeleteLobby(lobbyId, [lobbyId](discord::Result res) {
                Logger::error("Failed to delete Discord lobby {} (err {})", lobbyId, (int)res);
              });
          } else {
            Logger::error("Failed to create Discord lobby (err {})", (int)res);
          }
        });
    }
  }
#endif
}

}
