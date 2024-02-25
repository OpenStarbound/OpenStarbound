#pragma once

#include "StarImage.hpp"
#include "StarWorldTiles.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarSkyRenderData.hpp"
#include "StarParallax.hpp"
#include "StarParticle.hpp"
#include "StarWeatherTypes.hpp"
#include "StarEntity.hpp"
#include "StarThread.hpp"

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
  Image lightMap;
  Image tileLightMap;

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
  tiles.resize({0, 0}); // keep reserved

  entityDrawables.clear();
  particles = nullptr;
  overheadBars.clear();
  nametags.clear();
  backgroundOverlays.clear();
  foregroundOverlays.clear();
  parallaxLayers.clear();
}

}
