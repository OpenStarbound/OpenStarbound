#pragma once

#include "StarConfig.hpp"

namespace Star {

STAR_CLASS(DurabilityItem);

class DurabilityItem {
public:
  virtual ~DurabilityItem() {}
  virtual float durabilityStatus() = 0;
};

}
