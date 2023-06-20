#include "StarWormCave.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarInterpolation.hpp"

namespace Star {

char const* const WormCaveSelector::Name = "wormcave";

WormCaveSector::WormCaveSector(int sectorSize, Vec2I sector, Json const& config, size_t seed, float commonality)
  : m_sectorSize(sectorSize), m_sector(sector), m_values(sectorSize * sectorSize) {
  struct Worm {
    Vec2F pos;
    float angle;
    float goalAngle;
    float size;
    float length;
    float goalLength;
  };
  List<Worm> worms;

  Vec2F numberOfWormsPerSectorRange = jsonToVec2F(config.get("numberOfWormsPerSectorRange"));
  Vec2F wormSizeRange = jsonToVec2F(config.get("wormSizeRange"));
  Vec2F wormLengthRange = jsonToVec2F(config.get("wormLengthRange"));
  float wormTaperDistance = config.getFloat("wormTaperDistance");
  Vec2F wormAngleRange = jsonToVec2F(config.get("wormAngleRange"));
  float wormTurnChance = config.getFloat("wormTurnChance");
  float wormTurnRate = config.getFloat("wormTurnRate");

  float wormSpeed = config.getFloat("wormSpeed", 1);

  float twoPi = Constants::pi * 2;

  m_maxValue = wormSizeRange[1] / 2;

  // determine worms for the neighbouring sectors
  int sectorRadius = m_sectorSize * config.getInt("sectorRadius");
  for (int x = sector[0] - sectorRadius; x <= sector[0] + sectorRadius; x += m_sectorSize)
    for (int y = sector[1] - sectorRadius; y <= sector[1] + sectorRadius; y += m_sectorSize) {
      RandomSource rs(staticRandomU64(x, y, seed));
      float numberOfWorms = rs.randf(numberOfWormsPerSectorRange[0], numberOfWormsPerSectorRange[1]) * commonality;
      int intNumberOfWorms = (int)numberOfWorms;
      if (rs.randf() < (numberOfWorms - intNumberOfWorms))
        intNumberOfWorms++;
      for (int n = 0; n < intNumberOfWorms; ++n) {
        Worm worm;
        worm.pos = Vec2F(x, y) + Vec2F(rs.randf(0, m_sectorSize), rs.randf(0, m_sectorSize));
        worm.angle = rs.randf(wormAngleRange[0], wormAngleRange[1]);
        worm.goalAngle = rs.randf(wormAngleRange[0], wormAngleRange[1]);
        worm.size = rs.randf(wormSizeRange[0], wormSizeRange[1]) * commonality;
        worm.length = 0;
        worm.goalLength = rs.randf(wormLengthRange[0], wormLengthRange[1]) * commonality;
        worms.append(worm);
      }
    }

  while (true) {
    bool idle = true;

    for (auto& worm : worms) {
      if (worm.length >= worm.goalLength)
        continue;

      idle = false;

      // taper size
      float wormRadius = worm.size / 2;
      float taperFactor = 1.0f;
      if (worm.length < wormTaperDistance)
        taperFactor = std::sin(0.5 * Constants::pi * worm.length / wormTaperDistance);
      else if (worm.goalLength - worm.length < wormTaperDistance)
        taperFactor = std::sin(0.5 * Constants::pi * (worm.goalLength - worm.length) / wormTaperDistance);
      wormRadius *= taperFactor;

      // carve out worm area
      int size = ceil(wormRadius);
      for (float dx = -size; dx <= size; dx += 1)
        for (float dy = -size; dy <= size; dy += 1) {
          float m = sqrt((dx * dx) + (dy * dy));
          if (m <= wormRadius) {
            int x = floor(dx + worm.pos[0]);
            int y = floor(dy + worm.pos[1]);
            if (inside(x, y)) {
              float v = get(x, y);
              set(x, y, max(v, wormRadius - m));
            }
          }
        }

      // move the worm, slowing down a bit as we reach the ends to reduce
      // stutter
      float thisSpeed = max(0.75f, wormSpeed * taperFactor);
      worm.pos += Vec2F::withAngle(worm.angle) * thisSpeed;
      worm.length += thisSpeed;

      // maybe set new goal angle
      if (staticRandomFloat(worm.pos[0], worm.pos[1], seed, 1) < wormTurnChance * thisSpeed) {
        worm.goalAngle = pfmod(
            staticRandomFloatRange(wormAngleRange[0], wormAngleRange[1], worm.pos[0], worm.pos[1], seed, 2), twoPi);
      }

      if (worm.angle != worm.goalAngle) {
        // turn the worm toward goal angle
        float angleDiff = worm.goalAngle - worm.angle;

        // stop if we're close enough
        if (abs(angleDiff) < wormTurnRate * thisSpeed)
          worm.angle = worm.goalAngle;
        else {
          // turn the shortest angular distance
          if (abs(angleDiff) > twoPi)
            angleDiff = -angleDiff;
          worm.angle = pfmod(worm.angle + copysign(wormTurnRate * thisSpeed, angleDiff), twoPi);
        }
      }
    }
    if (idle)
      break;
  }
}

float WormCaveSector::get(int x, int y) {
  auto val = m_values[(x - m_sector[0]) + m_sectorSize * (y - m_sector[1])];
  if (val > 0)
    return val;
  else
    return -m_maxValue;
}

bool WormCaveSector::inside(int x, int y) {
  int x_ = x - m_sector[0];
  if (x_ < 0 || x_ >= m_sectorSize)
    return false;
  int y_ = y - m_sector[1];
  if (y_ < 0 || y_ >= m_sectorSize)
    return false;
  return true;
}

void WormCaveSector::set(int x, int y, float value) {
  m_values[(x - m_sector[0]) + m_sectorSize * (y - m_sector[1])] = value;
}

WormCaveSelector::WormCaveSelector(Json const& config, TerrainSelectorParameters const& parameters)
  : TerrainSelector(Name, config, parameters) {
  m_sectorSize = config.getUInt("sectorSize", 64);
  m_cache.setMaxSize(config.getUInt("lruCacheSize", 16));
}

float WormCaveSelector::get(int x, int y) const {
  Vec2I sector = Vec2I(x - pmod(x, m_sectorSize), y - pmod(y, m_sectorSize));
  return m_cache.get(sector, [=](Vec2I const& sector) {
      return WormCaveSector(m_sectorSize, sector, config, parameters.seed, parameters.commonality);
    }).get(x, y);
}

}
