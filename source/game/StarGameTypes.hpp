#ifndef STAR_GAME_TYPES_HPP
#define STAR_GAME_TYPES_HPP

#include "StarString.hpp"
#include "StarBiMap.hpp"
#include "StarVector.hpp"
#include "StarLiquidTypes.hpp"
#include "StarMaterialTypes.hpp"

namespace Star {

enum class Direction : uint8_t {
  Left,
  Right
};
extern EnumMap<Direction> const DirectionNames;

inline Direction operator-(Direction dir) {
  if (dir == Direction::Left)
    return Direction::Right;
  return Direction::Left;
}

inline int numericalDirection(Maybe<Direction> direction) {
  if (!direction)
    return 0;
  else
    return *direction == Direction::Left ? -1 : 1;
}

template <typename NumType>
inline Maybe<Direction> directionOf(NumType const& n) {
  if (n == 0)
    return {};
  else
    return n < 0 ? Direction::Left : Direction::Right;
}

enum class Gender : uint8_t {
  Male,
  Female
};
extern EnumMap<Gender> const GenderNames;

enum class FireMode : uint8_t {
  None,
  Primary,
  Alt
};
extern EnumMap<FireMode> const FireModeNames;

enum class ToolHand : uint8_t {
  Primary,
  Alt
};
extern EnumMap<ToolHand> const ToolHandNames;

enum class TileLayer : uint8_t {
  Foreground,
  Background
};
extern EnumMap<TileLayer> const TileLayerNames;

enum class MoveControlType : uint8_t {
  Left,
  Right,
  Down,
  Up,
  Jump
};
extern EnumMap<MoveControlType> const MoveControlTypeNames;

enum class PortraitMode {
  Head,
  Bust,
  Full,
  FullNeutral,
  FullNude,
  FullNeutralNude
};
extern EnumMap<PortraitMode> const PortraitModeNames;

enum class Rarity {
  Common,
  Uncommon,
  Rare,
  Legendary,
  Essential
};
extern EnumMap<Rarity> const RarityNames;

// Transformation from tile space to pixel space.  Number of pixels in 1.0
// distance (one tile).
unsigned const TilePixels = 8;

extern float WorldTimestep;
extern float ServerWorldTimestep;
float const SystemWorldTimestep = 1.0f / 20.0f;

size_t const WorldSectorSize = 32;

typedef int32_t EntityId;
EntityId const NullEntityId = 0;
EntityId const MinServerEntityId = 1;
EntityId const MaxServerEntityId = highest<EntityId>();

// Whether this entity is controlled by it's world, or synced from a different
// world.  Does not necessarily correspond to client / server world (player is
// master on client).
enum class EntityMode {
  Master,
  Slave
};

typedef uint16_t ConnectionId;
ConnectionId const ServerConnectionId = 0;
// Minimum and maximum valid client ids
ConnectionId const MinClientConnectionId = 1;
ConnectionId const MaxClientConnectionId = 32767;

template <typename Vec2T>
inline Vec2F centerOfTile(Vec2T const& tile) {
  return Vec2F(tile.floor()) + Vec2F::filled(0.5);
}

typedef uint16_t DungeonId;

static const DungeonId NoDungeonId = 65535;
static const DungeonId SpawnDungeonId = 65534;
static const DungeonId BiomeMicroDungeonId = 65533;
// meta dungeon signalling player built structures
static const DungeonId ConstructionDungeonId = 65532;
// indicates a block that has been destroyed
static const DungeonId DestroyedBlockDungeonId = 65531;

// dungeonId for zero-g areas with and without tile protection
static const DungeonId ZeroGDungeonId = 65525;
static const DungeonId ProtectedZeroGDungeonId = 65524;

// The first dungeon id that is reserved for special hard-coded dungeon values.
DungeonId const FirstMetaDungeonId = 65520;

inline bool isRealDungeon(DungeonId dungeon) {
  return dungeon < FirstMetaDungeonId;
}

// Returns the inclusive beginning and end of the entity id space for the
// given connection.  All client connection id spaces will be within the range
// [-2^31, -1].
pair<EntityId, EntityId> connectionEntitySpace(ConnectionId connectionId);

bool entityIdInSpace(EntityId entityId, ConnectionId connectionId);

ConnectionId connectionForEntity(EntityId entityId);

// Returns an angle in the range [-pi / 2, pi / 2], and the horizontal
// hemisphere of the angle.  The angle is specified as positive being upward
// rotation and negative being downward rotation, unless ccRotation is true, in
// which case the angle is always positive == counter-clocwise.
pair<float, Direction> getAngleSide(float angle, bool ccRotation = false);

enum class TileDamageResult {
  None = 0,
  Protected = 1,
  Normal = 2,
};

}

#endif
