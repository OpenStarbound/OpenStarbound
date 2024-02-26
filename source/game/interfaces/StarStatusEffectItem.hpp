#pragma once

#include "StarStatusTypes.hpp"

namespace Star {

STAR_CLASS(StatusEffectItem);

class StatusEffectItem {
public:
  virtual ~StatusEffectItem() {}
  virtual List<PersistentStatusEffect> statusEffects() const = 0;
};

}
