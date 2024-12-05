#include "StarCelestialLuaBindings.hpp"
#include "StarUniverseClient.hpp"
#include "StarSystemWorldClient.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarCelestialGraphics.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeCelestialCallbacks(UniverseClient* client) {
  LuaCallbacks callbacks;

  auto systemWorld = client->systemWorldClient();
  auto celestialDatabase = client->celestialDatabase();

  callbacks.registerCallback("skyFlying", [client]() {
      return client->currentSky()->flying();
    });
  callbacks.registerCallback("skyFlyingType", [client]() -> String {
      return FlyingTypeNames.getRight(client->currentSky()->flyingType());
    });
  callbacks.registerCallback("skyWarpPhase", [client]() -> String {
      return WarpPhaseNames.getRight(client->currentSky()->warpPhase());
    });
  callbacks.registerCallback("skyWarpProgress", [client]() -> float {
      return client->currentSky()->warpProgress();
    });
  callbacks.registerCallback("skyInHyperspace", [client]() -> bool {
      return client->currentSky()->inHyperspace();
    });

  callbacks.registerCallback("flyShip", [client,systemWorld](Vec3I const& system, Json const& destination, Json const& settings) {
      auto location = jsonToSystemLocation(destination);
      client->flyShip(system, location, settings);
    });
  callbacks.registerCallback("flying", [systemWorld]() {
      return systemWorld->flying();
    });
  callbacks.registerCallback("shipSystemPosition", [systemWorld]() -> Maybe<Vec2F> {
      return systemWorld->shipPosition();
    });
  callbacks.registerCallback("shipDestination", [systemWorld]() -> Json {
      return jsonFromSystemLocation(systemWorld->shipDestination());
    });
  callbacks.registerCallback("shipLocation", [systemWorld]() -> Json {
      return jsonFromSystemLocation(systemWorld->shipLocation());
    });
  callbacks.registerCallback("currentSystem", [systemWorld]() -> Json {
      return systemWorld->currentSystem().toJson();
    });

  callbacks.registerCallback("planetSize", [systemWorld](Json const& coords) -> float {
      return systemWorld->planetSize(CelestialCoordinate(coords));
    });
  callbacks.registerCallback("planetPosition", [systemWorld](Json const& coords) -> Vec2F {
      return systemWorld->planetPosition(CelestialCoordinate(coords));
    });
  callbacks.registerCallback("planetParameters", [celestialDatabase](Json const& coords) -> Json {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      if (auto celestialParameters = celestialDatabase->parameters(coordinate))
        return celestialParameters->parameters();
      return Json();
    });
  callbacks.registerCallback("visitableParameters", [celestialDatabase](Json const& coords) -> Json {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      if (auto parameters = celestialDatabase->parameters(coordinate)) {
        if (auto visitableParameters = parameters->visitableParameters())
          return visitableParameters->store();
      }
      return {};
    });
  callbacks.registerCallback("planetName", [celestialDatabase](Json const& coords) -> Maybe<String> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      if (auto parameters = celestialDatabase->parameters(coordinate))
        return parameters->name();
      return {};
    });
  callbacks.registerCallback("planetSeed", [celestialDatabase](Json const& coords) -> Maybe<uint64_t> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      if (auto parameters = celestialDatabase->parameters(coordinate))
        return parameters->seed();
      return {};
    });
  callbacks.registerCallback("clusterSize", [systemWorld](Json const& coords) -> float {
      return systemWorld->clusterSize(CelestialCoordinate(coords));
    });
  callbacks.registerCallback("planetOres", [celestialDatabase](Json const& coords, float threatLevel) -> List<String> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      auto parameters = celestialDatabase->parameters(coordinate);
      if (!parameters)
        return {};

      auto visitableParameters = parameters->visitableParameters();
      if (!visitableParameters)
        return {};

      auto biomeDatabase = Root::singleton().biomeDatabase();
      auto addOres = [biomeDatabase, threatLevel](StringSet& oreList, String const& biomeName) {
          oreList.addAll(biomeDatabase->biomeOres(biomeName, threatLevel));
        };

      StringSet planetOres;
      if (auto const& terrestrialParameters = as<TerrestrialWorldParameters>(visitableParameters)) {
        addOres(planetOres, terrestrialParameters->primaryBiome);
        addOres(planetOres, terrestrialParameters->surfaceLayer.primaryRegion.biome);
        addOres(planetOres, terrestrialParameters->subsurfaceLayer.primaryRegion.biome);
        for (auto layer : terrestrialParameters->undergroundLayers)
          addOres(planetOres, layer.primaryRegion.biome);
        addOres(planetOres, terrestrialParameters->coreLayer.primaryRegion.biome);
      } else if (auto const& asteroidParameters = as<AsteroidsWorldParameters>(visitableParameters)) {
        addOres(planetOres, asteroidParameters->asteroidBiome);
      }

      return planetOres.values();
    });

  callbacks.registerCallback("systemPosition", [systemWorld](Json const& l) -> Maybe<Vec2F> {
      auto location = jsonToSystemLocation(l);
      return systemWorld->systemLocationPosition(location);
    });
  callbacks.registerCallback("orbitPosition", [systemWorld](Json const& orbit) -> Vec2F {
      return systemWorld->orbitPosition(CelestialOrbit::fromJson(orbit));
    });

  callbacks.registerCallback("systemObjects", [systemWorld]() -> List<String> {
      return systemWorld->objects().transformed([](SystemObjectPtr object) { return object->uuid().hex(); });
    });
  callbacks.registerCallback("objectType", [systemWorld](String const& uuid) -> Maybe<String> {
      if (auto object = systemWorld->getObject(Uuid(uuid)))
        return object->name();
      else
        return {};
    });
  callbacks.registerCallback("objectParameters", [systemWorld](String const& uuid) -> Json {
      if (auto object = systemWorld->getObject(Uuid(uuid)))
        return object->parameters();
      else
        return {};
    });
  callbacks.registerCallback("objectWarpActionWorld", [systemWorld](String const& uuid) -> Maybe<String> {
      if (Maybe<WarpAction> action = systemWorld->objectWarpAction((Uuid(uuid)))) {
        return action.get().maybe<WarpToWorld>().apply([](WarpToWorld const& warp) { return printWorldId(warp.world); });
      } else {
        return {};
      }
    });
  callbacks.registerCallback("objectOrbit", [systemWorld](String const& uuid) -> Json {
      if (auto object = systemWorld->getObject(Uuid(uuid)))
        return jsonFromMaybe<CelestialOrbit>(object->orbit(), [](CelestialOrbit const& orbit) { return orbit.toJson(); });
      else
        return {};
    });
  callbacks.registerCallback("objectPosition", [systemWorld](String const& uuid) -> Maybe<Vec2F> {
      if (auto object = systemWorld->getObject(Uuid(uuid)))
        return object->position();
      else
        return {};
    });
  callbacks.registerCallback("objectTypeConfig", [](String const& typeName) -> Json {
      return SystemWorld::systemObjectTypeConfig(typeName);
    });
  callbacks.registerCallback("systemSpawnObject", [systemWorld](String const& typeName, Maybe<Vec2F> const& position, Maybe<String> uuidHex, Maybe<JsonObject> parameters) -> String {
      Maybe<Uuid> uuid = uuidHex.apply([](auto const& u) { return Uuid(u); });
      return systemWorld->spawnObject(typeName, position, uuid, parameters.value({})).hex();
    });

  callbacks.registerCallback("playerShips", [systemWorld]() -> List<String> {
      return systemWorld->ships().transformed([](SystemClientShipPtr ship) { return ship->uuid().hex(); });
    });
  callbacks.registerCallback("playerShipPosition", [systemWorld](String const& uuid) -> Maybe<Vec2F> {
      if (auto ship = systemWorld->getShip(Uuid(uuid)))
        return ship->position();
      else
        return {};
    });

  callbacks.registerCallback("hasChildren", [celestialDatabase](Json const& coords) -> Maybe<bool> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return celestialDatabase->hasChildren(coordinate);
    });
  callbacks.registerCallback("children", [celestialDatabase](Json const& coords) -> List<Json> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return celestialDatabase->children(coordinate).transformed([](CelestialCoordinate const& c) { return c.toJson(); });
    });
  callbacks.registerCallback("childOrbits", [celestialDatabase](Json const& coords) -> List<int> {
      return celestialDatabase->childOrbits(CelestialCoordinate(coords));
    });
  callbacks.registerCallback("scanSystems", [celestialDatabase](RectI const& region, Maybe<StringSet> const& includedTypes) -> List<Json> {
      return celestialDatabase->scanSystems(region, includedTypes).transformed([](CelestialCoordinate const& c) { return c.toJson(); });
    });
  callbacks.registerCallback("scanConstellationLines", [celestialDatabase](RectI const& region) -> List<pair<Vec2I, Vec2I>> {
      return celestialDatabase->scanConstellationLines(region);
    });
  callbacks.registerCallback("scanRegionFullyLoaded", [celestialDatabase](RectI const& region) -> bool {
      return celestialDatabase->scanRegionFullyLoaded(region);
    });

  callbacks.registerCallback("centralBodyImages", [celestialDatabase](Json const& coords) -> List<pair<String, float>> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return CelestialGraphics::drawSystemCentralBody(celestialDatabase, coordinate);
    });
  callbacks.registerCallback("planetaryObjectImages", [celestialDatabase](Json const& coords) -> List<pair<String, float>> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return CelestialGraphics::drawSystemPlanetaryObject(celestialDatabase, coordinate);
    });
  callbacks.registerCallback("worldImages", [celestialDatabase](Json const& coords) -> List<pair<String, float>> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return CelestialGraphics::drawWorld(celestialDatabase, coordinate);
    });
  callbacks.registerCallback("starImages", [celestialDatabase](Json const& coords, float twinkleTime) -> List<pair<String, float>> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return CelestialGraphics::drawSystemTwinkle(celestialDatabase, coordinate, twinkleTime);
    });

  return callbacks;
}

}
