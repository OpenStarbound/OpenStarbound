#ifndef STAR_PARTICLE_DATABASE_HPP
#define STAR_PARTICLE_DATABASE_HPP

#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarParticle.hpp"

namespace Star {

STAR_CLASS(ParticleConfig);
STAR_CLASS(ParticleDatabase);

class ParticleConfig {
public:
  ParticleConfig(Json const& config);

  String const& kind();
  Particle instance();

private:
  String m_kind;
  Particle m_particle;
  Particle m_variance;
};

class ParticleDatabase {
public:
  ParticleDatabase();

  ParticleConfigPtr config(String const& kind) const;

  // If the given variant is a string, loads the particle of that kind,
  // otherwise loads the given config directly.  If the config is given
  // directly it is assumed to optionally contain the variance config in-line.
  ParticleVariantCreator particleCreator(Json const& kindOrConfig, String const& relativePath = "") const;

  // Like particleCreator except just returns a single particle.  Probably not
  // what you want if you want to support particle variance.
  Particle particle(Json const& kindOrConfig, String const& relativePath = "") const;

private:
  StringMap<ParticleConfigPtr> m_configs;
};

}

#endif
