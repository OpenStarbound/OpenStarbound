#ifndef STAR_CONSUMABLE_ITEM_HPP
#define STAR_CONSUMABLE_ITEM_HPP

#include "StarItem.hpp"
#include "StarGameTypes.hpp"
#include "StarSwingableItem.hpp"

namespace Star {

class ConsumableItem : public Item, public SwingableItem {
public:
  ConsumableItem(Json const& config, String const& directory, Json const& data);

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;

  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void fireTriggered() override;
  void uninit() override;

private:
  bool canUse() const;

  void triggerEffects();
  void maybeConsume();

  StringSet m_blockingEffects;
  Maybe<float> m_foodValue;
  StringSet m_emitters;
  String m_emote;
  bool m_consuming;
};

}

#endif
