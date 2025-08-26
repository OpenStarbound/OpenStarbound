#pragma once

#include "StarDrawable.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(PortraitEntity);

class PortraitEntity : public virtual Entity {
public:
  virtual List<Drawable> portrait(PortraitMode mode) const = 0;
};

}
