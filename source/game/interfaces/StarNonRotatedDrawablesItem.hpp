#pragma once

#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(NonRotatedDrawablesItem);

class NonRotatedDrawablesItem {
public:
  virtual ~NonRotatedDrawablesItem() {}
  virtual List<Drawable> nonRotatedDrawables() const = 0;
};

}
