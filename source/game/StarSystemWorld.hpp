#pragma once

#include "StarCelestialParameters.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarUuid.hpp"
#include "StarWarping.hpp"
#include "StarSkyParameters.hpp"
#include "StarNetElementFloatFields.hpp"
#include "StarNetElementSystem.hpp"

namespace Star {

STAR_CLASS(CelestialDatabase);
STAR_CLASS(Celestial);
STAR_CLASS(Clock);
STAR_CLASS(ClientContext);
STAR_CLASS(SystemWorld);
STAR_CLASS(SystemWorldServer);
STAR_CLASS(SystemClientShip);
STAR_CLASS(SystemObject);

STAR_STRUCT(SystemObjectConfig);

struct CelestialOrbit {
  static CelestialOrbit fromJson(Json const& json);
  Json toJson() const;

  CelestialCoordinate target;
  int direction;
  double enterTime;
  Vec2F enterPosition;

  void write(DataStream& ds) const;
  void read(DataStream& ds);

  bool operator==(CelestialOrbit const& rhs) const;
};
DataStream& operator>>(DataStream& ds, CelestialOrbit& orbit);
DataStream& operator<<(DataStream& ds, CelestialOrbit const& orbit);

// in transit, at a planet, orbiting a planet,, at a system object, or at a vector position
typedef MVariant<CelestialCoordinate, CelestialOrbit, Uuid, Vec2F> SystemLocation;
Json jsonFromSystemLocation(SystemLocation const& location);
SystemLocation jsonToSystemLocation(Json const& json);

struct SystemWorldConfig {
  static SystemWorldConfig fromJson(Json const& config);

  float starGravitationalConstant;
  float planetGravitationalConstant;

  Map<unsigned, float> planetSizes;
  float emptyOrbitSize;
  float unvisitablePlanetSize;
  StringMap<float> floatingDungeonWorldSizes;

  float starSize;
  Vec2F planetaryOrbitPadding;
  Vec2F satelliteOrbitPadding;

  Vec2F arrivalRange;

  float objectSpawnPadding;
  float clientObjectSpawnPadding;
  Vec2F objectSpawnInterval;
  double objectSpawnCycle;
  float minObjectOrbitTime;

  float asteroidBeamDistance;

  SkyParameters emptySkyParameters;
};

class SystemWorld {
public:
  SystemWorld(ClockConstPtr universeClock, CelestialDatabasePtr celestialDatabase);

  virtual ~SystemWorld() = default;

  SystemWorldConfig const& systemConfig() const;
  double time() const;
  Vec3I location() const;
  List<CelestialCoordinate> planets() const;

  uint64_t coordinateSeed(CelestialCoordinate const& coord, String const& seedMix) const;
  float planetOrbitDistance(CelestialCoordinate const& coord) const;
  // assumes circular orbit
  float orbitInterval(float distance, bool isMoon) const;
  Vec2F orbitPosition(CelestialOrbit const& orbit) const;
  float clusterSize(CelestialCoordinate const& planet) const;
  float planetSize(CelestialCoordinate const& planet) const;
  Vec2F planetPosition(CelestialCoordinate const& planet) const;
  Maybe<Vec2F> systemLocationPosition(SystemLocation const& position) const;
  Vec2F randomArrivalPosition() const;
  Maybe<WarpAction> objectWarpAction(Uuid const& uuid) const;

  virtual List<SystemObjectPtr> objects() const = 0;
  virtual List<Uuid> objectKeys() const = 0;
  virtual SystemObjectPtr getObject(Uuid const& uuid) const = 0;

  SystemObjectConfig systemObjectConfig(String const& name, Uuid const& uuid) const;
  static Json systemObjectTypeConfig(String const& typeName);

protected:
  Vec3I m_location;
  CelestialDatabasePtr m_celestialDatabase;

private:
  ClockConstPtr m_universeClock;
  SystemWorldConfig m_config;
};

struct SystemObjectConfig {
  String name;

  bool moving;
  float speed;
  float orbitDistance;
  float lifeTime;

  // permanent system objects may only have a solar orbit and can never be removed
  bool permanent;

  WarpAction warpAction;
  Maybe<float> threatLevel;
  SkyParameters skyParameters;
  StringMap<String> generatedParameters;
  JsonObject parameters;
};

class SystemObject {
public:
  SystemObject(SystemObjectConfig config, Uuid uuid, Vec2F const& position, JsonObject parameters = {});
  SystemObject(SystemObjectConfig config, Uuid uuid, Vec2F const& position, double spawnTime, JsonObject parameters = {});
  SystemObject(SystemWorld* system, Json const& diskStore);

  void init();

  Uuid uuid() const;
  String name() const;
  bool permanent() const;
  Vec2F position() const;

  WarpAction warpAction() const;
  Maybe<float> threatLevel() const;
  SkyParameters skyParameters() const;
  JsonObject parameters() const;

  bool shouldDestroy() const;

  void enterOrbit(CelestialCoordinate const& target, Vec2F const& targetPosition, double time);
  Maybe<CelestialCoordinate> orbitTarget() const;
  Maybe<CelestialOrbit> orbit() const;

  void clientUpdate(float dt);
  void serverUpdate(SystemWorldServer* system, float dt);

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion, NetCompatibilityRules rules = {});
  void readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules = {});

  ByteArray netStore() const;
  Json diskStore() const;
private:

  void setPosition(Vec2F const& position);

  SystemObjectConfig m_config;
  Uuid m_uuid;
  double m_spawnTime;
  JsonObject m_parameters;

  Maybe<CelestialCoordinate> m_approach;

  bool m_shouldDestroy;

  NetElementTopGroup m_netGroup;
  NetElementFloat m_xPosition;
  NetElementFloat m_yPosition;
  NetElementData<Maybe<CelestialOrbit>> m_orbit;
};

class SystemClientShip {
public:
  SystemClientShip(SystemWorld* world, Uuid uuid, float speed, SystemLocation const& position);
  SystemClientShip(SystemWorld* world, Uuid uuid, SystemLocation const& position);

  Uuid uuid() const;
  Vec2F position() const;
  SystemLocation systemLocation() const;
  SystemLocation destination() const;
  void setDestination(SystemLocation const& destination);
  void setSpeed(float speed);
  void startFlying();

  bool flying() const;

  // update is only called on master
  void clientUpdate(float dt);
  void serverUpdate(SystemWorld* system, float dt);

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion, NetCompatibilityRules rules = {});
  void readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules = {});

  ByteArray netStore() const;
private:
  struct ClientShipConfig {
    float orbitDistance;
    float departTime;
    float spaceDepartTime;
  };

  void setPosition(Vec2F const& position);

  Uuid m_uuid;

  ClientShipConfig m_config;
  float m_departTimer;
  float m_speed;

  Maybe<CelestialOrbit> m_orbit;

  NetElementTopGroup m_netGroup;
  NetElementData<SystemLocation> m_systemLocation;
  NetElementData<SystemLocation> m_destination;
  NetElementFloat m_xPosition;
  NetElementFloat m_yPosition;
};

}
