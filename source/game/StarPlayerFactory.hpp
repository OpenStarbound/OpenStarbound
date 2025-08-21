#pragma once

#include "StarItemDescriptor.hpp"
#include "StarHumanoid.hpp"
#include "StarEntitySplash.hpp"
#include "StarLuaRoot.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_STRUCT(PlayerConfig);
STAR_CLASS(PlayerFactory);

STAR_EXCEPTION(PlayerException, StarException);

// The player has a large number of shared config states, so this is a shared
// config object to hold them.
struct PlayerConfig {
  PlayerConfig(JsonObject const& cfg);

  HumanoidIdentity defaultIdentity;
  Humanoid::HumanoidTiming humanoidTiming;

  List<ItemDescriptor> defaultItems;
  List<ItemDescriptor> defaultBlueprints;

  RectF metaBoundBox;

  Json movementParameters;
  Json zeroGMovementParameters;
  Json statusControllerSettings;

  float footstepTiming;
  Vec2F footstepSensor;

  Vec2F underwaterSensor;
  float underwaterMinWaterLevel;

  String effectsAnimator;

  float teleportInTime;
  float teleportOutTime;

  float deployInTime;
  float deployOutTime;

  String bodyMaterialKind;

  EntitySplashConfig splashConfig;

  Json companionsConfig;

  Json deploymentConfig;

  StringMap<String> genericScriptContexts;
};

class PlayerFactory {
public:
  PlayerFactory();

  PlayerPtr create() const;
  PlayerPtr diskLoadPlayer(Json const& diskStore) const;
  PlayerPtr netLoadPlayer(ByteArray const& netStore, NetCompatibilityRules rules = {}) const;

private:
  PlayerConfigPtr m_config;

  LuaRootPtr m_luaRoot;
  List<String> m_rebuildScripts;
};

}
