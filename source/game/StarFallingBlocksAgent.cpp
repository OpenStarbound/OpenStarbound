#include "StarFallingBlocksAgent.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

FallingBlocksAgent::FallingBlocksAgent(FallingBlocksFacadePtr worldFacade)
  : m_facade(std::move(worldFacade)) {
  m_immediateUpwardPropagateProbability = Root::singleton().assets()->json("/worldserver.config:fallingBlocksImmediateUpwardPropogateProbability").toFloat();
}

void FallingBlocksAgent::update() {
  HashSet<Vec2I> processing = take(m_pending);

  while (!processing.empty()) {
    List<Vec2I> positions;
    for (auto const& pos : take(processing))
      positions.append(pos);

    m_random.shuffle(positions);

    positions.sort([](auto const& a, auto const& b) {
        return a[1] < b[1];
      });

    for (auto const& pos : positions) {
      Vec2I belowPos = pos + Vec2I(0, -1);
      Vec2I belowLeftPos = pos + Vec2I(-1, -1);
      Vec2I belowRightPos = pos + Vec2I(1, -1);

      FallingBlockType thisBlock = m_facade->blockType(pos);
      FallingBlockType belowBlock = m_facade->blockType(belowPos);

      Maybe<Vec2I> moveTo;

      if (thisBlock == FallingBlockType::Falling) {
        if (belowBlock == FallingBlockType::Open)
          moveTo = belowPos;
      } else if (thisBlock == FallingBlockType::Cascading) {
        if (belowBlock == FallingBlockType::Open) {
          moveTo = belowPos;
        } else {
          FallingBlockType belowLeftBlock = m_facade->blockType(belowLeftPos);
          FallingBlockType belowRightBlock = m_facade->blockType(belowRightPos);

          if (belowLeftBlock == FallingBlockType::Open && belowRightBlock == FallingBlockType::Open)
            moveTo = m_random.randb() ? belowLeftPos : belowRightPos;
          else if (belowLeftBlock == FallingBlockType::Open)
            moveTo = belowLeftPos;
          else if (belowRightBlock == FallingBlockType::Open)
            moveTo = belowRightPos;
        }
      }

      if (moveTo) {
        m_facade->moveBlock(pos, *moveTo);
        if (m_random.randf() < m_immediateUpwardPropagateProbability) {
          processing.add(pos + Vec2I(0, 1));
          processing.add(pos + Vec2I(-1, 1));
          processing.add(pos + Vec2I(1, 1));
        }

        visitLocation(pos);
        visitLocation(*moveTo);
      }
    }
  }
}

void FallingBlocksAgent::visitLocation(Vec2I const& location) {
  visitRegion(RectI::withSize(location, Vec2I(1, 1)));
}

void FallingBlocksAgent::visitRegion(RectI const& region) {
  for (int x = region.xMin() - 1; x <= region.xMax(); ++x) {
    for (int y = region.yMin(); y <= region.yMax(); ++y) 
      m_pending.add({x, y});
  }
}

}
