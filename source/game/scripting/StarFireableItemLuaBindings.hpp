#pragma once

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(FireableItem);

namespace LuaBindings {
  LuaCallbacks makeFireableItemCallbacks(FireableItem* fireableItem);

  namespace FireableItemCallbacks {
    void fire(FireableItem* fireableItem, Maybe<String> const& mode);
    void triggerCooldown(FireableItem* fireableItem);
    void setCooldown(FireableItem* fireableItem, float cooldownTime);
    void endCooldown(FireableItem* fireableItem);
    float cooldownTime(FireableItem* fireableItem);
    Json fireableParam(FireableItem* fireableItem, String const& name, Json const& def);
    String fireMode(FireableItem* fireableItem);
    bool ready(FireableItem* fireableItem);
    bool firing(FireableItem* fireableItem);
    bool windingUp(FireableItem* fireableItem);
    bool coolingDown(FireableItem* fireableItem);
    bool ownerFullEnergy(FireableItem* fireableItem);
    bool ownerEnergy(FireableItem* fireableItem);
    bool ownerEnergyLocked(FireableItem* fireableItem);
    bool ownerConsumeEnergy(FireableItem* fireableItem, float energy);
  }
}
}
