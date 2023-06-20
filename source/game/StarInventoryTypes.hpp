#ifndef STAR_INVENTORY_TYPES_HPP
#define STAR_INVENTORY_TYPES_HPP

#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarStrongTypedef.hpp"

namespace Star {

enum class EquipmentSlot : uint8_t {
  Head = 0,
  Chest = 1,
  Legs = 2,
  Back = 3,
  HeadCosmetic = 4,
  ChestCosmetic = 5,
  LegsCosmetic = 6,
  BackCosmetic = 7
};
extern EnumMap<EquipmentSlot> const EquipmentSlotNames;

typedef pair<String, uint8_t> BagSlot;

strong_typedef(Empty, SwapSlot);
strong_typedef(Empty, TrashSlot);

// Any manageable location in the player inventory can be pointed to by an
// InventorySlot
typedef Variant<EquipmentSlot, BagSlot, SwapSlot, TrashSlot> InventorySlot;

InventorySlot jsonToInventorySlot(Json const& json);
Json jsonFromInventorySlot(InventorySlot const& slot);

std::ostream& operator<<(std::ostream& ostream, InventorySlot const& slot);

// Special items in the player inventory that are not generally manageable
enum class EssentialItem : uint8_t {
  BeamAxe = 0,
  WireTool = 1,
  PaintTool = 2,
  InspectionTool = 3
};
extern EnumMap<EssentialItem> const EssentialItemNames;

// A player's action bar is a collection of custom item shortcuts, and special
// hard coded shortcuts to the essential items.  There is one location selected
// at a time, which is either an entry on the custom bar, or one of the
// essential items, or nothing.
typedef uint8_t CustomBarIndex;
typedef MVariant<CustomBarIndex, EssentialItem> SelectedActionBarLocation;

SelectedActionBarLocation jsonToSelectedActionBarLocation(Json const& json);
Json jsonFromSelectedActionBarLocation(SelectedActionBarLocation const& location);

static uint8_t const EquipmentSize = 8;
static uint8_t const EssentialItemCount = 4;

}

#endif
