#include "StarInteractiveEntity.hpp"

namespace Star {

RectF InteractiveEntity::interactiveBoundBox() const {
  return metaBoundBox();
}

bool InteractiveEntity::isInteractive() const {
  return true;
}

List<QuestArcDescriptor> InteractiveEntity::offeredQuests() const {
  return {};
}

StringSet InteractiveEntity::turnInQuests() const {
  return {};
}

Vec2F InteractiveEntity::questIndicatorPosition() const {
  return position();
}

}
