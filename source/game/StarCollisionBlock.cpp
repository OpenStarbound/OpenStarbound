#include "StarCollisionBlock.hpp"

namespace Star {

EnumMap<TileCollisionOverride> const TileCollisionOverrideNames = {
    {TileCollisionOverride::None, "None"},
    {TileCollisionOverride::Empty, "Empty"},
    {TileCollisionOverride::Platform, "Platform"},
    {TileCollisionOverride::Block, "Block"}
  };

EnumMap<CollisionKind> const CollisionKindNames{
  {CollisionKind::Null, "Null"},
  {CollisionKind::None, "None"},
  {CollisionKind::Platform, "Platform"},
  {CollisionKind::Dynamic, "Dynamic"},
  {CollisionKind::Slippery, "Slippery"},
  {CollisionKind::Block, "Block"}
};

}
