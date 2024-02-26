#pragma once

#include "StarHumanoid.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(EmoteEntity);

class EmoteEntity : public virtual Entity {
public:
  virtual void playEmote(HumanoidEmote emote) = 0;
};

}
