#include "StarSystemWorldServer.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarCelestialGraphics.hpp"
#include "StarClientContext.hpp"
#include "StarNetPackets.hpp"
#include "StarMathCommon.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

SystemWorldServer::SystemWorldServer(Vec3I location, ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase)
  : SystemWorld(std::move(universeClock), std::move(celestialDatabase)) {
  m_location = std::move(location);

  placeInitialObjects();

  m_lastSpawn = time() - systemConfig().objectSpawnCycle;
  m_objectSpawnTime = Random::randf(systemConfig().objectSpawnInterval[0], systemConfig().objectSpawnInterval[1]);
  spawnObjects();
}

SystemWorldServer::SystemWorldServer(Json const& diskStore, ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase)
  : SystemWorld(std::move(universeClock), std::move(celestialDatabase)) {
  m_location = jsonToVec3I(diskStore.get("location"));

  for (auto objectStore : diskStore.getArray("objects")) {
    auto object = make_shared<SystemObject>(this, objectStore);
    m_objects.set(object->uuid(), object);
  }

  m_lastSpawn = diskStore.getDouble("lastSpawn");
  m_objectSpawnTime = diskStore.getDouble("objectSpawnTime");
  spawnObjects();
}

void SystemWorldServer::setClientDestination(ConnectionId const& clientId, SystemLocation const& destination) {
  auto uuid = m_clientShips.get(clientId);
  m_ships[uuid]->setDestination(destination);
}

SystemClientShipPtr SystemWorldServer::clientShip(ConnectionId clientId) const {
  if (m_clientShips.contains(clientId) && m_ships.contains(m_clientShips.get(clientId)))
    return m_ships.get(m_clientShips.get(clientId));
  else
    return {};
}

SystemLocation SystemWorldServer::clientShipLocation(ConnectionId clientId) const {
  return m_ships.get(m_clientShips.get(clientId))->systemLocation();
}

Maybe<pair<WarpAction, WarpMode>> SystemWorldServer::clientWarpAction(ConnectionId clientId) const {
  auto ship = m_ships.get(m_clientShips.get(clientId));
  if (auto objectUuid = ship->systemLocation().maybe<Uuid>()) {
    if (auto action = objectWarpAction(*objectUuid)) {
      return pair<WarpAction, WarpMode>(*action, WarpMode::DeployOnly);
    }
  } else if (auto coordinate = ship->systemLocation().maybe<CelestialCoordinate>()) {
    WarpAction warpAction = WarpToWorld(CelestialWorldId(*coordinate));
    return pair<WarpAction, WarpMode>(warpAction, WarpMode::BeamOrDeploy);
  } else if (auto position = ship->systemLocation().maybe<Vec2F>()) {
    // player can beam to asteroid fields simply by being in proximity to them
    for (auto planet : planets()) {
      if (abs(planetPosition(planet).magnitude() - position->magnitude()) > systemConfig().asteroidBeamDistance)
        continue;

      if (auto parameters = m_celestialDatabase->parameters(planet)) {
        if (auto awp = as<AsteroidsWorldParameters>(parameters->visitableParameters())) {
          float targetX = (position->angle() / (2 * Constants::pi)) * awp->worldSize[0];
          return pair<WarpAction, WarpMode>(WarpAction(WarpToWorld(CelestialWorldId(planet), SpawnTargetX(targetX))),
            WarpMode::DeployOnly);
        }
      }
    }
  }

  return {};
}

SkyParameters SystemWorldServer::clientSkyParameters(ConnectionId clientId) const {
  auto uuid = m_clientShips.get(clientId);
  return locationSkyParameters(m_ships.get(uuid)->systemLocation());
}

List<ConnectionId> SystemWorldServer::clients() const {
  return m_clientShips.keys();
}

void SystemWorldServer::addClientShip(ConnectionId clientId, Uuid const& uuid, float shipSpeed, SystemLocation location) {
  if (auto objectUuid = location.maybe<Uuid>()) {
    if (getObject(*objectUuid) == nullptr)
      location.reset();
  }
  if (!location)
    location = randomArrivalPosition();

  SystemClientShipPtr ship = make_shared<SystemClientShip>(this, uuid, shipSpeed, location);
  m_clientShips.set(clientId, ship->uuid());
  m_ships.set(ship->uuid(), ship);
  m_clientNetVersions.set(clientId, {{}, {} });
  m_outgoingPackets.set(clientId, {});

  List<ByteArray> objectStores = m_objects.values().transformed([](SystemObjectPtr const& o) { return o->netStore(); });
  List<ByteArray> shipStores = m_ships.values().filtered([uuid](SystemClientShipPtr const& s) {
    return s->uuid() != uuid;
  }).transformed([](SystemClientShipPtr const& s) {
    return s->netStore();
  });
  pair<Uuid, SystemLocation> clientShip = {ship->uuid(), ship->systemLocation()};
  m_outgoingPackets[clientId].append(make_shared<SystemWorldStartPacket>(m_location, objectStores, shipStores, clientShip));

  for (ConnectionId otherClient : m_clientShips.keys()) {
    if (otherClient != clientId)
      m_outgoingPackets[otherClient].append(make_shared<SystemShipCreatePacket>(ship->netStore()));
  }
}

void SystemWorldServer::removeClientShip(ConnectionId clientId) {
  m_shipDestroyQueue.append(m_clientShips.get(clientId));
  m_clientShips.remove(clientId);
  m_clientNetVersions.remove(clientId);
  m_outgoingPackets.remove(clientId);
}

List<SystemClientShipPtr> SystemWorldServer::shipsAtLocation(SystemLocation const& location) const {
  return m_ships.values().filtered([location](auto const& ship) { return ship->systemLocation() == location; });
}

List<InstanceWorldId> SystemWorldServer::activeInstanceWorlds() const {
  // Find the warp actions for all ships located at objects
  List<Maybe<WarpAction>> warpActions = m_clientShips.keys().transformed([this](ConnectionId const& clientId) -> Maybe<WarpAction> {
      return clientWarpAction(clientId).apply([](auto const& p) { return p.first; });
    });
  // Return a list of the ones which lead to instance worlds
  return warpActions.filtered([](Maybe<WarpAction> const& action) {
      if (action.isNothing())
        return false;

      if (auto warpToWorld = action->maybe<WarpToWorld>()) {
        if (auto instanceWorldId = warpToWorld->world.maybe<InstanceWorldId>())
          return true;
      }
      return false;
  }).transformed([](Maybe<WarpAction> const& action) { return action->get<WarpToWorld>().world.get<InstanceWorldId>(); });

}

void SystemWorldServer::removeObject(Uuid objectUuid) {
  if (!m_objects.contains(objectUuid))
    throw StarException(strf("Cannot remove object with uuid '{}', object doesn't exist.", objectUuid.hex()));

  if (m_objects[objectUuid]->permanent())
    throw StarException(strf("Cannot remove object with uuid '{}', object is marked permanent", objectUuid.hex()));

  // already removing it
  if (m_objectDestroyQueue.contains(objectUuid))
    return;

  // fly away any active ships that are located at the object
  for (auto p : m_clientShips) {
    auto ship = m_ships.get(p.second);
    auto location = ship->systemLocation();
    if (location == objectUuid || ship->destination() == objectUuid) {
      ship->setDestination(*systemLocationPosition(objectUuid));
      if (!ship->flying())
        m_shipFlights.append(p.first);
    }
  }

  m_objectDestroyQueue.append(objectUuid);
}

bool SystemWorldServer::addObject(SystemObjectPtr object, bool doRangeCheck) {
  if (doRangeCheck) {
    CelestialCoordinate system = CelestialCoordinate(m_location);
    CelestialCoordinate outer = system.child(m_celestialDatabase->childOrbits(system).sorted().last());
    List<pair<float, float>> orbitDistances;
    for (auto planet : planets()) {
      orbitDistances.append({planetOrbitDistance(planet), clusterSize(planet) / 2.0});
    }
    for (auto o : m_objects.values()) {
      if (o->permanent())
        orbitDistances.append({o->position().magnitude(), 0.0});
    }

    float maxRange = planetOrbitDistance(outer) + (clusterSize(outer) / 2.0) + systemConfig().clientObjectSpawnPadding;
    // Allow objectSpawnPadding of room outside the farthest orbit to have an object placed in it
    maxRange += systemConfig().clientObjectSpawnPadding;
    float minRange = (planetSize(system) / 2.0) + systemConfig().clientObjectSpawnPadding;
    float radius = object->position().magnitude();
    if (radius > maxRange || radius < minRange)
      return false;
    for (pair<float, float> p : orbitDistances) {
      if (abs(radius - p.first) < p.second + systemConfig().clientObjectSpawnPadding)
        return false;
    }
  }

  m_objects.set(object->uuid(), object);

  auto objectStore = object->netStore();
  for (auto clientId : m_clientShips.keys()) {
    m_outgoingPackets[clientId].append(make_shared<SystemObjectCreatePacket>(objectStore));
  }

  m_triggerStorage = true;
  return true;
}

void SystemWorldServer::update(float dt) {
  for (auto p : m_ships)
    p.second->serverUpdate(this, dt);

  for (auto p : m_objects) {
    p.second->serverUpdate(this, dt);

    // don't destroy objects that still have players at them
    if (p.second->shouldDestroy() && shipsAtLocation(p.first).size() == 0)
      removeObject(p.first);
  }

  spawnObjects();

  queueUpdatePackets();

  // remove objects and ships after queueing update packets to ensure they're not updated after being removed
  for (auto objectUuid : take(m_objectDestroyQueue)) {
    for (auto p : m_clientNetVersions) {
      p.second.objects.remove(objectUuid);
      m_outgoingPackets[p.first].append(make_shared<SystemObjectDestroyPacket>(objectUuid));
    }
    m_objects.remove(objectUuid);
    m_triggerStorage = true;
  }
  for (auto shipUuid : take(m_shipDestroyQueue)) {
    for (auto p : m_clientNetVersions) {
      p.second.ships.remove(shipUuid);
      m_outgoingPackets[p.first].append(make_shared<SystemShipDestroyPacket>(shipUuid));
    }
    m_ships.remove(shipUuid);
    m_triggerStorage = true;
  }
}

List<SystemObjectPtr> SystemWorldServer::objects() const {
  return m_objects.values();
}

List<Uuid> SystemWorldServer::objectKeys() const {
  return m_objects.keys();
}

SystemObjectPtr SystemWorldServer::getObject(Uuid const& uuid) const {
  return m_objects.maybe(uuid).value({});
}

List<ConnectionId> SystemWorldServer::pullShipFlights() {
  return take(m_shipFlights);
}

void SystemWorldServer::queueUpdatePackets() {
  for (auto clientId : m_clientNetVersions.keys()) {
    auto versions = m_clientNetVersions.ptr(clientId);

    HashMap<Uuid, ByteArray> shipUpdates;
    for (auto ship : m_ships.values()) {
      uint64_t version = versions->ships.maybe(ship->uuid()).value(0);
      auto shipUpdate = ship->writeNetState(version);
      versions->ships.set(ship->uuid(), shipUpdate.second);
      if (!shipUpdate.first.empty())
        shipUpdates.set(ship->uuid(), shipUpdate.first);
    }

    HashMap<Uuid, ByteArray> objectUpdates;
    for (auto object : m_objects.values()) {
      uint64_t version = versions->objects.maybe(object->uuid()).value(0);
      auto objectUpdate = object->writeNetState(version);
      versions->objects.set(object->uuid(), objectUpdate.second);
      if (!objectUpdate.first.empty())
        objectUpdates.set(object->uuid(), objectUpdate.first);
    }
    m_outgoingPackets[clientId].append(make_shared<SystemWorldUpdatePacket>(objectUpdates, shipUpdates));
  }
}

void SystemWorldServer::handleIncomingPacket(ConnectionId, PacketPtr packet) {
  if (auto objectSpawn = as<SystemObjectSpawnPacket>(packet)) {
    RandomSource rand = RandomSource();
    Vec2F position = objectSpawn->position.value(randomObjectSpawnPosition(rand));
    auto object = make_shared<SystemObject>(systemObjectConfig(objectSpawn->typeName, objectSpawn->uuid), objectSpawn->uuid, position, time(), objectSpawn->parameters);
    addObject(object, objectSpawn->position.isValid());
  }
}

List<PacketPtr> SystemWorldServer::pullOutgoingPackets(ConnectionId clientId) {
  return take(m_outgoingPackets[clientId]);
}

bool SystemWorldServer::triggeredStorage() {
  bool store = m_triggerStorage;
  m_triggerStorage = false;
  return store;
}

Json SystemWorldServer::diskStore() {
  JsonArray storedObjects;
  for (auto o : m_objects)
    storedObjects.append(o.second->diskStore());

  JsonObject store;
  store.set("location", jsonFromVec3I(m_location));
  store.set("objects", storedObjects);
  store.set("lastSpawn", m_lastSpawn);
  store.set("objectSpawnTime", m_objectSpawnTime);
  return store;
}

void SystemWorldServer::placeInitialObjects() {
  auto config = Root::singleton().assets()->json("/systemworld.config");
  RandomSource rand(staticRandomU64("SystemWorldGeneration", toString(m_location)));

  WeightedPool<JsonArray> spawnPools = jsonToWeightedPool<JsonArray>(config.getArray("initialObjectPools"));
  JsonArray spawn = spawnPools.select(rand);
  int count = spawn.get(0).toInt();
  if (count > 0) {
    WeightedPool<String> objectPool = jsonToWeightedPool<String>(spawn.get(1).toArray());
    for (int i = 0; i < count; i++) {
      Uuid uuid = Uuid();
      auto objectConfig = systemObjectConfig(objectPool.select(rand), uuid);
      Vec2F position = randomObjectSpawnPosition(rand);

      auto object = make_shared<SystemObject>(objectConfig, uuid, position, time());
      object->enterOrbit(CelestialCoordinate(m_location), { 0.0, 0.0 }, time()); // orbit center of system
      m_objects.set(uuid, object);
    }
  }
}

void SystemWorldServer::spawnObjects() {
  double diff = min(systemConfig().objectSpawnCycle, time() - m_lastSpawn);
  m_lastSpawn = time() - diff;
  while (diff > m_objectSpawnTime) {
    m_lastSpawn += m_objectSpawnTime;
    m_objectSpawnTime = Random::randf(systemConfig().objectSpawnInterval[0], systemConfig().objectSpawnInterval[1]);
    diff = time() - m_lastSpawn;

    WeightedPool<String> spawnPool = jsonToWeightedPool<String>(Root::singleton().assets()->json("/systemworld.config:objectSpawnPool").toArray());
    String name = spawnPool.select();
    Uuid uuid = Uuid();
    auto objectConfig = systemObjectConfig(name, uuid);

    SystemObjectPtr object;
    RandomSource rand = RandomSource(Random::randu64());
    Vec2F position = randomObjectSpawnPosition(rand);
    if (time() > m_lastSpawn + m_objectSpawnTime && objectConfig.moving) {
      // if this is not the last object we're spawning, and it's moving, immediately put it in orbit around a planet
      auto targets = planets().filtered([this](CelestialCoordinate const& p) {
        auto objectsAtPlanet = objects().filtered([p](SystemObjectPtr const& o) { return o->orbitTarget() == p; });
        return objectsAtPlanet.size() == 0;
      });
      if (targets.size() > 0) {
        auto target = Random::randFrom(targets);

        Vec2F targetPosition = planetPosition(target);
        Vec2F relativeOrbit = (position - targetPosition).normalized() * (clusterSize(target) / 2.0 + objectConfig.orbitDistance);
        object = make_shared<SystemObject>(objectConfig, uuid, targetPosition + relativeOrbit, m_lastSpawn);

        object->enterOrbit(target, planetPosition(target), m_lastSpawn);
      } else {
        object = make_shared<SystemObject>(objectConfig, uuid, position, m_lastSpawn);
      }
    } else {
      object = make_shared<SystemObject>(objectConfig, uuid, position, m_lastSpawn);
    }
    addObject(object);
  }
}

Vec2F SystemWorldServer::randomObjectSpawnPosition(RandomSource& rand) const {
  List<Vec2F> spawnRanges;
  CelestialCoordinate system = CelestialCoordinate(m_location);
  auto config = systemConfig();
  auto orbits = m_celestialDatabase->childOrbits(CelestialCoordinate(m_location)).sorted();

  auto addSpawn = [this,&config,&spawnRanges](CelestialCoordinate const& inner, CelestialCoordinate const& outer) {
    float min = planetOrbitDistance(inner) + (clusterSize(inner) / 2.0) + config.objectSpawnPadding;
    float max = planetOrbitDistance(outer) - (clusterSize(outer) / 2.0) - config.objectSpawnPadding;
    spawnRanges.append(Vec2F(min, max));
  };

  addSpawn(system, system.child(orbits[0]));
  for (size_t i = 1; i < orbits.size(); i++)
    addSpawn(system.child(orbits[i - 1]), system.child(orbits[i]));

  CelestialCoordinate outer = system.child(orbits.last());
  float rim = planetOrbitDistance(outer) + (clusterSize(outer) / 2.0) + config.objectSpawnPadding;
  spawnRanges.append(Vec2F(rim, rim + config.objectSpawnPadding));

  auto range = rand.randFrom(spawnRanges);
  return Vec2F::withAngle(rand.randf() * Constants::pi * 2.0, range[0] + (rand.randf() * (range[1] - range[0])));
}

SkyParameters SystemWorldServer::locationSkyParameters(SystemLocation const& location) const {
  SkyParameters skyParameters = systemConfig().emptySkyParameters;

  if (auto coordinate = location.maybe<CelestialCoordinate>()) {
    return SkyParameters(*coordinate, m_celestialDatabase);
  } else if (auto position = location.maybe<Vec2F>()) {
    for (auto planet : planets()) {
      if (abs(position->magnitude() - planetPosition(planet).magnitude()) > systemConfig().asteroidBeamDistance)
        continue;

      if (auto parameters = m_celestialDatabase->parameters(planet)) {
        if (auto asteroidsParameters = as<AsteroidsWorldParameters>(parameters->visitableParameters())) {
          return SkyParameters(planet, m_celestialDatabase);
        }
      }
    }
  } else if (!location.empty()) {
    CelestialCoordinate orbitTarget;
    if (auto objectUuid = location.maybe<Uuid>()) {
      auto object = getObject(*objectUuid);
      skyParameters = object->skyParameters();
      if (auto target = object->orbitTarget())
        orbitTarget = *target;
    } else if (auto orbit = location.maybe<CelestialOrbit>()) {
      orbitTarget = orbit->target;
    }

    if (orbitTarget.isPlanetaryBody()) {
      auto parameters = m_celestialDatabase->parameters(orbitTarget);

      if (auto visitableParameters = parameters->visitableParameters()) {
        if (is<TerrestrialWorldParameters>(visitableParameters)) {
          uint64_t seed = staticRandomU64(toString(m_location));
          List<CelestialParameters> worlds;
          if (auto planet = m_celestialDatabase->parameters(orbitTarget))
            worlds.append(*planet);
          for (auto coordinate : m_celestialDatabase->children(orbitTarget)) {
            if (auto satellite = m_celestialDatabase->parameters(coordinate))
              worlds.append(*satellite);
          }

          for (uint64_t i = 0; i < worlds.size(); i++) {
            auto world = worlds.get(i);
            Vec2F pos = {
              staticRandomFloat(seed, world.seed(), "x"),
              staticRandomFloat(seed, world.seed(), "y")
            };
            CelestialParameters parent = i > 0 ? worlds[0] : CelestialParameters();
            skyParameters.nearbyMoons.append({CelestialGraphics::drawWorld(world, parent), pos});
          }
        } else {
          // put orbited horizon behind existing horizon images
          skyParameters.horizonImages.insertAllAt(0, CelestialGraphics::worldHorizonImages(*parameters));
        }
      }
    }
    return skyParameters;
  }

  return skyParameters;
}

}
