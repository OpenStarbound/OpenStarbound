#pragma once

#include "StarSkyTypes.hpp"
#include "StarEither.hpp"
#include "StarCelestialCoordinate.hpp"

namespace Star {

STAR_CLASS(CelestialParameters);
STAR_CLASS(CelestialDatabase);

STAR_STRUCT(SkyParameters);

STAR_STRUCT(VisitableWorldParameters);

// This struct is a stripped down version of CelestialParameters that only
// contains the required inforamtion to generate a sky.  It's constructable
// from a CelestialParameters or importantly from Json.  This allows places
// without a coordinate (and therefore without CelestialParameters) to have a
// valid sky. (Instances, outposts and the like.)
// Additionally, a copy-ish constructor is provided to allow changing elements
// derived from the visitableworldparameters without reconstructing all sky
// parameters, e.g. for terraforming
struct SkyParameters {
  SkyParameters();
  SkyParameters(CelestialCoordinate const& coordinate, CelestialDatabasePtr const& celestialDatabase);
  SkyParameters(SkyParameters const& oldSkyParameters, VisitableWorldParametersConstPtr newVisitableParameters);
  explicit SkyParameters(Json const& config);

  Json toJson() const;

  void read(DataStream& ds);
  void write(DataStream& ds) const;

  void readVisitableParameters(VisitableWorldParametersConstPtr visitableParameters);

  uint64_t seed;
  Maybe<float> dayLength;
  Maybe<pair<List<pair<String, float>>, Vec2F>> nearbyPlanet;
  List<pair<List<pair<String, float>>, Vec2F>> nearbyMoons;
  List<pair<String, String>> horizonImages;
  bool horizonClouds;
  SkyType skyType;
  Either<SkyColoring, Color> skyColoring;
  Maybe<float> spaceLevel;
  Maybe<float> surfaceLevel;
  String sunType;
};

DataStream& operator>>(DataStream& ds, SkyParameters& sky);
DataStream& operator<<(DataStream& ds, SkyParameters const& sky);
}
