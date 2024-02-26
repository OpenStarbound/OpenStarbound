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
  ClientContext(Uuid serverUuid, Uuid playerUuid);

  Uuid serverUuid() const;
  // The player Uuid can differ from the mainPlayer's Uuid
  //  if the player has swapped character - use this for ship saving.
  Uuid playerUuid() const;

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

  void readUpdate(ByteArray data);
  ByteArray writeUpdate();

private:
  Uuid m_serverUuid;
  Uuid m_playerUuid;

  JsonRpcPtr m_rpc;

  NetElementTopGroup m_netGroup;
  NetElementData<Maybe<pair<WarpAction, WarpMode>>> m_orbitWarpActionNetState;
  NetElementData<WorldId> m_playerWorldIdNetState;
  NetElementBool m_isAdminNetState;
  NetElementData<EntityDamageTeam> m_teamNetState;
  NetElementData<ShipUpgrades> m_shipUpgrades;
  NetElementData<CelestialCoordinate> m_shipCoordinate;

  WorldChunks m_newShipUpdates;
};

}
