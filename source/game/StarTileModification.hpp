#pragma once

#include "StarDataStream.hpp"
#include "StarVariant.hpp"
#include "StarGameTypes.hpp"
#include "StarCollisionBlock.hpp"
#include "StarNetCompatibility.hpp"

namespace Star {

struct PlaceMaterial {
  TileLayer layer;
  MaterialId material;
  // If the material hue shift is not set it will get the natural hue shift for
  // the environment.
  Maybe<MaterialHue> materialHueShift;
  TileCollisionOverride collisionOverride = TileCollisionOverride::None;
};
DataStream& operator>>(DataStream& ds, PlaceMaterial& tileMaterialPlacement);
DataStream& operator<<(DataStream& ds, PlaceMaterial const& tileMaterialPlacement);

struct PlaceMod {
  TileLayer layer;
  ModId mod;

  // If the mod hue shift is not set it will get the natural hue shift for the
  // environment.
  Maybe<MaterialHue> modHueShift;
};
DataStream& operator>>(DataStream& ds, PlaceMod& tileModPlacement);
DataStream& operator<<(DataStream& ds, PlaceMod const& tileModPlacement);

struct PlaceMaterialColor {
  TileLayer layer;
  MaterialColorVariant color;
};
DataStream& operator>>(DataStream& ds, PlaceMaterialColor& tileMaterialColorPlacement);
DataStream& operator<<(DataStream& ds, PlaceMaterialColor const& tileMaterialColorPlacement);

// Sets only the hue of what is already on the tile.
// oSB servers only since a vanilla server cannot read it.
struct SetHueShift {
  TileLayer layer;
  bool mod;
  MaterialHue hueShift;
};
DataStream& operator>>(DataStream& ds, SetHueShift& tileHueShift);
DataStream& operator<<(DataStream& ds, SetHueShift const& tileHueShift);

struct PlaceLiquid {
  LiquidId liquid;
  float liquidLevel;
};
DataStream& operator>>(DataStream& ds, PlaceLiquid& tileLiquidPlacement);
DataStream& operator<<(DataStream& ds, PlaceLiquid const& tileLiquidPlacement);

typedef MVariant<PlaceMaterial, PlaceMod, PlaceMaterialColor, PlaceLiquid, SetHueShift> TileModification;
typedef List<pair<Vec2I, TileModification>> TileModificationList;

// Whether a peer with these net rules can read the modification.
// A legacy peer cannot read SetHueShift so it must never be sent one.
inline bool tileModificationSupported(TileModification const& modification, NetCompatibilityRules const& netRules) {
  return !(modification.is<SetHueShift>() && netRules.isLegacy());
}

}
