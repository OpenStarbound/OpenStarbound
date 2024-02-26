#pragma once

#include "StarMathCommon.hpp"

namespace Star {

typedef uint8_t LiquidId;
LiquidId const EmptyLiquidId = 0;

struct LiquidLevel {
  LiquidLevel();
  LiquidLevel(LiquidId liquid, float level);

  LiquidLevel take(float amount);

  LiquidId liquid;
  float level;
};

struct LiquidNetUpdate {
  LiquidId liquid;
  uint8_t level;

  LiquidLevel liquidLevel() const;
};

struct LiquidStore : LiquidLevel {
  // Returns a LiquidStore of the given liquid
  static LiquidStore filled(LiquidId liquid, float level, Maybe<float> pressure = {});
  // Returns a LiquidStore source liquid block
  static LiquidStore endless(LiquidId liquid, float pressure);

  LiquidStore();
  LiquidStore(LiquidId liquid, float level, float pressure, bool source);

  LiquidNetUpdate netUpdate() const;

  Maybe<LiquidNetUpdate> update(LiquidId liquid, float level, float pressure);

  LiquidLevel take(float amount);

  float pressure;
  bool source;
};

inline LiquidLevel::LiquidLevel()
  : liquid(EmptyLiquidId), level(0.0f) {}

inline LiquidLevel::LiquidLevel(LiquidId liquid, float level)
  : liquid(liquid), level(level) {}

inline LiquidLevel LiquidNetUpdate::liquidLevel() const {
  return LiquidLevel{liquid, byteToFloat(level)};
}

}
