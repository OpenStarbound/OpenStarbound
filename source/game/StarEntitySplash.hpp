#pragma once

#include "StarJson.hpp"
#include "StarParticle.hpp"

namespace Star {

STAR_CLASS(World);
STAR_STRUCT(EntitySplashConfig);
STAR_CLASS(EntitySplashHelper);

struct EntitySplashConfig {
  EntitySplashConfig();
  EntitySplashConfig(Json const& config);
  float splashSpeedMin;
  Vec2F splashBottomSensor;
  Vec2F splashTopSensor;
  float splashMinWaterLevel;
  int numSplashParticles;
  Particle splashParticle;
  Particle splashParticleVariance;
  float splashYVelocityFactor;

  List<Particle> doSplash(Vec2F position, Vec2F velocity, World* world) const;
};

}
