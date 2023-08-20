#ifndef STAR_NEW_TILE_PAINTER_HPP
#define STAR_NEW_TILE_PAINTER_HPP

#include "StarTtlCache.hpp"
#include "StarWorldRenderData.hpp"
#include "StarMaterialRenderProfile.hpp"
#include "StarRenderer.hpp"
#include "StarWorldCamera.hpp"
#include "StarTileDrawer.hpp"

namespace Star {

STAR_CLASS(Assets);
STAR_CLASS(MaterialDatabase);
STAR_CLASS(TilePainter);

class TilePainter : public TileDrawer {
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

  typedef tuple<MaterialId, MaterialRenderPieceIndex, MaterialHue, bool> MaterialPieceTextureKey;
  typedef String AssetTextureKey;
  typedef Variant<MaterialPieceTextureKey, AssetTextureKey> TextureKey;

  struct TextureKeyHash {
    size_t operator()(TextureKey const& key) const;
  };

  // chunkIndex here is the index of the render chunk such that chunkIndex *
  // RenderChunkSize results in the coordinate of the lower left most tile in
  // the render chunk.

  static ChunkHash terrainChunkHash(WorldRenderData& renderData, Vec2I chunkIndex);
  static ChunkHash liquidChunkHash(WorldRenderData& renderData, Vec2I chunkIndex);

  void renderTerrainChunks(WorldCamera const& camera, TerrainLayer terrainLayer);

  shared_ptr<TerrainChunk const> getTerrainChunk(WorldRenderData& renderData, Vec2I chunkIndex);
  shared_ptr<LiquidChunk const> getLiquidChunk(WorldRenderData& renderData, Vec2I chunkIndex);

  bool produceTerrainPrimitives(HashMap<QuadZLevel, List<RenderPrimitive>>& primitives,
      TerrainLayer terrainLayer, Vec2I const& pos, WorldRenderData const& renderData);
  void produceLiquidPrimitives(HashMap<LiquidId, List<RenderPrimitive>>& primitives, Vec2I const& pos, WorldRenderData const& renderData);

  float liquidDrawLevel(float liquidLevel) const;

  List<LiquidInfo> m_liquids;

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

}

#endif
