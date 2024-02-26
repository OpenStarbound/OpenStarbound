#pragma once

#include "StarPortraitEntity.hpp"

namespace Star {

STAR_CLASS(DamageBarEntity);

enum class DamageBarType : uint8_t {
  Default,
  None,
  Special
};
extern EnumMap<DamageBarType> const DamageBarTypeNames;

class DamageBarEntity : public virtual PortraitEntity {
public:
  virtual float health() const = 0;
  virtual float maxHealth() const = 0;
  virtual String name() const = 0;
  virtual DamageBarType damageBar() const = 0;
};

}
