#pragma once

#include "StarVector.hpp"
#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarParticle.hpp"

namespace Star {

STAR_CLASS(AudioInstance);
STAR_CLASS(EffectSource);
STAR_CLASS(EffectSourceConfig);
STAR_CLASS(EffectSourceDatabase);

class EffectSource {
public:
  EffectSource(String const& kind, String suggestedSpawnLocation, Json const& definition);
  String const& kind() const;
  void tick(float dt);
  bool expired() const;
  void stop();
  List<String> particles();
  List<AudioInstancePtr> sounds(Vec2F offset);
  void postRender();
  String effectSpawnLocation() const;
  String suggestedSpawnLocation() const;

private:
  String m_kind;
  Json m_config;
  bool m_loops;
  float m_loopDuration;
  float m_durationVariance;
  String m_effectSpawnLocation;
  String m_suggestedSpawnLocation;

  bool m_initialTick;
  bool m_loopTick;
  bool m_finalTick;
  float m_timer;
  bool m_expired;
  bool m_stop;

  List<AudioInstancePtr> m_mainSounds;
};

class EffectSourceConfig {
public:
  EffectSourceConfig(Json const& config);
  String const& kind();
  EffectSourcePtr instance(String const& suggestedSpawnLocation);

private:
  String m_kind;
  Json m_config;
};

class EffectSourceDatabase {
public:
  EffectSourceDatabase();

  EffectSourceConfigPtr effectSourceConfig(String const& kind) const;

private:
  StringMap<EffectSourceConfigPtr> m_sourceConfigs;
};

List<Particle> particlesFromDefinition(Json const& config, Vec2F const& position = Vec2F());
List<AudioInstancePtr> soundsFromDefinition(Json const& config, Vec2F const& position = Vec2F());

}
