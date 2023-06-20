#ifndef STAR_EFFECT_SOURCE_ITEM_HPP
#define STAR_EFFECT_SOURCE_ITEM_HPP

#include "StarString.hpp"

namespace Star {

STAR_CLASS(EffectSourceItem);

class EffectSourceItem {
public:
  virtual ~EffectSourceItem() {}
  virtual StringSet effectSources() const = 0;
};

}

#endif
