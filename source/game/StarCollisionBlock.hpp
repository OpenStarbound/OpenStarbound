#ifndef STAR_COLLISION_BLOCK_HPP
#define STAR_COLLISION_BLOCK_HPP

#include "StarPoly.hpp"
#include "StarList.hpp"
#include "StarBiMap.hpp"

namespace Star {

enum class CollisionKind : uint8_t {
  // Special collision block that is used for unloaded / un-generated tiles.
  // Collides the same as "Block", but does not tile with it.
  Null,
  None,
  Platform,
  Dynamic,
  Slippery,
  Block
};

enum class TileCollisionOverride : uint8_t {
  None,
  Empty,
  Platform,
  Dynamic
};

inline CollisionKind collisionKindFromOverride(TileCollisionOverride const& over) {
  switch (over) {
    case TileCollisionOverride::Empty:
      return CollisionKind::None;
    case TileCollisionOverride::Platform:
      return CollisionKind::Platform;
    case TileCollisionOverride::Dynamic:
      return CollisionKind::Dynamic;
    default:
      return CollisionKind::Null;
  }
}

class CollisionSet {
public:
  CollisionSet();
  CollisionSet(initializer_list<CollisionKind> kinds);

  void insert(CollisionKind kind);
  void remove(CollisionKind kind);
  bool contains(CollisionKind kind) const;

private:
  static uint8_t kindBit(CollisionKind kind);

  uint8_t m_kinds;
};

// The default CollisionSet consists of Null, Slippery, Dynamic and Block
CollisionSet const DefaultCollisionSet({CollisionKind::Null, CollisionKind::Slippery, CollisionKind::Dynamic, CollisionKind::Block});

// Defines what can be "blocks" e.g. for tile rendering: Block and Slippery
CollisionSet const BlockCollisionSet({CollisionKind::Block, CollisionKind::Slippery});

extern EnumMap<CollisionKind> const CollisionKindNames;

bool isColliding(CollisionKind kind, CollisionSet const& collisionSet);
bool isSolidColliding(CollisionKind kind);

// Returns the highest priority collision kind, where Block > Slippery >
// Dynamic > Platform > None > Null
CollisionKind maxCollision(CollisionKind first, CollisionKind second);

struct CollisionBlock {
  // Make a null collision block for the given space.
  static CollisionBlock nullBlock(Vec2I const& space);

  CollisionKind kind;
  Vec2I space;
  PolyF poly;
  RectF polyBounds;
};

inline CollisionSet::CollisionSet()
  : m_kinds(0) {}

inline CollisionSet::CollisionSet(initializer_list<CollisionKind> kinds)
  : CollisionSet() {
  for (auto kind : kinds) {
    insert(kind);
  }
}

inline void CollisionSet::insert(CollisionKind kind) {
  m_kinds = m_kinds | kindBit(kind);
}

inline void CollisionSet::remove(CollisionKind kind) {
  m_kinds = m_kinds & ~kindBit(kind);
}

inline bool CollisionSet::contains(CollisionKind kind) const {
  return m_kinds & kindBit(kind);
}

inline uint8_t CollisionSet::kindBit(CollisionKind kind) {
  return 1 << ((uint8_t)kind + 1);
}

inline bool isColliding(CollisionKind kind, CollisionSet const& collisionSet) {
  return collisionSet.contains(kind);
}

inline bool isSolidColliding(CollisionKind kind) {
  return isColliding(kind, DefaultCollisionSet);
}

inline CollisionKind maxCollision(CollisionKind first, CollisionKind second) {
  return max(first, second);
}

inline CollisionBlock CollisionBlock::nullBlock(Vec2I const& space) {
  CollisionBlock block;
  block.kind = CollisionKind::Null;
  block.space = space;
  block.poly = {
    Vec2F(space) + Vec2F(0, 0),
    Vec2F(space) + Vec2F(1, 0),
    Vec2F(space) + Vec2F(1, 1),
    Vec2F(space) + Vec2F(0, 1)
  };
  block.polyBounds = RectF::withSize(Vec2F(space), Vec2F(1, 1));
  return block;
}

}

#endif
