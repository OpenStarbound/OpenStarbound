#include "StarInteractionTypes.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

DataStream& operator>>(DataStream& ds, InteractRequest& ir) {
  ds >> ir.sourceId;
  ds >> ir.sourcePosition;
  ds >> ir.targetId;
  ds >> ir.interactPosition;
  return ds;
}

DataStream& operator<<(DataStream& ds, InteractRequest const& ir) {
  ds << ir.sourceId;
  ds << ir.sourcePosition;
  ds << ir.targetId;
  ds << ir.interactPosition;
  return ds;
}

EnumMap<InteractActionType> const InteractActionTypeNames{{InteractActionType::None, "None"},
    {InteractActionType::OpenContainer, "OpenContainer"},
    {InteractActionType::SitDown, "SitDown"},
    {InteractActionType::OpenCraftingInterface, "OpenCraftingInterface"},
    {InteractActionType::OpenSongbookInterface, "OpenSongbookInterface"},
    {InteractActionType::OpenNpcCraftingInterface, "OpenNpcCraftingInterface"},
    {InteractActionType::OpenMerchantInterface, "OpenMerchantInterface"},
    {InteractActionType::OpenAiInterface, "OpenAiInterface"},
    {InteractActionType::OpenTeleportDialog, "OpenTeleportDialog"},
    {InteractActionType::ShowPopup, "ShowPopup"},
    {InteractActionType::ScriptPane, "ScriptPane"},
    {InteractActionType::Message, "Message"}};

InteractAction::InteractAction() {
  type = InteractActionType::None;
  entityId = NullEntityId;
}

InteractAction::InteractAction(InteractActionType type, EntityId entityId, Json data)
  : type(type), entityId(entityId), data(data) {}

InteractAction::InteractAction(String const& typeName, EntityId entityId, Json data)
  : type(InteractActionTypeNames.getLeft(typeName)), entityId(entityId), data(data) {}

InteractAction::operator bool() const {
  return type != InteractActionType::None;
}

DataStream& operator>>(DataStream& ds, InteractAction& ir) {
  ds.read(ir.type);
  ds.read(ir.entityId);
  ds.read(ir.data);
  return ds;
}

DataStream& operator<<(DataStream& ds, InteractAction const& ir) {
  ds.write(ir.type);
  ds.write(ir.entityId);
  ds.write(ir.data);
  return ds;
}

}
