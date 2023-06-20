#ifndef STAR_AGGRESSIVE_ENTITY_HPP
#define STAR_AGGRESSIVE_ENTITY_HPP

#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(AggressiveEntity);

class AggressiveEntity : public virtual Entity {
public:
  virtual bool aggressive() const = 0;
};

}

#endif
