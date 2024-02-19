#include "StarCelestialGraphics.hpp"
#include "StarJsonExtra.hpp"
#include "StarLexicalCast.hpp"
#include "StarFormat.hpp"
#include "StarImageProcessing.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarParallax.hpp"
#include "StarRoot.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarTerrainDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarAssets.hpp"

namespace Star {

List<pair<String, float>> CelestialGraphics::drawSystemPlanetaryObject(CelestialParameters const& parameters) {
  return {{parameters.getParameter("smallImage").toString(), parameters.getParameter("smallImageScale").toFloat()}};
}

List<pair<String, float>> CelestialGraphics::drawSystemCentralBody(CelestialParameters const& parameters) {
  return {{parameters.getParameter("image").toString(), parameters.getParameter("imageScale").toFloat()}};
}

List<pair<String, float>> CelestialGraphics::drawWorld(
    CelestialParameters const& celestialParameters, Maybe<CelestialParameters> const& overrideShadowParameters) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto liquidsDatabase = root.liquidsDatabase();

  CelestialParameters shadowParameters = overrideShadowParameters.value(celestialParameters);

  String type = celestialParameters.getParameter("worldType").toString();

  List<pair<String, float>> layers;

  if (type == "Terrestrial") {
    auto terrestrialParameters = as<TerrestrialWorldParameters>(celestialParameters.visitableParameters());
    if (!terrestrialParameters)
      return {};

    auto gfxConfig = jsonMerge(assets->json("/celestial.config:terrestrialGraphics").get("default"),
        assets->json("/celestial.config:terrestrialGraphics").get(terrestrialParameters->typeName, JsonObject()));

    auto liquidImages = gfxConfig.getString("liquidImages", "");
    auto baseImages = gfxConfig.getString("baseImages", "");
    auto shadowImages = gfxConfig.getString("shadowImages", "");
    auto baseCount = gfxConfig.getInt("baseCount", 0);
    auto dynamicsImages = gfxConfig.getString("dynamicsImages", "");
    float imageScale = celestialParameters.getParameter("imageScale", 1.0f).toFloat();

    // If the planet has water, then draw the corresponding water image as the
    // base layer, otherwise use the bottom most mask image.
    if (terrestrialParameters->primarySurfaceLiquid != EmptyLiquidId && !liquidImages.empty()) {
      String liquidBaseImage = liquidImages.replace("<liquid>", liquidsDatabase->liquidName(terrestrialParameters->primarySurfaceLiquid));
      layers.append({std::move(liquidBaseImage), imageScale});
    } else {
      if (baseCount > 0) {
        String baseLayer = strf("{}?hueshift={}", baseImages.replace("<biome>",
            terrestrialParameters->primaryBiome).replace("<num>", toString(baseCount)), terrestrialParameters->hueShift);
        layers.append({std::move(baseLayer), imageScale});
      }
    }

    // Then draw all the biome layers
    for (int i = 0; i < baseCount; ++i) {
      String baseImage = baseImages.replace("<num>", toString(baseCount - i));
      String hueShiftString, dynamicMaskString;
      if (!dynamicsImages.empty())
        dynamicMaskString = "?addmask=" + dynamicsImages.replace("<num>", toString(celestialParameters.randomizeParameterRange(gfxConfig.getArray("dynamicsRange"), i).toInt()));
      if (terrestrialParameters->hueShift != 0)
        hueShiftString = strf("?hueshift={}", terrestrialParameters->hueShift);
      String layer = baseImage + hueShiftString + dynamicMaskString;
      layers.append({std::move(layer), imageScale});
    }

    if (!shadowImages.empty()) {
      String shadow = shadowImages.replace("<num>", toString(shadowParameters.randomizeParameterRange(gfxConfig.getArray("shadowNumber")).toInt()));
      layers.append({std::move(shadow), imageScale});
    }

  } else if (type == "Asteroids") {
    String maskImages = celestialParameters.getParameter("maskImages").toString();
    int maskCount = celestialParameters.getParameter("masks").toInt();
    String dynamicsImages = celestialParameters.getParameter("dynamicsImages").toString();
    float imageScale = celestialParameters.getParameter("imageScale", 1.0f).toFloat();

    for (int i = 0; i < maskCount; ++i) {
      String biomeMaskBase = maskImages.replace("<num>", toString(maskCount - i));
      String dynamicMask = dynamicsImages.replace("<num>", toString(celestialParameters.randomizeParameterRange("dynamicsRange", i).toInt()));
      String layer = strf("{}?addmask={}", biomeMaskBase, dynamicMask);
      layers.append({std::move(layer), imageScale});
    }

  } else if (type == "FloatingDungeon") {
    String image = celestialParameters.getParameter("image").toString();
    float imageScale = celestialParameters.getParameter("imageScale", 1.0f).toFloat();
    layers.append({std::move(image), imageScale});

    if (!celestialParameters.getParameter("dynamicsImages").toString().empty()) {
      String dynamicsImages = celestialParameters.getParameter("dynamicsImages", "").toString();
      String dynamicsImage = dynamicsImages.replace("<num>", toString(celestialParameters.randomizeParameterRange("dynamicsRange").toInt()));
      layers.append({std::move(dynamicsImage), imageScale});
    }

  } else if (type == "GasGiant") {
    auto gfxConfig = assets->json("/celestial.config:gasGiantGraphics");

    auto baseImage = gfxConfig.getString("baseImage", "");
    auto shadowImages = gfxConfig.getString("shadowImages", "");
    auto dynamicsImages = gfxConfig.getString("dynamicsImages", "");
    auto overlayImages = gfxConfig.getString("overlayImages", "");
    auto overlayCount = gfxConfig.getInt("overlayCount", 0);
    float imageScale = celestialParameters.getParameter("imageScale", 1.0f).toFloat();

    float hueShift = celestialParameters.randomizeParameterRange(gfxConfig.getArray("primaryHueShiftRange")).toFloat();
    if (!baseImage.empty())
      layers.append({strf("{}?hueshift={}", baseImage, hueShift), imageScale});

    if (!overlayImages.empty()) {
      for (int i = 0; i < overlayCount; ++i) {
        hueShift += celestialParameters.randomizeParameterRange(gfxConfig.getArray("hueShiftOffsetRange")).toFloat();
        String maskImage = dynamicsImages.replace("<num>", toString(celestialParameters.randomizeParameterRange(gfxConfig.getArray("dynamicsRange"), i).toInt()));
        String overlayImage = overlayImages.replace("<num>", toString(i));
        layers.append({strf("{}?hueshift={}?addmask={}", overlayImage, hueShift, maskImage), imageScale});
      }
    }

    if (!shadowImages.empty()) {
      String shadow = shadowImages.replace("<num>", toString(shadowParameters.randomizeParameterRange(gfxConfig.getArray("shadowNumber")).toInt()));
      layers.append({std::move(shadow), imageScale});
    }
  }

  return layers;
}

List<pair<String, String>> CelestialGraphics::worldHorizonImages(CelestialParameters const& celestialParameters) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto liquidsDatabase = root.liquidsDatabase();

  auto getLR = [](String const& base) -> pair<String, String> {
    return pair<String, String>(base.replace("<selector>", "l"), base.replace("<selector>", "r"));
  };

  String type = celestialParameters.getParameter("worldType").toString();

  List<pair<String, String>> res;

  if (type == "Terrestrial") {
    auto terrestrialParameters = as<TerrestrialWorldParameters>(celestialParameters.visitableParameters());
    if (!terrestrialParameters)
      return {};

    auto gfxConfig = jsonMerge(assets->json("/celestial.config:terrestrialHorizonGraphics").get("default"),
        assets->json("/celestial.config:terrestrialHorizonGraphics").get(terrestrialParameters->typeName, JsonObject()));

    String baseImages = gfxConfig.getString("baseImages");
    String atmoTextures = gfxConfig.getString("atmosphereTextures");
    String shadowTextures = gfxConfig.getString("shadowTextures");
    String maskTextures = gfxConfig.getString("maskTextures");
    String liquidTextures = gfxConfig.getString("liquidTextures");
    auto numMasks = jsonToVec2I(gfxConfig.get("maskRange"));
    auto maskPerPlanetRange = jsonToVec2I(gfxConfig.get("maskPerPlanetRange"));

    auto biomeHueShift = "?" + imageOperationToString(HueShiftImageOperation::hueShiftDegrees(terrestrialParameters->hueShift));

    if (terrestrialParameters->primarySurfaceLiquid != EmptyLiquidId) {
      auto seed = celestialParameters.seed();
      RandomSource rand(seed);

      int numPlanetMasks = rand.randInt(maskPerPlanetRange[0], maskPerPlanetRange[1]);
      List<int> masks;
      for (int i = 0; i < numPlanetMasks; ++i)
        masks.append(rand.randInt(numMasks[0], numMasks[1]));

      String liquidBase = liquidTextures.replace("<liquid>", liquidsDatabase->liquidName(terrestrialParameters->primarySurfaceLiquid));
      res.append(getLR(liquidBase));

      StringList planetMaskListL;
      StringList planetMaskListR;
      for (auto m : masks) {
        String base = maskTextures.replace("<mask>", toString(m));
        auto lr = getLR(base);
        planetMaskListL.append(lr.first);
        planetMaskListR.append(lr.second);
      }

      String leftMask, rightMask;
      if (!planetMaskListL.empty())
        leftMask = "?" + imageOperationToString(AlphaMaskImageOperation{AlphaMaskImageOperation::Additive, planetMaskListL, {0, 0}});
      if (!planetMaskListR.empty())
        rightMask = "?" + imageOperationToString(AlphaMaskImageOperation{AlphaMaskImageOperation::Additive, planetMaskListR, {0, 0}});

      auto toAppend = getLR(baseImages + biomeHueShift);
      res.append({toAppend.first + leftMask, toAppend.second + rightMask});
    } else {
      res.append(getLR(baseImages + biomeHueShift));
    }

    if (celestialParameters.getParameter("atmosphere", true).toBool())
      res.append(getLR(atmoTextures));

    res.append(getLR(shadowTextures));

  } else if (type == "Asteroids") {
    res.append(getLR(assets->json("/celestial.config:asteroidsHorizons").toString()));

  } else if (type == "FloatingDungeon") {
    auto dungeonParameters = as<FloatingDungeonWorldParameters>(celestialParameters.visitableParameters());
    auto dungeonHorizons = assets->json("/celestial.config:floatingDungeonHorizons");
    if (dungeonHorizons.contains(dungeonParameters->primaryDungeon))
      res.append(getLR(dungeonHorizons.get(dungeonParameters->primaryDungeon).toString()));
  }

  return res;
}

int CelestialGraphics::worldRadialPosition(CelestialParameters const& parameters) {
  if (parameters.coordinate().isPlanetaryBody())
    return staticRandomU32(parameters.seed(), "RadialNumber") % planetRadialPositions();
  if (parameters.coordinate().isSatelliteBody())
    return staticRandomU32(parameters.seed(), "RadialNumber") % satelliteRadialPositions();
  return 0;
}

int CelestialGraphics::planetRadialPositions() {
  return Root::singleton().assets()->json("/celestial.config:planetRadialSlots").toInt();
}

int CelestialGraphics::satelliteRadialPositions() {
  return Root::singleton().assets()->json("/celestial.config:satelliteRadialSlots").toInt();
}

List<pair<String, float>> CelestialGraphics::drawSystemTwinkle(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& system, double time) {
  auto parameters = celestialDatabase->parameters(system);
  if (!parameters)
    return {};

  auto assets = Root::singleton().assets();

  int twinkleFrameCount = assets->json("/celestial.config:twinkleFrames").toInt();
  float twinkleScale = assets->json("/celestial.config:twinkleScale").toFloat();
  String twinkleFrameset = parameters->getParameter("twinkleFrames").toString();
  float twinkleTime = parameters->randomizeParameterRange("twinkleTime").toFloat();
  String twinkleBackground = parameters->getParameter("twinkleBackground").toString();

  String twinkleFrame = strf("{}:{}", twinkleFrameset, (int)(std::fmod<double>(time / twinkleTime, 1.0f) * twinkleFrameCount));

  return {{std::move(twinkleBackground), 1.0f}, {std::move(twinkleFrame), twinkleScale}};
}

List<pair<String, float>> CelestialGraphics::drawSystemPlanetaryObject(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate) {
  if (auto params = celestialDatabase->parameters(coordinate))
    return drawSystemPlanetaryObject(params.take());
  return {};
}

List<pair<String, float>> CelestialGraphics::drawSystemCentralBody(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate) {
  if (auto params = celestialDatabase->parameters(coordinate))
    return drawSystemCentralBody(params.take());
  return {};
}

List<pair<String, float>> CelestialGraphics::drawWorld(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate) {
  auto params = celestialDatabase->parameters(coordinate);
  if (!params)
    return {};

  if (coordinate.isSatelliteBody())
    return drawWorld(params.take(), celestialDatabase->parameters(coordinate.parent()));
  else
    return drawWorld(params.take());
}

List<pair<String, String>> CelestialGraphics::worldHorizonImages(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate) {
  if (auto params = celestialDatabase->parameters(coordinate))
    return worldHorizonImages(params.take());
  return {};
}

int CelestialGraphics::worldRadialPosition(CelestialDatabasePtr celestialDatabase, CelestialCoordinate const& coordinate) {
  if (auto params = celestialDatabase->parameters(coordinate))
    return worldRadialPosition(params.take());
  return 0;
}

}
