#include "StarMaterialItem.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarWorld.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldTemplate.hpp"

namespace Star {

MaterialItem::MaterialItem(Json const& config, String const& directory, Json const& settings)
  : Item(config, directory, settings), FireableItem(config), BeamItem(config) {
  m_material = config.getInt("materialId");
  m_materialHueShift = materialHueFromDegrees(instanceValue("materialHueShift", 0).toFloat());

  if (materialHueShift() != MaterialHue()) {
    auto drawables = iconDrawables();
    for (auto& d : drawables) {
      if (d.isImage()) {
        String image = strf("?hueshift={}", materialHueToDegrees(m_materialHueShift));
        d.imagePart().addDirectives(image, false);
      }
    }
    setIconDrawables(move(drawables));
  }

  setTwoHanded(config.getBool("twoHanded", true));

  auto defaultParameters = Root::singleton().assets()->json("/items/defaultParameters.config");
  setCooldownTime(config.queryFloat("materialItems.cooldown", defaultParameters.queryFloat("materialItems.cooldown")));
  m_blockRadius = config.getFloat("blockRadius", defaultParameters.getFloat("blockRadius"));
  m_altBlockRadius = config.getFloat("altBlockRadius", defaultParameters.getFloat("altBlockRadius"));

  auto materialDatabase = Root::singleton().materialDatabase();
  m_multiplace = config.getBool("allowMultiplace", BlockCollisionSet.contains(materialDatabase->materialCollisionKind(m_material)));
  m_placeSounds = jsonToStringList(config.get("placeSounds", JsonArray()));
  if (m_placeSounds.empty()) {
    auto miningSound = materialDatabase->miningSound(m_material);
    if (!miningSound.empty())
      m_placeSounds.append(move(miningSound));
    auto stepSound = materialDatabase->footstepSound(m_material);
    if (!stepSound.empty())
      m_placeSounds.append(move(stepSound));
    else if (m_placeSounds.empty())
      m_placeSounds.append(materialDatabase->defaultFootstepSound());
  }
  m_shifting = false;
}

ItemPtr MaterialItem::clone() const {
  return make_shared<MaterialItem>(*this);
}

void MaterialItem::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
}

void MaterialItem::uninit() {
  m_lastAimPosition.reset();
}

void MaterialItem::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
  if (shifting || !multiplaceEnabled())
    setEnd(BeamItem::EndType::Tile);
  else
    setEnd(BeamItem::EndType::TileGroup);
  m_shifting = shifting;
}

List<Drawable> MaterialItem::nonRotatedDrawables() const {
  return beamDrawables(canPlace(m_shifting));
}

void MaterialItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!initialized() || !ready() || !owner()->inToolRange())
    return;

  auto layer = (mode == FireMode::Primary || !twoHanded() ? TileLayer::Foreground : TileLayer::Background);
  TileModificationList modifications;

  float radius;

  if (!shifting)
    radius = m_blockRadius;
  else
    radius = m_altBlockRadius;

  if (!multiplaceEnabled())
    radius = 1;

  auto geo = world()->geometry();
  auto aimPosition = owner()->aimPosition();
  if (!m_lastAimPosition)
    m_lastAimPosition = aimPosition;

  unsigned steps = 1;
  Vec2F diff = {};
  if (*m_lastAimPosition != aimPosition) {
    diff = geo.diff(*m_lastAimPosition, aimPosition);
    float magnitude = diff.magnitude();
    if (magnitude > 32) {
      m_lastAimPosition = aimPosition + diff.normalized() * 32;
      magnitude = 32;
    }

    steps = (int)ceil(magnitude);
  }

  unsigned total = 0;
  bool fail = true;
  for (int i = 0; i != steps; ++i) {
    auto placementOrigin = aimPosition + diff * ((float)i / steps);
    for (Vec2I pos : tileAreaBrush(radius, placementOrigin, true))
      modifications.append({ pos, PlaceMaterial{layer, materialId(), placementHueShift(pos)} });

    // Make sure not to make any more modifications than we have consumables.
    if (modifications.size() > count())
      modifications.resize(count());
    size_t failed = world()->applyTileModifications(modifications, false).size();
    if (failed < modifications.size()) {
      fail = false;
      unsigned placed = modifications.size() - failed;
      consume(placed);
      total += placed;
    }
  }

  if (!fail) {
    auto sound = Random::randValueFrom(m_placeSounds, "");
    if (total && !sound.empty()) {
      float intensity = clamp((float)total / 20, 0.0f, 1.0f);
      owner()->addSound(sound, 1.0f + intensity, (1.25f - intensity * 0.75f) * Random::randf(0.9f, 1.1f));
    }
    FireableItem::fire(mode, shifting, edgeTriggered);
  }

  m_lastAimPosition = aimPosition;
}

void MaterialItem::endFire(FireMode mode, bool shifting) {
  m_lastAimPosition.reset();
}

MaterialId MaterialItem::materialId() const {
  return m_material;
}

MaterialHue MaterialItem::materialHueShift() const {
  return m_materialHueShift;
}

bool MaterialItem::canPlace(bool shifting) const {
  if (initialized()) {
    MaterialId material = materialId();

    float radius;
    if (!shifting)
      radius = m_blockRadius;
    else
      radius = m_altBlockRadius;

    if (!multiplaceEnabled())
      radius = 1;

    for (auto pos : tileAreaBrush(radius, owner()->aimPosition(), true)) {
      MaterialHue hueShift = placementHueShift(pos);
      if (world()->canModifyTile(pos, PlaceMaterial{TileLayer::Foreground, material, hueShift}, false)
          || world()->canModifyTile(pos, PlaceMaterial{TileLayer::Background, material, hueShift}, false))
        return true;
    }
  }
  return false;
}

bool MaterialItem::multiplaceEnabled() const {
  return m_multiplace && count() > 1;
}

List<PreviewTile> MaterialItem::preview(bool shifting) const {
  List<PreviewTile> result;
  if (initialized()) {
    Color lightColor = Color::rgba(owner()->favoriteColor());
    Vec3B light = lightColor.toRgb();

    auto material = materialId();
    auto color = DefaultMaterialColorVariant;

    float radius;

    if (!shifting)
      radius = m_blockRadius;
    else
      radius = m_altBlockRadius;

    if (!multiplaceEnabled())
      radius = 1;

    size_t c = 0;

    for (auto pos : tileAreaBrush(radius, owner()->aimPosition(), true)) {
      MaterialHue hueShift = placementHueShift(pos);
      if (c >= count())
        break;
      if (world()->canModifyTile(pos, PlaceMaterial{TileLayer::Foreground, material, hueShift}, false)) {
        result.append({pos, true, material, hueShift, true});
        c++;
      } else if (twoHanded()
          && world()->canModifyTile(pos, PlaceMaterial{TileLayer::Background, material, hueShift}, false)) {
        result.append({pos, true, material, hueShift, true, light, true, color});
        c++;
      } else {
        result.append({pos, true, material, hueShift, true});
      }
    }
  }
  return result;
}

MaterialHue MaterialItem::placementHueShift(Vec2I const& pos) const {
  if (auto hue = instanceValue("materialHueShift")) {
    return materialHueFromDegrees(hue.toFloat());
  } else if (auto worldClient = as<WorldClient>(world())) {
    auto worldTemplate = worldClient->currentTemplate();
    return worldTemplate->biomeMaterialHueShift(worldTemplate->blockBiomeIndex(pos[0], pos[1]), m_material);
  } else {
    return materialHueShift();
  }
}

}
