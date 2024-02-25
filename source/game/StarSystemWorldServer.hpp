#pragma once

#include "StarSystemWorld.hpp"
#include "StarUuid.hpp"

namespace Star {

STAR_CLASS(SystemWorldServer);

STAR_STRUCT(Packet);

class SystemWorldServer : public SystemWorld {
public:
  // create new system world server
  SystemWorldServer(Vec3I location, ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase);
  // load system world server from storage
  SystemWorldServer(Json const& diskStore, ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase);

  void setClientDestination(ConnectionId const& clientId, SystemLocation const& destination);

  // null while flying, system coordinate when in space or at a system object
  // planet coordinates while orbiting a planet
  SystemClientShipPtr clientShip(ConnectionId clientId) const;
  SystemLocation clientShipLocation(ConnectionId clientId) const;
  Maybe<pair<WarpAction, WarpMode>> clientWarpAction(ConnectionId clientId) const;
  SkyParameters clientSkyParameters(ConnectionId clientId) const;

  List<ConnectionId> clients() const;
  void addClientShip(ConnectionId clientId, Uuid const& uuid, float shipSpeed, SystemLocation location);
  void removeClientShip(ConnectionId clientId);
  List<SystemClientShipPtr> shipsAtLocation(SystemLocation const& location) const;
  List<InstanceWorldId> activeInstanceWorlds() const;

  // removeObject queues up object for destruction, any ships at the location
  // are moved away
  void removeObject(Uuid objectUuid);
  bool addObject(SystemObjectPtr object, bool doRangeCheck = false);

  void update(float dt);

  List<SystemObjectPtr> objects() const override;
  List<Uuid> objectKeys() const override;
  SystemObjectPtr getObject(Uuid const& uuid) const override;

  List<ConnectionId> pullShipFlights();

  void handleIncomingPacket(ConnectionId clientId, PacketPtr packet);
  List<PacketPtr> pullOutgoingPackets(ConnectionId clientId);

  bool triggeredStorage();
  Json diskStore();

private:
  struct ClientNetVersions {
    HashMap<Uuid, uint64_t> ships;
    HashMap<Uuid, uint64_t> objects;
  };

  void queueUpdatePackets();
  void setClientShipDestination(ConnectionId clientId, SystemLocation const& destination);

  // random position for new ships entering the system
  void placeInitialObjects();
  void spawnObjects();
  Vec2F randomObjectSpawnPosition(RandomSource& rand) const;

  SkyParameters locationSkyParameters(SystemLocation const& location) const;

  // setting this to true asynchronously triggers storage from the server thread
  bool m_triggerStorage;
  
  double m_lastSpawn;
  double m_objectSpawnTime;

  // objects to be destroyed as soon as there are no ships at the location
  List<Uuid> m_objectDestroyQueue;
  // ships to be destroyed after update packets have been queued
  List<Uuid> m_shipDestroyQueue;

  HashMap<ConnectionId, ClientNetVersions> m_clientNetVersions;
  HashMap<ConnectionId, Uuid> m_clientShips;
  HashMap<Uuid, SystemObjectPtr> m_objects;
  HashMap<Uuid, SystemClientShipPtr> m_ships;
  // client ID and flight start position
  List<ConnectionId> m_shipFlights;

  HashMap<ConnectionId, List<PacketPtr>> m_outgoingPackets;
};


}
