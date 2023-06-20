#include "StarChatAction.hpp"

namespace Star {

SayChatAction::SayChatAction() {
  entity = NullEntityId;
}

SayChatAction::SayChatAction(EntityId entity, String const& text, Vec2F const& position)
  : entity(entity), text(text), position(position) {}

SayChatAction::SayChatAction(EntityId entity, String const& text, Vec2F const& position, Json const& config)
  : entity(entity), text(text), position(position), config(config) {}

SayChatAction::operator bool() const {
  return !text.empty();
}

PortraitChatAction::PortraitChatAction() {
  entity = NullEntityId;
}

PortraitChatAction::PortraitChatAction(
    EntityId entity, String const& portrait, String const& text, Vec2F const& position)
  : entity(entity), portrait(portrait), text(text), position(position) {}

PortraitChatAction::PortraitChatAction(
    EntityId entity, String const& portrait, String const& text, Vec2F const& position, Json const& config)
  : entity(entity), portrait(portrait), text(text), position(position), config(config) {}

PortraitChatAction::operator bool() const {
  return !text.empty();
}

}
