#pragma once

#include "StarMaybe.hpp"
#include "StarWeightedPool.hpp"
#include "StarParticle.hpp"

namespace Star {

struct WeatherType {
  struct ParticleConfig {
    Particle particle;
    float density;
    bool autoRotate;
  };

  struct ProjectileConfig {
    String projectile;
    Json parameters;
    Vec2F velocity;
    float ratePerX;
    int spawnAboveRegion;
    int spawnHorizontalPad;
    float windAffectAmount;
  };

  WeatherType();
  WeatherType(Json config, String path = String());

  Json toJson() const;

  String name;

  List<ParticleConfig> particles;
  List<ProjectileConfig> projectiles;
  StringList statusEffects;

  float maximumWind;
  Vec2F duration;
  StringList weatherNoises;

  // Resolved from local assets and intentionally excluded from legacy binary serialization.
  Maybe<String> parallax;
};

typedef WeightedPool<String> WeatherPool;

DataStream& operator>>(DataStream& ds, WeatherType& weatherType);
DataStream& operator<<(DataStream& ds, WeatherType const& weatherType);
}
