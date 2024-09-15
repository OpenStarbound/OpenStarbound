#pragma once

#include "StarNetElementSystem.hpp"
#include "StarThread.hpp"
#include "StarUuid.hpp"
#include "StarJsonRpc.hpp"
#include "StarDamageTypes.hpp"
#include "StarGameTypes.hpp"
#include "StarHostAddress.hpp"
#include "StarClientContext.hpp"
#include "StarWorldStorage.hpp"
#include "StarSystemWorld.hpp"

namespace Star {

STAR_CLASS(WorldServerThread);
STAR_CLASS(SystemWorldServerThread);
STAR_CLASS(ServerClientContext);

class ServerClientContext {
public:
  ServerClientContext(ConnectionId clientId, Maybe<HostAddress> remoteAddress, NetCompatibilityRules netRules, Uuid playerUuid,
      String playerName, String playerSpecies, bool canBecomeAdmin, WorldChunks initialShipChunks);

  ConnectionId clientId() const;
  Maybe<HostAddress> const& remoteAddress() const;
  Uuid const& playerUuid() const;
  String const& playerName() const;
  String const& playerSpecies() const;
  bool canBecomeAdmin() const;
  NetCompatibilityRules netRules() const;
  String descriptiveName() const;

  // Register additional rpc methods from other server side services.
  void registerRpcHandlers(JsonRpcHandlers const& rpcHandlers);

  // The coordinate for the world which the *player's* ship is currently
  // orbiting, if it is currently orbiting a world.
  CelestialCoordinate shipCoordinate() const;
  void setShipCoordinate(CelestialCoordinate shipCoordinate);

  SystemLocation shipLocation() const;
  void setShipLocation(SystemLocation location);

  // Warp action and warp mode to the planet the player is currently orbiting
  // valid when the player is on any ship world orbiting a location
  Maybe<pair<WarpAction, WarpMode>> orbitWarpAction() const;
  void setOrbitWarpAction(Maybe<pair<WarpAction, WarpMode>> warpAction);

  bool isAdmin() const;
  void setAdmin(bool admin);

  EntityDamageTeam team() const;
  void setTeam(EntityDamageTeam team);

  ShipUpgrades shipUpgrades() const;
  void setShipUpgrades(ShipUpgrades shipUpgrades);

  WorldChunks shipChunks() const;
  void updateShipChunks(WorldChunks newShipChunks);

  ByteArray writeInitialState() const;

  void readUpdate(ByteArray data);
  ByteArray writeUpdate();

  void setPlayerWorld(WorldServerThreadPtr worldThread);
  WorldServerThreadPtr playerWorld() const;
  WorldId playerWorldId() const;
  void clearPlayerWorld();

  void setSystemWorld(SystemWorldServerThreadPtr systemWorldThread);
  SystemWorldServerThreadPtr systemWorld() const;
  void clearSystemWorld();

  WarpToWorld playerReturnWarp() const;
  void setPlayerReturnWarp(WarpToWorld warp);

  WarpToWorld playerReviveWarp() const;
  void setPlayerReviveWarp(WarpToWorld warp);

  // Store and load the data for this client that should be persisted on the
  // server, such as celestial log data, admin state, team, and current ship
  // location, and warp history.  Does not store ship data or ship upgrades.
  void loadServerData(Json const& store);
  Json storeServerData();

private:
  ConnectionId const m_clientId;
  Maybe<HostAddress> const m_remoteAddress;
  NetCompatibilityRules m_netRules;
  Uuid const m_playerUuid;
  String const m_playerName;
  String const m_playerSpecies;
  bool const m_canBecomeAdmin;

  mutable RecursiveMutex m_mutex;

  WorldChunks m_shipChunks;
  WorldChunks m_shipChunksUpdate;

  SystemLocation m_shipSystemLocation;
  JsonRpc m_rpc;
  WorldServerThreadPtr m_worldThread;
  WarpToWorld m_returnWarp;
  WarpToWorld m_reviveWarp;

  SystemWorldServerThreadPtr m_systemWorldThread;

  NetElementTopGroup m_netGroup;
  uint64_t m_netVersion = 0;

  NetElementData<Maybe<pair<WarpAction, WarpMode>>> m_orbitWarpActionNetState;
  NetElementData<WorldId> m_playerWorldIdNetState;
  NetElementBool m_isAdminNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;
  NetElementData<ShipUpgrades> m_shipUpgrades;
  NetElementData<CelestialCoordinate> m_shipCoordinate;
};

}
