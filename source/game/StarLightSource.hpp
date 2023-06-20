#ifndef STAR_LIGHT_SOURCE_HPP
#define STAR_LIGHT_SOURCE_HPP

#include "StarVector.hpp"
#include "StarDataStream.hpp"

namespace Star {

struct LightSource {
  Vec2F position;
  Vec3B color;

  bool pointLight;
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

#endif
