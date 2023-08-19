#ifndef STAR_TILE_MODIFICATION_HPP
#define STAR_TILE_MODIFICATION_HPP

#include "StarDataStream.hpp"
#include "StarVariant.hpp"
#include "StarGameTypes.hpp"
#include "StarCollisionBlock.hpp"

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

struct PlaceLiquid {
  LiquidId liquid;
  float liquidLevel;
};
DataStream& operator>>(DataStream& ds, PlaceLiquid& tileLiquidPlacement);
DataStream& operator<<(DataStream& ds, PlaceLiquid const& tileLiquidPlacement);

typedef MVariant<PlaceMaterial, PlaceMod, PlaceMaterialColor, PlaceLiquid> TileModification;
typedef List<pair<Vec2I, TileModification>> TileModificationList;

}

#endif
