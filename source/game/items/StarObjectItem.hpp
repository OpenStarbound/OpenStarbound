#ifndef STAR_OBJECT_ITEM_HPP
#define STAR_OBJECT_ITEM_HPP

#include "StarItem.hpp"
#include "StarFireableItem.hpp"
#include "StarBeamItem.hpp"

namespace Star {

STAR_CLASS(ObjectItem);

class ObjectItem : public Item, public FireableItem, public BeamItem {
public:
  ObjectItem(Json const& config, String const& directory, Json const& objectParameters);
  virtual ~ObjectItem() {}

  ItemPtr clone() const override;

  void init(ToolUserEntity* owner, ToolHand hand) override;
  void update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  List<Drawable> nonRotatedDrawables() const override;

  float cooldownTime() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;

  String objectName() const;
  Json objectParameters() const;

  bool placeInWorld(FireMode mode, bool shifting);
  bool canPlace(bool shifting) const;

private:
  bool m_shifting;
};

}

#endif
