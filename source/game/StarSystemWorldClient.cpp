#include "StarSystemWorldClient.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarClientContext.hpp"
#include "StarPlayerUniverseMap.hpp"

namespace Star {

SystemWorldClient::SystemWorldClient(ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase, PlayerUniverseMapPtr universeMap)
  : SystemWorld(universeClock, celestialDatabase), m_universeMap(std::move(universeMap)) {}

CelestialCoordinate SystemWorldClient::currentSystem() const {
  return CelestialCoordinate(m_location);
}

Maybe<Vec2F> SystemWorldClient::shipPosition() const {
  if (m_ship)
    return m_ship->position();
  else
    return {};
}

SystemLocation SystemWorldClient::shipLocation() const {
  if (m_ship)
    return m_ship->systemLocation();
  else
    return {};
}

SystemLocation SystemWorldClient::shipDestination() const {
  if (m_ship)
    return m_ship->destination();
  else
    return {};
}

// ship is flying if the system world is uninitialized or the ship doesn't have a location
bool SystemWorldClient::flying() const {
  if (m_ship)
    return m_ship->systemLocation().empty();
  return true;
}

void SystemWorldClient::update(float dt) {
  if (!m_ship)
    return;

  m_ship->clientUpdate(dt);

  auto location = m_ship->systemLocation();
  if (auto uuid = location.maybe<Uuid>()) {
    if (auto object = getObject(*uuid)) {
      Maybe<CelestialOrbit> orbit;

      if (object->permanent())
        m_universeMap->addMappedObject(CelestialCoordinate(m_location), *uuid, object->name(), object->orbit(), object->parameters());
      else
        m_universeMap->addMappedObject(CelestialCoordinate(m_location), *uuid, object->name());
    }
  } else if (auto coordinate = location.maybe<CelestialCoordinate>()) {
    if (coordinate->isPlanetaryBody() || coordinate->isSatelliteBody())
      m_universeMap->addMappedCoordinate(coordinate->planet());
  } else if (auto orbit = location.maybe<CelestialOrbit>()) {
    if (orbit->target.isPlanetaryBody() || orbit->target.isSatelliteBody())
      m_universeMap->addMappedCoordinate(orbit->target.planet());
  }

  for (auto p : m_clientShips)
    p.second->clientUpdate(dt);
  for (auto p : m_objects)
    p.second->clientUpdate(dt);

  if (currentSystem().isNull()) {
    m_objects.clear();
    m_clientShips.clear();
    m_ship = {};
    m_location = Vec3I();
  } else if (auto celestialSlave = as<CelestialSlaveDatabase>(m_celestialDatabase))
    celestialSlave->signalSystem(currentSystem()); // keeps the celestial chunk for our current system alive
}

List<SystemObjectPtr> SystemWorldClient::objects() const {
  return m_objects.values();
}

List<Uuid> SystemWorldClient::objectKeys() const {
  return m_objects.keys();
}

SystemObjectPtr SystemWorldClient::getObject(Uuid const& uuid) const {
  return m_objects.maybe(uuid).value({});
}

List<SystemClientShipPtr> SystemWorldClient::ships() const
{
  return m_clientShips.values();
}

SystemClientShipPtr SystemWorldClient::getShip(Uuid const & uuid) const
{
  return m_clientShips.maybe(uuid).value({});
}

Uuid SystemWorldClient::spawnObject(String typeName, Maybe<Vec2F> position, Maybe<Uuid> const& uuid, JsonObject overrides) {
  Uuid objectUuid = uuid.value(Uuid());
  m_outgoingPackets.append(make_shared<SystemObjectSpawnPacket>(std::move(typeName), objectUuid, std::move(position), std::move(overrides)));
  return objectUuid;
}

bool SystemWorldClient::handleIncomingPacket(PacketPtr packet) {
  if (auto updatePacket = as<SystemWorldUpdatePacket>(packet)) {
    auto location = m_ship->systemLocation();
    for (auto p : updatePacket->shipUpdates) {
      if (m_ship && p.first == m_ship->uuid())
        m_ship->readNetState(p.second, SystemWorldTimestep);
      else
        m_clientShips[p.first]->readNetState(p.second, SystemWorldTimestep);
    }
    for (auto p : updatePacket->objectUpdates) {
      auto object = getObject(p.first);
      object->readNetState(p.second, SystemWorldTimestep);
    }

  } else if (auto createPacket = as<SystemObjectCreatePacket>(packet)) {
    auto object = netLoadObject(createPacket->objectStore);
    m_objects.set(object->uuid(), object);

  } else if (auto destroyPacket = as<SystemObjectDestroyPacket>(packet)) {
    m_objects.remove(destroyPacket->objectUuid);
    m_universeMap->removeMappedObject(CelestialCoordinate(m_location), destroyPacket->objectUuid);

  } else if (auto shipCreatePacket = as<SystemShipCreatePacket>(packet)) {
    auto ship = netLoadShip(shipCreatePacket->shipStore);
    m_clientShips.set(ship->uuid(), ship);

  } else if (auto shipDestroyPacket = as<SystemShipDestroyPacket>(packet)) {
    m_clientShips.remove(shipDestroyPacket->shipUuid);

  } else if (auto startPacket = as<SystemWorldStartPacket>(packet)) {
    m_objects.clear();
    m_clientShips.clear();
    m_location = startPacket->location;
    for (auto netStore: startPacket->objectStores) {
      auto object = netLoadObject(netStore);
      m_objects.set(object->uuid(), object);
    }
    for (auto netStore : startPacket->shipStores) {
      auto ship = netLoadShip(netStore);
      m_clientShips.set(ship->uuid(), ship);
    }
    m_ship = make_shared<SystemClientShip>(this, startPacket->clientShip.first, startPacket->clientShip.second);

    m_universeMap->addMappedCoordinate(CelestialCoordinate(m_location));
    m_universeMap->filterMappedObjects(CelestialCoordinate(m_location), m_objects.keys());
  } else {
    // packet type not handled by system world client
    return false;
  }

  // packet was handled
  return true;
}

List<PacketPtr> SystemWorldClient::pullOutgoingPackets() {
  return take(m_outgoingPackets);
}

SystemObjectPtr SystemWorldClient::netLoadObject(ByteArray netStore) {
  DataStreamBuffer ds(std::move(netStore));
  Uuid uuid = ds.read<Uuid>();

  String name = ds.read<String>();
  auto objectConfig = systemObjectConfig(name, uuid);

  Vec2F position = ds.read<Vec2F>();

  JsonObject parameters = ds.read<JsonObject>();
  return make_shared<SystemObject>(objectConfig, uuid, position, parameters);
}

SystemClientShipPtr SystemWorldClient::netLoadShip(ByteArray netStore)
{
  DataStreamBuffer ds(std::move(netStore));
  Uuid uuid = ds.read<Uuid>();
  SystemLocation location = ds.read<SystemLocation>();
  return make_shared<SystemClientShip>(this, uuid, location);
}

}
