#pragma once

#include "StarRect.hpp"
#include "StarGameTypes.hpp"
#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(Image);

namespace LuaBindings {
  LuaCallbacks makeRootCallbacks();

  namespace RootCallbacks {
    String assetData(Root* root, String const& path);
    Image assetImage(Root* root, String const& path);
    Json assetFrames(Root* root, String const& path);
    Json assetJson(Root* root, String const& path);
    Json makeCurrentVersionedJson(Root* root, String const& identifier, Json const& content);
    Json loadVersionedJson(Root* root, Json const& versionedJson, String const& expectedIdentifier);
    double evalFunction(Root* root, String const& arg1, double arg2);
    double evalFunction2(Root* root, String const& arg1, double arg2, double arg3);
    Vec2U imageSize(Root* root, String const& arg1);
    List<Vec2I> imageSpaces(Root* root, String const& arg1, Vec2F const& arg2, float arg3, bool arg4);
    RectU nonEmptyRegion(Root* root, String const& arg1);
    Json npcConfig(Root* root, String const& arg1);
    float projectileGravityMultiplier(Root* root, String const& arg1);
    Json projectileConfig(Root* root, String const& arg1);
    JsonArray recipesForItem(Root* root, String const& arg1);
    JsonArray allRecipes(Root* root, Maybe<StringSet> filter);
    String itemType(Root* root, String const& itemName);
    Json itemTags(Root* root, String const& itemName);
    bool itemHasTag(Root* root, String const& itemName, String const& itemTag);
    Json itemConfig(Root* root, Json const& descriptor, Maybe<float> const& level, Maybe<uint64_t> const& seed);
    Json createItem(Root* root, Json const& descriptor, Maybe<float> const& level, Maybe<uint64_t> const& seed);
    Json tenantConfig(Root* root, String const& tenantName);
    JsonArray getMatchingTenants(Root* root, StringMap<unsigned> const& colonyTags);
    Json liquidStatusEffects(Root* root, LiquidId arg1);
    String generateName(Root* root, String const& rulesAsset, Maybe<uint64_t> seed);
    Json questConfig(Root* root, String const& templateId);
    JsonArray npcPortrait(Root* root,
        String const& portraitMode,
        String const& species,
        String const& typeName,
        float level,
        Maybe<uint64_t> seed,
        Maybe<JsonObject> const& parameters);
    Json npcVariant(Root* root,
        String const& species,
        String const& typeName,
        float level,
        Maybe<uint64_t> seed,
        Maybe<JsonObject> const& parameters);
    JsonArray monsterPortrait(Root* root, String const& typeName, Maybe<JsonObject> const& parameters);
    bool isTreasurePool(Root* root, String const& pool);
    JsonArray createTreasure(Root* root, String const& pool, float level, Maybe<uint64_t> seed);
    Maybe<String> materialMiningSound(Root* root, String const& materialName, Maybe<String> const& modName);
    Maybe<String> materialFootstepSound(Root* root, String const& materialName, Maybe<String> const& modName);
  }
}
}
