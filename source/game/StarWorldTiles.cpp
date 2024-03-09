#include "StarWorldTiles.hpp"
#include "StarLiquidTypes.hpp"

namespace Star {

bool WorldTile::isConnectable(TileLayer layer, bool materialOnly) const {
  if (layer == TileLayer::Foreground) {
    if (isConnectableMaterial(foreground))
      return true;
  } else {
    if (isConnectableMaterial(background))
      return true;
  }
  if (materialOnly)
    return false;

  if (layer == TileLayer::Foreground)
    return collision == CollisionKind::Block || collision == CollisionKind::Platform;
  else
    return false;
}

bool WorldTile::isColliding(CollisionSet const& collisionSet) const {
  return Star::isColliding(collision, collisionSet);
}

VersionNumber const ServerTile::CurrentSerializationVersion = 418;

ServerTile::ServerTile() : objectCollision(CollisionKind::None) {}

ServerTile::ServerTile(ServerTile const& serverTile) : WorldTile() {
  *this = serverTile;
}

ServerTile& ServerTile::operator=(ServerTile const& serverTile) {
  WorldTile::operator=(serverTile);

  liquid = serverTile.liquid;
  rootSource = serverTile.rootSource;
  objectCollision = serverTile.objectCollision;
  return *this;
}

void ServerTile::write(DataStream& ds) const {
  ds.write(foreground);
  ds.write(foregroundHueShift);
  ds.write(foregroundColorVariant);
  ds.write(foregroundMod);
  ds.write(foregroundModHueShift);
  ds.write(background);
  ds.write(backgroundHueShift);
  ds.write(backgroundColorVariant);
  ds.write(backgroundMod);
  ds.write(backgroundModHueShift);
  ds.write(liquid.liquid);
  ds.write(liquid.level);
  ds.write(liquid.pressure);
  ds.write(liquid.source);
  ds.write(collision);
  ds.write(dungeonId);
  ds.write(blockBiomeIndex);
  ds.write(environmentBiomeIndex);
  ds.write(biomeTransition);
  ds.write(rootSource);
}

void ServerTile::read(DataStream& ds, VersionNumber serializationVersion) {
  if (serializationVersion < 416 || serializationVersion > CurrentSerializationVersion)
    throw StarException::format("Cannot read ServerTile - serialization version {} incompatible with current version {}\n", serializationVersion, CurrentSerializationVersion);

  ds.read(foreground);
  ds.read(foregroundHueShift);
  ds.read(foregroundColorVariant);
  ds.read(foregroundMod);
  ds.read(foregroundModHueShift);
  ds.read(background);
  ds.read(backgroundHueShift);
  ds.read(backgroundColorVariant);
  ds.read(backgroundMod);
  ds.read(backgroundModHueShift);
  ds.read(liquid.liquid);
  ds.read(liquid.level);
  ds.read(liquid.pressure);
  ds.read(liquid.source);
  ds.read(collision);
  ds.read(dungeonId);
  ds.read(blockBiomeIndex);
  ds.read(environmentBiomeIndex);
  if (serializationVersion < 417)
    biomeTransition = false;
  else
    ds.read(biomeTransition);
  if (serializationVersion < 418) {
    ds.readBytes(1);
    rootSource = {};
  } else {
    ds.read(rootSource);
  }
  collisionCacheDirty = true;
}

bool ServerTile::updateCollision(CollisionKind kind) {
  if (collision != kind) {
    collision = kind;
    collisionCacheDirty = true;
    collisionCache.clear();
    return true;
  }
  return false;
}

bool ServerTile::updateObjectCollision(CollisionKind kind) {
  if (objectCollision != kind) {
    objectCollision = kind;
    collisionCacheDirty = true;
    collisionCache.clear();
    return true;
  }
  return false;
}

CollisionKind ServerTile::getCollision() const {
  CollisionKind kind = collision;
  if (objectCollision != CollisionKind::None
      && (objectCollision != CollisionKind::Platform || kind == CollisionKind::None)) {
    kind = objectCollision;
  }
  return kind;
}

PredictedTile::operator bool() const {
  return
     background
  || backgroundHueShift
  || backgroundColorVariant
  || backgroundMod
  || backgroundModHueShift
  || foreground
  || foregroundHueShift
  || foregroundColorVariant
  || foregroundMod
  || foregroundModHueShift
  || liquid
  || collision;
}

DataStream& operator>>(DataStream& ds, NetTile& tile) {
  ds.read(tile.background);
  if (tile.background == 0) {
    tile.background = EmptyMaterialId;
    tile.backgroundHueShift = MaterialHue();
    tile.backgroundColorVariant = DefaultMaterialColorVariant;
    tile.backgroundMod = NoModId;
    tile.backgroundModHueShift = MaterialHue();
  } else {
    ds.read(tile.backgroundHueShift);
    ds.read(tile.backgroundColorVariant);
    ds.read(tile.backgroundMod);
    if (tile.backgroundMod == 0) {
      tile.backgroundMod = NoModId;
      tile.backgroundModHueShift = MaterialHue();
    } else {
      ds.read(tile.backgroundModHueShift);
    }
  }
  ds.read(tile.foreground);
  if (tile.foreground == 0) {
    tile.foreground = EmptyMaterialId;
    tile.foregroundHueShift = MaterialHue();
    tile.foregroundColorVariant = DefaultMaterialColorVariant;
    tile.foregroundMod = NoModId;
    tile.foregroundModHueShift = MaterialHue();
  } else {
    ds.read(tile.foregroundHueShift);
    ds.read(tile.foregroundColorVariant);
    ds.read(tile.foregroundMod);
    if (tile.foregroundMod == 0) {
      tile.foregroundMod = NoModId;
      tile.foregroundModHueShift = MaterialHue();
    } else {
      ds.read(tile.foregroundModHueShift);
    }
  }

  ds.read(tile.collision);
  ds.read(tile.blockBiomeIndex);
  ds.read(tile.environmentBiomeIndex);
  ds.read(tile.liquid.liquid);
  if (tile.liquid.liquid != EmptyLiquidId)
    ds.read(tile.liquid.level);
  else
    tile.liquid.level = 0.0f;

  ds.vuread(tile.dungeonId);

  return ds;
}

DataStream& operator<<(DataStream& ds, NetTile const& tile) {
  if (tile.background == EmptyMaterialId) {
    ds.cwrite<MaterialId>(0);
  } else {
    ds.write(tile.background);
    ds.write(tile.backgroundHueShift);
    ds.write(tile.backgroundColorVariant);
    if (tile.backgroundMod == NoModId) {
      ds.write<ModId>(0);
    } else {
      ds.write(tile.backgroundMod);
      ds.write(tile.backgroundModHueShift);
    }
  }

  if (tile.foreground == EmptyMaterialId) {
    ds.cwrite<MaterialId>(0);
  } else {
    ds.write(tile.foreground);
    ds.write(tile.foregroundHueShift);
    ds.write(tile.foregroundColorVariant);
    if (tile.foregroundMod == NoModId) {
      ds.write<ModId>(0);
    } else {
      ds.write(tile.foregroundMod);
      ds.write(tile.foregroundModHueShift);
    }
  }

  ds.write(tile.collision);
  ds.write(tile.blockBiomeIndex);
  ds.write(tile.environmentBiomeIndex);
  ds.write(tile.liquid.liquid);
  if (tile.liquid.liquid != EmptyLiquidId)
    ds.write(tile.liquid.level);

  ds.vuwrite(tile.dungeonId);

  return ds;
}

DataStream& operator>>(DataStream& ds, RenderTile& tile) {
  ds >> tile.foreground;
  ds >> tile.foregroundHueShift;
  ds >> tile.foregroundMod;
  ds >> tile.foregroundModHueShift;
  ds >> tile.foregroundColorVariant;
  ds >> tile.foregroundDamageLevel;
  ds >> tile.foregroundDamageType;
  ds >> tile.background;
  ds >> tile.backgroundHueShift;
  ds >> tile.backgroundMod;
  ds >> tile.backgroundModHueShift;
  ds >> tile.backgroundColorVariant;
  ds >> tile.backgroundDamageLevel;
  ds >> tile.backgroundDamageType;
  ds >> tile.liquidId;
  ds >> tile.liquidLevel;

  return ds;
}

DataStream& operator<<(DataStream& ds, RenderTile const& tile) {
  ds << tile.foreground;
  ds << tile.foregroundHueShift;
  ds << tile.foregroundMod;
  ds << tile.foregroundModHueShift;
  ds << tile.foregroundColorVariant;
  ds << tile.foregroundDamageLevel;
  ds << tile.foregroundDamageType;
  ds << tile.background;
  ds << tile.backgroundHueShift;
  ds << tile.backgroundMod;
  ds << tile.backgroundModHueShift;
  ds << tile.backgroundColorVariant;
  ds << tile.backgroundDamageLevel;
  ds << tile.backgroundDamageType;
  ds << tile.liquidId;
  ds << tile.liquidLevel;

  return ds;
}

}
