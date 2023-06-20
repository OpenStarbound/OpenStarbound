#include "StarLightSource.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

void LightSource::translate(Vec2F const& pos) {
  position += pos;
}

DataStream& operator<<(DataStream& ds, LightSource const& lightSource) {
  ds.write(lightSource.position);
  ds.write(lightSource.color);
  ds.write(lightSource.pointLight);
  ds.write(lightSource.pointBeam);
  ds.write(lightSource.beamAngle);
  ds.write(lightSource.beamAmbience);

  return ds;
}

DataStream& operator>>(DataStream& ds, LightSource& lightSource) {
  ds.read(lightSource.position);
  ds.read(lightSource.color);
  ds.read(lightSource.pointLight);
  ds.read(lightSource.pointBeam);
  ds.read(lightSource.beamAngle);
  ds.read(lightSource.beamAmbience);

  return ds;
}

}
