#pragma once

#include "StarColor.hpp"
#include "StarBiMap.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(SkyException, StarException);

enum class SkyType : uint8_t {
  Barren,
  Atmospheric,
  Atmosphereless,
  Orbital,
  Warp,
  Space
};
extern EnumMap<SkyType> const SkyTypeNames;

enum class FlyingType : uint8_t {
  None,
  Disembarking,
  Warp,
  Arriving
};
extern EnumMap<FlyingType> const FlyingTypeNames;

enum class WarpPhase : int8_t {
  SlowingDown = -1,
  Maintain = 0,
  SpeedingUp = 1
};
extern EnumMap<WarpPhase> const WarpPhaseNames;

struct SkyColoring {
  SkyColoring();
  explicit SkyColoring(Json const& variant);

  Json toJson() const;

  Color mainColor;

  pair<Color, Color> morningColors;
  pair<Color, Color> dayColors;
  pair<Color, Color> eveningColors;
  pair<Color, Color> nightColors;

  Color morningLightColor;
  Color dayLightColor;
  Color eveningLightColor;
  Color nightLightColor;
};

DataStream& operator>>(DataStream& ds, SkyColoring& skyColoring);
DataStream& operator<<(DataStream& ds, SkyColoring const& skyColoring);

enum class SkyOrbiterType { Sun, Moon, HorizonCloud, SpaceDebris };

struct SkyOrbiter {
  SkyOrbiter();
  SkyOrbiter(SkyOrbiterType type, float scale, float angle, String const& image, Vec2F position);

  SkyOrbiterType type;
  float scale;
  float angle;
  String image;
  Vec2F position;
};

struct SkyWorldHorizon {
  SkyWorldHorizon();
  SkyWorldHorizon(Vec2F center, float scale, float rotation);

  bool empty() const;

  Vec2F center;

  float scale;
  float rotation;

  // List of L/R images for each layer of the world horizon, bottom to top.
  List<pair<String, String>> layers;
};

}
