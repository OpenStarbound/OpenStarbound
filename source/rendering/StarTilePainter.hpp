#ifndef STAR_NEW_TILE_PAINTER_HPP
#define STAR_NEW_TILE_PAINTER_HPP

#include "StarTtlCache.hpp"
#include "StarWorldRenderData.hpp"
#include "StarMaterialRenderProfile.hpp"
#include "StarRenderer.hpp"
#include "StarWorldCamera.hpp"

namespace Star {

STAR_CLASS(Assets);
STAR_CLASS(MaterialDatabase);
STAR_CLASS(TilePainter);

class TilePainter {
public:
  // The rendered tiles are split and cached in chunks of RenderChunkSize x
  // RenderChunkSize.  This means that, around the border, there may be as many
  // as RenderChunkSize - 1 tiles rendered outside of the viewing area from
  // chunk alignment.  In addition to this, there is also a region around each
  // tile that is used for neighbor based rendering rules which has a max of
  // MaterialRenderProfileMaxNeighborDistance.  If the given tile data does not
  // extend RenderChunkSize + MaterialRenderProfileMaxNeighborDistance - 1
  // around the viewing area, then border chunks can continuously change hash,
  // and will be recomputed too often.
  static unsigned const RenderChunkSize = 16;
  static unsigned const BorderTileSize = RenderChunkSize + MaterialRenderProfileMaxNeighborDistance - 1;

  TilePainter(RendererPtr renderer);

  // Adjusts lighting levels for liquids.
  void adjustLighting(WorldRenderData& renderData) const;

  // Sets up chunk data for every chunk that intersects the rendering region
  // and prepares it for rendering.  Do not call cleanup in between calling
  // setup and each render method.
  void setup(WorldCamera const& camera, WorldRenderData& renderData);

  void renderBackground(WorldCamera const& camera);
  void renderMidground(WorldCamera const& camera);
  void renderLiquid(WorldCamera const& camera);
  void renderForeground(WorldCamera const& camera);

  // Clears any render data, as well as cleaning up old cached textures and
  // chunks.
  void cleanup();

private:
  typedef uint64_t QuadZLevel;
  typedef uint64_t ChunkHash;

  enum class TerrainLayer { Background, Midground, Foreground };

  struct LiquidInfo {
    TexturePtr texture;
    Vec4B color;
    Vec3F bottomLightMix;
    float textureMovementFactor;
  };

  typedef HashMap<TerrainLayer, HashMap<QuadZLevel, RenderBufferPtr>> TerrainChunk;
  typedef HashMap<LiquidId, RenderBufferPtr> LiquidChunk;

  typedef size_t MaterialRenderPieceIndex;
  typedef tuple<MaterialId, MaterialRenderPieceIndex, MaterialHue, bool> MaterialPieceTextureKey;
  typedef String AssetTextureKey;
  typedef Variant<MaterialPieceTextureKey, AssetTextureKey> TextureKey;

  typedef List<pair<MaterialRenderPieceConstPtr, Vec2F>> MaterialPieceResultList;

  struct TextureKeyHash {
    size_t operator()(TextureKey const& key) const;
  };

  // chunkIndex here is the index of the render chunk such that chunkIndex *
  // RenderChunkSize results in the coordinate of the lower left most tile in
  // the render chunk.

  static ChunkHash terrainChunkHash(WorldRenderData& renderData, Vec2I chunkIndex);
  static ChunkHash liquidChunkHash(WorldRenderData& renderData, Vec2I chunkIndex);

  static QuadZLevel materialZLevel(uint32_t zLevel, MaterialId material, MaterialHue hue, MaterialColorVariant colorVariant);
  static QuadZLevel modZLevel(uint32_t zLevel, ModId mod, MaterialHue hue, MaterialColorVariant colorVariant);
  static QuadZLevel damageZLevel();

  static RenderTile const& getRenderTile(WorldRenderData const& renderData, Vec2I const& worldPos);

  template <typename Function>
  static void forEachRenderTile(WorldRenderData const& renderData, RectI const& worldCoordRange, Function&& function);

  void renderTerrainChunks(WorldCamera const& camera, TerrainLayer terrainLayer);

  shared_ptr<TerrainChunk const> getTerrainChunk(WorldRenderData& renderData, Vec2I chunkIndex);
  shared_ptr<LiquidChunk const> getLiquidChunk(WorldRenderData& renderData, Vec2I chunkIndex);

  bool produceTerrainPrimitives(HashMap<QuadZLevel, List<RenderPrimitive>>& primitives,
      TerrainLayer terrainLayer, Vec2I const& pos, WorldRenderData const& renderData);
  void produceLiquidPrimitives(HashMap<LiquidId, List<RenderPrimitive>>& primitives, Vec2I const& pos, WorldRenderData const& renderData);

  bool determineMatchingPieces(MaterialPieceResultList& resultList, bool* occlude, MaterialDatabaseConstPtr const& materialDb, MaterialRenderMatchList const& matchList,
      WorldRenderData const& renderData, Vec2I const& basePos, TileLayer layer, bool isMod);

  float liquidDrawLevel(float liquidLevel) const;

  List<LiquidInfo> m_liquids;

  Vec4B m_backgroundLayerColor;
  Vec4B m_foregroundLayerColor;
  Vec2F m_liquidDrawLevels;

  RendererPtr m_renderer;
  TextureGroupPtr m_textureGroup;

  HashTtlCache<TextureKey, TexturePtr, TextureKeyHash> m_textureCache;
  HashTtlCache<pair<Vec2I, ChunkHash>, shared_ptr<TerrainChunk const>> m_terrainChunkCache;
  HashTtlCache<pair<Vec2I, ChunkHash>, shared_ptr<LiquidChunk const>> m_liquidChunkCache;

  List<shared_ptr<TerrainChunk const>> m_pendingTerrainChunks;
  List<shared_ptr<LiquidChunk const>> m_pendingLiquidChunks;

  Maybe<Vec2F> m_lastCameraCenter;
  Vec2F m_cameraPan;
};

template <typename Function>
void TilePainter::forEachRenderTile(WorldRenderData const& renderData, RectI const& worldCoordRange, Function&& function) {
  RectI indexRect = RectI::withSize(renderData.geometry.diff(worldCoordRange.min(), renderData.tileMinPosition), worldCoordRange.size());
  indexRect.limit(RectI::withSize(Vec2I(0, 0), Vec2I(renderData.tiles.size())));

  if (!indexRect.isEmpty()) {
    renderData.tiles.forEach(Array2S(indexRect.min()), Array2S(indexRect.size()), [&](Array2S const& index, RenderTile const& tile) {
        return function(worldCoordRange.min() + (Vec2I(index) - indexRect.min()), tile);
      });
  }
}

}

#endif
