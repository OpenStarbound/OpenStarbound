#pragma once

#include "StarDrawable.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(PreviewableItem);

class PreviewableItem {
public:
  virtual ~PreviewableItem() {}
  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const = 0;
};

}
