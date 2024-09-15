#pragma once

#include "StarNetElementSystem.hpp"
#include "StarWeatherTypes.hpp"
#include "StarWorldGeometry.hpp"

namespace Star {

STAR_CLASS(Clock);
STAR_CLASS(Projectile);

STAR_CLASS(ServerWeather);
STAR_CLASS(ClientWeather);

// Callback used to determine whether weather effects should be spawned in
// the given tile location.  Other checks that enable / disable weather such as
// whether or not the region is below the underground level are performed
// separately of this, this is just to check the actual tile data.
typedef function<bool(Vec2I)> WeatherEffectsActiveQuery;

class ServerWeather {
public:
  ServerWeather();

  void setup(WeatherPool weatherPool, float undergroundLevel, WorldGeometry worldGeometry,
      WeatherEffectsActiveQuery weatherEffectsActiveQuery);

  void setReferenceClock(ClockConstPtr referenceClock = {});

  void setClientVisibleRegions(List<RectI> regions);

  pair<ByteArray, uint64_t> writeUpdate(uint64_t fromVersion = 0, NetCompatibilityRules rules = {});

  void update(double dt);

  float wind() const;
  float weatherIntensity() const;

  StringList statusEffects() const;

  List<ProjectilePtr> pullNewProjectiles();

private:
  void setNetStates();

  void spawnWeatherProjectiles(float dt);

  WeatherPool m_weatherPool;
  float m_undergroundLevel;
  WorldGeometry m_worldGeometry;
  WeatherEffectsActiveQuery m_weatherEffectsActiveQuery;

  List<RectI> m_clientVisibleRegions;

  size_t m_currentWeatherIndex;
  Maybe<WeatherType> m_currentWeatherType;
  float m_currentWeatherIntensity;
  float m_currentWind;

  ClockConstPtr m_referenceClock;
  Maybe<double> m_clockTrackingTime;

  double m_currentTime;
  double m_lastWeatherChangeTime;
  double m_nextWeatherChangeTime;

  List<ProjectilePtr> m_newProjectiles;

  NetElementTopGroup m_netGroup;
  NetElementBytes m_weatherPoolNetState;
  NetElementFloat m_undergroundLevelNetState;
  NetElementSize m_currentWeatherIndexNetState;
  NetElementFloat m_currentWeatherIntensityNetState;
  NetElementFloat m_currentWindNetState;
};

class ClientWeather {
public:
  ClientWeather();

  void setup(WorldGeometry worldGeometry, WeatherEffectsActiveQuery weatherEffectsActiveQuery);

  void readUpdate(ByteArray data, NetCompatibilityRules rules);

  void setVisibleRegion(RectI visibleRegion);

  void update(double dt);

  float wind() const;
  float weatherIntensity() const;

  StringList statusEffects() const;

  List<Particle> pullNewParticles();
  StringList weatherTrackOptions() const;

private:
  void getNetStates();

  void spawnWeatherParticles(RectF newClientRegion, float dt);

  WeatherPool m_weatherPool;
  float m_undergroundLevel;
  WorldGeometry m_worldGeometry;
  WeatherEffectsActiveQuery m_weatherEffectsActiveQuery;

  size_t m_currentWeatherIndex;
  Maybe<WeatherType> m_currentWeatherType;
  float m_currentWeatherIntensity;
  float m_currentWind;

  double m_currentTime;
  RectI m_visibleRegion;

  List<Particle> m_particles;
  RectF m_lastParticleVisibleRegion;

  NetElementTopGroup m_netGroup;
  NetElementBytes m_weatherPoolNetState;
  NetElementFloat m_undergroundLevelNetState;
  NetElementSize m_currentWeatherIndexNetState;
  NetElementFloat m_currentWeatherIntensityNetState;
  NetElementFloat m_currentWindNetState;
};

}
