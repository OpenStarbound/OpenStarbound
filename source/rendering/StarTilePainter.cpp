#include "StarTilePainter.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarXXHash.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarTileDrawer.hpp"

namespace Star {

TilePainter::TilePainter(RendererPtr renderer) : TileDrawer() {
  m_renderer = move(renderer);
  m_textureGroup = m_renderer->createTextureGroup(TextureGroupSize::Large);

  auto& root = Root::singleton();
  auto assets = root.assets();

  m_terrainChunkCache.setTimeToLive(assets->json("/rendering.config:chunkCacheTimeout").toInt());
  m_terrainChunkCache.setTimeSmear(m_terrainChunkCache.timeToLive() / 4);

  m_liquidChunkCache.setTimeToLive(assets->json("/rendering.config:chunkCacheTimeout").toInt());
  m_liquidChunkCache.setTimeSmear(m_liquidChunkCache.timeToLive() / 4);

  m_textureCache.setTimeToLive(assets->json("/rendering.config:textureTimeout").toInt());

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
  auto cameraCenter = camera.centerWorldPosition();
  if (m_lastCameraCenter)
    m_cameraPan = renderData.geometry.diff(cameraCenter, *m_lastCameraCenter);
  m_lastCameraCenter = cameraCenter;

  //Kae: Padded by one to fix culling issues with certain tile pieces at chunk borders, such as grass.
  RectI chunkRange = RectI::integral(RectF(camera.worldTileRect().padded(1)).scaled(1.0f / RenderChunkSize));

  size_t chunks = chunkRange.volume();
  m_pendingTerrainChunks.resize(chunks);
  m_pendingLiquidChunks.resize(chunks);

  size_t i = 0;
  for (int x = chunkRange.xMin(); x < chunkRange.xMax(); ++x) {
    for (int y = chunkRange.yMin(); y < chunkRange.yMax(); ++y) {
      size_t index = i++;
      m_pendingTerrainChunks[index] = getTerrainChunk(renderData, {x, y});
      m_pendingLiquidChunks [index] =  getLiquidChunk(renderData, {x, y});
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
  //XXHash3 hasher;
  static ByteArray buffer;
  buffer.clear();
  RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize)).padded(MaterialRenderProfileMaxNeighborDistance);
  forEachRenderTile(renderData, tileRange, [&](Vec2I const&, RenderTile const& renderTile) {
    //renderTile.hashPushTerrain(hasher);
    buffer.append((char*)&renderTile, offsetof(RenderTile, liquidId));
  });

  //return hasher.digest();
  return XXH3_64bits(buffer.ptr(), buffer.size());
}

TilePainter::ChunkHash TilePainter::liquidChunkHash(WorldRenderData& renderData, Vec2I chunkIndex) {
  ///XXHash3 hasher;
  RectI tileRange = RectI::withSize(chunkIndex * RenderChunkSize, Vec2I::filled(RenderChunkSize)).padded(MaterialRenderProfileMaxNeighborDistance);
  static ByteArray buffer;
  buffer.clear();

  forEachRenderTile(renderData, tileRange, [&](Vec2I const&, RenderTile const& renderTile) {
    //renderTile.hashPushLiquid(hasher);
    buffer.append((char*)&renderTile.liquidId, sizeof(LiquidId) + sizeof(LiquidLevel));
  });

  //return hasher.digest();
  return XXH3_64bits(buffer.ptr(), buffer.size());
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
          rb->set(zLevelPair.second);
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
        rb->set(p.second);
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
  MaterialColorVariant materialColorVariant = 0;
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
  if (terrainLayer == (isBlock ? TerrainLayer::Midground : TerrainLayer::Foreground))
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
      auto variant = piecePair.first->variants.ptr(materialColorVariant);
      if (!variant) continue;
      RectF textureCoords = variant->wrap(variance);
      RectF worldCoords = RectF::withSize(piecePair.second / TilePixels + Vec2F(pos), textureCoords.size() / TilePixels);
      quadList.emplace_back(std::in_place_type_t<RenderQuad>(), move(texture),
          worldCoords  .min(),
          textureCoords.min(),
          Vec2F(  worldCoords.xMax(),   worldCoords.yMin()),
          Vec2F(textureCoords.xMax(), textureCoords.yMin()),
          worldCoords  .max(),
          textureCoords.max(),
          Vec2F(  worldCoords.xMin(),   worldCoords.yMax()),
          Vec2F(textureCoords.xMin(), textureCoords.yMax()),
          color, 1.0f);
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
      auto variant = piecePair.first->variants.ptr(modColorVariant);
      if (!variant) continue;
      auto& textureCoords = variant->wrap(variance);
      RectF worldCoords = RectF::withSize(piecePair.second / TilePixels + Vec2F(pos), textureCoords.size() / TilePixels);
      quadList.emplace_back(std::in_place_type_t<RenderQuad>(), move(texture),
          worldCoords.min(), textureCoords.min(),
          Vec2F(worldCoords.xMax(), worldCoords.yMin()), Vec2F(textureCoords.xMax(), textureCoords.yMin()),
          worldCoords.max(), textureCoords.max(),
          Vec2F(worldCoords.xMin(), worldCoords.yMax()), Vec2F(textureCoords.xMin(), textureCoords.yMax()),
        color, 1.0f);
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

    quadList.emplace_back(std::in_place_type_t<RenderQuad>(), move(texture),
        worldCoords.min(), textureCoords.min(),
        Vec2F(worldCoords.xMax(), worldCoords.yMin()), Vec2F(textureCoords.xMax(), textureCoords.yMin()),
        worldCoords.max(), textureCoords.max(),
        Vec2F(worldCoords.xMin(), worldCoords.yMax()), Vec2F(textureCoords.xMin(), textureCoords.yMax()),
      color, 1.0f);
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
  primitives[tile.liquidId].emplace_back(std::in_place_type_t<RenderQuad>(), liquid.texture,
      worldRect.min(), texRect.min(),
      Vec2F(worldRect.xMax(), worldRect.yMin()), Vec2F(texRect.xMax(), texRect.yMin()),
      worldRect.max(), texRect.max(),
      Vec2F(worldRect.xMin(), worldRect.yMax()), Vec2F(texRect.xMin(), texRect.yMax()),
    liquid.color, 1.0f);
}

float TilePainter::liquidDrawLevel(float liquidLevel) const {
  return clamp((liquidLevel - m_liquidDrawLevels[0]) / (m_liquidDrawLevels[1] - m_liquidDrawLevels[0]), 0.0f, 1.0f);
}

}
