#pragma once

#include "StarSkyParameters.hpp"

namespace Star {

struct SkyRenderData {
  Json settings;
  SkyParameters skyParameters;

  SkyType type;
  float dayLevel;
  float skyAlpha;

  float dayLength;
  float timeOfDay;
  double epochTime;

  Vec2F starOffset;
  float starRotation;
  Vec2F worldOffset;
  float worldRotation;
  float orbitAngle;

  size_t starFrames;
  StringList starList;
  StringList hyperStarList;

  Color environmentLight;
  Color mainSkyColor;
  Color topRectColor;
  Color bottomRectColor;
  Color flashColor;

  StringList starTypes() const;

  // Star and orbiter positions here are in view space, from (0, 0) to viewSize

  List<SkyOrbiter> backOrbiters(Vec2F const& viewSize) const;
  SkyWorldHorizon worldHorizon(Vec2F const& viewSize) const;
  List<SkyOrbiter> frontOrbiters(Vec2F const& viewSize) const;
};

DataStream& operator>>(DataStream& ds, SkyRenderData& skyRenderData);
DataStream& operator<<(DataStream& ds, SkyRenderData const& skyRenderData);
}
