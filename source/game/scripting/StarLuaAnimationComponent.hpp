#ifndef STAR_LUA_ANIMATION_COMPONENT_HPP
#define STAR_LUA_ANIMATION_COMPONENT_HPP

#include "StarLuaComponents.hpp"
#include "StarJsonExtra.hpp"
#include "StarLightSource.hpp"
#include "StarDrawable.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarMixer.hpp"
#include "StarParticleDatabase.hpp"
#include "StarParticle.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarLuaConverters.hpp"

namespace Star {

STAR_EXCEPTION(LuaAnimationComponentException, LuaComponentException);

// Lua component that allows lua to directly produce drawables, light sources,
// audios, and particles.  Adds a "localAnimation" callback table.
template <typename Base>
class LuaAnimationComponent : public Base {
public:
  LuaAnimationComponent();

  List<pair<Drawable, Maybe<EntityRenderLayer>>> const& drawables();
  List<LightSource> const& lightSources();

  List<Particle> pullNewParticles();
  List<AudioInstancePtr> pullNewAudios();

protected:
  // Clears looping audio on context shutdown
  void contextShutdown() override;

private:
  List<Particle> m_pendingParticles;
  List<AudioInstancePtr> m_pendingAudios;
  List<AudioInstancePtr> m_activeAudio;

  List<pair<Drawable, Maybe<EntityRenderLayer>>> m_drawables;
  List<LightSource> m_lightSources;
};

template <typename Base>
LuaAnimationComponent<Base>::LuaAnimationComponent() {
  LuaCallbacks animationCallbacks;
  animationCallbacks.registerCallback("playAudio", [this](String const& sound, Maybe<int> loops, Maybe<float> volume) {
      auto audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(sound));
      audio->setLoops(loops.value(0));
      audio->setVolume(volume.value(1.0f));
      m_pendingAudios.append(audio);
      m_activeAudio.append(audio);
    });
  animationCallbacks.registerCallback("spawnParticle", [this](Json const& particleConfig, Maybe<Vec2F> const& position) {
      auto particle = Root::singleton().particleDatabase()->particle(particleConfig);
      particle.translate(position.value());
      m_pendingParticles.append(particle);
    });
  animationCallbacks.registerCallback("clearDrawables", [this]() {
      m_drawables.clear();
    });
  animationCallbacks.registerCallback("addDrawable", [this](LuaTable const& drawableTable, Maybe<String> renderLayerName) {
      Maybe<EntityRenderLayer> renderLayer;
      if (renderLayerName)
        renderLayer = parseRenderLayer(*renderLayerName);

      Color color = drawableTable.get<Maybe<Color>>("color").value(Color::White);

      Drawable drawable;
      if (auto line = drawableTable.get<Maybe<Line2F>>("line"))
        drawable = Drawable::makeLine(line.take(), drawableTable.get<float>("width"), color);
      else if (auto poly = drawableTable.get<Maybe<PolyF>>("poly"))
        drawable = Drawable::makePoly(poly.take(), color);
      else if (auto image = drawableTable.get<Maybe<String>>("image"))
        drawable = Drawable::makeImage(image.take(), 1.0f / TilePixels, drawableTable.get<Maybe<bool>>("centered").value(true), Vec2F(), color);
      else
        throw LuaAnimationComponentException("Drawable table must have 'line', 'poly', or 'image'");

      if (auto transformation = drawableTable.get<Maybe<Mat3F>>("transformation"))
        drawable.transform(*transformation);
      if (auto rotation = drawableTable.get<Maybe<float>>("rotation"))
        drawable.rotate(*rotation);
      if (drawableTable.get<bool>("mirrored"))
        drawable.scale(Vec2F(-1, 1));
      if (auto scale = drawableTable.get<Maybe<float>>("scale"))
        drawable.scale(*scale);
      if (auto position = drawableTable.get<Maybe<Vec2F>>("position"))
        drawable.translate(*position);

      drawable.fullbright = drawableTable.get<bool>("fullbright");

      m_drawables.append({move(drawable), renderLayer});
    });
  animationCallbacks.registerCallback("clearLightSources", [this]() {
      m_lightSources.clear();
    });
  animationCallbacks.registerCallback("addLightSource", [this](LuaTable const& lightSourceTable) {
      m_lightSources.append({
          lightSourceTable.get<Vec2F>("position"),
          lightSourceTable.get<Color>("color").toRgb(),
          lightSourceTable.get<Maybe<bool>>("pointLight").value(),
          lightSourceTable.get<Maybe<float>>("pointBeam").value(),
          lightSourceTable.get<Maybe<float>>("beamAngle").value(),
          lightSourceTable.get<Maybe<float>>("beamAmbience").value()
        });
    });
  Base::addCallbacks("localAnimator", move(animationCallbacks));
}

template <typename Base>
List<pair<Drawable, Maybe<EntityRenderLayer>>> const& LuaAnimationComponent<Base>::drawables() {
  return m_drawables;
}

template <typename Base>
List<LightSource> const& LuaAnimationComponent<Base>::lightSources() {
  return m_lightSources;
}

template <typename Base>
List<Particle> LuaAnimationComponent<Base>::pullNewParticles() {
  return take(m_pendingParticles);
}

template <typename Base>
List<AudioInstancePtr> LuaAnimationComponent<Base>::pullNewAudios() {
  eraseWhere(m_activeAudio, [](AudioInstancePtr const& audio) {
      return audio->finished();
    });
  return take(m_pendingAudios);
}

template <typename Base>
void LuaAnimationComponent<Base>::contextShutdown() {
  for (auto const& audio : m_activeAudio)
    audio->setLoops(0);
  m_activeAudio.clear();
  Base::contextShutdown();
}

}

#endif
