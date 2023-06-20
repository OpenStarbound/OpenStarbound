#ifndef STAR_STATUS_EFFECT_ENTITY_HPP
#define STAR_STATUS_EFFECT_ENTITY_HPP

#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(StatusEffectEntity);

class StatusEffectEntity : public virtual Entity {
public:
  virtual List<PersistentStatusEffect> statusEffects() const = 0;
  virtual PolyF statusEffectArea() const = 0;
};

}

#endif
