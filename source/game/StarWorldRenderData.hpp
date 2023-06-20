#ifndef STAR_WORLD_CLIENT_DATA_HPP
#define STAR_WORLD_CLIENT_DATA_HPP

#include "StarImage.hpp"
#include "StarWorldTiles.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarSkyRenderData.hpp"
#include "StarParallax.hpp"
#include "StarParticle.hpp"
#include "StarWeatherTypes.hpp"
#include "StarEntity.hpp"

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

  List<EntityDrawables> entityDrawables;
  List<Particle> particles;

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
  tiles.clear();

  entityDrawables.clear();
  particles.clear();
  overheadBars.clear();
  nametags.clear();
  backgroundOverlays.clear();
  foregroundOverlays.clear();
  parallaxLayers.clear();
}

}

#endif
