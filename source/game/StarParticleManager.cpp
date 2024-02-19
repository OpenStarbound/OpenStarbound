#include "StarParticleManager.hpp"
#include "StarIterator.hpp"
#include "StarLogging.hpp"

namespace Star {

ParticleManager::ParticleManager(WorldGeometry const& worldGeometry, ClientTileSectorArrayPtr const& tileSectorArray)
  : m_worldGeometry(worldGeometry), m_undergroundLevel(0.0f), m_tileSectorArray(tileSectorArray) {}

void ParticleManager::add(Particle particle) {
  m_particles.push_back(std::move(particle));
}

void ParticleManager::addParticles(List<Particle> particles) {
  m_particles.appendAll(std::move(particles));
}

size_t ParticleManager::count() const {
  return m_particles.size();
}

void ParticleManager::clear() {
  m_particles.clear();
}

void ParticleManager::setUndergroundLevel(float undergroundLevel) {
  m_undergroundLevel = undergroundLevel;
}

void ParticleManager::update(float dt, RectF const& cullRegion, float wind) {
  if (!m_tileSectorArray)
    return;

  auto cullRects = m_worldGeometry.splitRect(cullRegion);

  for (auto& particle : m_particles) {
    bool inRegion = false;
    Vec2F worldPos = m_worldGeometry.xwrap(particle.position);
    for (auto cullRect : cullRects) {
      if (cullRect.contains(worldPos)) {
        inRegion = true;
        break;
      }
    }
    if (!inRegion)
      continue;

    particle.update(dt, Vec2F(wind, 0));
    Vec2I pos(particle.position.floor());
    TileType tiletype;
    auto const& tile = m_tileSectorArray->tile(pos);
    if (isSolidColliding(tile.collision))
      tiletype = TileType::Colliding;
    else if (tile.liquid.level > 0.5f)
      tiletype = TileType::Water;
    else
      tiletype = TileType::Empty;

    if (particle.collidesForeground && tiletype == TileType::Colliding) {
      RectF colRect;
      colRect.setXMax(std::ceil(particle.position[0]));
      colRect.setXMin(std::floor(particle.position[0]));
      colRect.setYMax(std::ceil(particle.position[1]));
      colRect.setYMin(std::floor(particle.position[1]));
      Line2F colLine(particle.position, particle.position - particle.velocity);

      auto collisionPosition = colRect.edgeIntersection(colLine).point;
      if (particle.position[0] > colRect.center()[0])
        collisionPosition[0] += 0.1f;
      else if (particle.position[0] < colRect.center()[0])
        collisionPosition[0] -= 0.1f;
      if (particle.position[1] > colRect.center()[1])
        collisionPosition[1] += 0.1f;
      else if (particle.position[1] < colRect.center()[1])
        collisionPosition[1] -= 0.1f;

      particle.collide(collisionPosition);
    }

    if (particle.underwaterOnly && tiletype == TileType::Empty)
      particle.destroy(false);

    if (particle.collidesLiquid && tiletype == TileType::Water)
      particle.destroy(false);

    if (particle.trail && particle.timeToLive >= 0.0f) {
      auto trail = particle;
      trail.trail = false;
      trail.timeToLive = 0;
      trail.velocity = {};
      m_nextParticles.append(std::move(trail));
    }

    if (!particle.dead())
      m_nextParticles.append(std::move(particle));
  }

  m_particles.clear();
  swap(m_particles, m_nextParticles);
}

List<Particle> const& ParticleManager::particles() const {
  return m_particles;
}

List<pair<Vec2F, Vec3B>> ParticleManager::lightSources() const {
  List<pair<Vec2F, Vec3B>> lsources;
  for (auto const& particle : m_particles) {
    if (particle.light != Color::Clear)
      lsources.append({particle.position, particle.light.toRgb()});
  }
  return lsources;
}

}
