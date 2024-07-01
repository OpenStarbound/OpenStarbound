#include "StarPlayerUniverseMap.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

template<> Json jsonFromBookmarkTarget<OrbitTarget>(OrbitTarget const& target) {
  if (auto uuid = target.maybe<Uuid>())
    return uuid->hex();
  else
    return target.get<CelestialCoordinate>().toJson();
};
template<> OrbitTarget jsonToBookmarkTarget<OrbitTarget>(Json const& json) {
  if (json.type() == Json::Type::String)
    return Uuid(json.toString());
  else
    return CelestialCoordinate(json);
};

template<> Json jsonFromBookmarkTarget<TeleportTarget>(TeleportTarget const& target) {
  return JsonArray{printWorldId(target.first), spawnTargetToJson(target.second)};
}
template<> TeleportTarget jsonToBookmarkTarget<TeleportTarget>(Json const& target) {
  return {parseWorldId(target.get(0).toString()), spawnTargetFromJson(target.get(1))};
}

template <typename T>
Bookmark<T> Bookmark<T>::fromJson(Json const& json) {
  Bookmark<T> bookmark;
  bookmark.target = jsonToBookmarkTarget<T>(json.get("target"));
  bookmark.targetName = json.getString("targetName");
  bookmark.bookmarkName = json.getString("bookmarkName");
  bookmark.icon = json.getString("icon");
  return bookmark;
}

template <typename T>
Json Bookmark<T>::toJson() const {
  JsonObject result;
  result["target"] = jsonFromBookmarkTarget<T>(target);
  result["targetName"] = targetName;
  result["bookmarkName"] = bookmarkName;
  result["icon"] = icon;
  return result;
}

PlayerUniverseMap::PlayerUniverseMap(Json const& json) {
  if (auto maps = json.optObject()) {
    for (auto p : *maps)
      m_universeMaps.set(Uuid(p.first), UniverseMap::fromJson(p.second));
  }
}

Json PlayerUniverseMap::toJson() const {
  JsonObject json;
  for (auto p : m_universeMaps)
    json.set(p.first.hex(), p.second.toJson());
  return json;
}

List<pair<Vec3I, OrbitBookmark>> PlayerUniverseMap::orbitBookmarks() const {
  if (m_serverUuid.isNothing())
    return {};

  List<pair<Vec3I, OrbitBookmark>> bookmarks;
  for (auto p : universeMap().systems) {
    bookmarks.appendAll(p.second.bookmarks.values().transformed([&p](OrbitBookmark const& b) {
      return pair<Vec3I, OrbitBookmark>(p.first, b);
    }));
  }
  return bookmarks;
}

bool PlayerUniverseMap::addOrbitBookmark(CelestialCoordinate const& system, OrbitBookmark const& bookmark) {
  if (system.isNull())
    throw StarException("Cannot add orbit bookmark to null system");

  return m_universeMaps[*m_serverUuid].systems[system.location()].bookmarks.add(std::move(bookmark));
}

bool PlayerUniverseMap::removeOrbitBookmark(CelestialCoordinate const& system, OrbitBookmark const& bookmark) {
  if (system.isNull())
    throw StarException("Cannot remove orbit bookmark from null system");

  return m_universeMaps[*m_serverUuid].systems[system.location()].bookmarks.remove(bookmark);
}

List<TeleportBookmark> PlayerUniverseMap::teleportBookmarks() const {
  return universeMap().teleportBookmarks.values();
}

bool PlayerUniverseMap::addTeleportBookmark(TeleportBookmark bookmark) {
  return m_universeMaps[*m_serverUuid].teleportBookmarks.add(std::move(bookmark));
}

bool PlayerUniverseMap::removeTeleportBookmark(TeleportBookmark const& bookmark) {
  return m_universeMaps[*m_serverUuid].teleportBookmarks.remove(bookmark);
}

void PlayerUniverseMap::invalidateWarpAction(WarpAction const& warpAction) {
  if (auto warpToWorld = warpAction.maybe<WarpToWorld>())
    removeTeleportBookmark({ {warpToWorld->world, warpToWorld->target}, "", "", ""});
}

Maybe<OrbitBookmark> PlayerUniverseMap::worldBookmark(CelestialCoordinate const& world) const {
  if (auto systemMap = universeMap().systems.ptr(world.location())) {
    for (auto& bookmark : systemMap->bookmarks) {
      if (bookmark.target == world)
        return bookmark;
    }
  }
  return {};
}

List<OrbitBookmark> PlayerUniverseMap::systemBookmarks(CelestialCoordinate const& system) const {
  if (auto systemMap = universeMap().systems.ptr(system.location()))
    return systemMap->bookmarks.values();
  return {};
}

List<OrbitBookmark> PlayerUniverseMap::planetBookmarks(CelestialCoordinate const& planet) const {
  if (auto systemMap = universeMap().systems.ptr(planet.location())) {
    return systemMap->bookmarks.values().filtered([planet](OrbitBookmark const& bookmark) {
      if (auto coordinate = bookmark.target.maybe<CelestialCoordinate>())
        return coordinate->planet().orbitNumber() == planet.planet().orbitNumber();
      return false;
    });
  }
  return {};
}

bool PlayerUniverseMap::isMapped(CelestialCoordinate const& coordinate) {
  if (coordinate.isNull())
    return false;

  auto& universeMap = m_universeMaps[*m_serverUuid];
  if (auto systemMap = universeMap.systems.ptr(coordinate.location()))
    return coordinate.isSystem() || systemMap->mappedPlanets.contains(coordinate.planet());
  else
    return false;
}

HashMap<Uuid, PlayerUniverseMap::MappedObject> PlayerUniverseMap::mappedObjects(CelestialCoordinate const& system) {
  auto& universeMap = m_universeMaps[*m_serverUuid];
  if (auto systemMap = universeMap.systems.ptr(system.location()))
    return systemMap->mappedObjects;
  else
    return {};
}

void PlayerUniverseMap::addMappedCoordinate(CelestialCoordinate const& coordinate) {
  if (coordinate.isNull())
    return;

  auto& universeMap = m_universeMaps[*m_serverUuid];
  auto& systemMap = universeMap.systems[coordinate.location()];
  if (!coordinate.isSystem())
    systemMap.mappedPlanets.add(coordinate.planet());
}

void PlayerUniverseMap::addMappedObject(CelestialCoordinate const& system, Uuid const& uuid, String const& typeName, Maybe<CelestialOrbit> const& orbit, JsonObject parameters) {
  MappedObject object {
    typeName,
    orbit,
    parameters
  };
  auto& universeMap = m_universeMaps[*m_serverUuid];
  universeMap.systems[system.location()].mappedObjects.set(uuid, object);
}

void PlayerUniverseMap::removeMappedObject(CelestialCoordinate const& system, Uuid const& uuid) {
  auto& universeMap = m_universeMaps[*m_serverUuid];
  if (auto systemMap = universeMap.systems.ptr(system.location()))
    systemMap->mappedObjects.remove(uuid);
}

void PlayerUniverseMap::filterMappedObjects(CelestialCoordinate const& system, List<Uuid> const& allowed) {
  auto& universeMap = m_universeMaps[*m_serverUuid];
  if (auto systemMap = universeMap.systems.ptr(system.location())) {
    auto& objects = systemMap->mappedObjects;
    for (auto& uuid : objects.keys()) {
      if (!allowed.contains(uuid))
        objects.remove(uuid);
    }
  }
}

void PlayerUniverseMap::setServerUuid(Maybe<Uuid> serverUuid) {
  m_serverUuid = std::move(serverUuid);
  if (m_serverUuid && !m_universeMaps.contains(*m_serverUuid))
    m_universeMaps.set(*m_serverUuid, UniverseMap());
}

PlayerUniverseMap::SystemMap PlayerUniverseMap::SystemMap::fromJson(Json const& json) {
  SystemMap map;

  for (auto m : json.getArray("mappedPlanets"))
    map.mappedPlanets.add(CelestialCoordinate(m));

  for (auto o : json.getObject("mappedObjects")) {
    MappedObject object;
    object.typeName = o.second.getString("typeName");
    object.orbit = jsonToMaybe<CelestialOrbit>(o.second.get("orbit"), [](Json const& o ) {
        return CelestialOrbit::fromJson(o);
      });
    object.parameters = o.second.getObject("parameters", {});
    map.mappedObjects.set(Uuid(o.first), object);
  }

  for (auto b : json.getArray("bookmarks"))
    map.bookmarks.add(OrbitBookmark::fromJson(b));

  return map;
}

Json PlayerUniverseMap::SystemMap::toJson() const {
  JsonObject json;

  JsonArray planets;
  for (auto m : mappedPlanets)
    planets.append(m.toJson());
  json.set("mappedPlanets", planets);

  JsonObject objects;
  for (auto o : mappedObjects) {
    JsonObject object;
    objects.set(o.first.hex(), JsonObject{
      {"typeName", o.second.typeName},
      {"orbit", jsonFromMaybe<CelestialOrbit>(
        o.second.orbit, [](CelestialOrbit const& orbit){ return orbit.toJson(); })},
      {"parameters", o.second.parameters}
    });
  }
  json.set("mappedObjects", objects);

  json.set("bookmarks", bookmarks.values().transformed([](OrbitBookmark const& b) {
      return b.toJson();
    }));

  return json;
}

PlayerUniverseMap::UniverseMap PlayerUniverseMap::UniverseMap::fromJson(Json const& json) {
  UniverseMap map;

  for (auto s : json.getArray("systems")) {
    Vec3I location = jsonToVec3I(s.get(0));
    map.systems.set(location, SystemMap::fromJson(s.get(1)));
  }

  for (auto bookmark : json.getArray("teleportBookmarks").transformed(&TeleportBookmark::fromJson))
    map.teleportBookmarks.add(bookmark);

  return map;
}

Json PlayerUniverseMap::UniverseMap::toJson() const {
  JsonObject json;

  JsonArray s;
  for (auto p : systems) {
    s.append(JsonArray{jsonFromVec3I(p.first), p.second.toJson()});
  }
  json.set("systems", s);

  JsonArray bookmarks = teleportBookmarks.values().transformed([](TeleportBookmark const& b) {
      return b.toJson();
    });
  json.set("teleportBookmarks", bookmarks);

  return json;
}

PlayerUniverseMap::UniverseMap const& PlayerUniverseMap::universeMap() const {
  if (m_serverUuid.isNothing())
    throw StarException("Cannot get universe map of null server uuid");

  return m_universeMaps.get(*m_serverUuid);
}

}
