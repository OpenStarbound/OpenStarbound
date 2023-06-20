#ifndef STAR_PORTRAIT_ENTITY_HPP
#define STAR_PORTRAIT_ENTITY_HPP

#include "StarDrawable.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(PortraitEntity);

class PortraitEntity : public virtual Entity {
public:
  virtual List<Drawable> portrait(PortraitMode mode) const = 0;
  virtual String name() const = 0;
};

}

#endif
