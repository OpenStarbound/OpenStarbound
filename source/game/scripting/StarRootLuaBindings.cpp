#include "StarRootLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarRoot.hpp"
#include "StarStoredFunctions.hpp"
#include "StarNpcDatabase.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarItemDatabase.hpp"
#include "StarTenantDatabase.hpp"
#include "StarTechDatabase.hpp"
#include "StarTreasure.hpp"
#include "StarBehaviorDatabase.hpp"
#include "StarNameGenerator.hpp"
#include "StarNpc.hpp"
#include "StarMonster.hpp"
#include "StarJsonExtra.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarVersioningDatabase.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarCollectionDatabase.hpp"
#include "StarBehaviorDatabase.hpp"
#include "StarDamageDatabase.hpp"
#include "StarDungeonGenerator.hpp"
#include "StarImageLuaBindings.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeRootCallbacks() {
  LuaCallbacks callbacks;

  auto root = Root::singletonPtr();

  callbacks.registerCallbackWithSignature<String, String>("assetData", bind(RootCallbacks::assetData, root, _1));
  callbacks.registerCallbackWithSignature<Image, String>("assetImage", bind(RootCallbacks::assetImage, root, _1));
  callbacks.registerCallbackWithSignature<Json, String>("assetFrames", bind(RootCallbacks::assetFrames, root, _1));
  callbacks.registerCallbackWithSignature<Json, String>("assetJson", bind(RootCallbacks::assetJson, root, _1));
  callbacks.registerCallbackWithSignature<Json, String, Json>("makeCurrentVersionedJson", bind(RootCallbacks::makeCurrentVersionedJson, root, _1, _2));
  callbacks.registerCallbackWithSignature<Json, Json, String>("loadVersionedJson", bind(RootCallbacks::loadVersionedJson, root, _1, _2));
  callbacks.registerCallbackWithSignature<double, String, double>("evalFunction", bind(RootCallbacks::evalFunction, root, _1, _2));
  callbacks.registerCallbackWithSignature<double, String, double, double>("evalFunction2", bind(RootCallbacks::evalFunction2, root, _1, _2, _3));
  callbacks.registerCallbackWithSignature<Vec2U, String>("imageSize", bind(RootCallbacks::imageSize, root, _1));
  callbacks.registerCallbackWithSignature<List<Vec2I>, String, Vec2F, float, bool>("imageSpaces", bind(RootCallbacks::imageSpaces, root, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<RectU, String>("nonEmptyRegion", bind(RootCallbacks::nonEmptyRegion, root, _1));
  callbacks.registerCallbackWithSignature<Json, String>("npcConfig", bind(RootCallbacks::npcConfig, root, _1));
  callbacks.registerCallbackWithSignature<float, String>("projectileGravityMultiplier", bind(RootCallbacks::projectileGravityMultiplier, root, _1));
  callbacks.registerCallbackWithSignature<Json, String>("projectileConfig", bind(RootCallbacks::projectileConfig, root, _1));
  callbacks.registerCallbackWithSignature<JsonArray, String>("recipesForItem", bind(RootCallbacks::recipesForItem, root, _1));
  callbacks.registerCallbackWithSignature<JsonArray>("allRecipes", bind(RootCallbacks::allRecipes, root));
  callbacks.registerCallbackWithSignature<String, String>("itemType", bind(RootCallbacks::itemType, root, _1));
  callbacks.registerCallbackWithSignature<Json, String>("itemTags", bind(RootCallbacks::itemTags, root, _1));
  callbacks.registerCallbackWithSignature<bool, String, String>("itemHasTag", bind(RootCallbacks::itemHasTag, root, _1, _2));
  callbacks.registerCallbackWithSignature<Json, Json, Maybe<float>, Maybe<uint64_t>>("itemConfig", bind(RootCallbacks::itemConfig, root, _1, _2, _3));
  callbacks.registerCallbackWithSignature<Json, Json, Maybe<float>, Maybe<uint64_t>>("createItem", bind(RootCallbacks::createItem, root, _1, _2, _3));
  callbacks.registerCallbackWithSignature<Json, String>("tenantConfig", bind(RootCallbacks::tenantConfig, root, _1));
  callbacks.registerCallbackWithSignature<Json, StringMap<unsigned>>("getMatchingTenants", bind(RootCallbacks::getMatchingTenants, root, _1));
  callbacks.registerCallbackWithSignature<Json, LiquidId>("liquidStatusEffects", bind(RootCallbacks::liquidStatusEffects, root, _1));
  callbacks.registerCallbackWithSignature<String, String, Maybe<uint64_t>>("generateName", bind(RootCallbacks::generateName, root, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String>("questConfig", bind(RootCallbacks::questConfig, root, _1));
  callbacks.registerCallbackWithSignature<JsonArray, String, String, String, float, Maybe<uint64_t>, Maybe<JsonObject>>("npcPortrait", bind(RootCallbacks::npcPortrait, root, _1, _2, _3, _4, _5, _6));
  callbacks.registerCallbackWithSignature<Json, String, String, float, Maybe<uint64_t>, Maybe<JsonObject>>("npcVariant", bind(RootCallbacks::npcVariant, root, _1, _2, _3, _4, _5));
  callbacks.registerCallbackWithSignature<JsonArray, String, Maybe<JsonObject>>("monsterPortrait", bind(RootCallbacks::monsterPortrait, root, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>("isTreasurePool", bind(RootCallbacks::isTreasurePool, root, _1));
  callbacks.registerCallbackWithSignature<JsonArray, String, float, Maybe<uint64_t>>("createTreasure", bind(RootCallbacks::createTreasure, root, _1, _2, _3));

  callbacks.registerCallbackWithSignature<Maybe<String>, String, Maybe<String>>("materialMiningSound", bind(RootCallbacks::materialMiningSound, root, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<String>, String, Maybe<String>>("materialFootstepSound", bind(RootCallbacks::materialFootstepSound, root, _1, _2));

  callbacks.registerCallback("assetsByExtension", [root](LuaEngine& engine, String const& extension) -> LuaTable {
    auto& extensions = root->assets()->scanExtension(extension);
    auto table = engine.createTable(extensions.size(), 0);
    size_t i = 0;
    for (auto& file : extensions)
      table.set(++i, file);
    return table;
  });

  callbacks.registerCallback("assetOrigin", [root](String const& path) -> Maybe<String> {
      auto assets = root->assets();
      if (auto descriptor = assets->assetDescriptor(path))
        return assets->assetSourcePath(descriptor->source);
      return {};
    });

  callbacks.registerCallback("assetPatches", [root](LuaEngine& engine, String const& path) -> Maybe<LuaTable> {
      auto assets = root->assets();
      if (auto descriptor = assets->assetDescriptor(path)) {
        auto& patches = descriptor->patchSources;
        auto table = engine.createTable(patches.size(), 0);
        for (size_t i = 0; i != patches.size(); ++i) {
          auto& patch = patches.at(i);
          auto patchTable = engine.createTable(2, 0);
          if (auto sourcePath = assets->assetSourcePath(patch.second))
            patchTable.set(1, *sourcePath);
          patchTable.set(2, patch.first);
          table.set(i + 1, patchTable);
        }
        return table;
      }
      return {};
    });

  callbacks.registerCallback("assetSourcePaths", [root](LuaEngine& engine, Maybe<bool> withMetadata) -> LuaTable {
      auto assets = root->assets();
      auto assetSources = assets->assetSources();
      auto table = engine.createTable(assetSources.size(), 0);
      if (withMetadata.value()) {
        for (auto& assetSource : assetSources)
          table.set(assetSource, assets->assetSourceMetadata(assetSource));
      }
      else {
        size_t i = 0;
        for (auto& assetSource : assetSources)
          table.set(++i, assetSource);
      }
      return table;
    });

  callbacks.registerCallback("materialConfig", [root](String const& materialName) -> Json {
      auto materialId = root->materialDatabase()->materialId(materialName);
      if (auto path = root->materialDatabase()->materialPath(materialId))
        return JsonObject{{"path", *path}, {"config", root->materialDatabase()->materialConfig(materialId).get()}};
      return {};
    });

  callbacks.registerCallback("modConfig", [root](String const& modName) -> Json {
      auto modId = root->materialDatabase()->modId(modName);
      if (auto path = root->materialDatabase()->modPath(modId))
        return JsonObject{{"path", *path}, {"config", root->materialDatabase()->modConfig(modId).get()}};
      return {};
    });

  callbacks.registerCallback("liquidConfig", [root](LuaEngine& engine, LuaValue nameOrId) -> Json {
      LiquidId liquidId;
      if (auto id = engine.luaMaybeTo<uint8_t>(nameOrId))
        liquidId = *id;
      else if (auto name = engine.luaMaybeTo<String>(nameOrId))
        liquidId = root->liquidsDatabase()->liquidId(*name);
      else
        return {};

      if (auto path = root->liquidsDatabase()->liquidPath(liquidId))
        return JsonObject{{"path", *path}, {"config", root->liquidsDatabase()->liquidConfig(liquidId).get()}};
      return {};
    });

  callbacks.registerCallback("liquidName", [root](LiquidId liquidId) -> String {
      return root->liquidsDatabase()->liquidName(liquidId);
    });

  callbacks.registerCallback("liquidId", [root](String liquidName) -> LiquidId {
      return root->liquidsDatabase()->liquidId(liquidName);
    });

  callbacks.registerCallback("monsterSkillParameter", [root](String const& skillName, String const& configParameterName) {
      return root->monsterDatabase()->skillConfigParameter(skillName, configParameterName);
    });

  callbacks.registerCallback("monsterParameters", [root](String const& monsterType, Maybe<uint64_t> seed) {
      return root->monsterDatabase()->monsterVariant(monsterType, seed.value(0)).parameters;
    });

  callbacks.registerCallback("monsterMovementSettings", [root](String const& monsterType, Maybe<uint64_t> seed) {
      return root->monsterDatabase()->monsterVariant(monsterType, seed.value(0)).movementSettings;
    });

  callbacks.registerCallback("createBiome", [root](String const& biomeName, uint64_t seed, float verticalMidPoint, float threatLevel) {
      try {
        return root->biomeDatabase()->createBiome(biomeName, seed, verticalMidPoint, threatLevel)->toJson();
      } catch (BiomeException const&) {
        return Json();
      }
    });

  callbacks.registerCallback("materialHealth", [root](String const& materialName) {
      auto materialId = root->materialDatabase()->materialId(materialName);
      return root->materialDatabase()->materialDamageParameters(materialId).totalHealth();
    });

  callbacks.registerCallback("techType", [root](String const& techName) {
      return TechTypeNames.getRight(root->techDatabase()->tech(techName).type);
    });

  callbacks.registerCallback("hasTech", [root](String const& tech) {
      return root->techDatabase()->contains(tech);
    });

  callbacks.registerCallback("techConfig", [root](String const& tech) {
      return root->techDatabase()->tech(tech).parameters;
    });

  callbacks.registerCallbackWithSignature<Maybe<String>, String>("treeStemDirectory", [root](String const& stemName) {
      return root->plantDatabase()->treeStemDirectory(stemName);
    });

  callbacks.registerCallbackWithSignature<Maybe<String>, String>("treeFoliageDirectory", [root](String const& foliageName) {
      return root->plantDatabase()->treeFoliageDirectory(foliageName);
    });

  callbacks.registerCallback("collection", [root](String const& collectionName) {
      return root->collectionDatabase()->collection(collectionName);
    });

  callbacks.registerCallback("collectables", [root](String const& collectionName) {
      return root->collectionDatabase()->collectables(collectionName);
    });

  callbacks.registerCallback("elementalResistance", [root](String const& damageKindName) -> String {
      DamageKind const& damageKind = root->damageDatabase()->damageKind(damageKindName);
      return root->damageDatabase()->elementalType(damageKind.elementalType).resistanceStat;
    });

  callbacks.registerCallback("dungeonMetadata", [root](String const& name) {
      return root->dungeonDefinitions()->getMetadata(name);
    });

  callbacks.registerCallback("systemObjectTypeConfig", [](String const& name) -> Json {
      return SystemWorld::systemObjectTypeConfig(name);
    });

  callbacks.registerCallback("itemDescriptorsMatch", [](Json const& descriptor1, Json const& descriptor2, Maybe<bool> exactMatch) -> bool {
      return ItemDescriptor(descriptor1).matches(ItemDescriptor(descriptor2), exactMatch.value(false));
    });

  callbacks.registerCallback("getConfiguration", [root](String const& key) -> Json {
      if (key == "title")
        throw StarException(strf("Cannot get {}", key));
      else
        return root->configuration()->get(key);
    });

  callbacks.registerCallback("setConfiguration", [root](String const& key, Json const& value) {
    if (key == "safeScripts" || key == "safe")
      throw StarException(strf("Cannot set {}", key));
    else
      root->configuration()->set(key, value);
    });

  
  callbacks.registerCallback("getConfigurationPath", [root](String const& path) -> Json {
    if (path.empty() || path.beginsWith("title"))
      throw ConfigurationException(strf("cannot get {}", path));
    else
      return root->configuration()->getPath(path);
    });

  callbacks.registerCallback("setConfigurationPath", [root](String const& path, Json const& value) {
    if (path.empty() || path.beginsWith("safeScripts") || path.splitAny("[].").get(0) == "safe")
      throw ConfigurationException(strf("cannot set {}", path));
    else
      root->configuration()->setPath(path, value);
    });

  return callbacks;
}

String LuaBindings::RootCallbacks::assetData(Root* root, String const& path) {
  auto bytes = root->assets()->bytes(path);
  return String(bytes->ptr(), bytes->size());
}

Image LuaBindings::RootCallbacks::assetImage(Root* root, String const& path) {
  return *root->assets()->image(path);
}

Json LuaBindings::RootCallbacks::assetFrames(Root* root, String const& path) {
  if (auto frames = root->assets()->imageFrames(path))
    return frames->toJson();
  return Json();
}

Json LuaBindings::RootCallbacks::assetJson(Root* root, String const& path) {
  return root->assets()->json(path);
}

Json LuaBindings::RootCallbacks::makeCurrentVersionedJson(Root* root, String const& identifier, Json const& content) {
  return root->versioningDatabase()->makeCurrentVersionedJson(identifier, content).toJson();
}

Json LuaBindings::RootCallbacks::loadVersionedJson(Root* root, Json const& versionedJson, String const& identifier) {
  return root->versioningDatabase()->loadVersionedJson(VersionedJson::fromJson(versionedJson), identifier);
}

double LuaBindings::RootCallbacks::evalFunction(Root* root, String const& arg1, double arg2) {
  return root->functionDatabase()->function(arg1)->evaluate(arg2);
}

double LuaBindings::RootCallbacks::evalFunction2(Root* root, String const& arg1, double arg2, double arg3) {
  return root->functionDatabase()->function2(arg1)->evaluate(arg2, arg3);
}

Vec2U LuaBindings::RootCallbacks::imageSize(Root* root, String const& arg1) {
  return root->imageMetadataDatabase()->imageSize(arg1);
}

List<Vec2I> LuaBindings::RootCallbacks::imageSpaces(
    Root* root, String const& arg1, Vec2F const& arg2, float arg3, bool arg4) {
  return root->imageMetadataDatabase()->imageSpaces(arg1, arg2, arg3, arg4);
}

RectU LuaBindings::RootCallbacks::nonEmptyRegion(Root* root, String const& arg1) {
  return root->imageMetadataDatabase()->nonEmptyRegion(arg1);
}

Json LuaBindings::RootCallbacks::npcConfig(Root* root, String const& arg1) {
  return root->npcDatabase()->buildConfig(arg1);
}

float LuaBindings::RootCallbacks::projectileGravityMultiplier(Root* root, String const& arg1) {
  auto projectileDatabase = root->projectileDatabase();
  return projectileDatabase->gravityMultiplier(arg1);
}

Json LuaBindings::RootCallbacks::projectileConfig(Root* root, String const& arg1) {
  auto projectileDatabase = root->projectileDatabase();
  return projectileDatabase->projectileConfig(arg1);
}

JsonArray LuaBindings::RootCallbacks::recipesForItem(Root* root, String const& arg1) {
  auto recipes = root->itemDatabase()->recipesForOutputItem(arg1);
  JsonArray result;
  result.reserve(recipes.size());
  for (auto& recipe : recipes)
    result.append(recipe.toJson());
  return result;
}

JsonArray LuaBindings::RootCallbacks::allRecipes(Root* root) {
  auto& recipes = root->itemDatabase()->allRecipes();
  JsonArray result;
  result.reserve(recipes.size());
  for (auto& recipe : recipes)
    result.append(recipe.toJson());
  return result;
}

String LuaBindings::RootCallbacks::itemType(Root* root, String const& itemName) {
  return ItemTypeNames.getRight(root->itemDatabase()->itemType(itemName));
}

Json LuaBindings::RootCallbacks::itemTags(Root* root, String const& itemName) {
  return jsonFromStringSet(root->itemDatabase()->itemTags(itemName));
}

bool LuaBindings::RootCallbacks::itemHasTag(Root* root, String const& itemName, String const& itemTag) {
  return root->itemDatabase()->itemTags(itemName).contains(itemTag);
}

Json LuaBindings::RootCallbacks::itemConfig(Root* root, Json const& descJson, Maybe<float> const& level, Maybe<uint64_t> const& seed) {
  ItemDescriptor descriptor(descJson);
  if (!root->itemDatabase()->hasItem(descriptor.name()))
    return {};
  auto config = root->itemDatabase()->itemConfig(descriptor.name(), descriptor.parameters(), level, seed);
  return JsonObject{{"directory", config.directory}, {"config", config.config}, {"parameters", config.parameters}};
}

Json LuaBindings::RootCallbacks::createItem(Root* root, Json const& descriptor, Maybe<float> const& level, Maybe<uint64_t> const& seed) {
  auto item = root->itemDatabase()->item(ItemDescriptor(descriptor), level, seed);
  return item->descriptor().toJson();
}

Json LuaBindings::RootCallbacks::tenantConfig(Root* root, String const& tenantName) {
  return root->tenantDatabase()->getTenant(tenantName)->config;
}

JsonArray LuaBindings::RootCallbacks::getMatchingTenants(Root* root, StringMap<unsigned> const& colonyTags) {
  return root->tenantDatabase()
      ->getMatchingTenants(colonyTags)
      .transformed([](TenantPtr const& tenant) { return tenant->config; });
}

Json LuaBindings::RootCallbacks::liquidStatusEffects(Root* root, LiquidId arg1) {
  if (auto liquidSettings = root->liquidsDatabase()->liquidSettings(arg1))
    return liquidSettings->statusEffects;
  return Json();
}

String LuaBindings::RootCallbacks::generateName(Root* root, String const& rulesAsset, Maybe<uint64_t> seed) {
  return root->nameGenerator()->generateName(rulesAsset, seed.value(Random::randu64()));
}

Json LuaBindings::RootCallbacks::questConfig(Root* root, String const& templateId) {
  return root->questTemplateDatabase()->questTemplate(templateId)->config;
}

JsonArray LuaBindings::RootCallbacks::npcPortrait(Root* root,
    String const& portraitMode,
    String const& species,
    String const& typeName,
    float level,
    Maybe<uint64_t> seed,
    Maybe<JsonObject> const& parameters) {
  auto npcDatabase = root->npcDatabase();
  auto npcVariant = npcDatabase->generateNpcVariant(species, typeName, level, seed.value(Random::randu64()), parameters.value(JsonObject{}));

  auto drawables = npcDatabase->npcPortrait(npcVariant, PortraitModeNames.getLeft(portraitMode));
  return drawables.transformed(mem_fn(&Drawable::toJson));
}

Json LuaBindings::RootCallbacks::npcVariant(Root* root, String const& species, String const& typeName, float level, Maybe<uint64_t> seed, Maybe<JsonObject> const& parameters) {
  auto npcDatabase = root->npcDatabase();
  auto npcVariant = npcDatabase->generateNpcVariant(
      species, typeName, level, seed.value(Random::randu64()), parameters.value(JsonObject{}));
  return npcDatabase->writeNpcVariantToJson(npcVariant);
}

JsonArray LuaBindings::RootCallbacks::monsterPortrait(Root* root, String const& typeName, Maybe<JsonObject> const& parameters) {
  auto monsterDatabase = root->monsterDatabase();
  auto seed = 0; // use a static seed to utilize caching
  auto monsterVariant = monsterDatabase->monsterVariant(typeName, seed, parameters.value(JsonObject{}));
  auto drawables = monsterDatabase->monsterPortrait(monsterVariant);
  return drawables.transformed(mem_fn(&Drawable::toJson));
}

bool LuaBindings::RootCallbacks::isTreasurePool(Root* root, String const& pool) {
  return root->treasureDatabase()->isTreasurePool(pool);
}

JsonArray LuaBindings::RootCallbacks::createTreasure(
    Root* root, String const& pool, float level, Maybe<uint64_t> seed) {
  auto treasure = root->treasureDatabase()->createTreasure(pool, level, seed.value(Random::randu64()));
  return treasure.transformed([](ItemPtr const& item) { return item->descriptor().toJson(); });
}

Maybe<String> LuaBindings::RootCallbacks::materialMiningSound(
    Root* root, String const& materialName, Maybe<String> const& modName) {
  auto materialDatabase = root->materialDatabase();
  auto materialId = materialDatabase->materialId(materialName);
  auto modId = modName.apply(bind(&MaterialDatabase::modId, materialDatabase, _1)).value(NoModId);
  auto sound = materialDatabase->miningSound(materialId, modId);
  if (sound.empty())
    return {};
  return sound;
}

Maybe<String> LuaBindings::RootCallbacks::materialFootstepSound(
    Root* root, String const& materialName, Maybe<String> const& modName) {
  auto materialDatabase = root->materialDatabase();
  auto materialId = materialDatabase->materialId(materialName);
  auto modId = modName.apply(bind(&MaterialDatabase::modId, materialDatabase, _1)).value(NoModId);
  auto sound = materialDatabase->footstepSound(materialId, modId);
  if (sound.empty())
    return {};
  return sound;
}

}
