#include "StarTileModification.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

DataStream& operator>>(DataStream& ds, PlaceMaterial& tileMaterialPlacement) {
  ds.read(tileMaterialPlacement.layer);
  ds.read(tileMaterialPlacement.material);
  ds.read(tileMaterialPlacement.materialHueShift);

  return ds;
}

DataStream& operator<<(DataStream& ds, PlaceMaterial const& tileMaterialPlacement) {
  ds.write(tileMaterialPlacement.layer);
  ds.write(tileMaterialPlacement.material);
  ds.write(tileMaterialPlacement.materialHueShift);

  return ds;
}

DataStream& operator>>(DataStream& ds, PlaceMod& tileModPlacement) {
  ds.read(tileModPlacement.layer);
  ds.read(tileModPlacement.mod);
  ds.read(tileModPlacement.modHueShift);

  return ds;
}

DataStream& operator<<(DataStream& ds, PlaceMod const& tileModPlacement) {
  ds.write(tileModPlacement.layer);
  ds.write(tileModPlacement.mod);
  ds.write(tileModPlacement.modHueShift);

  return ds;
}

DataStream& operator>>(DataStream& ds, PlaceMaterialColor& tileMaterialColorPlacement) {
  ds.read(tileMaterialColorPlacement.layer);
  ds.read(tileMaterialColorPlacement.color);

  return ds;
}

DataStream& operator<<(DataStream& ds, PlaceMaterialColor const& tileMaterialColorPlacement) {
  ds.write(tileMaterialColorPlacement.layer);
  ds.write(tileMaterialColorPlacement.color);

  return ds;
}

DataStream& operator>>(DataStream& ds, PlaceLiquid& tileLiquidPlacement) {
  ds.read(tileLiquidPlacement.liquid);
  ds.read(tileLiquidPlacement.liquidLevel);

  return ds;
}

DataStream& operator<<(DataStream& ds, PlaceLiquid const& tileLiquidPlacement) {
  ds.write(tileLiquidPlacement.liquid);
  ds.write(tileLiquidPlacement.liquidLevel);

  return ds;
}

}
