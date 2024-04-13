#include "StarCollisionBlock.hpp"

namespace Star {

CollisionSet const DefaultCollisionSet({CollisionKind::Null, CollisionKind::Slippery, CollisionKind::Dynamic, CollisionKind::Block});
CollisionSet const BlockCollisionSet({CollisionKind::Block, CollisionKind::Slippery});

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
