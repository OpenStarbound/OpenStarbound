#ifndef STAR_WORLD_PAINTER_HPP
#define STAR_WORLD_PAINTER_HPP

#include "StarWorldRenderData.hpp"
#include "StarTilePainter.hpp"
#include "StarEnvironmentPainter.hpp"
#include "StarTextPainter.hpp"
#include "StarDrawablePainter.hpp"
#include "StarRenderer.hpp"

namespace Star {

STAR_CLASS(WorldPainter);

// Will update client rendering window internally
class WorldPainter {
public:
  WorldPainter();

  void renderInit(RendererPtr renderer);

  void setCameraPosition(WorldGeometry const& worldGeometry, Vec2F const& position);

  WorldCamera& camera();

  void render(WorldRenderData& renderData);

private:
  void renderParticles(WorldRenderData& renderData, Particle::Layer layer);
  void renderBars(WorldRenderData& renderData);

  void drawEntityLayer(List<Drawable> drawables, EntityHighlightEffect highlightEffect = EntityHighlightEffect());

  void drawDrawable(Drawable drawable);
  void drawDrawableSet(List<Drawable>& drawable);

  WorldCamera m_camera;

  RendererPtr m_renderer;

  TextPainterPtr m_textPainter;
  DrawablePainterPtr m_drawablePainter;
  EnvironmentPainterPtr m_environmentPainter;
  TilePainterPtr m_tilePainter;

  Json m_highlightConfig;
  Map<EntityHighlightEffectType, pair<Directives, Directives>> m_highlightDirectives;

  Vec2F m_entityBarOffset;
  Vec2F m_entityBarSpacing;
  Vec2F m_entityBarSize;
  Vec2F m_entityBarIconOffset;

  // Updated every frame

  AssetsConstPtr m_assets;
  RectF m_worldScreenRect;

  Vec2F m_previousCameraCenter;
  Vec2F m_parallaxWorldPosition;

  float m_preloadTextureChance;
};

}

#endif
