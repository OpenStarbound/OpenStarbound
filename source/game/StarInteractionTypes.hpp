#ifndef STAR_INTERACTION_TYPES_HPP
#define STAR_INTERACTION_TYPES_HPP

#include "StarGameTypes.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(InteractActionException, StarException);

struct InteractRequest {
  EntityId sourceId;
  Vec2F sourcePosition;
  EntityId targetId;
  Vec2F interactPosition;
};

DataStream& operator>>(DataStream& ds, InteractRequest& ir);
DataStream& operator<<(DataStream& ds, InteractRequest const& ir);

enum class InteractActionType {
  None,
  OpenContainer,
  SitDown,
  OpenCraftingInterface,
  OpenSongbookInterface,
  OpenNpcCraftingInterface,
  OpenMerchantInterface,
  OpenAiInterface,
  OpenTeleportDialog,
  ShowPopup,
  ScriptPane,
  Message
};
extern EnumMap<InteractActionType> const InteractActionTypeNames;

struct InteractAction {
  InteractAction();
  InteractAction(InteractActionType type, EntityId entityId, Json data);
  InteractAction(String const& typeName, EntityId entityId, Json data);

  explicit operator bool() const;

  InteractActionType type;
  EntityId entityId;
  Json data;
};

DataStream& operator>>(DataStream& ds, InteractAction& ir);
DataStream& operator<<(DataStream& ds, InteractAction const& ir);

}

#endif
