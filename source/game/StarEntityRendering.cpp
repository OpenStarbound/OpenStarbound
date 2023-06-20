#include "StarEntityRendering.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

RenderCallback::~RenderCallback() {}

void RenderCallback::addDrawables(List<Drawable> drawables, EntityRenderLayer renderLayer, Vec2F translate) {
  for (auto& drawable : drawables) {
    drawable.translate(translate);
    addDrawable(move(drawable), renderLayer);
  }
}

void RenderCallback::addLightSources(List<LightSource> lightSources, Vec2F translate) {
  for (auto& lightSource : lightSources) {
    lightSource.translate(translate);
    addLightSource(move(lightSource));
  }
}

void RenderCallback::addParticles(List<Particle> particles, Vec2F translate) {
  for (auto& particle : particles) {
    particle.translate(translate);
    addParticle(move(particle));
  }
}

void RenderCallback::addAudios(List<AudioInstancePtr> audios, Vec2F translate) {
  for (auto& audio : audios) {
    audio->translate(translate);
    addAudio(move(audio));
  }
}

void RenderCallback::addTilePreviews(List<PreviewTile> previews) {
  for (auto& preview : previews)
    addTilePreview(move(preview));
}

void RenderCallback::addOverheadBars(List<OverheadBar> bars, Vec2F translate) {
  for (auto& bar : bars) {
    bar.entityPosition += translate;
    addOverheadBar(move(bar));
  }
}

}
