#include "StarSkyTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

EnumMap<SkyType> const SkyTypeNames{
    {SkyType::Barren, "barren"},
    {SkyType::Atmospheric, "atmospheric"},
    {SkyType::Atmosphereless, "atmosphereless"},
    {SkyType::Orbital, "orbital"},
    {SkyType::Warp, "warp"},
    {SkyType::Space, "space"}};

EnumMap<FlyingType> const FlyingTypeNames{
    {FlyingType::None, "none"},
    {FlyingType::Disembarking, "disembarking"},
    {FlyingType::Warp, "warp"},
    {FlyingType::Arriving, "arriving"}};

EnumMap<WarpPhase> const WarpPhaseNames{
    {WarpPhase::SlowingDown, "slowingdown"},
    {WarpPhase::Maintain, "maintain"},
    {WarpPhase::SpeedingUp, "speedingup"}};

SkyColoring::SkyColoring() {
  mainColor = Color::Clear;

  morningColors = {Color::Clear, Color::Clear};
  dayColors = {Color::Clear, Color::Clear};
  eveningColors = {Color::Clear, Color::Clear};
  nightColors = {Color::Clear, Color::Clear};

  morningLightColor = Color::Clear;
  dayLightColor = Color::Clear;
  eveningLightColor = Color::Clear;
  nightLightColor = Color::Clear;
}

SkyColoring::SkyColoring(Json const& variant) {
  auto getColorPair = [](Json const& pair) { return make_pair(jsonToColor(pair.get(0)), jsonToColor(pair.get(1))); };

  mainColor = jsonToColor(variant.get("mainColor"));

  morningColors = getColorPair(variant.get("morningColors"));
  dayColors = getColorPair(variant.get("dayColors"));
  eveningColors = getColorPair(variant.get("eveningColors"));
  nightColors = getColorPair(variant.get("nightColors"));

  morningLightColor = jsonToColor(variant.get("morningLightColor"));
  dayLightColor = jsonToColor(variant.get("dayLightColor"));
  eveningLightColor = jsonToColor(variant.get("eveningLightColor"));
  nightLightColor = jsonToColor(variant.get("nightLightColor"));
}

Json SkyColoring::toJson() const {
  auto makeColorPair = [](pair<Color, Color> const& p) {
    return JsonArray{jsonFromColor(p.first), jsonFromColor(p.second)};
  };

  return JsonObject{{"mainColor", jsonFromColor(mainColor)},
      {"morningColors", makeColorPair(morningColors)},
      {"dayColors", makeColorPair(dayColors)},
      {"eveningColors", makeColorPair(eveningColors)},
      {"nightColors", makeColorPair(nightColors)},
      {"morningLightColor", jsonFromColor(morningLightColor)},
      {"dayLightColor", jsonFromColor(dayLightColor)},
      {"eveningLightColor", jsonFromColor(eveningLightColor)},
      {"nightLightColor", jsonFromColor(nightLightColor)}};
}

DataStream& operator>>(DataStream& ds, SkyColoring& skyColoring) {
  ds.read(skyColoring.mainColor);
  ds.read(skyColoring.morningColors);
  ds.read(skyColoring.dayColors);
  ds.read(skyColoring.eveningColors);
  ds.read(skyColoring.nightColors);
  ds.read(skyColoring.morningLightColor);
  ds.read(skyColoring.dayLightColor);
  ds.read(skyColoring.eveningLightColor);
  ds.read(skyColoring.nightLightColor);

  return ds;
}

DataStream& operator<<(DataStream& ds, SkyColoring const& skyColoring) {
  ds.write(skyColoring.mainColor);
  ds.write(skyColoring.morningColors);
  ds.write(skyColoring.dayColors);
  ds.write(skyColoring.eveningColors);
  ds.write(skyColoring.nightColors);
  ds.write(skyColoring.morningLightColor);
  ds.write(skyColoring.dayLightColor);
  ds.write(skyColoring.eveningLightColor);
  ds.write(skyColoring.nightLightColor);

  return ds;
}

SkyOrbiter::SkyOrbiter() : type(), scale(), angle() {}

SkyOrbiter::SkyOrbiter(SkyOrbiterType type, float scale, float angle, String const& image, Vec2F position)
  : type(type), scale(scale), angle(angle), image(image), position(position) {}

SkyWorldHorizon::SkyWorldHorizon() : scale(), rotation() {}

SkyWorldHorizon::SkyWorldHorizon(Vec2F center, float scale, float rotation)
  : center(center), scale(scale), rotation(rotation) {}

bool SkyWorldHorizon::empty() const {
  return scale <= 0 || layers.empty();
}

}
