#ifndef STAR_CELESTIAL_TYPES_HPP
#define STAR_CELESTIAL_TYPES_HPP

#include "StarOrderedMap.hpp"
#include "StarRect.hpp"
#include "StarEither.hpp"
#include "StarCelestialParameters.hpp"

namespace Star {

STAR_STRUCT(CelestialSystemObjects);
STAR_STRUCT(CelestialChunk);
STAR_STRUCT(CelestialBaseInformation);

typedef List<pair<Vec2I, Vec2I>> CelestialConstellation;

struct CelestialOrbitRegion {
  String regionName;
  Vec2I orbitRange;
  float bodyProbability;
  WeightedPool<String> planetaryTypes;
  WeightedPool<String> satelliteTypes;
};

struct CelestialPlanet {
  CelestialParameters planetParameters;
  HashMap<int, CelestialParameters> satelliteParameters;
};
DataStream& operator>>(DataStream& ds, CelestialPlanet& planet);
DataStream& operator<<(DataStream& ds, CelestialPlanet const& planet);

struct CelestialSystemObjects {
  Vec3I systemLocation;
  HashMap<int, CelestialPlanet> planets;
};
DataStream& operator>>(DataStream& ds, CelestialSystemObjects& systemObjects);
DataStream& operator<<(DataStream& ds, CelestialSystemObjects const& systemObjects);

struct CelestialChunk {
  CelestialChunk();
  CelestialChunk(Json const& store);

  Json toJson() const;

  Vec2I chunkIndex;
  List<CelestialConstellation> constellations;
  HashMap<Vec3I, CelestialParameters> systemParameters;

  // System objects are kept separate from systemParameters here so that there
  // can be two phases of loading, one for basic system-level parameters for an
  // entire chunk the other for each set of sub objects for each system.
  HashMap<Vec3I, HashMap<int, CelestialPlanet>> systemObjects;
};
DataStream& operator>>(DataStream& ds, CelestialChunk& chunk);
DataStream& operator<<(DataStream& ds, CelestialChunk const& chunk);

typedef Either<Vec2I, Vec3I> CelestialRequest;
typedef Either<CelestialChunk, CelestialSystemObjects> CelestialResponse;

struct CelestialBaseInformation {
  int planetOrbitalLevels;
  int satelliteOrbitalLevels;
  int chunkSize;
  Vec2I xyCoordRange;
  Vec2I zCoordRange;
};
DataStream& operator>>(DataStream& ds, CelestialBaseInformation& celestialInformation);
DataStream& operator<<(DataStream& ds, CelestialBaseInformation const& celestialInformation);
}

#endif
