#include "StarEntitySplash.hpp"
#include "StarWorld.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

EntitySplashConfig::EntitySplashConfig() {}

EntitySplashConfig::EntitySplashConfig(Json const& config) {
  splashSpeedMin = config.get("splashSpeedMin").toFloat();
  splashMinWaterLevel = config.get("splashMinWaterLevel").toFloat();
  splashBottomSensor = jsonToVec2F(config.get("splashBottomSensor"));
  splashTopSensor = jsonToVec2F(config.get("splashTopSensor"));
  numSplashParticles = config.get("numSplashParticles").toInt();
  splashYVelocityFactor = config.get("splashYVelocityFactor").toFloat();
  splashParticle = Particle(config.get("splashParticle").toObject());
  splashParticleVariance = Particle(config.get("splashParticleVariance").toObject());
}

List<Particle> EntitySplashConfig::doSplash(Vec2F position, Vec2F velocity, World* world) const {
  List<Particle> particles;
  if (std::fabs(velocity[1]) >= splashSpeedMin) {
    auto liquidDb = Root::singleton().liquidsDatabase();
    Vec2I bottom = Vec2I::floor(position + splashBottomSensor);
    Vec2I top = Vec2I::floor(position + splashTopSensor);
    if (world->liquidLevel(bottom).level - world->liquidLevel(top).level >= splashMinWaterLevel) {
      LiquidId liquidType;
      auto bottomLiquid = world->liquidLevel(bottom);
      auto topLiquid = world->liquidLevel(top);
      if (bottomLiquid.level > 0 && (int)bottomLiquid.liquid)
        liquidType = bottomLiquid.liquid;
      else
        liquidType = topLiquid.liquid;
      Color particleColor = Color::rgba(liquidDb->liquidSettings(liquidType)->liquidColor);
      for (int i = 0; i < numSplashParticles; ++i) {
        Particle newSplashParticle = splashParticle;
        newSplashParticle.position = position;
        newSplashParticle.velocity[1] = std::fabs(velocity[1]) * splashYVelocityFactor;
        newSplashParticle.color = particleColor;
        newSplashParticle.applyVariance(splashParticleVariance);
        particles.append(newSplashParticle);
      }
    }
  }
  return particles;
}

}
