#include "StarTilePainter.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarXXHash.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

TilePainter::TilePainter(RendererPtr renderer) {
  m_renderer = move(renderer);
  m_textureGroup = m_renderer->createTextureGroup(TextureGroupSize::Large);

  auto& root = Root::singleton();
  auto assets = root.assets();

  m_terrainChunkCache.setTimeToLive(assets->json("/rendering.config:chunkCacheTimeout").toInt());
  m_liquidChunkCache.setTimeToLive(assets->json("/rendering.config:chunkCacheTimeout").toInt());
  m_textureCache.setTimeToLive(assets->json("/rendering.config:textureTimeout").toInt());

  m_backgroundLayerColor = jsonToColor(assets->json("/rendering.config:backgroundLayerColor")).toRgba();
  m_foregroundLayerColor = jsonToColor(assets->json("/rendering.config:foregroundLayerColor")).toRgba();
  m_liquidDrawLevels = jsonToVec2F(assets->json("/rendering.config:liquidDrawLevels"));

  for (auto const& liquid : root.liquidsDatabase()->allLiquidSettings()) {
    m_liquids.set(liquid->id, LiquidInfo{
        m_renderer->createTexture(*assets->image(liquid->config.getString("texture")), TextureAddressing::Wrap),
        jsonToColor(liquid->config.get("color")).toRgba(),
        jsonToColor(liquid->config.get("bottomLightMix")).toRgbF(),
        liquid->config.getFloat("textureMovementFactor")
      });
  }
}

void TilePainter::adjustLighting(WorldRenderData& renderData) const {
  RectI lightRange = RectI::withSize(renderData.lightMinPosition, Vec2I(renderData.lightMap.size()));
  forEachRenderTile(renderData, lightRange, [&](Vec2I const& pos, RenderTile const& tile) {
      // Only adjust lighting for full tiles
      float drawLevel = liquidDrawLevel(byteToFloat(tile.liquidLevel));
      if (drawLevel == 0.0f)
        return;

      auto lightIndex = Vec2U(pos - renderData.lightMinPosition);
      auto lightValue = renderData.lightMap.get(lightIndex).vec3();

      auto const& liquid = m_liquids[tile.liquidId];
      Vec3F tileLight = Vec3F(lightValue);
      float darknessLevel = (1 - tileLight.sum() / (3.0f * 255.0f)) * drawLevel;
      lightValue = Vec3B(tileLight.piecewiseMultiply(Vec3F::filled(1 - darknessLevel) + liquid.bottomLightMix * darknessLevel));

      renderData.lightMap.set(lightIndex, lightValue);
    });
}

void TilePainter::setup(WorldCamera const& camera, WorldRenderData& renderData) {
  m_pendingTerrainChunks.clear();
  m_pendingLiquidChunks.clear();

  auto cameraCenter = camera.centerWorldPosition();
  if (m_lastCameraCenter)
    m_cameraPan = renderData.geometry.diff(cameraCenter, *m_lastCameraCenter);
  m_lastCameraCenter = cameraCenter;

  //Kae: Padded by one to fix culling issues with certain tile pieces at chunk borders, such as grass.
  RectI chunkRange = RectI::integral(RectF(camera.worldTileRect().padded(1)).scaled(1.0f / RenderChunkSize));

  for (int x = chunkRange.xMin(); x < chunkRange.xMax(); ++x) {
    for (int y = chunkRange.yMin(); y < chunkRange.yMax(); ++y) {
      m_pendingTerrainChunks.append(getTerrainChunk(renderData, {x, y}));
      m_pendingLiquidChunks.append(getLiquidChunk(renderData, {x, y}));
    }
  }
}

void TilePainter::renderBackground(WorldCamera const& camera) {
  renderTerrainChunks(camera, TerrainLayer::Background);
}

void TilePainter::renderMidground(WorldCamera const& camera) {
  renderTerrainChunks(camera, TerrainLayer::Midground);
}

void TilePainter::renderLiquid(WorldCamera const& camera) {
  Mat3F transformation = Mat3F::identity();
  transformation.translate(-Vec2F(camera.worldTileRect().min()));
  transformation.scale(TilePixels * camera.pixelRatio());
  transformation.translate(camera.tileMinScreen());

  for (auto const& chunk : m_pendingLiquidChunks) {
    for (auto const& p : *chunk)
      m_renderer->renderBuffer(p.second, transformation);
  }

  m_renderer->flush();
}

void TilePainter::renderForeground(WorldCamera const& camera) {
  renderTerrainChunks(camera, TerrainLayer::Foreground);
}

void TilePainter::cleanup() {
  m_pendingTerrainChunks.clear();
  m_pendingLiquidChunks.clear();

  m_textureCache.cleanup();
  m_terrainChunkCache.cleanup();
  m_liquidChunkCache.cleanup();
}

size_t TilePainter::TextureKeyHash::operator()(TextureKey const& key) const {
  if (key.is<MaterialPieceTextureKey>())
    return hashOf(key.typeIndex(), key.get<MaterialPieceTextureKey>());
  else
    return hashOf(key.typeIndex(), key.get<AssetTextureKey>());
}

TilePainter::ChunkHash TilePainter::terrainChunkHash(WorldRenderData& renderData, Vec2I chunkIndex) {
  XXHash64 hasher;
  RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize)).padded(MaterialRenderProfileMaxNeighborDistance);

  forEachRenderTile(renderData, tileRange, [&](Vec2I const&, RenderTile const& renderTile) {
      renderTile.hashPushTerrain(hasher);
    });

  return hasher.digest();
}

TilePainter::ChunkHash TilePainter::liquidChunkHash(WorldRenderData& renderData, Vec2I chunkIndex) {
  XXHash64 hasher;
  RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize)).padded(MaterialRenderProfileMaxNeighborDistance);

  forEachRenderTile(renderData, tileRange, [&](Vec2I const&, RenderTile const& renderTile) {
      renderTile.hashPushLiquid(hasher);
    });

  return hasher.digest();
}

TilePainter::QuadZLevel TilePainter::materialZLevel(uint32_t zLevel, MaterialId material, MaterialHue hue, MaterialColorVariant colorVariant) {
  QuadZLevel quadZLevel = 0;
  quadZLevel |= (uint64_t)colorVariant;
  quadZLevel |= (uint64_t)hue << 8;
  quadZLevel |= (uint64_t)material << 16;
  quadZLevel |= (uint64_t)zLevel << 32;
  return quadZLevel;
}

TilePainter::QuadZLevel TilePainter::modZLevel(uint32_t zLevel, ModId mod, MaterialHue hue, MaterialColorVariant colorVariant) {
  QuadZLevel quadZLevel = 0;
  quadZLevel |= (uint64_t)colorVariant;
  quadZLevel |= (uint64_t)hue << 8;
  quadZLevel |= (uint64_t)mod << 16;
  quadZLevel |= (uint64_t)zLevel << 32;
  quadZLevel |= (uint64_t)1 << 63;
  return quadZLevel;
}

TilePainter::QuadZLevel TilePainter::damageZLevel() {
  return (uint64_t)(-1);
}

RenderTile const& TilePainter::getRenderTile(WorldRenderData const& renderData, Vec2I const& worldPos) {
  Vec2I arrayPos = renderData.geometry.diff(worldPos, renderData.tileMinPosition);

  Vec2I size = Vec2I(renderData.tiles.size());
  if (arrayPos[0] >= 0 && arrayPos[1] >= 0 && arrayPos[0] < size[0] && arrayPos[1] < size[1])
    return renderData.tiles(Vec2S(arrayPos));

  static RenderTile defaultRenderTile = {
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
  return defaultRenderTile;
}

void TilePainter::renderTerrainChunks(WorldCamera const& camera, TerrainLayer terrainLayer) {
  Map<QuadZLevel, List<RenderBufferPtr>> zOrderBuffers;
  for (auto const& chunk : m_pendingTerrainChunks) {
    for (auto const& pair : chunk->value(terrainLayer))
      zOrderBuffers[pair.first].append(pair.second);
  }

  Mat3F transformation = Mat3F::identity();
  transformation.translate(-Vec2F(camera.worldTileRect().min()));
  transformation.scale(TilePixels * camera.pixelRatio());
  transformation.translate(camera.tileMinScreen());

  for (auto const& pair : zOrderBuffers) {
    for (auto const& buffer : pair.second)
      m_renderer->renderBuffer(buffer, transformation);
  }

  m_renderer->flush();
}

shared_ptr<TilePainter::TerrainChunk const> TilePainter::getTerrainChunk(WorldRenderData& renderData, Vec2I chunkIndex) {
  pair<Vec2I, ChunkHash> chunkKey = {chunkIndex, terrainChunkHash(renderData, chunkIndex)};
  return m_terrainChunkCache.get(chunkKey, [&](auto const&) {
      HashMap<TerrainLayer, HashMap<QuadZLevel, List<RenderPrimitive>>> terrainPrimitives;

      RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize));
      for (int x = tileRange.xMin(); x < tileRange.xMax(); ++x) {
        for (int y = tileRange.yMin(); y < tileRange.yMax(); ++y) {
          bool occluded = this->produceTerrainPrimitives(terrainPrimitives[TerrainLayer::Foreground], TerrainLayer::Foreground, {x, y}, renderData);
          occluded = this->produceTerrainPrimitives(terrainPrimitives[TerrainLayer::Midground], TerrainLayer::Midground, {x, y}, renderData) || occluded;
          if (!occluded)
            this->produceTerrainPrimitives(terrainPrimitives[TerrainLayer::Background], TerrainLayer::Background, {x, y}, renderData);
        }
      }

      auto chunk = make_shared<TerrainChunk>();

      for (auto& layerPair : terrainPrimitives) {
        for (auto& zLevelPair : layerPair.second) {
          auto rb = m_renderer->createRenderBuffer();
          rb->set(move(zLevelPair.second));
          (*chunk)[layerPair.first][zLevelPair.first] = move(rb);
        }
      }

      return chunk;
    });
}

shared_ptr<TilePainter::LiquidChunk const> TilePainter::getLiquidChunk(WorldRenderData& renderData, Vec2I chunkIndex) {
  pair<Vec2I, ChunkHash> chunkKey = {chunkIndex, liquidChunkHash(renderData, chunkIndex)};
  return m_liquidChunkCache.get(chunkKey, [&](auto const&) {
      HashMap<LiquidId, List<RenderPrimitive>> liquidPrimitives;

      RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize));
      for (int x = tileRange.xMin(); x < tileRange.xMax(); ++x) {
        for (int y = tileRange.yMin(); y < tileRange.yMax(); ++y)
          this->produceLiquidPrimitives(liquidPrimitives, {x, y}, renderData);
      }

      auto chunk = make_shared<LiquidChunk>();

      for (auto& p : liquidPrimitives) {
        auto rb = m_renderer->createRenderBuffer();
        rb->set(move(p.second));
        chunk->set(p.first, move(rb));
      }

      return chunk;
    });
}

bool TilePainter::produceTerrainPrimitives(HashMap<QuadZLevel, List<RenderPrimitive>>& primitives,
    TerrainLayer terrainLayer, Vec2I const& pos, WorldRenderData const& renderData) {
  auto& root = Root::singleton();
  auto assets = Root::singleton().assets();
  auto materialDatabase = root.materialDatabase();

  RenderTile const& tile = getRenderTile(renderData, pos);

  MaterialId material = EmptyMaterialId;
  MaterialHue materialHue = 0;
  MaterialHue materialColorVariant = 0;
  ModId mod = NoModId;
  MaterialHue modHue = 0;
  float damageLevel = 0.0f;
  TileDamageType damageType = TileDamageType::Protected;
  Vec4B color;

  bool occlude = false;

  if (terrainLayer == TerrainLayer::Background) {
    material = tile.background;
    materialHue = tile.backgroundHueShift;
    materialColorVariant = tile.backgroundColorVariant;
    mod = tile.backgroundMod;
    modHue = tile.backgroundModHueShift;
    damageLevel = byteToFloat(tile.backgroundDamageLevel);
    damageType = tile.backgroundDamageType;
    color = m_backgroundLayerColor;
  } else {
    material = tile.foreground;
    materialHue = tile.foregroundHueShift;
    materialColorVariant = tile.foregroundColorVariant;
    mod = tile.foregroundMod;
    modHue = tile.foregroundModHueShift;
    damageLevel = byteToFloat(tile.foregroundDamageLevel);
    damageType = tile.foregroundDamageType;
    color = m_foregroundLayerColor;
  }

  // render non-block colliding things in the midground
  bool isBlock = BlockCollisionSet.contains(materialDatabase->materialCollisionKind(material));
  if ((isBlock && terrainLayer == TerrainLayer::Midground) || (!isBlock && terrainLayer == TerrainLayer::Foreground))
    return false;

  auto getPieceTexture = [this, assets](MaterialId material, MaterialRenderPieceConstPtr const& piece, MaterialHue hue, bool mod) {
    return m_textureCache.get(MaterialPieceTextureKey(material, piece->pieceId, hue, mod), [&](auto const&) {
        String texture;
        if (hue == 0)
          texture = piece->texture;
        else
          texture = strf("{}?hueshift={}", piece->texture, materialHueToDegrees(hue));

        return m_textureGroup->create(*assets->image(texture));
      });
  };

  auto materialRenderProfile = materialDatabase->materialRenderProfile(material);

  auto modRenderProfile = materialDatabase->modRenderProfile(mod);

  if (materialRenderProfile) {
    occlude = materialRenderProfile->occludesBehind;

    uint32_t variance = staticRandomU32(renderData.geometry.xwrap(pos[0]), pos[1], (int)terrainLayer, "main");
    auto& quadList = primitives[materialZLevel(materialRenderProfile->zLevel, material, materialHue, materialColorVariant)];

    MaterialPieceResultList pieces;
    determineMatchingPieces(pieces, &occlude, materialDatabase, materialRenderProfile->mainMatchList, renderData, pos,
        terrainLayer == TerrainLayer::Background ? TileLayer::Background : TileLayer::Foreground, false);
    for (auto const& piecePair : pieces) {
      TexturePtr texture = getPieceTexture(material, piecePair.first, materialHue, false);
      RectF textureCoords = piecePair.first->variants.get(materialColorVariant).wrap(variance);
      RectF worldCoords = RectF::withSize(piecePair.second / TilePixels + Vec2F(pos), textureCoords.size() / TilePixels);
      quadList.append(RenderQuad{
          move(texture),
          RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMin()), Vec2F(textureCoords.xMin(), textureCoords.yMin()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMin()), Vec2F(textureCoords.xMax(), textureCoords.yMin()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMax()), Vec2F(textureCoords.xMax(), textureCoords.yMax()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMax()), Vec2F(textureCoords.xMin(), textureCoords.yMax()), color, 1.0f}
        });
    }
  }

  if (modRenderProfile) {
    auto modColorVariant = modRenderProfile->multiColor ? materialColorVariant : 0;
    uint32_t variance = staticRandomU32(renderData.geometry.xwrap(pos[0]), pos[1], (int)terrainLayer, "mod");
    auto& quadList = primitives[modZLevel(modRenderProfile->zLevel, mod, modHue, modColorVariant)];

    MaterialPieceResultList pieces;
    determineMatchingPieces(pieces, &occlude, materialDatabase, modRenderProfile->mainMatchList, renderData, pos,
        terrainLayer == TerrainLayer::Background ? TileLayer::Background : TileLayer::Foreground, true);
    for (auto const& piecePair : pieces) {
      auto texture = getPieceTexture(mod, piecePair.first, modHue, true);
      auto textureCoords = piecePair.first->variants.get(modColorVariant).wrap(variance);
      RectF worldCoords = RectF::withSize(piecePair.second / TilePixels + Vec2F(pos), textureCoords.size() / TilePixels);
      quadList.append(RenderQuad{
          move(texture),
          RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMin()), Vec2F(textureCoords.xMin(), textureCoords.yMin()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMin()), Vec2F(textureCoords.xMax(), textureCoords.yMin()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMax()), Vec2F(textureCoords.xMax(), textureCoords.yMax()), color, 1.0f},
          RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMax()), Vec2F(textureCoords.xMin(), textureCoords.yMax()), color, 1.0f}
        });
    }
  }

  if (materialRenderProfile && damageLevel > 0 && isBlock) {
    auto& quadList = primitives[damageZLevel()];
    auto const& crackingImage = materialRenderProfile->damageImage(damageLevel, damageType);

    TexturePtr texture = m_textureCache.get(AssetTextureKey(crackingImage.first),
        [&](auto const&) { return m_textureGroup->create(*assets->image(crackingImage.first)); });

    Vec2F textureSize(texture->size());
    RectF textureCoords = RectF::withSize(Vec2F(), textureSize);
    RectF worldCoords = RectF::withSize(crackingImage.second / TilePixels + Vec2F(pos), textureCoords.size() / TilePixels);

    quadList.append(RenderQuad{
        move(texture),
        RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMin()), Vec2F(textureCoords.xMin(), textureCoords.yMin()), color, 1.0f},
        RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMin()), Vec2F(textureCoords.xMax(), textureCoords.yMin()), color, 1.0f},
        RenderVertex{Vec2F(worldCoords.xMax(), worldCoords.yMax()), Vec2F(textureCoords.xMax(), textureCoords.yMax()), color, 1.0f},
        RenderVertex{Vec2F(worldCoords.xMin(), worldCoords.yMax()), Vec2F(textureCoords.xMin(), textureCoords.yMax()), color, 1.0f}
      });
  }

  return occlude;
}

void TilePainter::produceLiquidPrimitives(HashMap<LiquidId, List<RenderPrimitive>>& primitives, Vec2I const& pos, WorldRenderData const& renderData) {
  RenderTile const& tile = getRenderTile(renderData, pos);

  float drawLevel = liquidDrawLevel(byteToFloat(tile.liquidLevel));
  if (drawLevel <= 0.0f)
    return;

  RenderTile const& tileBottom = getRenderTile(renderData, pos - Vec2I(0, 1));
  float bottomDrawLevel = liquidDrawLevel(byteToFloat(tileBottom.liquidLevel));

  RectF worldRect;
  if (tileBottom.foreground == EmptyMaterialId && bottomDrawLevel < 1.0f)
    worldRect = RectF::withSize(Vec2F(pos), Vec2F::filled(1.0f)).expanded(drawLevel);
  else
    worldRect = RectF::withSize(Vec2F(pos), Vec2F(1.0f, drawLevel));

  auto texRect = worldRect.scaled(TilePixels);

  auto const& liquid = m_liquids[tile.liquidId];
  primitives[tile.liquidId].append(RenderQuad{
      liquid.texture,
      RenderVertex{Vec2F(worldRect.xMin(), worldRect.yMin()), Vec2F(texRect.xMin(), texRect.yMin()), liquid.color, 1.0f},
      RenderVertex{Vec2F(worldRect.xMax(), worldRect.yMin()), Vec2F(texRect.xMax(), texRect.yMin()), liquid.color, 1.0f},
      RenderVertex{Vec2F(worldRect.xMax(), worldRect.yMax()), Vec2F(texRect.xMax(), texRect.yMax()), liquid.color, 1.0f},
      RenderVertex{Vec2F(worldRect.xMin(), worldRect.yMax()), Vec2F(texRect.xMin(), texRect.yMax()), liquid.color, 1.0f}
    });
}

bool TilePainter::determineMatchingPieces(MaterialPieceResultList& resultList, bool* occlude, MaterialDatabaseConstPtr const& materialDb, MaterialRenderMatchList const& matchList,
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

float TilePainter::liquidDrawLevel(float liquidLevel) const {
  return clamp((liquidLevel - m_liquidDrawLevels[0]) / (m_liquidDrawLevels[1] - m_liquidDrawLevels[0]), 0.0f, 1.0f);
}

}
