#pragma once

#include "StarCollisionBlock.hpp"
#include "StarMultiArray.hpp"

#include <functional>

namespace Star {

STAR_CLASS(CollisionGenerator);

// Turns cell geometry into "smoothed" polygonal geometry.  Used by World to
// generate ramps and slopes based on tiles.
class CollisionGenerator {
public:
  // The maximum number of spaces away from a block that can influence the
  // collision geometry of a given block.
  static int const BlockInfluenceRadius = 2;

  // The Maximum number of blocks that will be generated for a single tile
  // space.
  static size_t const MaximumCollisionsPerSpace = 4;

  // Callback function to tell what kind of collision geometry is in a cell.
  // Will be called up to BlockInfluenceRadius outside of the given query
  // region.
  typedef std::function<CollisionKind(int, int)> CollisionKindAccessor;

  void init(CollisionKindAccessor accessor);

  // Get collision geometry for the given block region.
  List<CollisionBlock> getBlocks(RectI const& region) const;

private:
  void getBlocksPlatforms(List<CollisionBlock>& output, RectI const& region, CollisionKind kind) const;
  void getBlocksMarchingSquares(List<CollisionBlock>& output, RectI const& region, CollisionKind kind) const;

  void populateCollisionBuffer(RectI const& region) const;
  CollisionKind collisionKind(int x, int y) const;

  CollisionKindAccessor m_accessor;

  mutable Vec2I m_collisionBufferCorner;
  mutable MultiArray<CollisionKind, 2> m_collisionBuffer;
};

}
