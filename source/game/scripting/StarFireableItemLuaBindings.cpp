#include "StarFireableItemLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarCasting.hpp"
#include "StarFireableItem.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeFireableItemCallbacks(FireableItem* fireableItem) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<void, Maybe<String>>("fire", bind(FireableItemCallbacks::fire, fireableItem, _1));
  callbacks.registerCallbackWithSignature<void>("triggerCooldown", bind(FireableItemCallbacks::triggerCooldown, fireableItem));
  callbacks.registerCallbackWithSignature<void, float>("setCooldown", bind(FireableItemCallbacks::setCooldown, fireableItem, _1));
  callbacks.registerCallbackWithSignature<void>("endCooldown", bind(FireableItemCallbacks::endCooldown, fireableItem));
  callbacks.registerCallbackWithSignature<float>("cooldownTime", bind(FireableItemCallbacks::cooldownTime, fireableItem));
  callbacks.registerCallbackWithSignature<Json, String, Json>("fireableParam", bind(FireableItemCallbacks::fireableParam, fireableItem, _1, _2));
  callbacks.registerCallbackWithSignature<String>("fireMode", bind(FireableItemCallbacks::fireMode, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ready", bind(FireableItemCallbacks::ready, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("firing", bind(FireableItemCallbacks::firing, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("windingUp", bind(FireableItemCallbacks::windingUp, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("coolingDown", bind(FireableItemCallbacks::coolingDown, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ownerFullEnergy", bind(FireableItemCallbacks::ownerFullEnergy, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ownerEnergy", bind(FireableItemCallbacks::ownerEnergy, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ownerEnergyLocked", bind(FireableItemCallbacks::ownerEnergyLocked, fireableItem));
  callbacks.registerCallbackWithSignature<bool, float>("ownerConsumeEnergy", bind(FireableItemCallbacks::ownerConsumeEnergy, fireableItem, _1));
  callbacks.registerCallbackWithSignature<Vec2F>("ownerAimPosition", [fireableItem]() {
      return fireableItem->owner()->aimPosition();
    });

  return callbacks;
}

// Triggers the item to fire
//
// @param[opt] fireMode fireMode to trigger; should be 'Primary' or 'Alt'
// (defaults to Primary)
// @return nil
void LuaBindings::FireableItemCallbacks::fire(FireableItem* fireableItem, Maybe<String> const& mode) {
  auto fireMode = FireMode::Primary;
  if (mode) {
    if (mode->equalsIgnoreCase("Alt"))
      fireMode = FireMode::Alt;
    else if (!mode->equalsIgnoreCase("Primary"))
      throw StarException("Invalid fire mode specified! Must be 'Primary' or 'Alt'");
  }

  if (fireableItem->ready())
    fireableItem->fire(fireMode, false, true);
}

// Triggers the item's cooldown
//
// @return nil
void LuaBindings::FireableItemCallbacks::triggerCooldown(FireableItem* fireableItem) {
  fireableItem->triggerCooldown();
}

// Sets the item's current cooldown to the specified time (will not change the
// default cooldown)
//
// @param cooldownTime time in seconds for this cooldown period
// @return nil
void LuaBindings::FireableItemCallbacks::setCooldown(FireableItem* fireableItem, float cooldownTime) {
  fireableItem->setCoolingDown(cooldownTime > 0);
  fireableItem->setFireTimer(cooldownTime);
}

// Ends the item's cooldown, readying it to fire
//
// @return nil
void LuaBindings::FireableItemCallbacks::endCooldown(FireableItem* fireableItem) {
  fireableItem->setCoolingDown(false);
  fireableItem->setFireTimer(0);
}

// Returns the item's default cooldown time
//
// @return the default cooldown time in seconds
float LuaBindings::FireableItemCallbacks::cooldownTime(FireableItem* fireableItem) {
  return fireableItem->cooldownTime();
}

// Gets the value of a configuration option for this item.
//
// @string name the name of the configuration parameter to get, as specified in
// the
// item's configuration
// @param[opt] default a default value that will be returned if the given
// configuration key does not exist in the item's configuration.
// @return[1] nil if the item has no configuration option with the given name,
// and no default value is given.
// @return[2] the value of the configuration option, or the given default value.
Json LuaBindings::FireableItemCallbacks::fireableParam(
    FireableItem* fireableItem, String const& name, Json const& def) {
  return fireableItem->fireableParam(name, def);
}

// Gets the current fire mode of the item
//
// @return string representation of the fire mode: 'Primary', 'Alt' or 'None'
String LuaBindings::FireableItemCallbacks::fireMode(FireableItem* fireableItem) {
  if (fireableItem->fireMode() == FireMode::Primary)
    return "Primary";
  else if (fireableItem->fireMode() == FireMode::Alt)
    return "Alt";
  else
    return "None";
}

// Determine whether the item is currently ready to be fired
//
// @return true if the item is ready
bool LuaBindings::FireableItemCallbacks::ready(FireableItem* fireableItem) {
  return fireableItem->ready();
}

// Determine whether the item is currently firing
//
// @return true if the item is firing
bool LuaBindings::FireableItemCallbacks::firing(FireableItem* fireableItem) {
  return fireableItem->firing();
}

// Determine whether the item is currently winding up to fire
//
// @return true if the item is winding up
bool LuaBindings::FireableItemCallbacks::windingUp(FireableItem* fireableItem) {
  return fireableItem->windup();
}

// Determine whether the item is currently cooling down from firing
//
// @return true if the item is cooling down
bool LuaBindings::FireableItemCallbacks::coolingDown(FireableItem* fireableItem) {
  return fireableItem->coolingDown();
}

// Determine whether the item's owner has full energy
//
// @return true if the owner's energy is full
bool LuaBindings::FireableItemCallbacks::ownerFullEnergy(FireableItem* fireableItem) {
  return fireableItem->owner()->fullEnergy();
}

// Determine the amount of energy that the item's owner currently has
//
// @return the owner's current energy
bool LuaBindings::FireableItemCallbacks::ownerEnergy(FireableItem* fireableItem) {
  return fireableItem->owner()->energy();
}

// Determine whether the item's owner's energy pool is currently locked
//
// @return true if the owner's energy pool is currently locked
bool LuaBindings::FireableItemCallbacks::ownerEnergyLocked(FireableItem* fireableItem) {
  return fireableItem->owner()->energyLocked();
}

// Attempt to consume the specified amount of the owner's energy
//
// @param amount the amount of energy to consume
// @return true if the energy was consumed successfully
bool LuaBindings::FireableItemCallbacks::ownerConsumeEnergy(FireableItem* fireableItem, float energy) {
  return fireableItem->owner()->consumeEnergy(energy);
}

}
