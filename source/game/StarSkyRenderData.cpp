#include "StarSkyRenderData.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRandomPoint.hpp"
#include "StarDrawable.hpp"

namespace Star {

StringList SkyRenderData::starTypes() const {
  if (type == SkyType::Warp)
    return hyperStarList;
  else
    return starList;
}

List<SkyOrbiter> SkyRenderData::backOrbiters(Vec2F const& viewSize) const {
  if (!settings)
    return {};

  float planetScale = settings.queryFloat("satellite.planetScale");
  float moonScale = settings.queryFloat("satellite.moonScale");

  List<tuple<List<pair<String, float>>, Vec2F, float>> orbitingCelestialObjects;

  // Gather up all the CelestialParameters and scales for all the celestial
  // objects to draw in the sky, we should draw the parent planet if we are a
  // satellite, as well as all the other satellites.
  if (skyParameters.nearbyPlanet)
    orbitingCelestialObjects.append(tuple_cat(*skyParameters.nearbyPlanet, tie(planetScale)));

  for (auto moon : skyParameters.nearbyMoons)
    orbitingCelestialObjects.append(tuple_cat(moon, tie(moonScale)));

  Vec2F satelliteArea = jsonToVec2F(settings.query("satellite.area"));
  auto planetCenter = Vec2F(viewSize[0] / 2, 0) - worldOffset;
  auto rotMatrix = Mat3F::rotation(worldRotation, planetCenter);

  List<SkyOrbiter> orbiters;

  for (auto const& object : orbitingCelestialObjects) {
    auto const& layers = get<0>(object);
    Vec2F pos = get<1>(object);
    pos = pos.piecewiseMultiply(satelliteArea);
    pos -= worldOffset;
    pos = rotMatrix.transformVec2(pos);
    for (auto const& l : layers)
      orbiters.append(SkyOrbiter{SkyOrbiterType::Moon, get<2>(object) * l.second, 0.0f, l.first, pos});
  }

  return orbiters;
}

SkyWorldHorizon SkyRenderData::worldHorizon(Vec2F const& viewSize) const {
  if (!settings)
    return {};

  SkyWorldHorizon worldHorizon;

  if (type == SkyType::Orbital) {
    worldHorizon.center = Vec2F(viewSize[0] / 2, 0) - worldOffset;
    worldHorizon.scale = settings.queryFloat("planetHorizon.scale");
    worldHorizon.rotation = worldRotation;
    worldHorizon.layers = skyParameters.horizonImages;
  }

  return worldHorizon;
}

List<SkyOrbiter> SkyRenderData::frontOrbiters(Vec2F const& viewSize) const {
  if (!settings)
    return {};

  struct HorizonCloud {
    float startAngle;
    String image;
    float speed;
    float radius;
  };
  List<HorizonCloud> horizonClouds;

  if (skyParameters.horizonClouds) {
    Vec2I cloudCountRange = jsonToVec2I(settings.query("planetHorizon.cloudCount"));
    Vec2F cloudRadiusRange = jsonToVec2F(settings.query("planetHorizon.cloudRadius"));
    Vec2F cloudSpeedRange = jsonToVec2F(settings.query("planetHorizon.cloudSpeed"));
    StringList cloudList = jsonToStringList(settings.query("planetHorizon.clouds"));

    int numClouds = staticRandomI32Range(cloudCountRange[0], cloudCountRange[1], "HorizonCloudCount");
    for (int i = 0; i < numClouds; ++i) {
      horizonClouds.append({staticRandomFloatRange(0, 2 * Constants::pi, i, "CloudStartAngle"),
          staticRandomFrom(cloudList, i, "Cloud"),
          staticRandomFloatRange(cloudSpeedRange[0], cloudSpeedRange[1], i, "CloudSpeed"),
          staticRandomFloatRange(cloudRadiusRange[0], cloudRadiusRange[1], i, "CloudRadius")});
    }
  }

  List<SkyOrbiter> orbiters;
  if (type == SkyType::Atmospheric || type == SkyType::Atmosphereless) {
    orbiters.append({SkyOrbiterType::Sun,
        1.0f,
        0.0f,
        settings.queryString("sun.image"),
        Vec2F::withAngle(orbitAngle, settings.queryFloat("sun.radius")) + viewSize / 2});
  } else if (type == SkyType::Orbital) {
    auto planetCenter = Vec2F(viewSize[0] / 2, 0)
        - Vec2F::withAngle(worldRotation - Constants::pi / 2, settings.queryFloat("planetHorizon.yCenter")) - worldOffset;

    float scale = settings.queryFloat("planetHorizon.scale");

    auto rotMatrix = Mat3F::rotation(worldRotation, planetCenter);

    if (skyParameters.horizonClouds) {
      for (auto const& horizonCloud : horizonClouds) {
        Vec2F position = Vec2F::withAngle(horizonCloud.startAngle + orbitAngle * horizonCloud.speed, horizonCloud.radius) + planetCenter;
        position = rotMatrix.transformVec2(position);
        orbiters.append({SkyOrbiterType::HorizonCloud, scale, worldRotation, horizonCloud.image, position});
      }
    }
  }

  return orbiters;
}

DataStream& operator>>(DataStream& ds, SkyRenderData& skyRenderData) {
  ds.read(skyRenderData.settings);
  ds.read(skyRenderData.skyParameters);
  ds.read(skyRenderData.type);
  ds.read(skyRenderData.dayLevel);
  ds.read(skyRenderData.skyAlpha);
  ds.read(skyRenderData.dayLength);
  ds.read(skyRenderData.timeOfDay);
  ds.read(skyRenderData.epochTime);
  ds.read(skyRenderData.starOffset);
  ds.read(skyRenderData.starRotation);
  ds.read(skyRenderData.worldOffset);
  ds.read(skyRenderData.worldRotation);
  ds.read(skyRenderData.orbitAngle);
  ds.readVlqS(skyRenderData.starFrames);
  ds.read(skyRenderData.starList);
  ds.read(skyRenderData.hyperStarList);
  ds.read(skyRenderData.environmentLight);
  ds.read(skyRenderData.mainSkyColor);
  ds.read(skyRenderData.topRectColor);
  ds.read(skyRenderData.bottomRectColor);
  ds.read(skyRenderData.flashColor);

  return ds;
}

DataStream& operator<<(DataStream& ds, SkyRenderData const& skyRenderData) {
  ds.write(skyRenderData.settings);
  ds.write(skyRenderData.skyParameters);
  ds.write(skyRenderData.type);
  ds.write(skyRenderData.dayLevel);
  ds.write(skyRenderData.skyAlpha);
  ds.write(skyRenderData.dayLength);
  ds.write(skyRenderData.timeOfDay);
  ds.write(skyRenderData.epochTime);
  ds.write(skyRenderData.starOffset);
  ds.write(skyRenderData.starRotation);
  ds.write(skyRenderData.worldOffset);
  ds.write(skyRenderData.worldRotation);
  ds.write(skyRenderData.orbitAngle);
  ds.writeVlqS(skyRenderData.starFrames);
  ds.write(skyRenderData.starList);
  ds.write(skyRenderData.hyperStarList);
  ds.write(skyRenderData.environmentLight);
  ds.write(skyRenderData.mainSkyColor);
  ds.write(skyRenderData.topRectColor);
  ds.write(skyRenderData.bottomRectColor);
  ds.write(skyRenderData.flashColor);

  return ds;
}

}
