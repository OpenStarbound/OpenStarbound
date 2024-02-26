#pragma once

#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(AggressiveEntity);

class AggressiveEntity : public virtual Entity {
public:
  virtual bool aggressive() const = 0;
};

}
