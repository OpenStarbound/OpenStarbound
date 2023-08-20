#include "StarMaterialItem.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarWorld.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldTemplate.hpp"
#include "StarInput.hpp"

namespace Star {

constexpr int BlockRadiusLimit = 16;

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
  m_blockRadius = instanceValue("blockRadius", defaultParameters.getFloat("blockRadius")).toFloat();
  m_altBlockRadius = instanceValue("altBlockRadius", defaultParameters.getFloat("altBlockRadius")).toFloat();

  auto materialDatabase = Root::singleton().materialDatabase();
  auto multiplace = instanceValue("allowMultiplace", BlockCollisionSet.contains(materialDatabase->materialCollisionKind(m_material)));
  if (multiplace.isType(Json::Type::Bool))
    m_multiplace = multiplace.toBool();
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
  m_lastTileAreaRadiusCache = 0.0f;
  m_collisionOverride = TileCollisionOverrideNames.maybeLeft(instanceValue("collisionOverride", "None").toString()).value(TileCollisionOverride::None);
}

ItemPtr MaterialItem::clone() const {
  return make_shared<MaterialItem>(*this);
}

void MaterialItem::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
  owner->addSound(Random::randValueFrom(m_placeSounds), 1.0f, 2.0f);
}

void MaterialItem::uninit() {
  m_lastAimPosition.reset();
}

void MaterialItem::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
  float radius = calcRadius(shifting);
  if (radius == 1)
    setEnd(BeamItem::EndType::Tile);
  else
    setEnd(BeamItem::EndType::TileGroup);
  m_shifting = shifting;

  if (owner()->isMaster()) {
    Input& input = Input::singleton();
    if (auto presses = input.bindDown("opensb", "materialCollisionCycle")) {
      CollisionKind baseKind = Root::singleton().materialDatabase()->materialCollisionKind(m_material);
      for (size_t i = 0; i != *presses; ++i) {
        constexpr auto limit = (uint8_t)TileCollisionOverride::Block + 1;
        while (true) {
          m_collisionOverride = TileCollisionOverride(((uint8_t)m_collisionOverride + 1) % limit);
          setInstanceValue("collisionOverride", TileCollisionOverrideNames.getRight(m_collisionOverride));
          if (collisionKindFromOverride(m_collisionOverride) != baseKind)
            break;
        }
      }
      owner()->addSound("/sfx/tools/cyclematcollision.ogg", 1.0f, Random::randf(0.9f, 1.1f));
    }

    if (auto presses = input.bindDown("opensb", "materialRadiusGrow")) {
      m_blockRadius = min(BlockRadiusLimit, int(m_blockRadius + *presses));
      setInstanceValue("blockRadius", m_blockRadius);
      owner()->addSound("/sfx/tools/matradiusgrow.wav", 1.0f, 1.0f + m_blockRadius / BlockRadiusLimit);
    }

    if (auto presses = input.bindDown("opensb", "materialRadiusShrink")) {
      m_blockRadius = max(1, int(m_blockRadius - *presses));
      setInstanceValue("blockRadius", m_blockRadius);
      owner()->addSound("/sfx/tools/matradiusshrink.wav", 1.0f, 1.0f + m_blockRadius / BlockRadiusLimit);
    }
  }
  else {
    auto blockRadius = instanceValue("blockRadius");
    if (blockRadius.isType(Json::Type::Float))
      m_blockRadius = blockRadius.toFloat();

    auto altBlockRadius = instanceValue("altBlockRadius");
    if (altBlockRadius.isType(Json::Type::Float))
      m_altBlockRadius = altBlockRadius.toFloat();

    auto collisionOverride = instanceValue("collisionOverride");
    if (collisionOverride.isType(Json::Type::String))
      m_collisionOverride = TileCollisionOverrideNames.maybeLeft(collisionOverride.toString()).value(TileCollisionOverride::None);
  }
}

void MaterialItem::render(RenderCallback* renderCallback, EntityRenderLayer renderLayer) {
  if (m_collisionOverride != TileCollisionOverride::None) {
    float pulseLevel = 1.f - 0.3f * 0.5f * ((float)sin(2 * Constants::pi * 4.0 * Time::monotonicTime()) + 1.f);
    Color color = Color::rgba(owner()->favoriteColor()).mix(Color::White);
    color.setAlphaF(color.alphaF() * pulseLevel * 0.95f);
    auto addIndicator = [&](String const& path) {
      Vec2F basePosition = Vec2F(0.5f, 0.5f);
      auto indicator = Drawable::makeImage(path, 1.0f / TilePixels, true, basePosition);
      indicator.fullbright = true;
      indicator.color = color;
      for (auto& tilePos : tileArea(calcRadius(m_shifting), owner()->aimPosition())) {
        indicator.position = basePosition + Vec2F(tilePos);
        renderCallback->addDrawable(indicator, RenderLayerForegroundTile);
      }
    };

    if (m_collisionOverride == TileCollisionOverride::Empty)
      addIndicator("/interface/building/collisionempty.png");
    else if (m_collisionOverride == TileCollisionOverride::Platform)
      addIndicator("/interface/building/collisionplatform.png");
    else if (m_collisionOverride == TileCollisionOverride::Block)
      addIndicator("/interface/building/collisionblock.png");
  }
}

List<Drawable> MaterialItem::nonRotatedDrawables() const {
  return beamDrawables(canPlace(m_shifting));
}

void MaterialItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!initialized() || !ready())
    return;

  auto layer = (mode == FireMode::Primary || !twoHanded() ? TileLayer::Foreground : TileLayer::Background);
  TileModificationList modifications;

  float radius = calcRadius(shifting);

  auto geo = world()->geometry();
  auto aimPosition = owner()->aimPosition();

  if (!m_lastAimPosition)
    m_lastAimPosition = aimPosition;

  unsigned steps = 1;
  Vec2F diff = {};
  if (*m_lastAimPosition != aimPosition) {
    diff = geo.diff(*m_lastAimPosition, aimPosition);
    float magnitude = diff.magnitude();
    float limit = max(4.f, 64.f / radius);
    if (magnitude > limit) {
      diff = diff.normalized() * limit;
      magnitude = limit;
    }

    steps = (unsigned)ceil(magnitude * (Constants::pi / 2));
  }

  size_t total = 0;
  for (int i = 0; i != steps; ++i) {
    auto placementOrigin = aimPosition + diff * (1.0f - ((float)i / steps));
    if (!owner()->inToolRange(placementOrigin))
      continue;

    for (Vec2I& pos : tileArea(radius, placementOrigin))
      modifications.emplaceAppend(pos, PlaceMaterial{layer, materialId(), placementHueShift(pos), m_collisionOverride});

    // Make sure not to make any more modifications than we have consumables.
    if (modifications.size() > count())
      modifications.resize(count());
    size_t failed = world()->applyTileModifications(modifications, false).size();
    if (failed < modifications.size()) {
      size_t placed = modifications.size() - failed;
      consume(placed);
      total += placed;
    }
  }

  if (total) {
    float intensity = clamp(sqrt((float)total) / 16, 0.0f, 1.0f);
    owner()->addSound(Random::randValueFrom(m_placeSounds), 1.0f + intensity, (1.125f - intensity * 0.75f) * Random::randf(0.9f, 1.1f));
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

float MaterialItem::calcRadius(bool shifting) const {
  if (!multiplaceEnabled())
    return 1;
  else
    return !shifting ? m_blockRadius : m_altBlockRadius;
}

List<Vec2I>& MaterialItem::tileArea(float radius, Vec2F const& position) const {
  if (m_lastTileAreaOriginCache != position || m_lastTileAreaRadiusCache != radius) {
    m_lastTileAreaOriginCache = position;
    m_lastTileAreaRadiusCache = radius;
    m_tileAreasCache = tileAreaBrush(radius, position, true);
  }
  return m_tileAreasCache;
}

MaterialHue MaterialItem::materialHueShift() const {
  return m_materialHueShift;
}

bool MaterialItem::canPlace(bool shifting) const {
  if (initialized()) {
    MaterialId material = materialId();

    float radius = calcRadius(shifting);

    for (auto& pos : tileArea(radius, owner()->aimPosition())) {
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

float& MaterialItem::blockRadius() {
  return m_blockRadius;
}

float& MaterialItem::altBlockRadius() {
  return m_altBlockRadius;
}

TileCollisionOverride& MaterialItem::collisionOverride() {
  return m_collisionOverride;
}

List<PreviewTile> MaterialItem::preview(bool shifting) const {
  List<PreviewTile> result;
  if (initialized()) {
    Color lightColor = Color::rgba(owner()->favoriteColor());
    Vec3B light = lightColor.toRgb();

    auto material = materialId();
    auto color = DefaultMaterialColorVariant;

    size_t c = 0;
    for (auto& pos : tileArea(calcRadius(shifting), owner()->aimPosition())) {
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
