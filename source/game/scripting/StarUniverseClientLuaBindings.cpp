#include "StarUniverseClientLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarWarping.hpp"
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
  
  callbacks.registerCallback("clientUuid", [universe]() {
    if (!universe->isConnected())
      throw StarException("Universe is not connected");
    return universe->clientContext()->playerUuid().hex();
  });
  
  callbacks.registerCallback("serverOpenProtocolVersion", [universe]() {
    return universe->connectionVersion();
  });
  
  callbacks.registerCallback("playerCount", [universe]() -> Vec2U {
    if (!universe->isConnected())
      return Vec2U();
    return Vec2U(universe->players(),universe->maxPlayers());
  });
  
  callbacks.registerCallback("subWorldActive", [universe](String const& worldId) -> bool {
    return universe->subWorldExistsOnWorld(parseWorldId(worldId));
  });
  
  callbacks.registerCallback("loadSubWorld", [universe](String const& worldId) {
    universe->getSubWorldOnWorld(parseWorldId(worldId));
  });
  
  callbacks.registerCallback("unloadSubWorld", [universe](String const& worldId) {
    universe->destroySubWorldOnWorld(parseWorldId(worldId));
  });
  
  callbacks.registerCallback("sendSubWorldMessage", [universe](String const& worldId, String const& message, LuaVariadic<Json> args) -> RpcThreadPromise<Json> {
    return universe->sendSubWorldOnWorldMessage(parseWorldId(worldId), message, JsonArray::from(std::move(args)));
  });
  
  callbacks.registerCallback("sendMainWorldMessage", [universe](String const& message, LuaVariadic<Json> args) -> RpcPromise<Json> {
    return universe->sendMainWorldMessage(message, JsonArray::from(std::move(args)));
  });
  
  // primarily useful on the main world and other non-universe contexts, as universe is accessible everywhere on the main client thread
  callbacks.registerCallback("callScriptContext", [universe](String const& contextName, String const& function, LuaVariadic<LuaValue> const& args) -> Maybe<LuaValue> {
    auto context = universe->scriptContext(contextName);
    if (!context)
      throw StarException::format("Context {} does not exist", contextName);
    return context->invoke(function, args);
  });
  
  // primarily useful on universe contexts
  callbacks.registerCallback("callMainWorldScriptContext", [universe](String const& contextName, String const& function, LuaVariadic<LuaValue> const& args) -> Maybe<LuaValue> {
    if (!universe->isConnected())
      throw StarException("Universe is not connected");
    auto world = universe->worldClient();
    if (!world->inWorld())
      throw StarException("Not in a world");
    auto context = world->scriptContext(contextName);
    if (!context)
      throw StarException::format("Context {} does not exist", contextName);
    return context->invoke(function, args);
  });

  return callbacks;
}

}
