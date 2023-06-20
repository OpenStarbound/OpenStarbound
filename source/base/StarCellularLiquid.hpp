#ifndef STAR_CELLULAR_LIQUID_HPP
#define STAR_CELLULAR_LIQUID_HPP

#include "StarVariant.hpp"
#include "StarRect.hpp"
#include "StarMultiArray.hpp"
#include "StarMap.hpp"
#include "StarOrderedSet.hpp"
#include "StarRandom.hpp"
#include "StarBlockAllocator.hpp"

namespace Star {

struct CellularLiquidCollisionCell {};

template <typename LiquidId>
struct CellularLiquidFlowCell {
  Maybe<LiquidId> liquid;
  float level;
  float pressure;
};

template <typename LiquidId>
struct CellularLiquidSourceCell {
  LiquidId liquid;
  float pressure;
};

template <typename LiquidId>
using CellularLiquidCell = Variant<CellularLiquidCollisionCell, CellularLiquidFlowCell<LiquidId>, CellularLiquidSourceCell<LiquidId>>;

template <typename LiquidId>
struct CellularLiquidWorld {
  virtual ~CellularLiquidWorld();

  virtual Vec2I uniqueLocation(Vec2I const& location) const;

  virtual CellularLiquidCell<LiquidId> cell(Vec2I const& location) const = 0;

  // Should return an amount between 0.0 and 1.0 as a percentage of liquid
  // drain at this position
  virtual float drainLevel(Vec2I const& location) const;

  // Will be called only on cells which for which the cell method returned a
  // flow cell, to update the flow cell.
  virtual void setFlow(Vec2I const& location, CellularLiquidFlowCell<LiquidId> const& flow) = 0;

  // Called once for every active liquid <-> liquid interaction of different
  // liquid types each update.  Will be called AFTER pushing all the flow
  // values back out so modifications to liquids are sensible.
  virtual void liquidInteraction(Vec2I const& a, LiquidId aLiquid, Vec2I const& b, LiquidId bLiquid);
  // Called once for every liquid collision each update.  Also called after
  // pushing all the flow values out, so changes to liquids can sensibly be
  // performed here.
  virtual void liquidCollision(Vec2I const& pos, LiquidId liquid, Vec2I const& collisionPos);
};

struct LiquidCellEngineParameters {
  float lateralMoveFactor;
  float spreadOverfillUpFactor;
  float spreadOverfillLateralFactor;
  float spreadOverfillDownFactor;
  float pressureEqualizeFactor;
  float pressureMoveFactor;
  float maximumPressureLevelImbalance;
  float minimumLivenPressureChange;
  float minimumLivenLevelChange;
  float minimumLiquidLevel;
  float interactTransformationLevel;
};

template <typename LiquidId>
class LiquidCellEngine {
public:
  typedef shared_ptr<CellularLiquidWorld<LiquidId>> CellularLiquidWorldPtr;

  LiquidCellEngine(LiquidCellEngineParameters parameters, CellularLiquidWorldPtr cellWorld);

  unsigned liquidTickDelta(LiquidId liquid);
  void setLiquidTickDelta(LiquidId liquid, unsigned tickDelta);

  void setProcessingLimit(Maybe<unsigned> processingLimit);

  List<RectI> noProcessingLimitRegions() const;
  void setNoProcessingLimitRegions(List<RectI> noProcessingLimitRegions);

  void visitLocation(Vec2I const& location);
  void visitRegion(RectI const& region);

  void update();

  size_t activeCells() const;
  size_t activeCells(LiquidId liquid) const;
  bool isActive(Vec2I const& pos) const;

private:
  enum class Adjacency {
    Left,
    Right,
    Bottom,
    Top
  };

  struct WorkingCell {
    Vec2I position;
    Maybe<LiquidId> liquid;
    bool sourceCell;
    float level;
    float pressure;

    WorkingCell* leftCell;
    WorkingCell* rightCell;
    WorkingCell* topCell;
    WorkingCell* bottomCell;
  };

  template <typename Key, typename Value>
  using BAHashMap = StableHashMap<Key, Value, hash<Key>, std::equal_to<Key>, BlockAllocator<pair<Key const, Value>, 4096>>;

  template <typename Value>
  using BAHashSet = HashSet<Value, hash<Value>, std::equal_to<Value>>;

  template <typename Value>
  using BAOrderedHashSet = OrderedHashSet<Value, hash<Value>, std::equal_to<Value>, BlockAllocator<Value, 4096>>;

  void setup();
  void applyPressure();
  void spreadPressure();
  void limitPressure();
  void pressureMove();
  void spreadOverfill();
  void levelMove();
  void findInteractions();
  void finish();

  WorkingCell* workingCell(Vec2I p);
  WorkingCell* adjacentCell(WorkingCell* cell, Adjacency adjacency);

  void setPressure(float pressure, WorkingCell& cell);
  void transferPressure(float amount, WorkingCell& source, WorkingCell& dest, bool allowReverse);
  void transferLevel(float amount, WorkingCell& source, WorkingCell& dest, bool allowReverse);
  void setLevel(float level, WorkingCell& cell);

  RandomSource m_random;
  LiquidCellEngineParameters m_engineParameters;
  CellularLiquidWorldPtr m_cellWorld;

  BAHashMap<LiquidId, BAOrderedHashSet<Vec2I>> m_activeCells;
  BAHashMap<LiquidId, unsigned> m_liquidTickDeltas;
  Maybe<unsigned> m_processingLimit;
  List<RectI> m_noProcessingLimitRegions;
  uint64_t m_step;

  BAHashMap<Vec2I, Maybe<WorkingCell>> m_workingCells;
  List<WorkingCell*> m_currentActiveCells;
  BAHashSet<Vec2I> m_nextActiveCells;
  BAHashSet<tuple<Vec2I, LiquidId, Vec2I, LiquidId>> m_liquidInteractions;
  BAHashSet<tuple<Vec2I, LiquidId, Vec2I>> m_liquidCollisions;
};

template <typename LiquidId>
CellularLiquidWorld<LiquidId>::~CellularLiquidWorld() {}

template <typename LiquidId>
Vec2I CellularLiquidWorld<LiquidId>::uniqueLocation(Vec2I const& location) const {
  return location;
}

template <typename LiquidId>
float CellularLiquidWorld<LiquidId>::drainLevel(Vec2I const&) const {
  return 0.0f;
}

template <typename LiquidId>
void CellularLiquidWorld<LiquidId>::liquidInteraction(Vec2I const&, LiquidId, Vec2I const&, LiquidId) {}

template <typename LiquidId>
void CellularLiquidWorld<LiquidId>::liquidCollision(Vec2I const&, LiquidId, Vec2I const&) {}

template <typename LiquidId>
LiquidCellEngine<LiquidId>::LiquidCellEngine(LiquidCellEngineParameters parameters, CellularLiquidWorldPtr cellWorld)
  : m_engineParameters(parameters), m_cellWorld(cellWorld), m_step(0) {}

template <typename LiquidId>
unsigned LiquidCellEngine<LiquidId>::liquidTickDelta(LiquidId liquid) {
  return m_liquidTickDeltas.value(liquid, 1);
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setLiquidTickDelta(LiquidId liquid, unsigned tickDelta) {
  m_liquidTickDeltas[liquid] = tickDelta;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setProcessingLimit(Maybe<unsigned> processingLimit) {
  m_processingLimit = processingLimit;
}

template <typename LiquidId>
List<RectI> LiquidCellEngine<LiquidId>::noProcessingLimitRegions() const {
  return m_noProcessingLimitRegions;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setNoProcessingLimitRegions(List<RectI> noProcessingLimitRegions) {
  m_noProcessingLimitRegions = noProcessingLimitRegions;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::visitLocation(Vec2I const& p) {
  m_nextActiveCells.add(p);
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::visitRegion(RectI const& region) {
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y)
      m_nextActiveCells.add({x, y});
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::update() {
  setup();
  applyPressure();
  spreadPressure();
  limitPressure();
  pressureMove();
  spreadOverfill();
  levelMove();
  findInteractions();
  finish();

  ++m_step;
}

template <typename LiquidId>
size_t LiquidCellEngine<LiquidId>::activeCells() const {
  size_t totalSize = 0;
  for (auto const& p : m_activeCells)
    totalSize += p.second.size();
  return totalSize;
}

template <typename LiquidId>
size_t LiquidCellEngine<LiquidId>::activeCells(LiquidId liquid) const {
  return m_activeCells.value(liquid).size();
}

template <typename LiquidId>
bool LiquidCellEngine<LiquidId>::isActive(Vec2I const& pos) const {
  for (auto const& p : m_activeCells) {
    if (p.second.contains(pos))
      return true;
  }
  return false;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setup() {
  // In case an exception occurred during the last update, clear potentially
  // stale data here
  m_workingCells.clear();
  m_currentActiveCells.clear();

  for (auto& activeCellsPair : m_activeCells) {
    unsigned tickDelta = liquidTickDelta(activeCellsPair.first);
    if (tickDelta == 0 || m_step % tickDelta != 0)
      continue;

    size_t limitedCellNumber = 0;
    for (auto const& pos : activeCellsPair.second.values()) {
      if (m_processingLimit) {
        bool foundInUnlimitedRegion = false;
        for (auto const& region : m_noProcessingLimitRegions) {
          if (region.contains(pos)) {
            foundInUnlimitedRegion = true;
            break;
          }
        }

        if (!foundInUnlimitedRegion) {
          if (limitedCellNumber < *m_processingLimit)
            ++limitedCellNumber;
          else
            continue;
        }
      }

      auto cell = workingCell(pos);
      if (!cell || cell->liquid != activeCellsPair.first) {
        activeCellsPair.second.remove(pos);
      } else {
        m_currentActiveCells.append(cell);
        activeCellsPair.second.remove(pos);
      }
    }
  }

  sort(m_currentActiveCells, [](WorkingCell* lhs, WorkingCell* rhs) {
      return lhs->position[1] < rhs->position[1];
    });
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::applyPressure() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid || selfCell->sourceCell)
      continue;

    auto topCell = adjacentCell(selfCell, Adjacency::Top);
    if (topCell && selfCell->liquid == topCell->liquid)
      setPressure(max(selfCell->pressure, topCell->pressure + min(topCell->level, 1.0f)), *selfCell);
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::spreadPressure() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid)
      continue;

    auto spreadPressure = [&](Adjacency adjacency, float bias) {
      auto targetCell = adjacentCell(selfCell, adjacency);
      if (targetCell && !targetCell->sourceCell)
        transferPressure((selfCell->pressure + bias - targetCell->pressure) * m_engineParameters.pressureEqualizeFactor, *selfCell, *targetCell, true);
    };

    if (m_random.randb()) {
      spreadPressure(Adjacency::Left, 0.0f);
      spreadPressure(Adjacency::Right, 0.0f);
    } else {
      spreadPressure(Adjacency::Right, 0.0f);
      spreadPressure(Adjacency::Left, 0.0f);
    }

    spreadPressure(Adjacency::Bottom, 1.0f);
    spreadPressure(Adjacency::Top, -1.0f);
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::limitPressure() {
  for (auto const& selfCell : m_currentActiveCells) {
    float level = min(selfCell->level, 1.0f);
    auto topCell = adjacentCell(selfCell, Adjacency::Top);

    // Force the pressure to the cell level if there is empty space above,
    // otherwise simply make sure the pressure is at least the level
    if (topCell && !topCell->liquid)
      setPressure(level, *selfCell);
    else
      setPressure(max(selfCell->pressure, level), *selfCell);
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::pressureMove() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid)
      continue;

    auto pressureMove = [&](Adjacency adjacency) {
      auto targetCell = adjacentCell(selfCell, adjacency);
      if (targetCell && !targetCell->sourceCell && targetCell->level >= selfCell->level) {
        float amount = (selfCell->pressure - targetCell->pressure) * m_engineParameters.pressureMoveFactor;
        amount = min(amount, selfCell->level - (1.0f - m_engineParameters.maximumPressureLevelImbalance));
        amount = min(amount, (1.0f + m_engineParameters.maximumPressureLevelImbalance) - targetCell->level);
        transferLevel(amount, *selfCell, *targetCell, false);
      }
    };

    if (m_random.randb()) {
      pressureMove(Adjacency::Left);
      pressureMove(Adjacency::Right);
    } else {
      pressureMove(Adjacency::Right);
      pressureMove(Adjacency::Left);
    }
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::spreadOverfill() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid || selfCell->sourceCell)
      continue;

    auto spreadOverfill = [&](Adjacency adjacency, float factor) {
      float overfill = selfCell->level - 1.0f;
      if (overfill > 0.0f) {
        auto targetCell = adjacentCell(selfCell, adjacency);
        if (targetCell)
          transferLevel(min(overfill, (selfCell->level - targetCell->level)) * factor, *selfCell, *targetCell, false);
      }
    };

    spreadOverfill(Adjacency::Top, m_engineParameters.spreadOverfillUpFactor);

    if (m_random.randb()) {
      spreadOverfill(Adjacency::Left, m_engineParameters.spreadOverfillLateralFactor);
      spreadOverfill(Adjacency::Right, m_engineParameters.spreadOverfillLateralFactor);
    } else {
      spreadOverfill(Adjacency::Right, m_engineParameters.spreadOverfillLateralFactor);
      spreadOverfill(Adjacency::Left, m_engineParameters.spreadOverfillLateralFactor);
    }

    spreadOverfill(Adjacency::Bottom, m_engineParameters.spreadOverfillDownFactor);
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::levelMove() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid)
      continue;

    auto belowCell = adjacentCell(selfCell, Adjacency::Bottom);
    if (belowCell)
      transferLevel(min(1.0f - belowCell->level, selfCell->level), *selfCell, *belowCell, false);

    setLevel(selfCell->level * (1.0f - m_cellWorld->drainLevel(selfCell->position)), *selfCell);

    auto lateralMove = [&](Adjacency adjacency) {
      auto targetCell = adjacentCell(selfCell, adjacency);
      if (targetCell)
        transferLevel((selfCell->level - targetCell->level) * m_engineParameters.lateralMoveFactor, *selfCell, *targetCell, false);
    };

    if (m_random.randb()) {
      lateralMove(Adjacency::Left);
      lateralMove(Adjacency::Right);
    } else {
      lateralMove(Adjacency::Right);
      lateralMove(Adjacency::Left);
    }
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::findInteractions() {
  for (auto const& selfCell : m_currentActiveCells) {
    if (!selfCell->liquid)
      continue;

    for (auto adjacency : {Adjacency::Bottom, Adjacency::Top, Adjacency::Left, Adjacency::Right}) {
      auto targetCell = adjacentCell(selfCell, adjacency);
      if (!targetCell) {
        Vec2I adjacentPos = selfCell->position;
        if (adjacency == Adjacency::Left)
          adjacentPos += Vec2I(-1, 0);
        else if (adjacency == Adjacency::Right)
          adjacentPos += Vec2I(1, 0);
        else if (adjacency == Adjacency::Bottom)
          adjacentPos += Vec2I(0, -1);
        else if (adjacency == Adjacency::Top)
          adjacentPos += Vec2I(0, 1);
        m_liquidCollisions.add(make_tuple(selfCell->position, *selfCell->liquid, adjacentPos));

      } else if (targetCell->liquid && *targetCell->liquid != *selfCell->liquid) {
        if (targetCell->level <= m_engineParameters.interactTransformationLevel
            || selfCell->level <= m_engineParameters.interactTransformationLevel) {
          if (selfCell->level > targetCell->level)
            targetCell->liquid = selfCell->liquid;
          else
            selfCell->liquid = targetCell->liquid;
        } else {
          // Make sure to add the point pair in a predictable order so that any
          // combination of Vec2I points will be unique in m_liquidInteractions
          if (selfCell->position < targetCell->position)
            m_liquidInteractions.add(make_tuple(selfCell->position, *selfCell->liquid, targetCell->position, *targetCell->liquid));
          else
            m_liquidInteractions.add(make_tuple(targetCell->position, *targetCell->liquid, selfCell->position, *selfCell->liquid));
        }
      }
    }
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::finish() {
  m_currentActiveCells.clear();

  for (auto& workingCellPair : take(m_workingCells)) {
    if (workingCellPair.second && !workingCellPair.second->sourceCell) {
      if (workingCellPair.second->liquid) {
        if (workingCellPair.second->level < m_engineParameters.minimumLiquidLevel)
          workingCellPair.second->level = 0.0f;
      } else {
        workingCellPair.second->level = 0.0f;
      }

      if (workingCellPair.second->level == 0.0f) {
        workingCellPair.second->liquid = {};
        workingCellPair.second->pressure = 0.0f;
      }

      m_cellWorld->setFlow(workingCellPair.second->position, CellularLiquidFlowCell<LiquidId>{
          workingCellPair.second->liquid, workingCellPair.second->level, workingCellPair.second->pressure});
    }
  }

  for (auto const& interaction : take(m_liquidInteractions))
    m_cellWorld->liquidInteraction(get<0>(interaction), get<1>(interaction), get<2>(interaction), get<3>(interaction));

  for (auto const& interaction : take(m_liquidCollisions))
    m_cellWorld->liquidCollision(get<0>(interaction), get<1>(interaction), get<2>(interaction));

  for (auto const& c : take(m_nextActiveCells)) {
    auto visit = [this](Vec2I p) {
      p = m_cellWorld->uniqueLocation(p);
      auto cell = workingCell(p);
      if (cell && cell->liquid)
        m_activeCells[*cell->liquid].add(p);
    };

    visit(c);
    visit(c + Vec2I(-1, 0));
    visit(c + Vec2I(1, 0));
    visit(c + Vec2I(0, -1));
    visit(c + Vec2I(0, 1));
  }

  eraseWhere(m_activeCells, [](auto const& p) {
      return p.second.empty();
    });
}

template <typename LiquidId>
typename LiquidCellEngine<LiquidId>::WorkingCell* LiquidCellEngine<LiquidId>::workingCell(Vec2I p) {
  p = m_cellWorld->uniqueLocation(p);

  auto res = m_workingCells.insert(make_pair(p, Maybe<WorkingCell>()));
  if (res.second) {
    auto cellData = m_cellWorld->cell(p);
    if (auto flowCell = cellData.template ptr<CellularLiquidFlowCell<LiquidId>>())
      res.first->second = WorkingCell{p, flowCell->liquid, false, flowCell->level, flowCell->pressure, nullptr, nullptr, nullptr, nullptr};
    else if (auto sourceCell = cellData.template ptr<CellularLiquidSourceCell<LiquidId>>())
      res.first->second = WorkingCell{p, sourceCell->liquid, true, 1.0f, sourceCell->pressure, nullptr, nullptr, nullptr, nullptr};
  }
  return res.first->second.ptr();
}

template <typename LiquidId>
typename LiquidCellEngine<LiquidId>::WorkingCell* LiquidCellEngine<LiquidId>::adjacentCell(
    WorkingCell* cell, Adjacency adjacency) {
  auto getCell = [this](WorkingCell*& cellptr, Vec2I cellPos) {
    if (cellptr)
      return cellptr;
    cellptr = workingCell(cellPos);
    return cellptr;
  };

  if (adjacency == Adjacency::Left)
    return getCell(cell->leftCell, cell->position + Vec2I(-1, 0));
  else if (adjacency == Adjacency::Right)
    return getCell(cell->rightCell, cell->position + Vec2I(1, 0));
  else if (adjacency == Adjacency::Bottom)
    return getCell(cell->bottomCell, cell->position + Vec2I(0, -1));
  else if (adjacency == Adjacency::Top)
    return getCell(cell->topCell, cell->position + Vec2I(0, 1));

  return nullptr;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setPressure(float pressure, WorkingCell& cell) {
  if (!cell.liquid || cell.sourceCell)
    return;

  if (fabs(cell.pressure - pressure) > m_engineParameters.minimumLivenPressureChange)
    m_nextActiveCells.add(cell.position);
  cell.pressure = pressure;
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::transferPressure(float amount, WorkingCell& source, WorkingCell& dest, bool allowReverse) {
  if (amount < 0.0f && allowReverse) {
    return transferPressure(-amount, dest, source, false);
  } else if (amount > 0.0f) {
    if (!source.liquid)
      return;

    if (source.sourceCell && dest.sourceCell)
      return;

    if (dest.liquid && dest.liquid != source.liquid)
      return;

    amount = min(amount, source.pressure);

    if (!source.sourceCell)
      source.pressure -= amount;

    if (dest.liquid && !dest.sourceCell)
      dest.pressure += amount;

    if (amount > m_engineParameters.minimumLivenPressureChange) {
      m_nextActiveCells.add(source.position);
      m_nextActiveCells.add(dest.position);
    }
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::setLevel(float level, WorkingCell& cell) {
  if (!cell.liquid || cell.sourceCell)
    return;

  if (fabs(cell.level - level) > m_engineParameters.minimumLivenLevelChange)
    m_nextActiveCells.add(cell.position);

  cell.level = level;

  if (cell.level <= 0.0f) {
    cell.liquid = {};
    cell.level = 0.0f;
  }
}

template <typename LiquidId>
void LiquidCellEngine<LiquidId>::transferLevel(
    float amount, WorkingCell& source, WorkingCell& dest, bool allowReverse) {
  if (amount < 0.0f && allowReverse) {
    transferLevel(-amount, dest, source, false);

  } else if (amount > 0.0f) {
    if (!source.liquid)
      return;

    if (source.sourceCell && dest.sourceCell)
      return;

    if (dest.liquid && dest.liquid != source.liquid)
      return;

    amount = min(amount, source.level);
    if (!source.sourceCell)
      source.level -= amount;

    if (!dest.sourceCell) {
      dest.level += amount;
      dest.liquid = source.liquid;
    }

    if (!source.sourceCell && source.level == 0.0f)
      source.liquid = {};

    if (amount > m_engineParameters.minimumLivenLevelChange) {
      m_nextActiveCells.add(source.position);
      m_nextActiveCells.add(dest.position);
    }
  }
}

}

#endif
