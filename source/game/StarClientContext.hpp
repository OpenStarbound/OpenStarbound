#pragma once

#include "StarNetElementSystem.hpp"
#include "StarJsonRpc.hpp"
#include "StarGameTypes.hpp"
#include "StarDamageTypes.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarWarping.hpp"
#include "StarWorldStorage.hpp"
#include "StarPlayerTypes.hpp"

namespace Star {

STAR_CLASS(CelestialLog);
STAR_CLASS(ClientContext);

class ClientContext {
public:
  ClientContext(Uuid serverUuid, Uuid playerUuid, Uuid clientUuid);

  Uuid serverUuid() const;
  // Original player uuid for the player used to join the server.
  // The player Uuid can differ from the mainPlayer's Uuid
  //  if the player has swapped character - use this for ship saving.
  Uuid playerUuid() const;
  
  // Client uuid sent to the server, used for ship/custom world ids as well as server client context data.
  // If consistent client uuid is enabled, differs from player uuid and is consistent.
  Uuid clientUuid() const;

  // The coordinate for the world which the player's ship is currently
  // orbiting.
  CelestialCoordinate shipCoordinate() const;

  Maybe<pair<WarpAction, WarpMode>> orbitWarpAction() const;

  // The current world id of the player
  WorldId playerWorldId() const;

  bool isAdmin() const;
  EntityDamageTeam team() const;

  JsonRpcInterfacePtr rpcInterface() const;

  WorldChunks newShipUpdates();
  ShipUpgrades shipUpgrades() const;
  
  StringMap<WorldChunks> newCustomWorldUpdates();

  void readUpdate(ByteArray data, NetCompatibilityRules rules);
  ByteArray writeUpdate(NetCompatibilityRules rules);

  void setConnectionId(ConnectionId connectionId);
  ConnectionId connectionId() const;

  void setNetCompatibilityRules(NetCompatibilityRules netCompatibilityRules);
  NetCompatibilityRules netCompatibilityRules() const;

private:
  Uuid m_serverUuid;
  Uuid m_playerUuid;
  Uuid m_clientUuid;
  ConnectionId m_connectionId = 0;
  NetCompatibilityRules m_netCompatibilityRules;

  JsonRpcPtr m_rpc;

  NetElementTopGroup m_netGroup;
  NetElementData<Maybe<pair<WarpAction, WarpMode>>> m_orbitWarpActionNetState;
  NetElementData<WorldId> m_playerWorldIdNetState;
  NetElementBool m_isAdminNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;
  NetElementData<ShipUpgrades> m_shipUpgrades;
  NetElementData<CelestialCoordinate> m_shipCoordinate;
  WorldChunks m_newShipUpdates;
  StringMap<WorldChunks> m_newCustomWorldUpdates;
};

}
