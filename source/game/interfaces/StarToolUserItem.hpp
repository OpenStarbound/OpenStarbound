#ifndef STAR_TOOL_USER_ITEM_HPP
#define STAR_TOOL_USER_ITEM_HPP

#include "StarToolUserEntity.hpp"
#include "StarPhysicsEntity.hpp"

namespace Star {

STAR_EXCEPTION(ToolUserItemException, StarException);

STAR_CLASS(ToolUserItem);

// FIXME: You know what another name for an item that a tool user uses is?  A
// Tool.  Three words when one will do, rename.
class ToolUserItem {
public:
  ToolUserItem();
  virtual ~ToolUserItem() = default;

  // Owner must be initialized when a ToolUserItem is initialized and
  // uninitialized before the owner is uninitialized.
  virtual void init(ToolUserEntity* owner, ToolHand hand);
  virtual void uninit();

  // Default implementation does nothing
  virtual void update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves);

  // Default implementations return empty list
  virtual List<DamageSource> damageSources() const;
  virtual List<PolyF> shieldPolys() const;
  virtual List<PhysicsForceRegion> forceRegions() const;

  bool initialized() const;

  // owner, entityMode, hand, and world throw ToolUserException if
  // initialized() is false
  ToolUserEntity* owner() const;
  EntityMode entityMode() const;
  ToolHand hand() const;
  World* world() const;

private:
  ToolUserEntity* m_owner;
  Maybe<ToolHand> m_hand;
};

}

#endif
