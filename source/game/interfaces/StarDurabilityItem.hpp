#ifndef STAR_DURABILITY_ITEM_HPP
#define STAR_DURABILITY_ITEM_HPP

#include "StarConfig.hpp"

namespace Star {

STAR_CLASS(DurabilityItem);

class DurabilityItem {
public:
  virtual ~DurabilityItem() {}
  virtual float durabilityStatus() = 0;
};

}

#endif
