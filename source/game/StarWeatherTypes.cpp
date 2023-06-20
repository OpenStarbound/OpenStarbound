#include "StarWeatherTypes.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarBiomeDatabase.hpp"

namespace Star {

WeatherType::WeatherType() {
  maximumWind = 0;
}

WeatherType::WeatherType(Json config, String path) {
  if (config.isType(Json::Type::String)) {
    path = config.toString();
    config = Root::singleton().assets()->json(path);
  }

  name = config.getString("name");

  for (auto v : config.getArray("particles", JsonArray())) {
    ParticleConfig config;
    config.particle = Particle(v.get("particle"), path);
    config.density = v.getFloat("density");
    config.autoRotate = v.getBool("autoRotate", false);
    particles.append(move(config));
  }

  for (auto v : config.getArray("projectiles", JsonArray())) {
    ProjectileConfig config;
    config.projectile = v.getString("projectile");
    config.parameters = v.get("parameters", {});
    config.velocity = jsonToVec2F(v.get("velocity"));
    config.ratePerX = v.getFloat("ratePerX");
    config.spawnAboveRegion = v.getInt("spawnAboveRegion");
    config.spawnHorizontalPad = v.getInt("spawnHorizontalPad");
    config.windAffectAmount = v.getFloat("windAffectAmount", 0.0f);
    projectiles.append(move(config));
  }

  maximumWind = config.getFloat("maximumWind", 0.0f);
  duration = jsonToVec2F(config.get("duration"));
  weatherNoises = jsonToStringList(config.get("weatherNoises", JsonArray()));
  statusEffects = jsonToStringList(config.get("statusEffects", JsonArray()));
}

Json WeatherType::toJson() const {
  return JsonObject{{"name", name},
      {"particles",
          particles.transformed([](ParticleConfig const& pc) -> Json {
            return JsonObject{
                {"particle", pc.particle.toJson()}, {"density", pc.density}, {"autoRotate", pc.autoRotate}};
          })},
      {"projectiles",
          projectiles.transformed([](ProjectileConfig const& pc) -> Json {
            return JsonObject{{"projectile", pc.projectile},
                {"parameters", pc.parameters},
                {"velocity", jsonFromVec2F(pc.velocity)},
                {"ratePerX", pc.ratePerX},
                {"spawnAboveRegion", pc.spawnAboveRegion},
                {"spawnHorizontalPad", pc.spawnHorizontalPad},
                {"windAffectAmount", pc.windAffectAmount}};
          })},
      {"maximumWind", maximumWind},
      {"duration", jsonFromVec2F(duration)},
      {"weatherNoises", jsonFromStringList(weatherNoises)},
      {"statusEffects", jsonFromStringList(statusEffects)}};
}

DataStream& operator>>(DataStream& ds, WeatherType& weatherType) {
  ds.read(weatherType.name);
  ds.readContainer(weatherType.particles,
      [](DataStream& ds, WeatherType::ParticleConfig& config) {
        ds.read(config.particle);
        ds.read(config.density);
        ds.read(config.autoRotate);
      });
  ds.readContainer(weatherType.projectiles,
      [](DataStream& ds, WeatherType::ProjectileConfig& config) {
        ds.read(config.projectile);
        ds.read(config.parameters);
        ds.read(config.velocity);
        ds.read(config.ratePerX);
        ds.read(config.spawnAboveRegion);
        ds.read(config.spawnHorizontalPad);
        ds.read(config.windAffectAmount);
      });
  ds.read(weatherType.maximumWind);
  ds.read(weatherType.duration);
  ds.readContainer(weatherType.weatherNoises);
  ds.readContainer(weatherType.statusEffects);

  return ds;
}

DataStream& operator<<(DataStream& ds, WeatherType const& weatherType) {
  ds.write(weatherType.name);
  ds.writeContainer(weatherType.particles,
      [](DataStream& ds, WeatherType::ParticleConfig const& config) {
        ds.write(config.particle);
        ds.write(config.density);
        ds.write(config.autoRotate);
      });
  ds.writeContainer(weatherType.projectiles,
      [](DataStream& ds, WeatherType::ProjectileConfig const& config) {
        ds.write(config.projectile);
        ds.write(config.parameters);
        ds.write(config.velocity);
        ds.write(config.ratePerX);
        ds.write(config.spawnAboveRegion);
        ds.write(config.spawnHorizontalPad);
        ds.write(config.windAffectAmount);
      });
  ds.write(weatherType.maximumWind);
  ds.write(weatherType.duration);
  ds.writeContainer(weatherType.weatherNoises);
  ds.writeContainer(weatherType.statusEffects);

  return ds;
}

}
