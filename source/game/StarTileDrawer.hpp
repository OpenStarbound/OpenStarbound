#pragma once

#include "StarTtlCache.hpp"
#include "StarWorldRenderData.hpp"
#include "StarMaterialRenderProfile.hpp"
#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(Assets);
STAR_CLASS(MaterialDatabase);
STAR_CLASS(TileDrawer);

class TileDrawer {
public:
  typedef uint64_t QuadZLevel;
  typedef HashMap<QuadZLevel, List<Drawable>> Drawables;

  typedef size_t MaterialRenderPieceIndex;
  typedef List<pair<MaterialRenderPieceConstPtr, Vec2F>> MaterialPieceResultList;

  enum class TerrainLayer { Background, Midground, Foreground };

  static RenderTile DefaultRenderTile;

  static TileDrawer* singletonPtr();
  static TileDrawer& singleton();

  TileDrawer();
  ~TileDrawer();

  bool produceTerrainDrawables(Drawables& drawables, TerrainLayer terrainLayer, Vec2I const& pos,
    WorldRenderData const& renderData, float scale = 1.0f, Vec2I variantOffset = {}, Maybe<TerrainLayer> variantLayer = {});

  WorldRenderData& renderData();
  MutexLocker lockRenderData();

  template <typename Function>
  static void forEachRenderTile(WorldRenderData const& renderData, RectI const& worldCoordRange, Function&& function);
private:
  friend class TilePainter;

  static TileDrawer* s_singleton;

  static RenderTile const& getRenderTile(WorldRenderData const& renderData, Vec2I const& worldPos);

  static QuadZLevel materialZLevel(uint32_t zLevel, MaterialId material, MaterialHue hue, MaterialColorVariant colorVariant);
  static QuadZLevel modZLevel(uint32_t zLevel, ModId mod, MaterialHue hue, MaterialColorVariant colorVariant);
  static QuadZLevel damageZLevel();

  static bool determineMatchingPieces(MaterialPieceResultList& resultList, bool* occlude, MaterialDatabaseConstPtr const& materialDb, MaterialRenderMatchList const& matchList,
    WorldRenderData const& renderData, Vec2I const& basePos, TileLayer layer, bool isMod);

  Vec4B m_backgroundLayerColor;
  Vec4B m_foregroundLayerColor;
  Vec2F m_liquidDrawLevels;

  WorldRenderData m_tempRenderData;
  Mutex m_tempRenderDataMutex;
};

template <typename Function>
void TileDrawer::forEachRenderTile(WorldRenderData const& renderData, RectI const& worldCoordRange, Function&& function) {
  RectI indexRect = RectI::withSize(renderData.geometry.diff(worldCoordRange.min(), renderData.tileMinPosition), worldCoordRange.size());
  indexRect.limit(RectI::withSize(Vec2I(0, 0), Vec2I(renderData.tiles.size())));

  if (!indexRect.isEmpty()) {
    renderData.tiles.forEach(Array2S(indexRect.min()), Array2S(indexRect.size()), [&](Array2S const& index, RenderTile const& tile) {
      return function(worldCoordRange.min() + (Vec2I(index) - indexRect.min()), tile);
      });
  }
}

}
