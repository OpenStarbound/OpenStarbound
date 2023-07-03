#include "StarWorldPainter.hpp"
#include "StarAnimation.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

WorldPainter::WorldPainter() {
  m_assets = Root::singleton().assets();

  m_camera.setScreenSize({800, 600});
  m_camera.setCenterWorldPosition(Vec2F());
  m_camera.setPixelRatio(Root::singleton().configuration()->get("zoomLevel").toFloat());

  m_highlightConfig = m_assets->json("/highlights.config");
  for (auto p : m_highlightConfig.get("highlightDirectives").iterateObject())
    m_highlightDirectives.set(EntityHighlightEffectTypeNames.getLeft(p.first), {p.second.getString("underlay", ""), p.second.getString("overlay", "")});

  m_entityBarOffset = jsonToVec2F(m_assets->json("/rendering.config:entityBarOffset"));
  m_entityBarSpacing = jsonToVec2F(m_assets->json("/rendering.config:entityBarSpacing"));
  m_entityBarSize = jsonToVec2F(m_assets->json("/rendering.config:entityBarSize"));
  m_entityBarIconOffset = jsonToVec2F(m_assets->json("/rendering.config:entityBarIconOffset"));
  m_preloadTextureChance = m_assets->json("/rendering.config:preloadTextureChance").toFloat();
}

void WorldPainter::renderInit(RendererPtr renderer) {
  m_assets = Root::singleton().assets();

  m_renderer = move(renderer);
  auto textureGroup = m_renderer->createTextureGroup(TextureGroupSize::Large);
  m_textPainter = make_shared<TextPainter>(m_renderer, textureGroup);
  m_tilePainter = make_shared<TilePainter>(m_renderer);
  m_drawablePainter = make_shared<DrawablePainter>(m_renderer, make_shared<AssetTextureGroup>(textureGroup));
  m_environmentPainter = make_shared<EnvironmentPainter>(m_renderer);
}

void WorldPainter::setCameraPosition(WorldGeometry const& geometry, Vec2F const& position) {
  m_camera.setWorldGeometry(geometry);
  m_camera.setCenterWorldPosition(position);
}

WorldCamera& WorldPainter::camera() {
  return m_camera;
}

void WorldPainter::render(WorldRenderData& renderData, function<void()> lightWaiter) {
  m_camera.setScreenSize(m_renderer->screenSize());
  m_camera.setTargetPixelRatio(Root::singleton().configuration()->get("zoomLevel").toFloat());

  m_assets = Root::singleton().assets();

  m_environmentPainter->update();

  m_tilePainter->setup(m_camera, renderData);

  // Stars, Debris Fields, Sky, and Orbiters

  // Use a fixed pixel ratio for certain things.
  float pixelRatioBasis = m_camera.screenSize()[1] / 1080.0f;
  float starAndDebrisRatio = lerp(0.0625f, pixelRatioBasis * 2.0f, m_camera.pixelRatio());
  float orbiterAndPlanetRatio = lerp(0.125f, pixelRatioBasis * 3.0f, m_camera.pixelRatio());

  m_environmentPainter->renderStars(starAndDebrisRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  m_environmentPainter->renderDebrisFields(starAndDebrisRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  m_environmentPainter->renderBackOrbiters(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  m_environmentPainter->renderPlanetHorizon(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  m_environmentPainter->renderSky(Vec2F(m_camera.screenSize()), renderData.skyRenderData);
  m_environmentPainter->renderFrontOrbiters(orbiterAndPlanetRatio, Vec2F(m_camera.screenSize()), renderData.skyRenderData);

  if (lightWaiter) {
    auto start = Time::monotonicMicroseconds();
    lightWaiter();
    LogMap::set("render_world_async_lighting_wait_time", strf(u8"{:05d}µs", Time::monotonicMicroseconds() - start));
  }

  if (renderData.isFullbright) {
    m_renderer->setEffectTexture("lightMap", Image::filled(Vec2U(1, 1), { 255, 255, 255, 255 }, PixelFormat::RGB24));
    m_renderer->setEffectTexture("tileLightMap", Image::filled(Vec2U(1, 1), { 0, 0, 0, 0 }, PixelFormat::RGBA32));
    m_renderer->setEffectParameter("lightMapMultiplier", 1.0f);
  } else {
    adjustLighting(renderData);
    m_renderer->setEffectParameter("lightMapMultiplier", m_assets->json("/rendering.config:lightMapMultiplier").toFloat());
    m_renderer->setEffectParameter("lightMapScale", Vec2F::filled(TilePixels * m_camera.pixelRatio()));
    m_renderer->setEffectParameter("lightMapOffset", m_camera.worldToScreen(Vec2F(renderData.lightMinPosition)));
    m_renderer->setEffectTexture("lightMap", renderData.lightMap);
    m_renderer->setEffectTexture("tileLightMap", renderData.tileLightMap);
  }

  // Parallax layers

  auto parallaxDelta = m_camera.worldGeometry().diff(m_camera.centerWorldPosition(), m_previousCameraCenter);
  if (parallaxDelta.magnitude() > 10)
    m_parallaxWorldPosition = m_camera.centerWorldPosition();
  else
    m_parallaxWorldPosition += parallaxDelta;
  m_previousCameraCenter = m_camera.centerWorldPosition();
  m_parallaxWorldPosition[1] = m_camera.centerWorldPosition()[1];

  if (!renderData.parallaxLayers.empty())
    m_environmentPainter->renderParallaxLayers(m_parallaxWorldPosition, m_camera, renderData.parallaxLayers, renderData.skyRenderData);

  // Main world layers

  Map<EntityRenderLayer, List<pair<EntityHighlightEffect, List<Drawable>>>> entityDrawables;
  for (auto& ed : renderData.entityDrawables) {
    for (auto& p : ed.layers)
      entityDrawables[p.first].append({ed.highlightEffect, move(p.second)});
  }

  auto entityDrawableIterator = entityDrawables.begin();
  auto renderEntitiesUntil = [this, &entityDrawables, &entityDrawableIterator](Maybe<EntityRenderLayer> until) {
    while (true) {
      if (entityDrawableIterator == entityDrawables.end())
        break;
      if (until && entityDrawableIterator->first >= *until)
        break;
      for (auto& edl : entityDrawableIterator->second)
        drawEntityLayer(move(edl.second), edl.first);
      ++entityDrawableIterator;
    }

    m_renderer->flush();
  };

  renderEntitiesUntil(RenderLayerBackgroundOverlay);
  drawDrawableSet(renderData.backgroundOverlays);
  renderEntitiesUntil(RenderLayerBackgroundTile);
  m_tilePainter->renderBackground(m_camera);
  renderEntitiesUntil(RenderLayerPlatform);
  m_tilePainter->renderMidground(m_camera);
  renderEntitiesUntil(RenderLayerBackParticle);
  renderParticles(renderData, Particle::Layer::Back);
  renderEntitiesUntil(RenderLayerLiquid);
  m_tilePainter->renderLiquid(m_camera);
  renderEntitiesUntil(RenderLayerMiddleParticle);
  renderParticles(renderData, Particle::Layer::Middle);
  renderEntitiesUntil(RenderLayerForegroundTile);
  m_tilePainter->renderForeground(m_camera);
  renderEntitiesUntil(RenderLayerForegroundOverlay);
  drawDrawableSet(renderData.foregroundOverlays);
  renderEntitiesUntil(RenderLayerFrontParticle);
  renderParticles(renderData, Particle::Layer::Front);
  renderEntitiesUntil(RenderLayerOverlay);
  drawDrawableSet(renderData.nametags);
  renderBars(renderData);
  renderEntitiesUntil({});

  auto dimLevel = round(renderData.dimLevel * 255);
  if (dimLevel != 0)
    m_renderer->render(renderFlatRect(RectF::withSize({}, Vec2F(m_camera.screenSize())), Vec4B(renderData.dimColor, dimLevel), 0.0f));

  int64_t textureTimeout = m_assets->json("/rendering.config:textureTimeout").toInt();
  m_textPainter->cleanup(textureTimeout);
  m_drawablePainter->cleanup(textureTimeout);
  m_environmentPainter->cleanup(textureTimeout);
  m_tilePainter->cleanup();
}

void WorldPainter::adjustLighting(WorldRenderData& renderData) {
  m_tilePainter->adjustLighting(renderData);
}

void WorldPainter::renderParticles(WorldRenderData& renderData, Particle::Layer layer) {
  const int textParticleFontSize = m_assets->json("/rendering.config:textParticleFontSize").toInt();
  const RectF particleRenderWindow = RectF::withSize(Vec2F(), Vec2F(m_camera.screenSize())).padded(m_assets->json("/rendering.config:particleRenderWindowPadding").toInt());

  for (Particle const& particle : renderData.particles) {
    if (layer != particle.layer)
      continue;

    Vec2F position = m_camera.worldToScreen(particle.position);

    if (!particleRenderWindow.contains(position))
      continue;

    Vec2F size = Vec2F::filled(particle.size * m_camera.pixelRatio());

    if (particle.type == Particle::Type::Ember) {
      m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(),
        RectF(position - size / 2, position + size / 2),
        particle.color.toRgba(),
        particle.fullbright ? 0.0f : 1.0f);

    } else if (particle.type == Particle::Type::Streak) {
      // Draw a rotated quad streaking in the direction the particle is coming from.
      // Sadly this looks awful.
      Vec2F dir = particle.velocity.normalized();
      Vec2F sideHalf = dir.rot90() * m_camera.pixelRatio() * particle.size / 2;
      float length = particle.length * m_camera.pixelRatio();
      Vec4B color = particle.color.toRgba();
      float lightMapMultiplier = particle.fullbright ? 0.0f : 1.0f;
      m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(),
        position - sideHalf,
        position + sideHalf,
        position - dir * length + sideHalf,
        position - dir * length - sideHalf,
        color, lightMapMultiplier);

    } else if (particle.type == Particle::Type::Textured || particle.type == Particle::Type::Animated) {
      Drawable drawable;
      if (particle.type == Particle::Type::Textured)
        drawable = Drawable::makeImage(particle.image, 1.0f / TilePixels, true, Vec2F(0, 0));
      else
        drawable = particle.animation->drawable(1.0f / TilePixels);

      if (particle.flip && particle.flippable)
        drawable.scale(Vec2F(-1, 1));
      if (drawable.isImage())
        drawable.imagePart().addDirectivesGroup(particle.directives, true);
      drawable.fullbright = particle.fullbright;
      drawable.color = particle.color;
      drawable.rotate(particle.rotation);
      drawable.scale(particle.size);
      drawable.translate(particle.position);
      drawDrawable(move(drawable));

    } else if (particle.type == Particle::Type::Text) {
      Vec2F position = m_camera.worldToScreen(particle.position);
      unsigned size = round((float)textParticleFontSize * m_camera.pixelRatio() * particle.size);
      if (size > 0) {
        m_textPainter->setFontSize(size);
        m_textPainter->setFontColor(particle.color.toRgba());
        m_textPainter->setProcessingDirectives("");
        m_textPainter->setFont("");
        m_textPainter->renderText(particle.string, {position, HorizontalAnchor::HMidAnchor, VerticalAnchor::VMidAnchor});
      }
    }
  }

  m_renderer->flush();
}

void WorldPainter::renderBars(WorldRenderData& renderData) {
  auto offset = m_entityBarOffset;
  for (auto const& bar : renderData.overheadBars) {
    auto position = bar.entityPosition + offset;
    offset += m_entityBarSpacing;
    if (bar.icon) {
      auto iconDrawPosition = position - (m_entityBarSize / 2).round() + m_entityBarIconOffset;
      drawDrawable(Drawable::makeImage(*bar.icon, 1.0f / TilePixels, true, iconDrawPosition));
    }

    if (!bar.detailOnly) {
      auto fullBar = RectF({}, {m_entityBarSize.x() * bar.percentage, m_entityBarSize.y()});
      auto emptyBar = RectF({m_entityBarSize.x() * bar.percentage, 0.0f}, m_entityBarSize);
      auto fullColor = bar.color;
      auto emptyColor = Color::Black;

      drawDrawable(Drawable::makePoly(PolyF(emptyBar), emptyColor, position));
      drawDrawable(Drawable::makePoly(PolyF(fullBar), fullColor, position));
    }
  }

  m_renderer->flush();
}

void WorldPainter::drawEntityLayer(List<Drawable> drawables, EntityHighlightEffect highlightEffect) {
  highlightEffect.level *= m_highlightConfig.getFloat("maxHighlightLevel", 1.0);
  if (m_highlightDirectives.contains(highlightEffect.type) && highlightEffect.level > 0) {
    // first pass, draw underlay
    auto underlayDirectives = m_highlightDirectives[highlightEffect.type].first;
    if (!underlayDirectives.empty()) {
      for (auto& d : drawables) {
        if (d.isImage()) {
          auto underlayDrawable = Drawable(d);
          underlayDrawable.fullbright = true;
          underlayDrawable.color = Color::rgbaf(1, 1, 1, highlightEffect.level);
          underlayDrawable.imagePart().addDirectives(underlayDirectives, true);
          drawDrawable(move(underlayDrawable));
        }
      }
    }

    // second pass, draw main drawables and overlays
    auto overlayDirectives = m_highlightDirectives[highlightEffect.type].second;
    for (auto& d : drawables) {
      drawDrawable(d);
      if (!overlayDirectives.empty() && d.isImage()) {
        auto overlayDrawable = Drawable(d);
        overlayDrawable.fullbright = true;
        overlayDrawable.color = Color::rgbaf(1, 1, 1, highlightEffect.level);
        overlayDrawable.imagePart().addDirectives(overlayDirectives, true);
        drawDrawable(move(overlayDrawable));
      }
    }
  } else {
    for (auto& d : drawables)
      drawDrawable(move(d));
  }
}

void WorldPainter::drawDrawable(Drawable drawable) {
  drawable.position = m_camera.worldToScreen(drawable.position);
  drawable.scale(m_camera.pixelRatio() * TilePixels, drawable.position);

  if (drawable.isLine())
    drawable.linePart().width *= m_camera.pixelRatio();

  // draw the drawable if it's on screen
  // if it's not on screen, there's a random chance to pre-load
  // pre-load is not done on every tick because it's expensive to look up images with long paths
  if (RectF::withSize(Vec2F(), Vec2F(m_camera.screenSize())).intersects(drawable.boundBox(false)))
    m_drawablePainter->drawDrawable(drawable);
  else if (drawable.isImage() && Random::randf() < m_preloadTextureChance)
    m_assets->tryImage(drawable.imagePart().image);
}

void WorldPainter::drawDrawableSet(List<Drawable>& drawables) {
  for (Drawable& drawable : drawables)
    drawDrawable(move(drawable));

  m_renderer->flush();
}

}
