#pragma once

#include "StarVector.hpp"
#include "StarDataStream.hpp"
#include "StarBiMap.hpp"

namespace Star {

enum class LightType : uint8_t {
  Spread = 0,
  Point = 1,
  PointAsSpread = 2 // Point with spread-like range
};

extern EnumMap<LightType> const LightTypeNames;

struct LightSource {
  Vec2F position;
  Vec3F color;
  LightType type;
  // pointBeam of 0.0 means light has no beam component, as pointBeam goes up,
  // the dropoff from the beamAngle becomes faster and faster.
  float pointBeam;
  // The angle of the beam component of the light in radians
  float beamAngle;
  // beamAmbience provides a floor to the  dropoff for beamed lights, so that
  // even where the beam is not pointing there will still be some light.  0.0
  // means no ambient floor, 1.0 effectively turns off beaming.
  float beamAmbience;

  void translate(Vec2F const& pos);
};

DataStream& operator<<(DataStream& ds, LightSource const& lightSource);
DataStream& operator>>(DataStream& ds, LightSource& lightSource);
}
