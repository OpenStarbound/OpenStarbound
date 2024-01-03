#ifndef STAR_RENDERING_HPP
#define STAR_RENDERING_HPP

#include "StarMixer.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarParticle.hpp"
#include "StarDrawable.hpp"
#include "StarLightSource.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_CLASS(RenderCallback);

// Callback interface for entities to produce light sources, particles,
// drawables, and sounds on render.  Everything added is expected to already be
// translated into world space.
class RenderCallback {
public:
  virtual ~RenderCallback();

  virtual void addDrawable(Drawable drawable, EntityRenderLayer renderLayer) = 0;
  virtual void addLightSource(LightSource lightSource) = 0;
  virtual void addParticle(Particle particle) = 0;
  virtual void addInstrumentAudio(AudioInstancePtr audio) = 0;
  virtual void addAudio(AudioInstancePtr audio) = 0;
  virtual void addTilePreview(PreviewTile preview) = 0;
  virtual void addOverheadBar(OverheadBar bar) = 0;

  // Convenience non-virtuals

  void addDrawables(List<Drawable> drawables, EntityRenderLayer renderLayer, Vec2F translate = Vec2F());
  void addLightSources(List<LightSource> lightSources, Vec2F translate = Vec2F());
  void addParticles(List<Particle> particles, Vec2F translate = Vec2F());
  void addInstrumentAudios(List<AudioInstancePtr> audios, Vec2F translate = Vec2F());
  void addAudios(List<AudioInstancePtr> audios, Vec2F translate = Vec2F());
  void addTilePreviews(List<PreviewTile> previews);
  void addOverheadBars(List<OverheadBar> bars, Vec2F translate = Vec2F());
};

}

#endif
