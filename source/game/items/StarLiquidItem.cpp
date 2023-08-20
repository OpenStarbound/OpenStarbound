#include "StarLiquidItem.hpp"
#include "StarJson.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarWorld.hpp"

namespace Star {

LiquidItem::LiquidItem(Json const& config, String const& directory, Json const& settings)
  : Item(config, directory, settings), FireableItem(config), BeamItem(config) {
  m_liquidId = Root::singleton().liquidsDatabase()->liquidId(config.getString("liquid"));

  setTwoHanded(config.getBool("twoHanded", true));

  auto assets = Root::singleton().assets();
  m_quantity = assets->json("/items/defaultParameters.config:liquidItems.bucketSize").toUInt();
  setCooldownTime(assets->json("/items/defaultParameters.config:liquidItems.cooldown").toFloat());
  m_blockRadius = assets->json("/items/defaultParameters.config:blockRadius").toFloat();
  m_altBlockRadius = assets->json("/items/defaultParameters.config:altBlockRadius").toFloat();
  m_shifting = false;
}

ItemPtr LiquidItem::clone() const {
  return make_shared<LiquidItem>(*this);
}

void LiquidItem::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
}

void LiquidItem::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
  if (shifting || !multiplaceEnabled())
    setEnd(BeamItem::EndType::Tile);
  else
    setEnd(BeamItem::EndType::TileGroup);

  m_shifting = shifting;
}

List<Drawable> LiquidItem::nonRotatedDrawables() const {
  return beamDrawables(canPlace(m_shifting));
}

void LiquidItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!initialized() || !ready() || !owner()->inToolRange())
    return;

  PlaceLiquid placeLiquid{liquidId(), liquidQuantity()};
  TileModificationList modifications;

  float radius;

  if (!shifting)
    radius = m_blockRadius;
  else
    radius = m_altBlockRadius;

  if (!multiplaceEnabled())
    radius = 1;

  for (auto pos : tileAreaBrush(radius, owner()->aimPosition(), true)) {
    if (canPlaceAtTile(pos))
      modifications.append({pos, placeLiquid});
  }

  // Make sure not to make any more modifications than we have consumables.
  if (modifications.size() > count())
    modifications.resize(count());
  size_t failed = world()->applyTileModifications(modifications, false).size();
  if (failed < modifications.size()) {
    FireableItem::fire(mode, shifting, edgeTriggered);
    consume(modifications.size() - failed);
  }
}

LiquidId LiquidItem::liquidId() const {
  return m_liquidId;
}

float LiquidItem::liquidQuantity() const {
  return m_quantity;
}

List<PreviewTile> LiquidItem::previewTiles(bool shifting) const {
  List<PreviewTile> result;
  if (initialized()) {
    auto liquid = liquidId();

    float radius;
    if (!shifting)
      radius = m_blockRadius;
    else
      radius = m_altBlockRadius;

    if (!multiplaceEnabled())
      radius = 1;

    size_t c = 0;

    for (auto pos : tileAreaBrush(radius, owner()->aimPosition(), true)) {
      if (c >= count())
        break;
      if (canPlaceAtTile(pos))
        c++;
      result.append({pos, liquid});
    }
  }
  return result;
}

bool LiquidItem::canPlace(bool shifting) const {
  if (initialized()) {
    float radius;
    if (!shifting)
      radius = m_blockRadius;
    else
      radius = m_altBlockRadius;

    if (!multiplaceEnabled())
      radius = 1;

    for (auto pos : tileAreaBrush(radius, owner()->aimPosition(), true)) {
      if (canPlaceAtTile(pos))
        return true;
    }
  }
  return false;
}

bool LiquidItem::canPlaceAtTile(Vec2I pos) const {
  auto bgTileMaterial = world()->material(pos, TileLayer::Background);
  if (bgTileMaterial != EmptyMaterialId) {
    auto fgTileMaterial = world()->material(pos, TileLayer::Foreground);
    if (fgTileMaterial == EmptyMaterialId) {
      auto tileLiquid = world()->liquidLevel(pos).liquid;
      if (tileLiquid == EmptyLiquidId || tileLiquid == liquidId())
        return true;
    }
  }
  return false;
}

bool LiquidItem::multiplaceEnabled() const {
  return (count() > 1);
}

}
