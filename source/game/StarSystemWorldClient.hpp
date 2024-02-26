#pragma once

#include "StarCelestialParameters.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarSystemWorld.hpp"
#include "StarNetPackets.hpp"

namespace Star {

STAR_CLASS(CelestialDatabase);
STAR_CLASS(PlayerUniverseMap);
STAR_CLASS(Celestial);
STAR_CLASS(Clock);
STAR_CLASS(ClientContext);

class SystemWorldClient : public SystemWorld {
public:
  SystemWorldClient(ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase, PlayerUniverseMapPtr clientContext);

  CelestialCoordinate currentSystem() const;

  Maybe<Vec2F> shipPosition() const;
  SystemLocation shipLocation() const;
  SystemLocation shipDestination() const;
  bool flying() const;

  void update(float dt);

  List<SystemObjectPtr> objects() const override;
  List<Uuid> objectKeys() const override;
  SystemObjectPtr getObject(Uuid const& uuid) const override;

  List<SystemClientShipPtr> ships() const;
  SystemClientShipPtr getShip(Uuid const& uuid) const;

  Uuid spawnObject(String typeName, Maybe<Vec2F> position = {}, Maybe<Uuid> const& uuid = {}, JsonObject parameters = {});

  // returns whether the packet was handled
  bool handleIncomingPacket(PacketPtr packet);
  List<PacketPtr> pullOutgoingPackets();
private:
  SystemObjectPtr netLoadObject(ByteArray netStore);
  SystemClientShipPtr netLoadShip(ByteArray netStore);

  // m_ship can be a null pointer, indicating that the system is not initialized
  SystemClientShipPtr m_ship;
  HashMap<Uuid, SystemObjectPtr> m_objects;
  HashMap<Uuid, SystemClientShipPtr> m_clientShips;

  PlayerUniverseMapPtr m_universeMap;

  List<PacketPtr> m_outgoingPackets;
};


}
