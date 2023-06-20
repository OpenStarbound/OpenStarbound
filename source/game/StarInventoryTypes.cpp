#include "StarInventoryTypes.hpp"
#include "StarFormat.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

EnumMap<EquipmentSlot> const EquipmentSlotNames{
  {EquipmentSlot::Head, "head"},
  {EquipmentSlot::Chest, "chest"},
  {EquipmentSlot::Legs, "legs"},
  {EquipmentSlot::Back, "back"},
  {EquipmentSlot::HeadCosmetic, "headCosmetic"},
  {EquipmentSlot::ChestCosmetic, "chestCosmetic"},
  {EquipmentSlot::LegsCosmetic, "legsCosmetic"},
  {EquipmentSlot::BackCosmetic, "backCosmetic"}
};

InventorySlot jsonToInventorySlot(Json const& json) {
  String type = json.getString("type");
  Json location = json.get("location", Json());
  if (type == "equipment")
    return EquipmentSlotNames.getLeft(location.toString());
  else if (type == "swap")
    return SwapSlot();
  else if (type == "trash")
    return TrashSlot();
  else
    return BagSlot(type, location.toUInt());
}

Json jsonFromInventorySlot(InventorySlot const& slot) {
  if (slot.is<EquipmentSlot>()) {
    return JsonObject{
      {"type", "equipment"},
      {"location", EquipmentSlotNames.getRight(slot.get<EquipmentSlot>())}
    };
  } else if (slot.is<SwapSlot>()) {
    return JsonObject{{"type", "swap"}};
  } else if (slot.is<TrashSlot>()) {
    return JsonObject{{"type", "trash"}};
  } else {
    auto bagSlot = slot.get<BagSlot>();
    return JsonObject{
      {"type", bagSlot.first},
      {"location", bagSlot.second}
    };
  }
}

std::ostream& operator<<(std::ostream& ostream, InventorySlot const& slot) {
  Json json = jsonFromInventorySlot(slot);

  String type = json.getString("type");
  Json location = json.get("location", {});

  if (location.isNull())
    format(ostream, "InventorySlot{type: %s}", type);
  if (location.isType(Json::Type::String))
    format(ostream, "InventorySlot{type: %s, location: %s}", type, location.toString());
  else
    format(ostream, "InventorySlot{type: %s, location: %s}", type, location.toInt());

  return ostream;
}

EnumMap<EssentialItem> const EssentialItemNames{
  {EssentialItem::BeamAxe, "beamaxe"},
  {EssentialItem::WireTool, "wiretool"},
  {EssentialItem::PaintTool, "painttool"},
  {EssentialItem::InspectionTool, "inspectiontool"}
};

SelectedActionBarLocation jsonToSelectedActionBarLocation(Json const& json) {
  if (json.isType(Json::Type::String))
    return EssentialItemNames.getLeft(json.toString());
  else if (json.isNull())
    return SelectedActionBarLocation();
  else
    return (CustomBarIndex)json.toUInt();
}

Json jsonFromSelectedActionBarLocation(SelectedActionBarLocation const& location) {
  if (location.is<CustomBarIndex>())
    return location.get<CustomBarIndex>();
  else if (location.is<EssentialItem>())
    return EssentialItemNames.getRight(location.get<EssentialItem>());
  else
    return Json();
}

}
