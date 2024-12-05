#include "StarSkyParameters.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarCelestialGraphics.hpp"
#include "StarCasting.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

SkyParameters::SkyParameters() : seed(), skyType(SkyType::Barren), skyColoring(makeRight(Color::Black)), settings(JsonObject()) {}

SkyParameters::SkyParameters(CelestialCoordinate const& coordinate, CelestialDatabasePtr const& celestialDatabase)
  : SkyParameters() {
  if (!coordinate || coordinate.isSystem())
    return;
  auto params = celestialDatabase->parameters(coordinate);
  if (!params)
    return;
  auto systemParams = celestialDatabase->parameters(coordinate.system());
  seed = staticRandomU64(params->seed(), "SkySeed");

  // Gather up all the CelestialParameters and scales for all the celestial
  // objects to draw in the sky, we should draw the parent planet if we are a
  // satellite, as well as all the other satellites.
  auto selfCoordinate = params->coordinate();
  if (selfCoordinate.isSatelliteBody()) {
    if (auto planet = celestialDatabase->parameters(selfCoordinate.parent())) {
      Vec2F pos;
      pos[0] = staticRandomFloat(params->seed(), planet->seed(), "x");
      pos[1] = staticRandomFloat(params->seed(), planet->seed(), "y");

      // My parent's parent is no one.
      nearbyPlanet = {{CelestialGraphics::drawWorld(*planet, {}), pos}};
    }
  }

  for (auto satelliteCoordinate : celestialDatabase->children(selfCoordinate.planet())) {
    if (satelliteCoordinate != selfCoordinate) {
      if (auto satellite = celestialDatabase->parameters(satelliteCoordinate)) {
        Vec2F pos;
        pos[0] = staticRandomFloat(params->seed(), satellite->seed(), "x");
        pos[1] = staticRandomFloat(params->seed(), satellite->seed(), "y");

        nearbyMoons.append(
            {CelestialGraphics::drawWorld(*satellite, celestialDatabase->parameters(satelliteCoordinate.parent())),
                pos});
      }
    }
  }

  horizonImages = CelestialGraphics::worldHorizonImages(*params);

  readVisitableParameters(params->visitableParameters());

  sunType = systemParams->getParameter("typeName").toString();
}

SkyParameters::SkyParameters(SkyParameters const& oldSkyParameters, VisitableWorldParametersConstPtr newVisitableParameters) : SkyParameters() {
  *this = oldSkyParameters;
  readVisitableParameters(newVisitableParameters);
}

SkyParameters::SkyParameters(Json const& config) : SkyParameters() {
  if (config.isNull())
    return;

  seed = config.getUInt("seed");
  dayLength = config.optFloat("dayLength");

  auto extractLayerData = [](Json const& v, uint64_t selfSeed) -> pair<List<pair<String, float>>, Vec2F> {
    Vec2F pos;

    if (v.contains("pos")) {
      pos = jsonToVec2F(v.get("pos"));
    } else if (v.contains("seed")) {
      pos[0] = staticRandomFloat(selfSeed, v.getUInt("seed"), "x");
      pos[1] = staticRandomFloat(selfSeed, v.getUInt("seed"), "y");
    }

    List<pair<String, float>> layers;
    for (auto const& l : v.get("layers").iterateArray())
      layers.append({l.getString("image"), l.getFloat("scale")});

    return {layers, pos};
  };

  if (config.contains("planet") && config.get("planet"))
    nearbyPlanet = extractLayerData(config.get("planet"), seed);

  if (config.contains("satellites"))
    nearbyMoons =
        jsonToList<pair<List<pair<String, float>>, Vec2F>>(config.get("satellites"), bind(extractLayerData, _1, seed));

  if (config.contains("horizonImages")) {
    horizonImages = jsonToList<pair<String, String>>(config.get("horizonImages"),
        [](Json const& v) -> pair<String, String> {
          return {v.getString("left"), v.getString("right")};
        });
  }

  horizonClouds = config.getBool("horizonClouds", true);

  skyType = SkyTypeNames.getLeft(config.getString("skyType", "barren"));
  if (auto colors = config.opt("skyColoring")) {
    skyColoring.setLeft(SkyColoring{*colors});
  } else if (auto ambientLightLevel = config.opt("ambientLightLevel")) {
    skyColoring.setRight(jsonToColor(*ambientLightLevel));
  } else {
    skyColoring.setRight(Color::Black);
  }

  spaceLevel = config.optFloat("spaceLevel");
  surfaceLevel = config.optFloat("surfaceLevel");

  sunType = config.getString("sunType", "");

  settings = config.get("settings", JsonObject());
}

Json SkyParameters::toJson() const {
  return JsonObject{
      {"seed", seed},
      {"dayLength", jsonFromMaybe<float>(dayLength)},
      {"planet",
          jsonFromMaybe<pair<List<pair<String, float>>, Vec2F>>(nearbyPlanet,
              [](pair<List<pair<String, float>>, Vec2F> p) -> Json {
                return JsonObject{
                    {"layers",
                        p.first.transformed([](pair<String, float> const& p) -> Json {
                          return JsonObject{{"image", p.first}, {"scale", p.second}};
                        })},
                    {"pos", jsonFromVec2F(p.second)},
                };
              })},
      {"satellites",
          jsonFromList<pair<List<pair<String, float>>, Vec2F>>(nearbyMoons,
              [](pair<List<pair<String, float>>, Vec2F> const& p) {
                return JsonObject{
                    {"layers",
                        p.first.transformed([](pair<String, float> const& p) -> Json {
                          return JsonObject{{"image", p.first}, {"scale", p.second}};
                        })},
                    {"pos", jsonFromVec2F(p.second)},
                };
              })},
      {"horizonImages",
          jsonFromList<pair<String, String>>(horizonImages,
              [](pair<String, String> p) {
                return JsonObject{
                    {"left", p.first}, {"right", p.second},
                };
              })},
      {"horizonClouds", horizonClouds},
      {"skyType", SkyTypeNames.getRight(skyType)},
      {"skyColoring", jsonFromMaybe<SkyColoring>(skyColoring.maybeLeft(), [](SkyColoring c) { return c.toJson(); })},
      {"ambientLightLevel", jsonFromMaybe<Color>(skyColoring.maybeRight(), [](Color c) { return jsonFromColor(c); })},
      {"spaceLevel", jsonFromMaybe<float>(spaceLevel)},
      {"surfaceLevel", jsonFromMaybe<float>(surfaceLevel)},
      {"sunType", sunType},
      {"settings", settings}
  };
}

void SkyParameters::read(DataStream& ds) {
  ds >> seed;
  ds >> dayLength;
  ds >> nearbyPlanet;
  ds >> nearbyMoons;
  ds >> horizonImages;
  ds >> horizonClouds;
  ds >> skyType;
  ds >> skyColoring;
  ds >> spaceLevel;
  ds >> surfaceLevel;
  ds >> sunType;
  if (ds.streamCompatibilityVersion() >= 3)
    ds >> settings;
}

void SkyParameters::write(DataStream& ds) const {
  ds << seed;
  ds << dayLength;
  ds << nearbyPlanet;
  ds << nearbyMoons;
  ds << horizonImages;
  ds << horizonClouds;
  ds << skyType;
  ds << skyColoring;
  ds << spaceLevel;
  ds << surfaceLevel;
  ds << sunType;
  if (ds.streamCompatibilityVersion() >= 3)
    ds << settings;
}

void SkyParameters::readVisitableParameters(VisitableWorldParametersConstPtr visitableParameters) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(visitableParameters)) {
    dayLength = terrestrialParameters->dayLength;
    if (terrestrialParameters->airless) {
      skyType = SkyType::Atmosphereless;
      horizonClouds = false;
    } else {
      skyType = SkyType::Atmospheric;
      horizonClouds = true;
    }
    skyColoring.setLeft(terrestrialParameters->skyColoring);
    spaceLevel = terrestrialParameters->spaceLayer.layerMinHeight;
    surfaceLevel = terrestrialParameters->atmosphereLayer.layerMinHeight;
  } else {
    skyType = SkyType::Barren;
    horizonClouds = false;

    if (auto asteroidsParameters = as<AsteroidsWorldParameters>(visitableParameters)) {
      skyColoring.setRight(asteroidsParameters->ambientLightLevel);
    } else if (auto floatingDungeonParameters = as<FloatingDungeonWorldParameters>(visitableParameters)) {
      skyColoring.setRight(floatingDungeonParameters->ambientLightLevel);
    } else {
      skyColoring.setRight(Color::Black);
    }
  }
}

DataStream& operator>>(DataStream& ds, SkyParameters& sky) {
  sky.read(ds);
  return ds;
}

DataStream& operator<<(DataStream& ds, SkyParameters const& sky) {
  sky.write(ds);
  return ds;
}

}
