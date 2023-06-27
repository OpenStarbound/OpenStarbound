#include "StarSpawner.hpp"
#include "StarSpawnTypeDatabase.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarMonster.hpp"
#include "StarWeightedPool.hpp"
#include "StarLogging.hpp"

namespace Star {

Spawner::Spawner() {
  auto assets = Root::singleton().assets();
  auto config = assets->json("/spawning.config");

  m_spawnCellSize = config.getUInt("spawnCellSize");
  m_spawnCellMinimumEmptyTiles = config.getUInt("spawnCellMinimumEmptyTiles");
  m_spawnCellMinimumLiquidTiles = config.getUInt("spawnCellMinimumLiquidTiles");
  m_spawnCellMinimumNearSurfaceTiles = config.getUInt("spawnCellMinimumNearSurfaceTiles");
  m_spawnCellMinimumNearCeilingTiles = config.getUInt("spawnCellMinimumNearCeilingTiles");
  m_spawnCellMinimumAirTiles = config.getUInt("spawnCellMinimumAirTiles");
  m_spawnCellMinimumExposedTiles = config.getUInt("spawnCellMinimumExposedTiles");
  m_spawnCellNearSurfaceDistance = config.getUInt("spawnCellNearSurfaceDistance");
  m_spawnCellNearCeilingDistance = config.getUInt("spawnCellNearCeilingDistance");

  m_minimumDayLevel = config.getFloat("minimumDayLevel");
  m_minimumLiquidLevel = config.getFloat("minimumLiquidLevel");
  m_spawnCheckResolution = config.getFloat("spawnCheckResolution");
  m_spawnSurfaceCheckDistance = config.getInt("spawnSurfaceCheckDistance");
  m_spawnCeilingCheckDistance = config.getInt("spawnCeilingCheckDistance");
  m_spawnProhibitedCheckPadding = config.getFloat("spawnProhibitedCheckPadding");

  m_spawnCellLifetime = config.getFloat("spawnCellLifetime");
  m_windowActivationBorder = config.getUInt("windowActivationBorder");

  m_active = config.getBool("defaultActive", true);

  m_debug = config.getBool("debug", false);
}

void Spawner::init(SpawnerFacadePtr facade) {
  m_facade = move(facade);
}

void Spawner::uninit() {
  for (auto entityId : m_spawnedEntities)
    m_facade->despawnEntity(entityId);
  m_facade.reset();
}

bool Spawner::active() const {
  return m_active;
}

void Spawner::setActive(bool active) {
  m_active = active;
}

void Spawner::activateRegion(RectF region) {
  for (auto const& cell : cellIndexesForRange(region)) {
    if (m_facade && m_facade->signalRegion(cellRegion(cell))) {
      if (m_active && !m_activeSpawnCells.contains(cell))
        spawnInCell(cell);
      m_activeSpawnCells[cell] = m_spawnCellLifetime;
    }
  }
}

void Spawner::activateEmptyRegion(RectF region) {
  for (auto const& cell : cellIndexesForRange(region))
    m_activeSpawnCells[cell] = m_spawnCellLifetime;
}

void Spawner::update() {
  if (!m_facade)
    return;

  for (auto const& window : m_facade->clientWindows()) {
    if (window != RectF())
      activateRegion(window.padded(m_windowActivationBorder));
  }

  eraseWhere(m_activeSpawnCells, [](auto& p) {
      p.second -= WorldTimestep;
      return p.second < 0.0f;
    });

  eraseWhere(m_spawnedEntities, [this](EntityId entityId) {
      auto entity = m_facade->getEntity(entityId);
      if (!entity)
        return true;

      if (!m_activeSpawnCells.contains(cellIndexForPosition(entity->position()))) {
        m_facade->despawnEntity(entity->entityId());
        return true;
      }

      return false;
    });

  if (m_active && m_debug)
    debugShowSpawnCells();
}

Vec2I Spawner::cellIndexForPosition(Vec2F const& position) const {
  return Vec2I::floor(position / m_spawnCellSize);
}

List<Vec2I> Spawner::cellIndexesForRange(RectF const& range) const {
  List<Vec2I> cellIndexes;
  for (auto srange : m_facade->geometry().splitRect(range)) {
    auto indexes = RectI::integral(RectF(srange).scaled(1.0f / m_spawnCellSize));
    for (int x = indexes.xMin(); x < indexes.xMax(); ++x) {
      for (int y = indexes.yMin(); y < indexes.yMax(); ++y)
        cellIndexes.append({x, y});
    }
  }

  return cellIndexes;
}

RectF Spawner::cellRegion(Vec2I const& cellIndex) const {
  return RectF::withSize(Vec2F(cellIndex) * m_spawnCellSize, Vec2F::filled(m_spawnCellSize));
}

Maybe<SpawnParameters> Spawner::spawnParametersForCell(Vec2I const& cellIndex) const {
  unsigned emptyCount = 0;
  unsigned nearSurfaceCount = 0;
  unsigned nearCeilingCount = 0;
  unsigned airCount = 0;
  unsigned liquidCount = 0;
  unsigned exposedCount = 0;

  auto region = RectI::withSize(cellIndex * m_spawnCellSize, Vec2I::filled(m_spawnCellSize));
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y) {
      // Only empty blocks count towards spawn totals
      if (m_facade->collision({x, y}) == CollisionKind::None) {
        ++emptyCount;

        if (m_facade->liquidLevel({x, y}).level > m_minimumLiquidLevel)
          ++liquidCount;

        if (m_facade->isBackgroundEmpty({x, y}))
          ++exposedCount;

        // The empty block will will either count as an air block, a
        // "near-surface" block, or a "near-ceiling" block.  It will count as a
        // near-surface block if it is within the NearSurfaceDistance of a
        // CollsionKind::Block or CollisionKind::Platform block. If it is not a
        // near-surface block, it will count as a near-ceiling block if it is
        // within the NearCeilingDistance of a CollisionKind::Block.
        bool nearSurface = false;
        for (unsigned sd = 1; sd <= m_spawnCellNearSurfaceDistance; ++sd) {
          auto collision = m_facade->collision({x, y - sd});
          if (BlockCollisionSet.contains(collision) || collision == CollisionKind::Platform) {
            nearSurface = true;
            break;
          }
        }

        bool nearCeiling = false;
        if (!nearSurface) {
          for (unsigned cd = 1; cd <= m_spawnCellNearCeilingDistance; ++cd) {
            auto collision = m_facade->collision({x, y + cd});
            if (BlockCollisionSet.contains(collision)) {
              nearCeiling = true;
              break;
            }
          }
        }

        if (nearSurface)
          ++nearSurfaceCount;
        else if (nearCeiling)
          ++nearCeilingCount;
        else
          ++airCount;
      }
    }
  }

  Set<SpawnParameters::Area> spawnAreas;
  if (liquidCount > m_spawnCellMinimumLiquidTiles)
    spawnAreas.add(SpawnParameters::Area::Liquid);
  if (nearSurfaceCount > m_spawnCellMinimumNearSurfaceTiles)
    spawnAreas.add(SpawnParameters::Area::Surface);
  if (nearCeilingCount > m_spawnCellMinimumNearCeilingTiles)
    spawnAreas.add(SpawnParameters::Area::Ceiling);
  if (airCount > m_spawnCellMinimumAirTiles)
    spawnAreas.add(SpawnParameters::Area::Air);
  if (emptyCount < m_spawnCellMinimumEmptyTiles)
    spawnAreas.add(SpawnParameters::Area::Solid);

  if (spawnAreas.empty())
    return {};

  SpawnParameters::Region spawnRegion = SpawnParameters::Region::Enclosed;
  if (exposedCount >= m_spawnCellMinimumExposedTiles)
    spawnRegion = SpawnParameters::Region::Exposed;

  SpawnParameters::Time spawnTime = SpawnParameters::Time::Night;
  if (m_facade->dayLevel() >= m_minimumDayLevel)
    spawnTime = SpawnParameters::Time::Day;

  return SpawnParameters(spawnAreas, spawnRegion, spawnTime);
}

Maybe<Vec2F> Spawner::adjustSpawnRegion(RectF const& spawnRegion, RectF const& boundBox, SpawnParameters const& spawnParameters) const {
  auto checkPosition = [&](Vec2F const& position) -> bool {
    RectF region = RectF(boundBox).translated(position);

    if (!m_facade->isFreeSpace(region))
      return spawnParameters.areas.contains(SpawnParameters::Area::Solid);

    if (m_facade->liquidLevel(Vec2I::floor(region.center())).level >= m_minimumLiquidLevel)
      return spawnParameters.areas.contains(SpawnParameters::Area::Liquid);

    if (m_facade->spawningProhibited(region.padded(m_spawnProhibitedCheckPadding)))
      return false;

    if (spawnParameters.areas.contains(SpawnParameters::Area::Air))
      return true;

    if (spawnParameters.areas.contains(SpawnParameters::Area::Surface)) {
      Vec2F startCheck = {region.center()[0], region.yMin()};
      for (int sd = 0; sd <= m_spawnSurfaceCheckDistance; ++sd) {
        auto collision = m_facade->collision(Vec2I::floor(startCheck - Vec2F(0, sd)));
        if (BlockCollisionSet.contains(collision) || collision == CollisionKind::Platform)
          return true;
      }

    } else if (spawnParameters.areas.contains(SpawnParameters::Area::Ceiling)) {
      Vec2F startCheck = {region.center()[0], region.yMax()};
      for (int cd = 0; cd <= m_spawnCeilingCheckDistance; ++cd) {
        auto collision = m_facade->collision(Vec2I::floor(startCheck + Vec2F(0, cd)));
        if (BlockCollisionSet.contains(collision))
          return true;
      }
    }

    return false;
  };

  List<Vec2F> tryPositions;
  for (float x = spawnRegion.xMin(); x <= spawnRegion.xMax(); x += m_spawnCheckResolution) {
    for (float y = spawnRegion.yMin(); y <= spawnRegion.yMax(); y += m_spawnCheckResolution)
      tryPositions.append({x, y});
  }

  Random::shuffle(tryPositions);
  for (auto const& p : tryPositions) {
    if (checkPosition(p))
      return p;
  }

  return {};
}

void Spawner::spawnInCell(Vec2I const& cell) {
  auto cellSpawnParameters = spawnParametersForCell(cell);
  if (!cellSpawnParameters)
    return;

  if (m_debug)
    m_debugSpawnInfo[cell] = SpawnCellDebugInfo{*cellSpawnParameters, 0, 0};

  auto monsterDatabase = Root::singleton().monsterDatabase();
  auto spawnTypeDatabase = Root::singleton().spawnTypeDatabase();

  RectF spawnRegion = cellRegion(cell);
  auto spawnProfile = m_facade->spawnProfile(spawnRegion.center());

  for (auto const& spawnTypeName : spawnProfile.spawnTypes) {
    auto spawnType = spawnTypeDatabase->spawnType(spawnTypeName);
    if (!spawnType.spawnParameters.compatible(*cellSpawnParameters))
      continue;

    if (Random::randf() < spawnType.spawnChance) {
      uint64_t spawnSeed = staticRandomU64(spawnType.seedMix, m_facade->spawnSeed());
      int targetGroupSize = Random::randInt(spawnType.groupSize[0], spawnType.groupSize[1]);
      for (int i = 0; i < targetGroupSize; ++i) {
        String monsterType;
        if (auto monsterPool = spawnType.monsterType.maybe<WeightedPool<String>>())
          monsterType = monsterPool->select();
        else
          monsterType = spawnType.monsterType.get<String>();

        auto monsterVariant = monsterDatabase->monsterVariant(monsterType, spawnSeed, spawnType.monsterParameters);
        auto monsterBoundBox = monsterVariant.movementSettings.standingPoly->boundBox();

        if (m_debug)
          m_debugSpawnInfo[cell].spawnAttempts++;

        if (auto position = adjustSpawnRegion(spawnRegion, monsterBoundBox, spawnType.spawnParameters)) {
          float level = m_facade->threatLevel();
          if (m_facade->dayLevel() >= m_minimumDayLevel)
            level += Random::randf(spawnType.dayLevelAdjustment[0], spawnType.dayLevelAdjustment[1]);
          else
            level += Random::randf(spawnType.nightLevelAdjustment[0], spawnType.nightLevelAdjustment[1]);

          auto spawnProfile = m_facade->spawnProfile(*position);

          auto entity = monsterDatabase->createMonster(monsterVariant, level, spawnProfile.monsterParameters);
          entity->setPosition(*position);
          entity->setKeepAlive(true);
          auto entityId = m_facade->spawnEntity(entity);
          if (entityId != NullEntityId)
            m_spawnedEntities.add(entityId);

          if (m_debug)
            m_debugSpawnInfo[cell].spawns++;
        }
      }
    }
  }
}

void Spawner::debugShowSpawnCells() {
  eraseWhere(m_debugSpawnInfo, [this](auto& p) {
      return !m_activeSpawnCells.contains(p.first);
    });

  auto regionVisibleToClient = [this](RectF const& region) {
    for (auto const& window : m_facade->clientWindows()) {
      if (m_facade->geometry().rectIntersectsRect(window, region))
        return true;
    }
    return false;
  };

  for (auto const& debugInfo : m_debugSpawnInfo) {
    RectF spawnRegion = Spawner::cellRegion(debugInfo.first);
    if (regionVisibleToClient(spawnRegion)) {
      SpatialLogger::logPoly("world", PolyF(spawnRegion), {128, 0, 0, 255});
      StringList areaList;
      for (auto area : debugInfo.second.spawnParameters.areas)
        areaList.append(SpawnParameters::AreaNames.getRight(area).slice(0, 3));
      SpatialLogger::logText("world", strf("Areas: {}", areaList.join(", ")), spawnRegion.min() + Vec2F(0.5, 2.5), {255, 255, 255, 255});
      SpatialLogger::logText("world", strf("Region: {}", SpawnParameters::RegionNames.getRight(debugInfo.second.spawnParameters.region)), spawnRegion.min() + Vec2F(0.5, 1.5), {255, 255, 255, 255});
      SpatialLogger::logText("world", strf("Time: {}", SpawnParameters::TimeNames.getRight(debugInfo.second.spawnParameters.time)), spawnRegion.min() + Vec2F(0.5, 0.5), {255, 255, 255, 255});

      if (debugInfo.second.spawnAttempts > 0)
        SpatialLogger::logText("world", strf("Spawns: {} / {}", debugInfo.second.spawns, debugInfo.second.spawnAttempts), spawnRegion.min() + Vec2F(0.5, 3.5), (debugInfo.second.spawnAttempts > debugInfo.second.spawns) ? Color::Red.toRgba() : Color::Green.toRgba());
    }
  }
}

}
