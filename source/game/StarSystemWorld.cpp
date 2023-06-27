#include "StarSystemWorld.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarClientContext.hpp"
#include "StarJsonExtra.hpp"
#include "StarSystemWorldServer.hpp"
#include "StarNameGenerator.hpp"

namespace Star {

CelestialOrbit CelestialOrbit::fromJson(Json const& json) {
  return CelestialOrbit {
    CelestialCoordinate(json.get("target")),
    (int)json.getInt("direction"),
    json.getDouble("enterTime"),
    jsonToVec2F(json.get("enterPosition"))
  };
}

Json CelestialOrbit::toJson() const {
  JsonObject json;
  json.set("target", target.toJson());
  json.set("direction", direction);
  json.set("enterTime", enterTime);
  json.set("enterPosition", jsonFromVec2F(enterPosition));
  return json;
}

void CelestialOrbit::write(DataStream& ds) const {
  ds.write(target);
  ds.write(direction);
  ds.write(enterTime);
  ds.write(enterPosition);
}

void CelestialOrbit::read(DataStream& ds) {
  target = ds.read<CelestialCoordinate>();
  direction = ds.read<int>();
  enterTime = ds.read<double>();
  enterPosition = ds.read<Vec2F>();
}

bool CelestialOrbit::operator==(CelestialOrbit const& rhs) const {
  return target == rhs.target
    && direction == rhs.direction
    && enterTime == rhs.enterTime
    && enterPosition == rhs.enterPosition;
}

DataStream& operator>>(DataStream& ds, CelestialOrbit& orbit) {
  orbit.read(ds);
  return ds;
}

DataStream& operator<<(DataStream& ds, CelestialOrbit const& orbit) {
  orbit.write(ds);
  return ds;
}

SystemLocation jsonToSystemLocation(Json const& json) {
  if (auto location = json.optArray()) {
    if (location->get(0).type() == Json::Type::String) {
      String type = location->get(0).toString();
      if (type == "coordinate")
        return CelestialCoordinate(location->get(1));
      else if (type == "orbit")
        return CelestialOrbit::fromJson(location->get(1));
      else if (type == "object")
        return Uuid(location->get(1).toString());
    } else if (auto position = jsonToMaybe<Vec2F>(*location, jsonToVec2F)) {
      return *position;
    }
  }
  return {};
}

Json jsonFromSystemLocation(SystemLocation const& location) {
  if (auto coordinate = location.maybe<CelestialCoordinate>())
    return JsonArray{"coordinate", coordinate->toJson()};
  else if (auto orbit = location.maybe<CelestialOrbit>())
    return JsonArray{"orbit", orbit->toJson()};
  else if(auto uuid = location.maybe<Uuid>())
    return JsonArray{"object", uuid->hex()};
  else
    return jsonFromMaybe<Vec2F>(location.maybe<Vec2F>(), jsonFromVec2F);
}

SystemWorldConfig SystemWorldConfig::fromJson(Json const& json) {
  SystemWorldConfig config;
  config.starGravitationalConstant = json.getFloat("starGravitationalConstant");
  config.planetGravitationalConstant = json.getFloat("planetGravitationalConstant");

  for (auto size : json.getArray("planetSizes"))
    config.planetSizes.set(size.getUInt(0), size.getFloat(1));
  config.emptyOrbitSize = json.getFloat("emptyOrbitSize");
  config.unvisitablePlanetSize = json.getFloat("unvisitablePlanetSize");
  for (auto p : json.getObject("floatingDungeonWorldSizes"))
    config.floatingDungeonWorldSizes.set(p.first, p.second.toFloat());

  config.starSize = json.getFloat("starSize");
  config.planetaryOrbitPadding = jsonToVec2F(json.get("planetaryOrbitPadding"));
  config.satelliteOrbitPadding = jsonToVec2F(json.get("satelliteOrbitPadding"));

  config.arrivalRange = jsonToVec2F(json.get("arrivalRange"));

  config.objectSpawnPadding = json.getFloat("objectSpawnPadding");
  config.clientObjectSpawnPadding = json.getFloat("clientObjectSpawnPadding");
  config.objectSpawnInterval = jsonToVec2F(json.get("objectSpawnInterval"));
  config.objectSpawnCycle = json.getDouble("objectSpawnCycle");
  config.minObjectOrbitTime = json.getFloat("minObjectOrbitTime");

  config.asteroidBeamDistance = json.getFloat("asteroidBeamDistance");

  config.emptySkyParameters = SkyParameters(json.get("emptySkyParameters"));
  return config;
}

SystemWorld::SystemWorld(ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase)
  : m_celestialDatabase(move(celestialDatabase)), m_universeClock(move(universeClock)) {
  m_config = SystemWorldConfig::fromJson(Root::singleton().assets()->json("/systemworld.config"));
}

SystemWorldConfig const& SystemWorld::systemConfig() const {
  return m_config;
}

double SystemWorld::time() const {
  return m_universeClock->time();
}

Vec3I SystemWorld::location() const {
  return m_location;
}

List<CelestialCoordinate> SystemWorld::planets() const {
  return m_celestialDatabase->children(CelestialCoordinate(m_location));
}

uint64_t SystemWorld::coordinateSeed(CelestialCoordinate const& coordinate, String const& seedMix) const {
  auto satellite = coordinate.isSatelliteBody() ? coordinate.orbitNumber() : 0;
  auto planet = coordinate.isSatelliteBody() ? coordinate.parent().orbitNumber() : (coordinate.isPlanetaryBody() && coordinate.orbitNumber()) || 0;
  return staticRandomU64(coordinate.location()[0], coordinate.location()[1], coordinate.location()[2], planet, satellite, seedMix);
}

float SystemWorld::planetOrbitDistance(CelestialCoordinate const& coordinate) const {
  RandomSource random(coordinateSeed(coordinate, "PlanetOrbitDistance"));

  if (coordinate.isSystem() || coordinate.isNull())
    return 0;

  float distance = planetSize(coordinate.parent()) / 2.0;
  for (int i = 0; i < coordinate.orbitNumber(); i++) {
    if (i > 0 && i < coordinate.orbitNumber())
      distance += clusterSize(coordinate.parent().child(i));

    if (coordinate.isPlanetaryBody())
      distance += random.randf(m_config.planetaryOrbitPadding[0], m_config.planetaryOrbitPadding[1]);
    else if (coordinate.isSatelliteBody())
      distance += random.randf(m_config.satelliteOrbitPadding[0], m_config.satelliteOrbitPadding[1]);
  }

  distance += clusterSize(coordinate) / 2.0;

  return distance;
}

float SystemWorld::orbitInterval(float distance, bool isMoon) const {
  float gravityConstant = isMoon ? m_config.planetGravitationalConstant : m_config.starGravitationalConstant;
  float speed =  sqrt(gravityConstant / distance);
  return (distance * 2 * Constants::pi) / speed;
}

Vec2F SystemWorld::orbitPosition(CelestialOrbit const& orbit) const {
  Vec2F targetPosition = orbit.target.isPlanetaryBody() || orbit.target.isSatelliteBody() ? planetPosition(orbit.target) : Vec2F(0, 0);
  float distance = orbit.enterPosition.magnitude();
  float interval = orbitInterval(distance, false);

  float timeOffset = fmod(time() - orbit.enterTime, interval) / interval;
  float angle = (orbit.enterPosition * -1).angle() + (orbit.direction * timeOffset * (Constants::pi * 2));
  return targetPosition + Vec2F::withAngle(angle, distance);
}

float SystemWorld::clusterSize(CelestialCoordinate const& coordinate) const {
  if (coordinate.isPlanetaryBody() && m_celestialDatabase->childOrbits(coordinate.parent()).contains(coordinate.orbitNumber())) {
    auto childOrbits = m_celestialDatabase->childOrbits(coordinate).sorted();
    if (childOrbits.size() > 0) {
      CelestialCoordinate outer = coordinate.child(childOrbits.get(childOrbits.size() - 1));
      return (planetOrbitDistance(outer) * 2) + planetSize(outer);
    } else {
      return planetSize(coordinate);
    }
  } else {
    return planetSize(coordinate);
  }
};

float SystemWorld::planetSize(CelestialCoordinate const& coordinate) const {
  if (coordinate.isNull())
    return 0;

  if (coordinate.isSystem())
    return m_config.starSize;

  if (!m_celestialDatabase->childOrbits(coordinate.parent()).contains(coordinate.orbitNumber()))
    return m_config.emptyOrbitSize;

  if (auto parameters = m_celestialDatabase->parameters(coordinate)) {
    if (auto visitableParameters = parameters->visitableParameters()) {
      float size = 0;
      if (is<FloatingDungeonWorldParameters>(visitableParameters)) {
        if (auto s = m_config.floatingDungeonWorldSizes.maybe(visitableParameters->typeName))
          return *s;
      }
      for (auto s : m_config.planetSizes) {
        if (visitableParameters->worldSize[0] >= s.first)
          size = s.second;
        else
          break;
      }
      return size;
    }
  }
  return m_config.unvisitablePlanetSize;
}

Vec2F SystemWorld::planetPosition(CelestialCoordinate const& coordinate) const {
  if (coordinate.isNull() || coordinate.isSystem())
    return {0.0, 0.0};

  RandomSource random(coordinateSeed(coordinate, "PlanetSystemPosition"));

  Vec2F parentPosition = planetPosition(coordinate.parent());
  float distance = planetOrbitDistance(coordinate);
  float interval = orbitInterval(distance, coordinate.isSatelliteBody());

  double start = random.randf();
  double offset = (fmod(m_universeClock->time(), interval) / interval);
  int direction = random.randf() > 0.5 ? 1 : -1;
  float angle = (start + direction * offset) * (Constants::pi * 2);

  return parentPosition + Vec2F(cos(angle), sin(angle)) * distance;
}

SystemObjectConfig SystemWorld::systemObjectConfig(String const& name, Uuid const& uuid) const {
  RandomSource rand(staticRandomU64(uuid.hex()));

  SystemObjectConfig object;
  auto config = systemObjectTypeConfig(name);
  auto orbitRange = jsonToVec2F(config.get("orbitRange"));
  auto lifeTimeRange = jsonToVec2F(config.get("lifeTime"));

  object.name = name;

  object.moving = config.getBool("moving");
  object.speed = config.getFloat("speed");
  object.orbitDistance = Random::randf(orbitRange[0], orbitRange[1]);
  object.lifeTime = Random::randf(lifeTimeRange[0], lifeTimeRange[1]);

  object.permanent = config.getBool("permanent", false);

  object.warpAction = parseWarpAction(config.getString("warpAction"));
  object.threatLevel = config.optFloat("threatLevel");
  object.skyParameters = SkyParameters(config.get("skyParameters"));
  object.parameters = config.getObject("parameters");

  if (config.contains("generatedParameters")) {
    for (auto p : config.getObject("generatedParameters"))
      object.generatedParameters[p.first] = p.second.toString();
  }

  return object;
}

Json SystemWorld::systemObjectTypeConfig(String const& name) {
  return Root::singleton().assets()->json(strf("/system_objects.config:{}", name));
}

Maybe<Vec2F> SystemWorld::systemLocationPosition(SystemLocation const& location) const {
  if (auto coordinate = location.maybe<CelestialCoordinate>()) {
    return planetPosition(*coordinate);
  } else if (auto orbit = location.maybe<CelestialOrbit>()) {
    return orbitPosition(*orbit);
  } else if (auto objectUuid = location.maybe<Uuid>()) {
    if (auto object = getObject(*objectUuid))
      return object->position();
  } else if (auto position = location.maybe<Vec2F>()) {
    return Vec2F(*position);
  }
  return {};
}

Vec2F SystemWorld::randomArrivalPosition() const {
  RandomSource rand;
  float range = m_config.arrivalRange[0] + (rand.randf() * (m_config.arrivalRange[1] - m_config.arrivalRange[0]));
  float angle = rand.randf() * Constants::pi * 2;
  return Vec2F::withAngle(angle, range);
}

Maybe<WarpAction> SystemWorld::objectWarpAction(Uuid const& uuid) const {
  if (auto object = getObject(uuid)) {
    WarpAction warpAction = object->warpAction();
    if (auto warpToWorld = warpAction.ptr<WarpToWorld>()) {
      if (auto instanceWorldId = warpToWorld->world.ptr<InstanceWorldId>()) {
        instanceWorldId->uuid = object->uuid();
        if (auto parameters = m_celestialDatabase->parameters(CelestialCoordinate(m_location))) {
          Maybe<float> systemThreatLevel = parameters->getParameter("spaceThreatLevel").optFloat();
          instanceWorldId->level = object->threatLevel().orMaybe(systemThreatLevel);
        } else {
          return {};
        }
      }
    }
    return warpAction;
  } else {
    return {};
  }
}

SystemObject::SystemObject(SystemObjectConfig config, Uuid uuid, Vec2F const& position, JsonObject parameters)
  : m_config(move(config)), m_uuid(move(uuid)), m_spawnTime(0.0f), m_parameters(move(parameters)) {
  setPosition(position);
  init();
}

SystemObject::SystemObject(SystemObjectConfig config, Uuid uuid, Vec2F const& position, double spawnTime, JsonObject parameters)
  : m_config(move(config)), m_uuid(move(uuid)), m_spawnTime(move(spawnTime)), m_parameters(move(parameters)) {
  setPosition(position);
  for (auto p : m_config.generatedParameters) {
    if (!m_parameters.contains(p.first))
      m_parameters[p.first] = Root::singleton().nameGenerator()->generateName(p.second);
  }
  init();
}

SystemObject::SystemObject(SystemWorld* system, Json const& diskStore) {
  m_uuid = Uuid(diskStore.getString("uuid"));
  auto name = diskStore.getString("name");
  m_config = system->systemObjectConfig(name, m_uuid);
  m_parameters = diskStore.getObject("parameters", {});

  m_orbit.set(jsonToMaybe<CelestialOrbit>(diskStore.get("orbit"), [](Json const& json) {
      return CelestialOrbit::fromJson(json);
    }));

  m_spawnTime = diskStore.getDouble("spawnTime");

  setPosition(jsonToVec2F(diskStore.get("position")));

  init();
}

void SystemObject::init() {
  m_shouldDestroy = false;

  m_xPosition.setInterpolator(lerp<float, float>);
  m_yPosition.setInterpolator(lerp<float, float>);

  m_netGroup.addNetElement(&m_xPosition);
  m_netGroup.addNetElement(&m_yPosition);
  m_netGroup.addNetElement(&m_orbit);
}

Uuid SystemObject::uuid() const {
  return m_uuid;
}

String SystemObject::name() const {
  return m_config.name;
}

bool SystemObject::permanent() const {
  return m_config.permanent;
}

Vec2F SystemObject::position() const {
  return Vec2F(m_xPosition.get(), m_yPosition.get());
}

WarpAction SystemObject::warpAction() const {
  return m_config.warpAction;
}

Maybe<float> SystemObject::threatLevel() const {
  return m_config.threatLevel;
}

SkyParameters SystemObject::skyParameters() const {
  return m_config.skyParameters;
}

JsonObject SystemObject::parameters() const {
  return jsonMerge(m_config.parameters, m_parameters).toObject();
}

bool SystemObject::shouldDestroy() const {
  return m_shouldDestroy;
}

void SystemObject::enterOrbit(CelestialCoordinate const& target, Vec2F const& targetPosition, double time) {
  int direction = Random::randf() > 0.5 ? 1 : -1; // random direction
  m_orbit.set(CelestialOrbit{target, direction, time, targetPosition - position()});
  m_approach.reset();
}

Maybe<CelestialCoordinate> SystemObject::orbitTarget() const {
  if (m_orbit.get())
    return m_orbit.get()->target;
  else
    return {};
}

Maybe<CelestialOrbit> SystemObject::orbit() const {
  return m_orbit.get();
}

void SystemObject::clientUpdate(float dt) {
  m_netGroup.tickNetInterpolation(dt);
}

void SystemObject::serverUpdate(SystemWorldServer* system, float dt) {
  if (!m_config.permanent && m_spawnTime > 0.0 && system->time() > m_spawnTime + m_config.lifeTime)
    m_shouldDestroy = true;

  if (m_orbit.get()) {
    setPosition(system->orbitPosition(*m_orbit.get()));
  } else if (m_config.permanent || !m_config.moving) {
    // permanent locations always have a solar orbit
    enterOrbit(CelestialCoordinate(system->location()), {0.0, 0.0}, system->time());
  } else if (m_approach && !m_approach->isNull()) {

    if (system->shipsAtLocation(m_uuid).size() > 0)
      return;

    if (m_approach->isPlanetaryBody()) {
      auto approach = system->planetPosition(*m_approach);
      auto toApproach = (approach - position());
      auto pos = position();
      setPosition(pos + toApproach.normalized() * m_config.speed * dt);

      if ((approach - position()).magnitude() < system->planetSize(*m_approach) + m_config.orbitDistance)
        enterOrbit(*m_approach, approach, system->time());
    } else {
      enterOrbit(*m_approach, {0.0, 0.0}, system->time());
    }
  } else {
    auto planets = system->planets().filtered([system](CelestialCoordinate const& p) {
        auto objectsAtPlanet = system->objects().filtered([p](SystemObjectPtr const& o) { return o->orbitTarget() == p; });
        return objectsAtPlanet.size() == 0;
      });

    if (planets.size() > 0)
      m_approach = Random::randFrom(planets);
  }
}

pair<ByteArray, uint64_t> SystemObject::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void SystemObject::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

ByteArray SystemObject::netStore() const {
  DataStreamBuffer ds;
  ds.write(m_uuid);
  ds.write(m_config.name);
  ds.write(position());
  ds.write(m_parameters);
  return ds.takeData();
}

Json SystemObject::diskStore() const {
  JsonObject store;
  store.set("uuid", m_uuid.hex());
  store.set("name", m_config.name);
  store.set("orbit", jsonFromMaybe<CelestialOrbit>(m_orbit.get(), [](CelestialOrbit const& o) {
      return o.toJson();
    }));
  store.set("spawnTime", m_spawnTime);
  store.set("position", jsonFromVec2F(position()));
  store.set("parameters", m_parameters);
  return store;
}

void SystemObject::setPosition(Vec2F const& position) {
  m_xPosition.set(position[0]);
  m_yPosition.set(position[1]);
}

SystemClientShip::SystemClientShip(SystemWorld* system, Uuid uuid, float speed, SystemLocation const& location)
  : m_uuid(move(uuid)) {
  m_systemLocation.set(location);
  setPosition(system->systemLocationPosition(location).value({}));

  // temporary
  auto shipConfig = Root::singleton().assets()->json("/systemworld.config:clientShip");
  m_config = ClientShipConfig{
    shipConfig.getFloat("orbitDistance"),
    shipConfig.getFloat("departTime"),
    shipConfig.getFloat("spaceDepartTime")
  };
  m_speed = speed;
  m_departTimer = 0.0f;

  // systemLocation should not be interpolated
  // if it's stale it can point to a removed system object
  m_netGroup.addNetElement(&m_systemLocation, false);
  m_netGroup.addNetElement(&m_destination);

  m_netGroup.addNetElement(&m_xPosition);
  m_netGroup.addNetElement(&m_yPosition);
  m_netGroup.enableNetInterpolation();

  m_xPosition.setInterpolator(lerp<float, float>);
  m_yPosition.setInterpolator(lerp<float, float>);
}

SystemClientShip::SystemClientShip(SystemWorld* system, Uuid uuid, SystemLocation const& location)
  : SystemClientShip(system, uuid, 0.0f, location) {}

Uuid SystemClientShip::uuid() const {
  return m_uuid;
}

Vec2F SystemClientShip::position() const {
  return {m_xPosition.get(), m_yPosition.get()};
}

SystemLocation SystemClientShip::systemLocation() const {
  return m_systemLocation.get();
}

SystemLocation SystemClientShip::destination() const {
  return m_destination.get();
}

void SystemClientShip::setDestination(SystemLocation const& destination) {
  auto location = m_systemLocation.get();
  if (location.is<CelestialCoordinate>() || location.is<Uuid>())
    m_departTimer = m_config.departTime;
  else if(m_destination.get().empty())
    m_departTimer = m_config.spaceDepartTime;
  m_destination.set(destination);
  m_systemLocation.set({});
}

void SystemClientShip::setSpeed(float speed) {
  m_speed = speed;
}

bool SystemClientShip::flying() const {
  return m_systemLocation.get().empty();
}

void SystemClientShip::clientUpdate(float dt) {
  m_netGroup.tickNetInterpolation(dt);
}

void SystemClientShip::serverUpdate(SystemWorld* system, float dt) {
  // if destination is an orbit we haven't started orbiting yet, update the time
  if (auto orbit = m_destination.get().maybe<CelestialOrbit>())
    orbit->enterTime = system->time();

  auto nearPlanetOrbit = [this,system](CelestialCoordinate const& planet) -> CelestialOrbit {
    Vec2F toShip = system->planetPosition(planet) - position();
    return CelestialOrbit {
      planet,
      1,
      system->time(),
      Vec2F::withAngle(toShip.angle(), system->planetSize(planet) / 2.0 + m_config.orbitDistance)
    };
  };

  if (auto coordinate = m_systemLocation.get().maybe<CelestialCoordinate>()) {
    if (!m_orbit || m_orbit->target != *coordinate)
      m_orbit = nearPlanetOrbit(*coordinate);
  } else if (m_systemLocation.get().empty()) {
    m_departTimer = max(0.0f, m_departTimer - dt);
    if (m_departTimer > 0.0f)
      return;

    if (auto coordinate = m_destination.get().maybe<CelestialCoordinate>()) {
      if (!m_orbit || m_orbit->target != *coordinate)
        m_orbit = nearPlanetOrbit(*coordinate);
    } else {
      m_orbit.reset();
    }

    Vec2F pos = position();
    Vec2F destination;
    if (m_orbit) {
      m_orbit->enterTime = system->time();
      destination = system->orbitPosition(*m_orbit);
    } else {
      destination = system->systemLocationPosition(m_destination.get()).value(pos);
    }

    auto toTarget = destination - pos;
    pos += toTarget.normalized() * (m_speed * dt);

    if (destination == pos || (destination - pos).normalized() * toTarget.normalized() < 0) {
      m_systemLocation.set(m_destination.get());
      m_destination.set({});
    } else {
      setPosition(pos);
      return;
    }
  }

  if (m_orbit) {
    setPosition(system->systemLocationPosition(*m_orbit).get());
  } else {
    setPosition(system->systemLocationPosition(m_systemLocation.get()).get());
  }
}

pair<ByteArray, uint64_t> SystemClientShip::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void SystemClientShip::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

ByteArray SystemClientShip::netStore() const {
  DataStreamBuffer ds;
  ds.write(m_uuid);
  ds.write(m_systemLocation.get());
  return ds.takeData();
}

void SystemClientShip::setPosition(Vec2F const& position) {
  m_xPosition.set(position[0]);
  m_yPosition.set(position[1]);
}

}
