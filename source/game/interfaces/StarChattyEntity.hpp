#ifndef STAR_CHATTY_ENTITY_HPP
#define STAR_CHATTY_ENTITY_HPP

#include "StarChatAction.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(ChattyEntity);

class ChattyEntity : public virtual Entity {
public:
  virtual Vec2F mouthPosition() const = 0;
  virtual List<ChatAction> pullPendingChatActions() = 0;
};

}

#endif
