#ifndef STAR_ITEM_LUA_BINDINGS_HPP
#define STAR_ITEM_LUA_BINDINGS_HPP

#include "StarLua.hpp"
#include "StarVector.hpp"

namespace Star {

STAR_CLASS(Item);

namespace LuaBindings {
  LuaCallbacks makeItemCallbacks(Item* item);

  namespace ItemCallbacks {
    String name(Item* item);
    size_t count(Item* item);
    size_t setCount(Item* item, size_t count);
    size_t maxStack(Item* item);
    bool matches(Item* item, Json const& descriptor, Maybe<bool> exactMatch);
    Json matchingDescriptors(Item* item);
    bool consume(Item* item, size_t count);
    bool empty(Item* item);
    Json descriptor(Item* item);
    String description(Item* item);
    String friendlyName(Item* item);
    int rarity(Item* item);
    String rarityString(Item* item);
    size_t price(Item* item);
    unsigned fuelAmount(Item* item);
    Json iconDrawables(Item* item);
    Json dropDrawables(Item* item);
    String largeImage(Item* item);
    String tooltipKind(Item* item);
    String category(Item* item);
    String pickupSound(Item* item);
    bool twoHanded(Item* item);
    float timeToLive(Item* item);
    Json learnBlueprintsOnPickup(Item* item);
    bool hasItemTag(Item* item, String const& itemTag);
    Json pickupQuestTemplates(Item* item);
  }
}
}

#endif
