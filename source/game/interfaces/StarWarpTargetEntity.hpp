#ifndef STAR_WARP_TARGET_ENTITY_HPP
#define STAR_WARP_TARGET_ENTITY_HPP

#include "StarWarping.hpp"
#include "StarTileEntity.hpp"

namespace Star {

STAR_CLASS(WarpTargetEntity);

class WarpTargetEntity : public virtual TileEntity {
public:
  // Foot position for things teleporting onto this entity, relative to root
  // position.
  virtual Vec2F footPosition() const = 0;
};

}

#endif
