#ifndef STAR_ANCHORABLE_ENTITY_HPP
#define STAR_ANCHORABLE_ENTITY_HPP

#include "StarEntity.hpp"

namespace Star {

STAR_STRUCT(EntityAnchor);

struct EntityAnchor {
  virtual ~EntityAnchor() = default;

  Vec2F position;
  // If set, the entity should place the bottom center of its collision poly on
  // the given position at exit
  Maybe<Vec2F> exitBottomPosition;
  Direction direction;
  float angle;
};

struct EntityAnchorState {
  EntityId entityId;
  size_t positionIndex;

  bool operator==(EntityAnchorState const& eas) const;
};

DataStream& operator>>(DataStream& ds, EntityAnchorState& anchorState);
DataStream& operator<<(DataStream& ds, EntityAnchorState const& anchorState);

class AnchorableEntity : public virtual Entity {
public:
  virtual size_t anchorCount() const = 0;
  virtual EntityAnchorConstPtr anchor(size_t anchorPositionIndex) const = 0;
};

}

#endif
