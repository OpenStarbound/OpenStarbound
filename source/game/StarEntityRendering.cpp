#include "StarEntityRendering.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

RenderCallback::~RenderCallback() {}

void RenderCallback::addDrawables(List<Drawable> drawables, EntityRenderLayer renderLayer, Vec2F translate) {
  for (auto& drawable : drawables) {
    drawable.translate(translate);
    addDrawable(std::move(drawable), renderLayer);
  }
}

void RenderCallback::addLightSources(List<LightSource> lightSources, Vec2F translate) {
  for (auto& lightSource : lightSources) {
    lightSource.translate(translate);
    addLightSource(std::move(lightSource));
  }
}

void RenderCallback::addParticles(List<Particle> particles, Vec2F translate) {
  for (auto& particle : particles) {
    particle.translate(translate);
    addParticle(std::move(particle));
  }
}

void RenderCallback::addAudios(List<AudioInstancePtr> audios, Vec2F translate) {
  for (auto& audio : audios) {
    audio->translate(translate);
    addAudio(std::move(audio));
  }
}

void RenderCallback::addTilePreviews(List<PreviewTile> previews) {
  for (auto& preview : previews)
    addTilePreview(std::move(preview));
}

void RenderCallback::addOverheadBars(List<OverheadBar> bars, Vec2F translate) {
  for (auto& bar : bars) {
    bar.entityPosition += translate;
    addOverheadBar(std::move(bar));
  }
}

}
