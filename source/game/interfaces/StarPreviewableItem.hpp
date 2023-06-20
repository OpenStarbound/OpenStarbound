#ifndef STAR_PREVIEWABLE_ITEM
#define STAR_PREVIEWABLE_ITEM

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

#endif
