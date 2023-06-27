#include "StarGameTypes.hpp"

namespace Star {

float WorldTimestep = 1.0f / 120.0f;
float ServerWorldTimestep = 1.0f / 20.0f;

EnumMap<Direction> const DirectionNames{
  {Direction::Left, "left"},
  {Direction::Right, "right"},
};

EnumMap<Gender> const GenderNames{
  {Gender::Male, "male"},
  {Gender::Female, "female"},
};

EnumMap<FireMode> const FireModeNames{
  {FireMode::None, "none"},
  {FireMode::Primary, "primary"},
  {FireMode::Alt, "alt"}
};

EnumMap<ToolHand> const ToolHandNames{
  {ToolHand::Primary, "primary"},
  {ToolHand::Alt, "alt"}
};

EnumMap<TileLayer> const TileLayerNames{
  {TileLayer::Foreground, "foreground"},
  {TileLayer::Background, "background"}
};

EnumMap<MoveControlType> const MoveControlTypeNames{
  {MoveControlType::Left, "left"},
  {MoveControlType::Right, "right"},
  {MoveControlType::Down, "down"},
  {MoveControlType::Up, "up"},
  {MoveControlType::Jump, "jump"}
};

EnumMap<PortraitMode> const PortraitModeNames{
  {PortraitMode::Head, "head"},
  {PortraitMode::Bust, "bust"},
  {PortraitMode::Full, "full"},
  {PortraitMode::FullNeutral, "fullneutral"},
  {PortraitMode::FullNude, "fullnude"},
  {PortraitMode::FullNeutralNude, "fullneutralnude"}
};

EnumMap<Rarity> const RarityNames{
  {Rarity::Common, "common"},
  {Rarity::Uncommon, "uncommon"},
  {Rarity::Rare, "rare"},
  {Rarity::Legendary, "legendary"},
  {Rarity::Essential, "essential"}
};

std::pair<EntityId, EntityId> connectionEntitySpace(ConnectionId connectionId) {
  if (connectionId == ServerConnectionId) {
    return {MinServerEntityId, MaxServerEntityId};
  } else if (connectionId >= MinClientConnectionId && connectionId <= MaxClientConnectionId) {
    EntityId beginIdSpace = (EntityId)connectionId * -65536;
    EntityId endIdSpace = beginIdSpace + 65535;
    return {beginIdSpace, endIdSpace};
  } else {
    throw StarException::format("Invalid connection id in clientEntitySpace({})", connectionId);
  }
}

bool entityIdInSpace(EntityId entityId, ConnectionId connectionId) {
  auto pair = connectionEntitySpace(connectionId);
  return entityId >= pair.first && entityId <= pair.second;
}

ConnectionId connectionForEntity(EntityId entityId) {
  if (entityId > 0)
    return ServerConnectionId;
  else
    return (-entityId - 1) / 65536 + 1;
}

pair<float, Direction> getAngleSide(float angle, bool ccRotation) {
  angle = constrainAngle(angle);
  Direction direction = Direction::Right;
  if (angle > Constants::pi / 2) {
    direction = Direction::Left;
    angle = Constants::pi - angle;
  } else if (angle < -Constants::pi / 2) {
    direction = Direction::Left;
    angle = -Constants::pi - angle;
  }

  if (direction == Direction::Left && ccRotation)
    angle *= -1;

  return make_pair(angle, direction);
}

}
