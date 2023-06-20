#ifndef STAR_ACTIVATABLE_ITEM_HPP
#define STAR_ACTIVATABLE_ITEM_HPP

#include "StarConfig.hpp"

namespace Star {

STAR_CLASS(ActivatableItem);

class ActivatableItem {
public:
  virtual ~ActivatableItem() {}
  virtual bool active() const = 0;
  virtual void setActive(bool active) = 0;
  virtual bool usable() const = 0;
  virtual void activate() = 0;
};

}

#endif
