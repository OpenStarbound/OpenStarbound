#ifndef STAR_WARPING_HPP
#define STAR_WARPING_HPP

#include "StarStrongTypedef.hpp"
#include "StarUuid.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarGameTypes.hpp"

namespace Star {

enum class WarpMode : uint8_t {
  None,
  BeamOnly,
  DeployOnly,
  BeamOrDeploy
};
extern EnumMap<WarpMode> WarpModeNames;

struct InstanceWorldId {
  String instance;
  Maybe<Uuid> uuid;
  Maybe<float> level;

  InstanceWorldId();
  InstanceWorldId(String instance, Maybe<Uuid> uuid = {}, Maybe<float> level = {});

  bool operator==(InstanceWorldId const& rhs) const;
  bool operator<(InstanceWorldId const& rhs) const;
};

template <>
struct hash<InstanceWorldId> {
  size_t operator()(InstanceWorldId const& id) const;
};

DataStream& operator>>(DataStream& ds, InstanceWorldId& missionWorldId);
DataStream& operator<<(DataStream& ds, InstanceWorldId const& missionWorldId);

strong_typedef(CelestialCoordinate, CelestialWorldId);
strong_typedef(Uuid, ClientShipWorldId);
typedef MVariant<CelestialWorldId, ClientShipWorldId, InstanceWorldId> WorldId;

String printWorldId(WorldId const& worldId);
WorldId parseWorldId(String const& printedId);

// Same as outputting printWorldId
std::ostream& operator<<(std::ostream& os, CelestialWorldId const& worldId);
std::ostream& operator<<(std::ostream& os, ClientShipWorldId const& worldId);
std::ostream& operator<<(std::ostream& os, InstanceWorldId const& worldId);
std::ostream& operator<<(std::ostream& os, WorldId const& worldId);

strong_typedef(String, SpawnTargetUniqueEntity);
strong_typedef(Vec2F, SpawnTargetPosition);
strong_typedef_builtin(float, SpawnTargetX);
typedef MVariant<SpawnTargetUniqueEntity, SpawnTargetPosition, SpawnTargetX> SpawnTarget;

Json spawnTargetToJson(SpawnTarget spawnTarget);
SpawnTarget spawnTargetFromJson(Json v);

String printSpawnTarget(SpawnTarget spawnTarget);

struct WarpToWorld {
  WarpToWorld();
  explicit WarpToWorld(WorldId world, SpawnTarget spawn = {});
  explicit WarpToWorld(Json v);

  WorldId world;
  SpawnTarget target;

  bool operator==(WarpToWorld const& rhs) const;
  explicit operator bool() const;

  Json toJson() const;
};

strong_typedef(Uuid, WarpToPlayer);

enum class WarpAlias {
  Return,
  OrbitedWorld,
  OwnShip
};

typedef MVariant<WarpToWorld, WarpToPlayer, WarpAlias> WarpAction;

WarpAction parseWarpAction(String const& warpString);
String printWarpAction(WarpAction const& warpAction);

DataStream& operator>>(DataStream& ds, WarpToWorld& warpToWorld);
DataStream& operator<<(DataStream& ds, WarpToWorld const& warpToWorld);

}

template <> struct fmt::formatter<Star::CelestialWorldId> : ostream_formatter {};
template <> struct fmt::formatter<Star::ClientShipWorldId> : ostream_formatter {};
template <> struct fmt::formatter<Star::InstanceWorldId> : ostream_formatter {};
template <> struct fmt::formatter<Star::WorldId> : ostream_formatter {};
template <> struct fmt::formatter<Star::WarpToWorld> : ostream_formatter {};

#endif
