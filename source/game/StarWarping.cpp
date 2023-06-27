#include "StarWarping.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

EnumMap<WarpMode> WarpModeNames {
  {WarpMode::None, "None"},
  {WarpMode::BeamOnly, "BeamOnly"},
  {WarpMode::DeployOnly, "DeployOnly"},
  {WarpMode::BeamOrDeploy, "BeamOrDeploy"}
};

InstanceWorldId::InstanceWorldId() {}

InstanceWorldId::InstanceWorldId(String instance, Maybe<Uuid> uuid, Maybe<float> level)
  : instance(move(instance)), uuid(move(uuid)), level(move(level)) {}

bool InstanceWorldId::operator==(InstanceWorldId const& rhs) const {
  return tie(instance, uuid, level) == tie(rhs.instance, rhs.uuid, rhs.level);
}

bool InstanceWorldId::operator<(InstanceWorldId const& rhs) const {
  return tie(instance, uuid, rhs.level) < tie(rhs.instance, rhs.uuid, rhs.level);
}

size_t hash<InstanceWorldId>::operator()(InstanceWorldId const& id) const {
  return hashOf(id.instance, id.uuid, id.level);
}

DataStream& operator>>(DataStream& ds, InstanceWorldId& instanceWorldId) {
  ds >> instanceWorldId.instance;
  ds >> instanceWorldId.uuid;
  ds >> instanceWorldId.level;
  return ds;
}

DataStream& operator<<(DataStream& ds, InstanceWorldId const& instanceWorldId) {
  ds << instanceWorldId.instance;
  ds << instanceWorldId.uuid;
  ds << instanceWorldId.level;
  return ds;
}

String printWorldId(WorldId const& worldId) {
  if (auto instanceWorldId = worldId.ptr<InstanceWorldId>()) {
    if (instanceWorldId->level && *instanceWorldId->level < 0.0f)
      throw StarException::format("InstanceWorldId level component cannot be negative");

    String uuidPart = instanceWorldId->uuid ? instanceWorldId->uuid->hex() : "-";
    String levelPart = instanceWorldId->level ? toString(*instanceWorldId->level) : "-";
    return strf("InstanceWorld:{}:{}:{}", instanceWorldId->instance, uuidPart, levelPart);
  } else if (auto celestialWorldId = worldId.ptr<CelestialWorldId>()) {
    return strf("CelestialWorld:{}", *celestialWorldId);
  } else if (auto clientShipWorldId = worldId.ptr<ClientShipWorldId>()) {
    return strf("ClientShipWorld:{}", clientShipWorldId->hex());
  } else {
    return "Nowhere";
  }
}

WorldId parseWorldId(String const& printedId) {
  if (printedId.empty())
    return WorldId();

  auto parts = printedId.split(':', 1);
  String const& type = parts.at(0);

  if (type.equalsIgnoreCase("InstanceWorld")) {
    auto rest = parts.at(1).split(":", 2);
    if (rest.size() == 0 || rest.size() > 3)
      throw StarException::format("Wrong number of parts in InstanceWorldId");
    auto getOptPart = [](String part) -> Maybe<String> {
      if (part.empty() || part == "-")
        return {};
      return part;
    };

    InstanceWorldId instanceWorldId{rest.at(0)};
    if (rest.size() > 1) {
      if (auto uuid = getOptPart(rest.at(1)))
        instanceWorldId.uuid = Uuid(*uuid);
      if (rest.size() > 2) {
        if (auto level = getOptPart(rest.at(2))) {
          instanceWorldId.level = lexicalCast<float>(*level);
          if (*instanceWorldId.level < 0)
            throw StarException::format("InstanceWorldId level component cannot be negative");
        }
      }
    }

    return instanceWorldId;
  } else if (type.equalsIgnoreCase("CelestialWorld")) {
    return CelestialWorldId(CelestialCoordinate(parts.at(1)));
  } else if (type.equalsIgnoreCase("ClientShipWorld")) {
    return ClientShipWorldId(Uuid(parts.at(1)));
  } else if (type.equalsIgnoreCase("Nowhere")) {
    return {};
  } else {
    throw StarException::format("Improper WorldId type '{}'", type);
  }
}

std::ostream& operator<<(std::ostream& os, CelestialWorldId const& worldId) {
  os << (CelestialCoordinate)worldId;
  return os;
}


std::ostream& operator<<(std::ostream& os, ClientShipWorldId const& worldId) {
  os << ((Uuid)worldId).hex();
  return os;
}

std::ostream& operator<<(std::ostream& os, InstanceWorldId const& worldId) {
  os << printWorldId(worldId);
  return os;
}

std::ostream& operator<<(std::ostream& os, WorldId const& worldId) {
  os << printWorldId(worldId);
  return os;
}

Json spawnTargetToJson(SpawnTarget spawnTarget) {
  if (spawnTarget.is<SpawnTargetUniqueEntity>())
    return spawnTarget.get<SpawnTargetUniqueEntity>();
  else if (spawnTarget.is<SpawnTargetPosition>())
    return jsonFromVec2F(spawnTarget.get<SpawnTargetPosition>());
  else if (spawnTarget.is<SpawnTargetX>())
    return Json(spawnTarget.get<SpawnTargetX>());
  else
    return Json();
}

SpawnTarget spawnTargetFromJson(Json v) {
  if (v.isNull())
    return {};
  else if (v.isType(Json::Type::String))
    return SpawnTargetUniqueEntity(v.toString());
  else if (v.isType(Json::Type::Float))
    return SpawnTargetX(v.toFloat());
  else
    return SpawnTargetPosition(jsonToVec2F(v));
}

String printSpawnTarget(SpawnTarget spawnTarget) {
  if (auto str = spawnTarget.ptr<SpawnTargetUniqueEntity>())
    return *str;
  else if (auto pos = spawnTarget.ptr<SpawnTargetPosition>())
    return strf("{}.{}", (*pos)[0], (*pos)[1]);
  else if (auto x = spawnTarget.ptr<SpawnTargetX>())
    return toString(x->t);
  else
    return "";
}

WarpToWorld::WarpToWorld() {}

WarpToWorld::WarpToWorld(WorldId world, SpawnTarget target) : world(move(world)), target(move(target)) {}

WarpToWorld::WarpToWorld(Json v) {
  if (v) {
    world = parseWorldId(v.getString("world"));
    target = spawnTargetFromJson(v.get("target"));
  }
}

bool WarpToWorld::operator==(WarpToWorld const& rhs) const {
  return tie(world, target) == tie(rhs.world, rhs.target);
}

WarpToWorld::operator bool() const {
  return (bool)world;
}

Json WarpToWorld::toJson() const {
  return JsonObject{{"world", printWorldId(world)}, {"target", spawnTargetToJson(target)}};
}

WarpAction parseWarpAction(String const& warpString) {
  if (warpString.equalsIgnoreCase("Return")) {
    return WarpAlias::Return;
  } else if (warpString.equalsIgnoreCase("OrbitedWorld")) {
    return WarpAlias::OrbitedWorld;
  } else if (warpString.equalsIgnoreCase("OwnShip")) {
    return WarpAlias::OwnShip;
  } else if (warpString.beginsWith("Player:", String::CaseSensitivity::CaseInsensitive)) {
    auto parts = warpString.split(':', 1);
    return WarpToPlayer(Uuid(parts.at(1)));
  } else {
    auto parts = warpString.split('=', 1);
    auto world = parseWorldId(parts.at(0));
    SpawnTarget target;
    if (parts.size() == 2) {
      auto const& targetPart = parts.at(1);
      if (targetPart.regexMatch("\\d+.\\d+")) {
        auto pos = targetPart.split(".", 1);
        target = SpawnTargetPosition(Vec2F(lexicalCast<int>(pos.at(0)), lexicalCast<int>(pos.at(1))));
      } else if (targetPart.regexMatch("\\d+")) {
        target = SpawnTargetX(lexicalCast<int>(targetPart));
      } else {
        target = SpawnTargetUniqueEntity(targetPart);
      }
    }
    return WarpToWorld(world, target);
  }
}

String printWarpAction(WarpAction const& warpAction) {
  if (auto warpAlias = warpAction.ptr<WarpAlias>()) {
    if (*warpAlias == WarpAlias::Return)
      return "Return";
    else if (*warpAlias == WarpAlias::OrbitedWorld)
      return "OrbitedWorld";
    else if (*warpAlias == WarpAlias::OwnShip)
      return "OwnShip";
  } else if (auto warpToPlayer = warpAction.ptr<WarpToPlayer>()) {
    return strf("Player:{}", warpToPlayer->hex());
  } else if (auto warpToWorld = warpAction.ptr<WarpToWorld>()) {
    auto toWorldString = printWorldId(warpToWorld->world);
    if (auto spawnTarget = warpToWorld->target)
      toWorldString = strf("{}={}", toWorldString, printSpawnTarget(spawnTarget));
    return toWorldString;
  }

  return "UnknownWarpAction";
}

DataStream& operator>>(DataStream& ds, WarpToWorld& warpToWorld) {
  ds >> warpToWorld.world;
  ds >> warpToWorld.target;
  return ds;
}

DataStream& operator<<(DataStream& ds, WarpToWorld const& warpToWorld) {
  ds << warpToWorld.world;
  ds << warpToWorld.target;
  return ds;
}

}
