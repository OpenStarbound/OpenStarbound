#pragma once

#include "StarString.hpp"

namespace Star {

STAR_CLASS(EffectSourceItem);

class EffectSourceItem {
public:
  virtual ~EffectSourceItem() {}
  virtual StringSet effectSources() const = 0;
};

}
