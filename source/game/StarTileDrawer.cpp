#include "StarTileDrawer.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarXXHash.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

RenderTile TileDrawer::DefaultRenderTile{
    NullMaterialId,
    NoModId,
    NullMaterialId,
    NoModId,
    0,
    0,
    DefaultMaterialColorVariant,
    TileDamageType::Protected,
    0,
    0,
    0,
    DefaultMaterialColorVariant,
    TileDamageType::Protected,
    0,
    EmptyLiquidId,
    0
};

TileDrawer* TileDrawer::s_singleton;

TileDrawer* TileDrawer::singletonPtr() {
  return s_singleton;
}

TileDrawer& TileDrawer::singleton() {
  if (!s_singleton)
    throw StarException("TileDrawer::singleton() called with no TileDrawer instance available");
  else
    return *s_singleton;
}

TileDrawer::TileDrawer() {
  auto assets = Root::singleton().assets();

  m_backgroundLayerColor = jsonToColor(assets->json("/rendering.config:backgroundLayerColor")).toRgba();
  m_foregroundLayerColor = jsonToColor(assets->json("/rendering.config:foregroundLayerColor")).toRgba();
  m_liquidDrawLevels = jsonToVec2F(assets->json("/rendering.config:liquidDrawLevels"));
  s_singleton = this;
}

TileDrawer::~TileDrawer() {
  if (s_singleton == this)
    s_singleton = nullptr;
}

bool TileDrawer::produceTerrainDrawables(Drawables& drawables,
  TerrainLayer terrainLayer, Vec2I const& pos, WorldRenderData const& renderData, float scale, Vec2I offset, Maybe<TerrainLayer> variantLayer) {
  auto& root = Root::singleton();
  auto assets = Root::singleton().assets();
  auto materialDatabase = root.materialDatabase();

  RenderTile const& tile = getRenderTile(renderData, pos);

  MaterialId material = EmptyMaterialId;
  MaterialHue materialHue = 0;
  MaterialColorVariant colorVariant = 0;
  ModId mod = NoModId;
  MaterialHue modHue = 0;
  float damageLevel = 0.0f;
  TileDamageType damageType = TileDamageType::Protected;
  Color color;

  bool occlude = false;

  if (terrainLayer == TerrainLayer::Background) {
    material = tile.background;
    materialHue = tile.backgroundHueShift;
    colorVariant = tile.backgroundColorVariant;
    mod = tile.backgroundMod;
    modHue = tile.backgroundModHueShift;
    damageLevel = byteToFloat(tile.backgroundDamageLevel);
    damageType = tile.backgroundDamageType;
    color = Color::rgba(m_backgroundLayerColor);
  } else {
    material = tile.foreground;
    materialHue = tile.foregroundHueShift;
    colorVariant = tile.foregroundColorVariant;
    mod = tile.foregroundMod;
    modHue = tile.foregroundModHueShift;
    damageLevel = byteToFloat(tile.foregroundDamageLevel);
    damageType = tile.foregroundDamageType;
    color = Color::rgba(m_foregroundLayerColor);
  }

  // render non-block colliding things in the midground
  bool isBlock = BlockCollisionSet.contains(materialDatabase->materialCollisionKind(material));
  if ((isBlock && terrainLayer == TerrainLayer::Midground) || (!isBlock && terrainLayer == TerrainLayer::Foreground))
    return false;

  auto getPieceImage = [](MaterialRenderPieceConstPtr const& piece, Box<float, 2> const& box, MaterialHue hue, Directives const* directives) -> AssetPath {
    AssetPath image = hue == 0
      ? strf("{}?crop={};{};{};{}", piece->texture, box.xMin(), box.yMin(), box.xMax(), box.yMax())
      : strf("{}?crop={};{};{};{}?hueshift={}", piece->texture, box.xMin(), box.yMin(), box.xMax(), box.yMax(), materialHueToDegrees(hue));
    if (directives)
      image.directives += *directives;

    return image;
  };

  auto materialRenderProfile = materialDatabase->materialRenderProfile(material);
  auto modRenderProfile = materialDatabase->modRenderProfile(mod);

  if (materialRenderProfile) {
    occlude = materialRenderProfile->occludesBehind;
    auto materialColorVariant = materialRenderProfile->colorVariants > 0 ? colorVariant % materialRenderProfile->colorVariants : 0;
    uint32_t variance = staticRandomU32(renderData.geometry.xwrap(pos[0]) + offset[0], pos[1] + offset[1], (int)variantLayer.value(terrainLayer), "main");
    auto& drawList = drawables[materialZLevel(materialRenderProfile->zLevel, material, materialHue, materialColorVariant)];

    MaterialPieceResultList pieces;
    determineMatchingPieces(pieces, &occlude, materialDatabase, materialRenderProfile->mainMatchList, renderData, pos,
        terrainLayer == TerrainLayer::Background ? TileLayer::Background : TileLayer::Foreground, false);
    Directives const* directives = materialRenderProfile->colorDirectives.empty()
      ? nullptr
      : &materialRenderProfile->colorDirectives.wrap(materialColorVariant);
    for (auto const& piecePair : pieces) {
      auto variant = piecePair.first->variants.ptr(materialColorVariant);
      if (!variant) variant = piecePair.first->variants.ptr(0);
      if (!variant) continue;
      auto& textureCoords = variant->wrap(variance);
      auto image = getPieceImage(piecePair.first, textureCoords, materialHue, directives);
      drawList.emplace_back(Drawable::makeImage(image, scale, false, piecePair.second * scale + Vec2F(pos), color));
    }
  }

  if (modRenderProfile) {
    auto modColorVariant = modRenderProfile->colorVariants > 0 ? colorVariant % modRenderProfile->colorVariants : 0;
    uint32_t variance = staticRandomU32(renderData.geometry.xwrap(pos[0]), pos[1], (int)variantLayer.value(terrainLayer), "mod");
    auto& drawList = drawables[modZLevel(modRenderProfile->zLevel, mod, modHue, modColorVariant)];

    MaterialPieceResultList pieces;
    determineMatchingPieces(pieces, &occlude, materialDatabase, modRenderProfile->mainMatchList, renderData, pos,
        terrainLayer == TerrainLayer::Background ? TileLayer::Background : TileLayer::Foreground, true);
    Directives const* directives = modRenderProfile->colorDirectives.empty()
      ? nullptr
      : &modRenderProfile->colorDirectives.wrap(modColorVariant);
    for (auto const& piecePair : pieces) {
      auto variant = piecePair.first->variants.ptr(modColorVariant);
      if (!variant) variant = piecePair.first->variants.ptr(0);
      if (!variant) continue;
      auto& textureCoords = variant->wrap(variance);
      auto image = getPieceImage(piecePair.first, textureCoords, modHue, directives);
      drawList.emplace_back(Drawable::makeImage(image, scale, false, piecePair.second * scale + Vec2F(pos), color));
    }
  }

  if (materialRenderProfile && damageLevel > 0 && isBlock) {
    auto& drawList = drawables[damageZLevel()];
    auto const& crackingImage = materialRenderProfile->damageImage(damageLevel, damageType);

    drawList.emplace_back(Drawable::makeImage(crackingImage.first, scale, false, crackingImage.second * scale + Vec2F(pos), color));
  }

  return occlude;
}

WorldRenderData& TileDrawer::renderData() {
  return m_tempRenderData;
}

MutexLocker TileDrawer::lockRenderData() {
  return MutexLocker(m_tempRenderDataMutex);
}

RenderTile const& TileDrawer::getRenderTile(WorldRenderData const& renderData, Vec2I const& worldPos) {
  Vec2I arrayPos = renderData.geometry.diff(worldPos, renderData.tileMinPosition);

  Vec2I size = Vec2I(renderData.tiles.size());
  if (arrayPos[0] >= 0 && arrayPos[1] >= 0 && arrayPos[0] < size[0] && arrayPos[1] < size[1])
    return renderData.tiles(Vec2S(arrayPos));

  return DefaultRenderTile;
}

TileDrawer::QuadZLevel TileDrawer::materialZLevel(uint32_t zLevel, MaterialId material, MaterialHue hue, MaterialColorVariant colorVariant) {
  QuadZLevel quadZLevel = 0;
  quadZLevel |= (uint64_t)colorVariant;
  quadZLevel |= (uint64_t)hue << 8;
  quadZLevel |= (uint64_t)material << 16;
  quadZLevel |= (uint64_t)zLevel << 32;
  return quadZLevel;
}

TileDrawer::QuadZLevel TileDrawer::modZLevel(uint32_t zLevel, ModId mod, MaterialHue hue, MaterialColorVariant colorVariant) {
  QuadZLevel quadZLevel = 0;
  quadZLevel |= (uint64_t)colorVariant;
  quadZLevel |= (uint64_t)hue << 8;
  quadZLevel |= (uint64_t)mod << 16;
  quadZLevel |= (uint64_t)zLevel << 32;
  quadZLevel |= (uint64_t)1 << 63;
  return quadZLevel;
}

TileDrawer::QuadZLevel TileDrawer::damageZLevel() {
  return (uint64_t)(-1);
}

bool TileDrawer::determineMatchingPieces(MaterialPieceResultList& resultList, bool* occlude, MaterialDatabaseConstPtr const& materialDb, MaterialRenderMatchList const& matchList,
    WorldRenderData const& renderData, Vec2I const& basePos, TileLayer layer, bool isMod) {
  RenderTile const& tile = getRenderTile(renderData, basePos);

  auto matchSetMatches = [&](MaterialRenderMatchConstPtr const& match) -> bool {
    if (match->requiredLayer && *match->requiredLayer != layer)
      return false;

    if (match->matchPoints.empty())
      return true;

    bool matchValid = match->matchJoin == MaterialJoinType::All;
    for (auto const& matchPoint : match->matchPoints) {
      auto const& neighborTile = getRenderTile(renderData, basePos + matchPoint.position);

      bool neighborShadowing = false;
      if (layer == TileLayer::Background) {
        if (auto profile = materialDb->materialRenderProfile(neighborTile.foreground))
          neighborShadowing = !profile->foregroundLightTransparent;
      }

      MaterialHue baseHue = layer == TileLayer::Foreground ? tile.foregroundHueShift : tile.backgroundHueShift;
      MaterialHue neighborHue = layer == TileLayer::Foreground ? neighborTile.foregroundHueShift : neighborTile.backgroundHueShift;
      MaterialHue baseModHue = layer == TileLayer::Foreground ? tile.foregroundModHueShift : tile.backgroundModHueShift;
      MaterialHue neighborModHue = layer == TileLayer::Foreground ? neighborTile.foregroundModHueShift : neighborTile.backgroundModHueShift;
      MaterialId baseMaterial = layer == TileLayer::Foreground ? tile.foreground : tile.background;
      MaterialId neighborMaterial = layer == TileLayer::Foreground ? neighborTile.foreground : neighborTile.background;
      ModId baseMod = layer == TileLayer::Foreground ? tile.foregroundMod : tile.backgroundMod;
      ModId neighborMod = layer == TileLayer::Foreground ? neighborTile.foregroundMod : neighborTile.backgroundMod;

      bool rulesValid = matchPoint.rule->join == MaterialJoinType::All;
      for (auto const& ruleEntry : matchPoint.rule->entries) {
        bool valid = true;
        if (isMod) {
          if (ruleEntry.rule.is<MaterialRule::RuleEmpty>()) {
            valid = neighborMod == NoModId;
          } else if (ruleEntry.rule.is<MaterialRule::RuleConnects>()) {
            valid = isConnectableMaterial(neighborMaterial);
          } else if (ruleEntry.rule.is<MaterialRule::RuleShadows>()) {
            valid = neighborShadowing;
          } else if (auto equalsSelf = ruleEntry.rule.ptr<MaterialRule::RuleEqualsSelf>()) {
            valid = neighborMod == baseMod;
            if (equalsSelf->matchHue)
              valid = valid && baseModHue == neighborModHue;
          } else if (auto equalsId = ruleEntry.rule.ptr<MaterialRule::RuleEqualsId>()) {
            valid = neighborMod == equalsId->id;
          } else if (auto propertyEquals = ruleEntry.rule.ptr<MaterialRule::RulePropertyEquals>()) {
            if (auto profile = materialDb->modRenderProfile(neighborMod))
              valid = profile->ruleProperties.get(propertyEquals->propertyName, Json()) == propertyEquals->compare;
            else
              valid = false;
          }
        } else {
          if (ruleEntry.rule.is<MaterialRule::RuleEmpty>()) {
            valid = neighborMaterial == EmptyMaterialId;
          } else if (ruleEntry.rule.is<MaterialRule::RuleConnects>()) {
            valid = isConnectableMaterial(neighborMaterial);
          } else if (ruleEntry.rule.is<MaterialRule::RuleShadows>()) {
            valid = neighborShadowing;
          } else if (auto equalsSelf = ruleEntry.rule.ptr<MaterialRule::RuleEqualsSelf>()) {
            valid = neighborMaterial == baseMaterial;
            if (equalsSelf->matchHue)
              valid = valid && baseHue == neighborHue;
          } else if (auto equalsId = ruleEntry.rule.ptr<MaterialRule::RuleEqualsId>()) {
            valid = neighborMaterial == equalsId->id;
          } else if (auto propertyEquals = ruleEntry.rule.ptr<MaterialRule::RulePropertyEquals>()) {
            if (auto profile = materialDb->materialRenderProfile(neighborMaterial))
              valid = profile->ruleProperties.get(propertyEquals->propertyName) == propertyEquals->compare;
            else
              valid = false;
          }
        }
        if (ruleEntry.inverse)
          valid = !valid;

        if (matchPoint.rule->join == MaterialJoinType::All) {
          rulesValid = valid && rulesValid;
          if (!rulesValid)
            break;
        } else {
          rulesValid = valid || rulesValid;
        }
      }

      if (match->matchJoin == MaterialJoinType::All) {
        matchValid = matchValid && rulesValid;
        if (!matchValid)
          return matchValid;
      } else {
        matchValid = matchValid || rulesValid;
      }
    }
    return matchValid;
  };

  bool subMatchResult = false;
  for (auto const& match : matchList) {
    if (matchSetMatches(match)) {
      if (match->occlude)
        *occlude = match->occlude.get();

      subMatchResult = true;

      for (auto const& piecePair : match->resultingPieces)
        resultList.append({piecePair.first, piecePair.second});

      if (determineMatchingPieces(resultList, occlude, materialDb, match->subMatches, renderData, basePos, layer, isMod) && match->haltOnSubMatch)
        break;

      if (match->haltOnMatch)
        break;
    }
  }

  return subMatchResult;
}

}