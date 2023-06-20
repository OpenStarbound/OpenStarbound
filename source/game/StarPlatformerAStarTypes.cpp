#include "StarPlatformerAStarTypes.hpp"

namespace Star {
namespace PlatformerAStar {

EnumMap<Action> const ActionNames{
  {Action::Walk, "Walk"},
  {Action::Jump, "Jump"},
  {Action::Arc, "Arc"},
  {Action::Drop, "Drop"},
  {Action::Swim, "Swim"},
  {Action::Fly, "Fly"},
  {Action::Land, "Land"}
};

Node Node::withVelocity(Vec2F velocity) const {
  return {position, velocity};
}

}
}
