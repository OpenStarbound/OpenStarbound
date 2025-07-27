#include "StarWeather.hpp"
#include "StarIterator.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarTime.hpp"
#include "StarAssets.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarProjectile.hpp"
#include "StarBiomeDatabase.hpp"

namespace Star {

ServerWeather::ServerWeather() {
  m_undergroundLevel = 0.0f;
  m_currentWeatherIndex = NPos;
  m_currentWeatherIntensity = 0.0f;
  m_currentWind = 0.0f;

  m_currentTime = 0.0;
  m_lastWeatherChangeTime = 0.0;
  m_nextWeatherChangeTime = 0.0;

  m_netGroup.addNetElement(&m_weatherPoolNetState);
  m_netGroup.addNetElement(&m_undergroundLevelNetState);
  m_netGroup.addNetElement(&m_currentWeatherIndexNetState);
  m_netGroup.addNetElement(&m_currentWeatherIntensityNetState);
  m_netGroup.addNetElement(&m_currentWindNetState);
}

void ServerWeather::setup(WeatherPool weatherPool, float undergroundLevel, WorldGeometry worldGeometry,
    WeatherEffectsActiveQuery weatherEffectsActiveQuery) {
  m_weatherPool = weatherPool;
  m_undergroundLevel = undergroundLevel;

  m_worldGeometry = worldGeometry;
  m_weatherEffectsActiveQuery = weatherEffectsActiveQuery;

  m_currentWeatherIndex = NPos;
  m_currentWeatherType = {};

  m_currentTime = 0.0;
  m_lastWeatherChangeTime = 0.0;
  m_nextWeatherChangeTime = 0.0;
}

void ServerWeather::setReferenceClock(ClockConstPtr referenceClock) {
  m_referenceClock = std::move(referenceClock);
  if (m_referenceClock)
    m_clockTrackingTime = m_referenceClock->time();
  else
    m_clockTrackingTime = {};
}

void ServerWeather::setClientVisibleRegions(List<RectI> regions) {
  m_clientVisibleRegions = std::move(regions);
}

pair<ByteArray, uint64_t> ServerWeather::writeUpdate(uint64_t fromVersion, NetCompatibilityRules rules) {
  setNetStates();
  return m_netGroup.writeNetState(fromVersion, rules);
}

void ServerWeather::update(double dt) {
  spawnWeatherProjectiles(dt);

  if (m_referenceClock) {
    double clockTime = m_referenceClock->time();
    if (!m_clockTrackingTime) {
      m_clockTrackingTime = clockTime;
    } else {
      // If our reference clock is set, and we have a valid tracking time, then
      // the dt should be driven by the reference clock.
      dt = clockTime - *m_clockTrackingTime;
      m_clockTrackingTime = clockTime;
    }
  }

  m_currentTime += dt;

  if (!m_weatherPool.empty()) {
    auto assets = Root::singleton().assets();
    double weatherCooldownTime = assets->json("/weather.config:weatherCooldownTime").toDouble();
    double weatherWarmupTime = assets->json("/weather.config:weatherWarmupTime").toDouble();

    if (m_currentTime >= m_nextWeatherChangeTime) {
      m_currentWeatherIndex = m_weatherPool.selectIndex();
      if (m_currentWeatherIndex == NPos)
        m_currentWeatherType = {};
      else
        m_currentWeatherType = Root::singleton().biomeDatabase()->weatherType(m_weatherPool.item(m_currentWeatherIndex));

      m_lastWeatherChangeTime = m_nextWeatherChangeTime;
      m_nextWeatherChangeTime = m_currentTime + Random::randd(m_currentWeatherType->duration[0], m_currentWeatherType->duration[1]);

      // TODO: For now just set the wind at maximum either left or right, nothing exciting.
      m_currentWind = m_currentWeatherType->maximumWind * (Random::randb() ? 1 : -1);
    }

    m_currentWeatherIntensity = min(clamp((m_currentTime - m_lastWeatherChangeTime) / weatherWarmupTime, 0.0, 1.0),
        clamp((m_nextWeatherChangeTime - m_currentTime) / weatherCooldownTime, 0.0, 1.0));

  } else {
    m_currentWeatherIndex = NPos;
    m_currentWeatherType = {};
  }
}

float ServerWeather::wind() const {
  return m_currentWind * m_currentWeatherIntensity;
}

float ServerWeather::weatherIntensity() const {
  return m_currentWeatherIntensity;
}

StringList ServerWeather::statusEffects() const {
  if (m_currentWeatherType && m_currentWeatherIntensity == 1.0)
    return m_currentWeatherType->statusEffects;
  return {};
}

List<ProjectilePtr> ServerWeather::pullNewProjectiles() {
  return take(m_newProjectiles);
}

void ServerWeather::setWeather(String const& weatherName) {
  size_t index = NPos;
  for (size_t i = 0; i < m_weatherPool.size(); ++i) {
    if (m_weatherPool.item(i) == weatherName) {
      index = i;
      break;
    }
  }

  setWeatherIndex(index);
}

void ServerWeather::setWeatherIndex(size_t weatherIndex) {
  if (weatherIndex == NPos || weatherIndex >= m_weatherPool.size()) {
    m_currentWeatherIndex = NPos;
    m_currentWeatherType = {};
    m_currentWeatherIntensity = 0.0f;
    m_currentWind = 0.0f;
  } else {
    m_currentWeatherIndex = weatherIndex;
    m_currentWeatherType =
        Root::singleton().biomeDatabase()->weatherType(m_weatherPool.item(m_currentWeatherIndex));
    m_currentWeatherIntensity = 1.0f;
    m_currentWind = m_currentWeatherType->maximumWind * (Random::randb() ? 1 : -1);
  }

  m_lastWeatherChangeTime = m_currentTime;
  m_nextWeatherChangeTime =
      m_currentWeatherType ? m_currentTime + Random::randd(m_currentWeatherType->duration[0],
                                                          m_currentWeatherType->duration[1])
                           : m_currentTime;

  setNetStates();
}

void ServerWeather::setNetStates() {
  m_weatherPoolNetState.set(DataStreamBuffer::serializeContainer(m_weatherPool.items()));
  m_undergroundLevelNetState.set(m_undergroundLevel);
  m_currentWeatherIndexNetState.set(m_currentWeatherIndex);
  m_currentWeatherIntensityNetState.set(m_currentWeatherIntensity);
  m_currentWindNetState.set(m_currentWind);
}

void ServerWeather::spawnWeatherProjectiles(float dt) {
  if (!m_currentWeatherType || m_clientVisibleRegions.empty())
    return;

  auto projectileDatabase = Root::singleton().projectileDatabase();

  // TODO: The complexity of this method is TERRIBLE, if this becomes a problem
  // for any reason there are large numbers of ways to make this much better,
  // but this was the lazy, simple-ish, and clear (hah) way.

  for (auto const& projectileConfig : m_currentWeatherType->projectiles) {
    // Gather all the tops of the client regions together with the proper
    // padding, splitting at the world wrap boundary.
    List<pair<Vec2I, int>> baseSpawnRegions;
    for (auto const& clientRegion : m_clientVisibleRegions) {
      Vec2I baseRegion = {clientRegion.xMin() - projectileConfig.spawnHorizontalPad, clientRegion.xMax() + projectileConfig.spawnHorizontalPad};
      int height = clientRegion.yMax();
      for (auto const& region : m_worldGeometry.splitXRegion(baseRegion))
        baseSpawnRegions.append({region, height});
    }

    // We are going to have to eliminate vertically redundant sections of
    // spawning regions, so gather up every left and right edge of a spawn
    // region is a "split point"
    List<int> splitPoints;
    for (auto const& baseSpawnRegion : baseSpawnRegions) {
      splitPoints.append(baseSpawnRegion.first[0]);
      splitPoints.append(baseSpawnRegion.first[1]);
    }

    // Split every spawn region on every split point.
    List<pair<Vec2I, int>> splitSpawnRegions;
    for (auto const& baseSpawnRegion : baseSpawnRegions) {
      List<Vec2I> regions = {baseSpawnRegion.first};
      for (auto splitPoint : splitPoints) {
        auto prevRegions = take(regions);
        for (auto const& region : prevRegions) {
          if (splitPoint > region[0] && splitPoint < region[1]) {
            regions.append({region[0], splitPoint});
            regions.append({splitPoint, region[1]});
          } else {
            regions.append(region);
          }
        }
      }
      for (auto const& region : regions)
        splitSpawnRegions.append({region, baseSpawnRegion.second});
    }

    // Sort the split spawn regions by leftmost point then height, preparing to
    // remove the lower overlapping sections.
    sort(splitSpawnRegions,
        [](pair<Vec2I, int> const& lhs, pair<Vec2I, int> rhs) {
          return tie(lhs.first[0], lhs.second) < tie(rhs.first[0], rhs.second);
        });

    // For each region, at this point, if the region to the right shares the
    // same starting X, because we've split up each region on each possible
    // overlapping point, then they totally overlap.  The lower region (which
    // should come before in the list) is totally redundant and should be
    // removed.
    auto sit = makeSMutableIterator(splitSpawnRegions);
    while (sit.hasNext()) {
      auto const& leftRegion = sit.next();
      if (sit.hasNext()) {
        auto const& rightRegion = sit.peekNext();
        if (leftRegion.first[0] == rightRegion.first[0])
          sit.remove();
      }
    }

    for (auto const& spawnRegion : splitSpawnRegions) {
      RectF spawnRect = RectF(spawnRegion.first[0],
          spawnRegion.second,
          spawnRegion.first[1],
          spawnRegion.second + projectileConfig.spawnAboveRegion);

      // Figure out a good target value based on the rate per x tile, making
      // sure to handle very low count values appropriately on average.
      float count = projectileConfig.ratePerX * spawnRect.width() * dt * m_currentWeatherIntensity;
      if (Random::randf() > fpart(count))
        count = floor(count);
      else
        count = ceil(count);

      for (int i = 0; i < count; ++i) {
        Vec2F position = {Random::randf() * spawnRect.width() + spawnRect.xMin(), Random::randf() * spawnRect.height() + spawnRect.yMin()};

        if (position[1] > m_undergroundLevel && (!m_weatherEffectsActiveQuery || m_weatherEffectsActiveQuery(Vec2I::floor(position)))) {
          // Make sure not to spawn projectiles if they intersect any client
          // visible region.
          bool intersectsVisibleRegion = false;
          for (auto const& visibleRegion : m_clientVisibleRegions) {
            if (RectF(visibleRegion).contains(position)) {
              intersectsVisibleRegion = true;
              break;
            }
          }

          if (!intersectsVisibleRegion) {
            auto newProjectile = projectileDatabase->createProjectile(projectileConfig.projectile, projectileConfig.parameters);
            newProjectile->setInitialPosition(position);
            newProjectile->setInitialVelocity(projectileConfig.velocity + Vec2F(projectileConfig.windAffectAmount * wind(), 0));
            newProjectile->setTeam(EntityDamageTeam(TeamType::Environment));
            m_newProjectiles.append(newProjectile);
          }
        }
      }
    }
  }
}

ClientWeather::ClientWeather() {
  m_undergroundLevel = 0.0f;
  m_currentWeatherIndex = NPos;
  m_currentWeatherIntensity = 0.0f;
  m_currentWind = 0.0f;
  m_currentTime = 0.0;

  m_netGroup.addNetElement(&m_weatherPoolNetState);
  m_netGroup.addNetElement(&m_undergroundLevelNetState);
  m_netGroup.addNetElement(&m_currentWeatherIndexNetState);
  m_netGroup.addNetElement(&m_currentWeatherIntensityNetState);
  m_netGroup.addNetElement(&m_currentWindNetState);
}

void ClientWeather::setup(WorldGeometry worldGeometry, WeatherEffectsActiveQuery weatherEffectsActiveQuery) {
  m_worldGeometry = worldGeometry;
  m_weatherEffectsActiveQuery = weatherEffectsActiveQuery;
  m_currentTime = 0.0;
}

void ClientWeather::readUpdate(ByteArray data, NetCompatibilityRules rules) {
  if (!data.empty()) {
    m_netGroup.readNetState(data, 0.0f, rules);
    getNetStates();
  }
}

void ClientWeather::setVisibleRegion(RectI visibleRegion) {
  m_visibleRegion = visibleRegion;
}

void ClientWeather::update(double dt) {
  m_currentTime += dt;

  if (m_currentWeatherIndex == NPos) {
    m_currentWeatherType = {};
  } else {
    if (m_visibleRegion.yMax() > m_undergroundLevel)
      m_currentWeatherType = Root::singleton().biomeDatabase()->weatherType(m_weatherPool.item(m_currentWeatherIndex));
    else
      m_currentWeatherType = {};
  }

  if (m_currentWeatherType)
    spawnWeatherParticles(RectF(m_visibleRegion), dt);
}

float ClientWeather::wind() const {
  return m_currentWind * m_currentWeatherIntensity;
}

float ClientWeather::weatherIntensity() const {
  return m_currentWeatherIntensity;
}

StringList ClientWeather::statusEffects() const {
  if (m_currentWeatherIntensity == 1.0 && m_currentWeatherType)
    return m_currentWeatherType->statusEffects;
  return {};
}

List<Particle> ClientWeather::pullNewParticles() {
  return take(m_particles);
}

StringList ClientWeather::weatherTrackOptions() const {
  if (m_currentWeatherType)
    return m_currentWeatherType->weatherNoises;
  return {};
}

void ClientWeather::getNetStates() {
  if (m_weatherPoolNetState.pullUpdated())
    m_weatherPool = WeatherPool(DataStreamBuffer::deserializeContainer<WeatherPool::ItemsList>(m_weatherPoolNetState.get()));
  m_undergroundLevel = m_undergroundLevelNetState.get();
  m_currentWeatherIndex = m_currentWeatherIndexNetState.get();
  m_currentWeatherIntensity = m_currentWeatherIntensityNetState.get();
  m_currentWind = m_currentWindNetState.get();
}

void ClientWeather::spawnWeatherParticles(RectF newClientRegion, float dt) {
  if (!m_currentWeatherType)
    return;

  for (auto const& particleConfig : m_currentWeatherType->particles) {
    // Move client region to same wrap region as newClientRegion
    RectF visibleRegion(m_worldGeometry.nearestTo(newClientRegion.min(), m_lastParticleVisibleRegion.min()),
        m_worldGeometry.nearestTo(newClientRegion.min(), m_lastParticleVisibleRegion.max()));

    Vec2F targetVelocity = particleConfig.particle.velocity + Vec2F(wind(), 0);
    float angleChange = Vec2F::angleBetween2(Vec2F(0, 1), targetVelocity);
    visibleRegion.translate(targetVelocity * dt);

    for (auto const& renderZone : newClientRegion.subtract(visibleRegion)) {
      float count = particleConfig.density * renderZone.width() * renderZone.height() * m_currentWeatherIntensity;
      if (Random::randf() > fpart(count))
        count = std::floor(count);
      else
        count = std::ceil(count);

      for (int i = 0; i < count; ++i) {
        auto newParticle = particleConfig.particle;
        float x = Random::randf() * renderZone.width() + renderZone.xMin();
        float y = Random::randf() * renderZone.height() + renderZone.yMin();
        newParticle.position += m_worldGeometry.xwrap(Vec2F(x, y));
        newParticle.velocity = targetVelocity;
        if (y > m_undergroundLevel && (!m_weatherEffectsActiveQuery || m_weatherEffectsActiveQuery(Vec2I::floor(newParticle.position)))) {
          if (particleConfig.autoRotate)
            newParticle.rotation += angleChange;
          m_particles.append(std::move(newParticle));
        }
      }
    }
  }

  m_lastParticleVisibleRegion = newClientRegion;
}

}
