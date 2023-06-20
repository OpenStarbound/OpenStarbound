#ifndef PLAYER_TYPES_HPP
#define PLAYER_TYPES_HPP

#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarEither.hpp"

namespace Star {

enum class PlayerMode {
  Casual,
  Survival,
  Hardcore
};
extern EnumMap<PlayerMode> const PlayerModeNames;

enum class PlayerBusyState {
  None,
  Chatting,
  Menu
};
extern EnumMap<PlayerBusyState> const PlayerBusyStateNames;

struct PlayerWarpRequest {
  String action;
  Maybe<String> animation;
  bool deploy;
};

struct PlayerModeConfig {
  explicit PlayerModeConfig(Json config = {});

  bool hunger;
  bool allowBeamUpUnderground;
  float reviveCostPercentile;
  Either<String, StringList> deathDropItemTypes;
  bool permadeath;
};

struct ShipUpgrades {
  explicit ShipUpgrades(Json config = {});
  Json toJson() const;

  ShipUpgrades& apply(Json const& upgrades);

  bool operator==(ShipUpgrades const& rhs) const;

  unsigned shipLevel;
  unsigned maxFuel;
  unsigned crewSize;
  float fuelEfficiency;
  float shipSpeed;
  StringSet capabilities;
};

DataStream& operator>>(DataStream& ds, ShipUpgrades& upgrades);
DataStream& operator<<(DataStream& ds, ShipUpgrades const& upgrades);

}

#endif
