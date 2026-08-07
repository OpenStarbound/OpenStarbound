#pragma once

#include "StarCellularLighting.hpp"
#include "StarEntity.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarImage.hpp"
#include "StarParallax.hpp"
#include "StarParticle.hpp"
#include "StarSkyRenderData.hpp"
#include "StarThread.hpp"
#include "StarWeatherTypes.hpp"
#include "StarWorldTiles.hpp"

namespace Star {

struct EntityDrawables {
  EntityHighlightEffect highlightEffect;
  Map<EntityRenderLayer, List<Drawable>> layers;
};

struct WorldRenderData {
  void clear();

  WorldGeometry geometry;

  Vec2I tileMinPosition;
  RenderTileArray tiles;
  Vec2I lightMinPosition;
  Lightmap lightMap;
  // Bumped by the world client whenever a freshly computed lightmap is
  // published; lets the painter skip redundant texture uploads.
  uint64_t lightMapVersion = 0;

  List<EntityDrawables> entityDrawables;
  List<Particle> const* particles;

  List<OverheadBar> overheadBars;
  List<Drawable> nametags;

  List<Drawable> backgroundOverlays;
  List<Drawable> foregroundOverlays;

  List<ParallaxLayer> parallaxLayers;

  SkyRenderData skyRenderData;

  bool isFullbright = false;
  float dimLevel = 0.0f;
  Vec3B dimColor;
};

inline void WorldRenderData::clear() {
  tiles.resize({0, 0});// keep reserved

  entityDrawables.clear();
  particles = nullptr;
  overheadBars.clear();
  nametags.clear();
  backgroundOverlays.clear();
  foregroundOverlays.clear();
  parallaxLayers.clear();
}

}// namespace Star
