#ifndef STAR_FALLING_BLOCKS_AGENT_HPP
#define STAR_FALLING_BLOCKS_AGENT_HPP

#include "StarVector.hpp"
#include "StarSet.hpp"
#include "StarMap.hpp"
#include "StarRandom.hpp"
#include "StarGameTypes.hpp"
#include "StarWorldTiles.hpp"

namespace Star {

STAR_CLASS(MaterialDatabase);
STAR_CLASS(FallingBlocksFacade);
STAR_CLASS(FallingBlocksAgent);

enum class FallingBlockType {
  Immovable,
  Falling,
  Cascading,
  Open
};

class FallingBlocksFacade {
public:
  virtual ~FallingBlocksFacade() = default;

  virtual FallingBlockType blockType(Vec2I const& pos) = 0;
  virtual void moveBlock(Vec2I const& from, Vec2I const& to) = 0;
};

class FallingBlocksAgent {
public:
  FallingBlocksAgent(FallingBlocksFacadePtr worldFacade);

  void update();

  void visitLocation(Vec2I const& location);
  void visitRegion(RectI const& region);

private:
  FallingBlocksFacadePtr m_facade;
  float m_immediateUpwardPropagateProbability;
  HashSet<Vec2I> m_pending;
  RandomSource m_random;
};

}

#endif
