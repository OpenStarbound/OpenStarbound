#ifndef STAR_NON_ROTATED_DRAWABLES_ITEM_HPP
#define STAR_NON_ROTATED_DRAWABLES_ITEM_HPP

#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(NonRotatedDrawablesItem);

class NonRotatedDrawablesItem {
public:
  virtual ~NonRotatedDrawablesItem() {}
  virtual List<Drawable> nonRotatedDrawables() const = 0;
};

}

#endif
