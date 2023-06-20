#ifndef STAR_STATUS_EFFECT_ITEM_HPP
#define STAR_STATUS_EFFECT_ITEM_HPP

#include "StarStatusTypes.hpp"

namespace Star {

STAR_CLASS(StatusEffectItem);

class StatusEffectItem {
public:
  virtual ~StatusEffectItem() {}
  virtual List<PersistentStatusEffect> statusEffects() const = 0;
};

}

#endif
