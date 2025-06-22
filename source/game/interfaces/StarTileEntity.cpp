#include "StarTileEntity.hpp"
#include "StarWorld.hpp"
#include "StarRoot.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

DataStream& operator<<(DataStream& ds, MaterialSpace const& materialSpace) {
  ds.write(materialSpace.space);
  ds.write(materialSpace.material);
  return ds;
}

DataStream& operator>>(DataStream& ds, MaterialSpace& materialSpace) {
  ds.read(materialSpace.space);
  ds.read(materialSpace.material);
  return ds;
}

TileEntity::TileEntity() {
  setPersistent(true);
}

Vec2F TileEntity::position() const {
  return Vec2F(tilePosition());
}

List<Vec2I> TileEntity::spaces() const {
  return {};
}

List<Vec2I> TileEntity::roots() const {
  return {};
}

List<MaterialSpace> TileEntity::materialSpaces() const {
  return {};
}

bool TileEntity::damageTiles(List<Vec2I> const&, Vec2F const&, TileDamage const&) {
  return false;
}

bool TileEntity::canBeDamaged() const {
  return true;
}

bool TileEntity::isInteractive() const {
  return false;
}

List<Vec2I> TileEntity::interactiveSpaces() const {
  return spaces();
}

InteractAction TileEntity::interact(InteractRequest const& request) {
  _unused(request);
  return InteractAction();
}

List<QuestArcDescriptor> TileEntity::offeredQuests() const {
  return {};
}

StringSet TileEntity::turnInQuests() const {
  return StringSet();
}

Vec2F TileEntity::questIndicatorPosition() const {
  return position();
}

bool TileEntity::anySpacesOccupied(List<Vec2I> const& spaces) const {
  Vec2I tp = tilePosition();
  for (auto pos : spaces) {
    pos += tp;
    if (isConnectableMaterial(world()->material(pos, TileLayer::Foreground)))
      return true;
  }

  return false;
}

bool TileEntity::allSpacesOccupied(List<Vec2I> const& spaces) const {
  Vec2I tp = tilePosition();
  for (auto pos : spaces) {
    pos += tp;
    if (!isConnectableMaterial(world()->material(pos, TileLayer::Foreground)))
      return false;
  }

  return true;
}

float TileEntity::spacesLiquidFillLevel(List<Vec2I> const& relativeSpaces) const {
  float total = 0.0f;
  for (auto pos : relativeSpaces) {
    pos += tilePosition();
    total += world()->liquidLevel(pos).level;
  }
  return total / relativeSpaces.size();
}

}
