#ifndef STAR_CHAT_ACTION_HPP
#define STAR_CHAT_ACTION_HPP

#include "StarJson.hpp"
#include "StarGameTypes.hpp"

namespace Star {

struct SayChatAction {
  SayChatAction();
  SayChatAction(EntityId entity, String const& text, Vec2F const& position);
  SayChatAction(EntityId entity, String const& text, Vec2F const& position, Json const& config);

  explicit operator bool() const;

  EntityId entity;
  String text;
  Vec2F position;
  Json config;
};

struct PortraitChatAction {
  PortraitChatAction();
  PortraitChatAction(EntityId entity, String const& portrait, String const& text, Vec2F const& position);
  PortraitChatAction(EntityId entity, String const& portrait, String const& text, Vec2F const& position, Json const& config);

  explicit operator bool() const;

  EntityId entity;
  String portrait;
  String text;
  Vec2F position;
  Json config;
};

typedef MVariant<SayChatAction, PortraitChatAction> ChatAction;

}

#endif
