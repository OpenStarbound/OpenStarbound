#pragma once

#include "StarPoly.hpp"
#include "StarVariant.hpp"
#include "StarJson.hpp"
#include "StarEntity.hpp"
#include "StarForceRegions.hpp"
#include "StarCollisionBlock.hpp"

namespace Star {

STAR_CLASS(PhysicsEntity);

struct PhysicsMovingCollision {
  static PhysicsMovingCollision fromJson(Json const& json);

  RectF boundBox() const;

  void translate(Vec2F const& pos);

  bool operator==(PhysicsMovingCollision const& rhs) const;

  Vec2F position;
  PolyF collision;
  CollisionKind collisionKind;
  PhysicsCategoryFilter categoryFilter;
};

DataStream& operator>>(DataStream& ds, PhysicsMovingCollision& pmc);
DataStream& operator<<(DataStream& ds, PhysicsMovingCollision const& pmc);

struct MovingCollisionId {
  MovingCollisionId();
  MovingCollisionId(EntityId physicsEntityId, size_t collisionIndex);

  bool operator==(MovingCollisionId const& rhs);

  // Returns true if the MovingCollisionId is not empty, i.e. default
  // constructed
  bool valid() const;
  operator bool() const;

  EntityId physicsEntityId;
  size_t collisionIndex;
};

DataStream& operator>>(DataStream& ds, MovingCollisionId& mci);
DataStream& operator<<(DataStream& ds, MovingCollisionId const& mci);

class PhysicsEntity : public virtual Entity {
public:
  virtual List<PhysicsForceRegion> forceRegions() const;

  virtual size_t movingCollisionCount() const;
  virtual Maybe<PhysicsMovingCollision> movingCollision(size_t positionIndex) const;
};

}
