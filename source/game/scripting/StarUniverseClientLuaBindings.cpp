#include "StarUniverseClientLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarUniverseClient.hpp"
#include "StarWorldTemplate.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeUniverseClientCallbacks(UniverseClientPtr universe) {
  LuaCallbacks callbacks;
  
  callbacks.registerCallback("createClientCustomWorld", [universe](String name, Json templateData) {
    universe->createCustomWorld(name, templateData);
  });
  
  callbacks.registerCallback("createClientCustomWorldFromConfig", [universe](String name, Json worldConfig) {
    uint64_t worldSeed;
    if (worldConfig.contains("seed"))
      worldSeed = worldConfig.getUInt("seed");
    else
      worldSeed = Random::randu64();

    String worldType = worldConfig.getString("type");

    VisitableWorldParametersPtr worldParameters;
    if (worldType.equalsIgnoreCase("Terrestrial"))
      worldParameters = generateTerrestrialWorldParameters(worldConfig.getString("planetType"), worldConfig.getString("planetSize"), worldSeed);
    else if (worldType.equalsIgnoreCase("Asteroids"))
      worldParameters = generateAsteroidsWorldParameters(worldSeed);
    else if (worldType.equalsIgnoreCase("FloatingDungeon"))
      worldParameters = generateFloatingDungeonWorldParameters(worldConfig.getString("dungeonWorld"));
    else
      throw StarException(strf("Unknown world type: '{}'\n", worldType));

    if (worldConfig.contains("level"))
      worldParameters->threatLevel = worldConfig.getFloat("level");

    if (worldConfig.contains("beamUpRule"))
      worldParameters->beamUpRule = BeamUpRuleNames.getLeft(worldConfig.getString("beamUpRule"));
    worldParameters->disableDeathDrops = worldConfig.getBool("disableDeathDrops", false);

    SkyParameters skyParameters = SkyParameters(worldConfig.get("skyParameters", Json()));
    auto worldTemplate = WorldTemplate(worldParameters, skyParameters, worldSeed);
    universe->createCustomWorld(name, worldTemplate.store());
  });

  return callbacks;
}

}
