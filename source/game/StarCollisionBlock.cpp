#include "StarCollisionBlock.hpp"

namespace Star {

EnumMap<CollisionKind> const CollisionKindNames{
  {CollisionKind::Null, "Null"},
  {CollisionKind::None, "None"},
  {CollisionKind::Platform, "Platform"},
  {CollisionKind::Dynamic, "Dynamic"},
  {CollisionKind::Slippery, "Slippery"},
  {CollisionKind::Block, "Block"}
};

}
