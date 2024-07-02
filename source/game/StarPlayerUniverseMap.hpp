#pragma once

#include "StarJson.hpp"
#include "StarWarping.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarSystemWorld.hpp"

namespace Star {

STAR_CLASS(PlayerUniverseMap);

template <typename T>
Json jsonFromBookmarkTarget(T const& target);

template <typename T>
T jsonToBookmarkTarget(Json const& json);

// Bookmark<T> requires T to implement jsonToBookmarkTarget<T> and jsonFromBookmarkTarget<T>
// also operator== and operator!=
template<typename T>
struct Bookmark {
  T target;
  String targetName;
  String bookmarkName;
  String icon;

  static Bookmark fromJson(Json const& json);
  Json toJson() const;

  bool operator==(Bookmark<T> const& rhs) const;
  bool operator!=(Bookmark<T> const& rhs) const;
  bool operator<(Bookmark<T> const& rhs) const;
};

typedef Variant<CelestialCoordinate, Uuid> OrbitTarget;
typedef pair<WorldId, SpawnTarget> TeleportTarget;

typedef Bookmark<OrbitTarget> OrbitBookmark;
typedef Bookmark<TeleportTarget> TeleportBookmark;


class PlayerUniverseMap {
public:
  struct MappedObject {
    String typeName;
    Maybe<CelestialOrbit> orbit;
    JsonObject parameters;
  };

  PlayerUniverseMap(Json const& json = {});

  Json toJson() const;

  // pair of system location and bookmark, not all orbit bookmarks include the system
  List<pair<Vec3I, OrbitBookmark>> orbitBookmarks() const;
  bool addOrbitBookmark(CelestialCoordinate const& system, OrbitBookmark const& bookmark);
  bool removeOrbitBookmark(CelestialCoordinate const& system, OrbitBookmark const& bookmark);

  List<TeleportBookmark> teleportBookmarks() const;
  bool addTeleportBookmark(TeleportBookmark bookmark);
  bool removeTeleportBookmark(TeleportBookmark const& bookmark);
  void invalidateWarpAction(WarpAction const& bookmark);

  Maybe<OrbitBookmark> worldBookmark(CelestialCoordinate const& world) const;
  List<OrbitBookmark> systemBookmarks(CelestialCoordinate const& system) const;
  List<OrbitBookmark> planetBookmarks(CelestialCoordinate const& planet) const;

  bool isMapped(CelestialCoordinate const& coordinate);
  HashMap<Uuid, MappedObject> mappedObjects(CelestialCoordinate const& system);

  void addMappedCoordinate(CelestialCoordinate const& coordinate);
  void addMappedObject(CelestialCoordinate const& system, Uuid const& uuid, String const& typeName, Maybe<CelestialOrbit> const& orbit = {}, JsonObject parameters = {});
  void removeMappedObject(CelestialCoordinate const& system, Uuid const& uuid);
  void filterMappedObjects(CelestialCoordinate const& system, List<Uuid> const& allowed);

  void setServerUuid(Maybe<Uuid> serverUuid);

private:
  struct SystemMap {
    Set<CelestialCoordinate> mappedPlanets;
    HashMap<Uuid, MappedObject> mappedObjects;
    Set<OrbitBookmark> bookmarks;

    static SystemMap fromJson(Json const& json);
    Json toJson() const;
  };
  struct UniverseMap {
    HashMap<Vec3I, SystemMap> systems;
    Set<TeleportBookmark> teleportBookmarks;

    static UniverseMap fromJson(Json const& json);
    Json toJson() const;
  };

  UniverseMap const& universeMap() const;
  UniverseMap& universeMap();

  Maybe<Uuid> m_serverUuid;
  HashMap<Uuid, UniverseMap> m_universeMaps;
};

template <typename T>
bool Bookmark<T>::operator==(Bookmark<T> const& rhs) const {
  return target == rhs.target;
}

template <typename T>
bool Bookmark<T>::operator!=(Bookmark<T> const& rhs) const {
  return target != rhs.target;
}

template <typename T>
bool Bookmark<T>::operator<(Bookmark<T> const& rhs) const {
  return target < rhs.target;
}

}
