#pragma once

#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(NametagEntity);

class NametagEntity : public virtual Entity {
public:
  virtual String name() const = 0;
  virtual Maybe<String> statusText() const = 0;
  virtual bool displayNametag() const = 0;
  virtual Vec3B nametagColor() const = 0;
  virtual Vec2F nametagOrigin() const = 0;
};

}
