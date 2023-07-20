#include "StarTools.hpp"
#include "StarRoot.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarWiring.hpp"
#include "StarWorld.hpp"
#include "StarWorldClient.hpp"
#include "StarParticleDatabase.hpp"

namespace Star {

MiningTool::MiningTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), SwingableItem(config) {
  auto assets = Root::singleton().assets();

  m_image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_frames = instanceValue("frames", 1).toInt();
  m_frameCycle = instanceValue("animationCycle", 1.0f).toFloat();
  m_frameTiming = 0;
  for (size_t i = 0; i < (size_t)m_frames; i++)
    m_animationFrame.append(m_image.replace("{frame}", toString(i)));
  m_idleFrame = m_image.replace("{frame}", "idle");
  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_blockRadius = instanceValue("blockRadius").toFloat();
  m_altBlockRadius = instanceValue("altBlockRadius").toFloat();
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_breakSound = instanceValue("breakSound", "").toString();
  m_pointable = instanceValue("pointable", false).toBool();

  m_toolVolume = assets->json("/sfx.config:miningToolVolume").toFloat();
  m_blockVolume = assets->json("/sfx.config:miningBlockVolume").toFloat();
}

ItemPtr MiningTool::clone() const {
  return make_shared<MiningTool>(*this);
}

List<Drawable> MiningTool::drawables() const {
  if (m_frameTiming == 0) {
    return {Drawable::makeImage(m_idleFrame, 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  } else {
    int frame = std::max(0, std::min(m_frames - 1, (int)std::floor((m_frameTiming / m_frameCycle) * m_frames)));
    return {Drawable::makeImage(m_animationFrame[frame], 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  }
}

Vec2F MiningTool::handPosition() const {
  return m_handPosition;
}

void MiningTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  auto materialDatabase = Root::singleton().materialDatabase();

  if (initialized()) {
    bool used = false;
    int radius = !shifting ? m_blockRadius : m_altBlockRadius;
    String blockSound;
    List<Vec2I> brushArea;

    auto layer = (mode == FireMode::Primary ? TileLayer::Foreground : TileLayer::Background);
    if (owner()->isAdmin() || owner()->inToolRange()) {
      brushArea = tileAreaBrush(radius, owner()->aimPosition(), true);
      for (auto pos : brushArea) {
        blockSound = materialDatabase->miningSound(world()->material(pos, layer), world()->mod(pos, layer));
        if (!blockSound.empty())
          break;
      }
      if (blockSound.empty()) {
        for (auto pos : brushArea) {
          blockSound = materialDatabase->footstepSound(world()->material(pos, layer), world()->mod(pos, layer));
          if (!blockSound.empty()
              && blockSound != Root::singleton().assets()->json("/client.config:defaultFootstepSound").toString())
            break;
        }
      }

      TileDamage damage;
      damage.type = TileDamageTypeNames.getLeft(instanceValue("tileDamageType", "blockish").toString());

      if (durabilityStatus() == 0)
        damage.amount = instanceValue("tileDamageBlunted", 0.1f).toFloat();
      else
        damage.amount = instanceValue("tileDamage", 1.0f).toFloat();

      damage.harvestLevel = instanceValue("harvestLevel", 1).toUInt();

      auto damageResult = world()->damageTiles(brushArea, layer, owner()->position(), damage, owner()->entityId());

      if (damageResult != TileDamageResult::None) {
        used = true;
        if (!owner()->isAdmin())
          changeDurability(instanceValue("durabilityPerUse", 1.0f).toFloat());
      }

      if (damageResult == TileDamageResult::Protected) {
        blockSound = Root::singleton().assets()->json("/client.config:defaultDingSound").toString();
      }
    }

    if (used) {
      owner()->addSound(Random::randValueFrom(m_strikeSounds), m_toolVolume);
      owner()->addSound(blockSound, m_blockVolume);
      List<Particle> miningParticles;
      for (auto pos : brushArea) {
        if (auto miningParticleConfig = materialDatabase->miningParticle(world()->material(pos, layer), world()->mod(pos, layer))) {
          auto miningParticle = miningParticleConfig->instance();
          miningParticle.position += (Vec2F)pos;
          miningParticles.append(miningParticle);
        }
      }
      owner()->addParticles(miningParticles);
      SwingableItem::fire(mode, shifting, edgeTriggered);
    }
  }
}

void MiningTool::update(float dt, FireMode mode, bool shifting, HashSet<MoveControlType> const& moves) {
  SwingableItem::update(dt, mode, shifting, moves);

  if (!ready() && !coolingDown())
    m_frameTiming = std::fmod((m_frameTiming + dt), m_frameCycle);
  else
    m_frameTiming = 0;
}

float MiningTool::durabilityStatus() {
  return clamp(
      1.0f - instanceValue("durabilityHit", 0.0f).toFloat() / instanceValue("durability").toFloat(), 0.0f, 1.0f);
}

float MiningTool::getAngle(float aimAngle) {
  if ((!ready() && !coolingDown()) || !m_pointable)
    return SwingableItem::getAngle(aimAngle);
  return aimAngle;
}

void MiningTool::changeDurability(float amount) {
  setInstanceValue("durabilityHit", clamp(instanceValue("durabilityHit", 0.0f).toFloat() + amount, 0.0f, instanceValue("durability").toFloat()));
  if (durabilityStatus() == 0.0f && !instanceValue("canBeRepaired", false).toBool()) {
    owner()->addSound(m_breakSound);
    consume(1);
  }
}

HarvestingTool::HarvestingTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), SwingableItem(config) {
  auto assets = Root::singleton().assets();

  m_image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_frames = instanceValue("frames", 1).toInt();
  m_frameCycle = instanceValue("animationCycle", 1.0f).toFloat();
  for (size_t i = 0; i < (size_t)m_frames; i++)
    m_animationFrame.append(m_image.replace("{frame}", toString(i)));
  m_idleFrame = m_image.replace("{frame}", "idle");

  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_toolVolume = assets->json("/sfx.config:harvestToolVolume").toFloat();
  m_harvestPower = instanceValue("harvestPower", 1.0f).toFloat();
  m_frameTiming = 0;
}

ItemPtr HarvestingTool::clone() const {
  return make_shared<HarvestingTool>(*this);
}

List<Drawable> HarvestingTool::drawables() const {
  if (m_frameTiming == 0)
    return {Drawable::makeImage(m_idleFrame, 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  else {
    int frame = std::max(0, std::min(m_frames - 1, (int)std::floor((m_frameTiming / m_frameCycle) * m_frames)));
    return {Drawable::makeImage(m_animationFrame[frame], 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  }
}

Vec2F HarvestingTool::handPosition() const {
  return m_handPosition;
}

void HarvestingTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  if (owner()) {
    bool used = false;

    if (owner()->isAdmin() || owner()->inToolRange()) {
      auto layer = (mode == FireMode::Primary ? TileLayer::Foreground : TileLayer::Background);
      used = world()->damageTile(Vec2I::floor(owner()->aimPosition()), layer, owner()->position(), {TileDamageType::Plantish, m_harvestPower}) != TileDamageResult::None;
    }

    if (used) {
      owner()->addSound(Random::randValueFrom(m_strikeSounds), m_toolVolume);
      SwingableItem::fire(mode, shifting, edgeTriggered);
    }
  }
}

void HarvestingTool::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  SwingableItem::update(dt, fireMode, shifting, moves);

  if (!ready() && !coolingDown())
    m_frameTiming = std::fmod((m_frameTiming + dt), m_frameCycle);
  else
    m_frameTiming = 0;
}

float HarvestingTool::getAngle(float aimAngle) {
  if (!ready() && !coolingDown())
    return SwingableItem::getAngle(aimAngle);
  return aimAngle;
}

Flashlight::Flashlight(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters) {
  m_image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_lightPosition = jsonToVec2F(instanceValue("lightPosition"));
  m_lightColor = jsonToColor(instanceValue("lightColor"));
  m_beamWidth = instanceValue("beamLevel").toFloat();
  m_ambientFactor = instanceValue("beamAmbience").toFloat();
}

ItemPtr Flashlight::clone() const {
  return make_shared<Flashlight>(*this);
}

List<Drawable> Flashlight::drawables() const {
  return {Drawable::makeImage(m_image, 1.0f / TilePixels, true, -m_handPosition / TilePixels)};
}

List<LightSource> Flashlight::lightSources() const {
  if (!initialized())
    return {};

  float angle = world()->geometry().diff(owner()->aimPosition(), owner()->position()).angle();
  LightSource lightSource;
  lightSource.pointLight = true;
  lightSource.position = owner()->position() + owner()->handPosition(hand(), (m_lightPosition - m_handPosition) / TilePixels);
  lightSource.color = m_lightColor.toRgb();
  lightSource.pointBeam = m_beamWidth;
  lightSource.beamAngle = angle;
  lightSource.beamAmbience = m_ambientFactor;
  return {move(lightSource)};
}

WireTool::WireTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), FireableItem(config), BeamItem(config.setAll(parameters.toObject())) {
  auto assets = Root::singleton().assets();

  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_toolVolume = assets->json("/sfx.config:miningToolVolume").toFloat();
  m_wireConnector = 0;
  m_endType = EndType::Wire;
}

ItemPtr WireTool::clone() const {
  return make_shared<WireTool>(*this);
}

void WireTool::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
  m_wireConnector = 0;
}

void WireTool::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
}

List<Drawable> WireTool::drawables() const {
  return BeamItem::drawables();
}

List<Drawable> WireTool::nonRotatedDrawables() const {
  if (m_wireConnector && m_wireConnector->connecting())
    return BeamItem::nonRotatedDrawables();
  return {};
}

void WireTool::setEnd(EndType) {
  m_endType = EndType::Wire;
}

Vec2F WireTool::handPosition() const {
  return m_handPosition;
}

void WireTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  auto ownerp = owner();
  auto worldp = world();

  if (ownerp && worldp && m_wireConnector) {
    Vec2F pos(ownerp->aimPosition());
    if (ownerp->isAdmin() || ownerp->inToolRange()) {
      auto swingResult = m_wireConnector->swing(worldp->geometry(), pos, mode);
      if (swingResult == WireConnector::Connect) {
        ownerp->addSound(Random::randValueFrom(m_strikeSounds), m_toolVolume);
        FireableItem::fire(mode, shifting, edgeTriggered);
      } else if (swingResult == WireConnector::Mismatch || swingResult == WireConnector::Protected) {
        auto wireErrorSound = Root::singleton().assets()->json("/client.config:wireFailSound").toString();
        ownerp->addSound(wireErrorSound, m_toolVolume);
        FireableItem::fire(mode, shifting, edgeTriggered);
      }
    }
  }
}

float WireTool::getAngle(float aimAngle) {
  return BeamItem::getAngle(aimAngle);
}

void WireTool::setConnector(WireConnector* connector) {
  m_wireConnector = connector;
}

BeamMiningTool::BeamMiningTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), FireableItem(config), BeamItem(config.setAll(parameters.toObject())) {
  auto assets = Root::singleton().assets();

  m_blockRadius = instanceValue("blockRadius").toFloat();
  m_altBlockRadius = instanceValue("altBlockRadius").toFloat();
  m_tileDamage = instanceValue("tileDamage", 1.0f).toFloat();
  m_harvestLevel = instanceValue("harvestLevel", 1).toUInt();
  m_canCollectLiquid = instanceValue("canCollectLiquid", false).toBool();
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_toolVolume = assets->json("/sfx.config:miningToolVolume").toFloat();
  m_blockVolume = assets->json("/sfx.config:miningBlockVolume").toFloat();
  m_endType = EndType::Object;

  m_inhandStatusEffects = instanceValue("inhandStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
}

ItemPtr BeamMiningTool::clone() const {
  return make_shared<BeamMiningTool>(*this);
}

List<Drawable> BeamMiningTool::drawables() const {
  return BeamItem::drawables();
}

void BeamMiningTool::setEnd(EndType) {
  m_endType = EndType::Object;
}

List<PreviewTile> BeamMiningTool::preview(bool shifting) const {
  List<PreviewTile> result;
  auto ownerp = owner();
  auto worldp = world();

  if (ownerp && worldp) {
    if (ownerp->isAdmin() || ownerp->inToolRange()) {
      Vec3B light = Color::rgba(ownerp->favoriteColor()).toRgb();
      int radius = !shifting ? m_blockRadius : m_altBlockRadius;
      for (auto pos : tileAreaBrush(radius, ownerp->aimPosition(), true)) {
        if (worldp->tileIsOccupied(pos, TileLayer::Foreground, true)) {
          result.append({pos, true, light, true});
        } else if (worldp->tileIsOccupied(pos, TileLayer::Background, true)) {
          result.append({pos, false, light, true});
        }
      }
    }
  }
  return result;
}

void BeamMiningTool::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
}

void BeamMiningTool::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
}

List<PersistentStatusEffect> BeamMiningTool::statusEffects() const {
  return m_inhandStatusEffects;
}

List<Drawable> BeamMiningTool::nonRotatedDrawables() const {
  if (!ready() && !coolingDown())
    return BeamItem::nonRotatedDrawables();
  return {};
}

void BeamMiningTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  auto materialDatabase = Root::singleton().materialDatabase();

  auto worldp = world();
  auto ownerp = owner();
  if (ownerp && worldp) {
    bool used = false;
    int radius = !shifting ? m_blockRadius : m_altBlockRadius;
    String blockSound;
    List<Vec2I> brushArea;

    auto layer = (mode == FireMode::Primary ? TileLayer::Foreground : TileLayer::Background);
    if (ownerp->isAdmin() || ownerp->inToolRange()) {
      brushArea = tileAreaBrush(radius, ownerp->aimPosition(), true);
      auto aimPosition = Vec2I(ownerp->aimPosition());

      for (auto pos : brushArea) {
        blockSound = materialDatabase->miningSound(worldp->material(pos, layer), worldp->mod(pos, layer));
        if (!blockSound.empty())
          break;
      }
      if (blockSound.empty()) {
        for (auto pos : brushArea) {
          blockSound = materialDatabase->footstepSound(worldp->material(pos, layer), worldp->mod(pos, layer));
          if (!blockSound.empty()
              && blockSound != Root::singleton().assets()->json("/client.config:defaultFootstepSound").toString())
            break;
        }
      }

      auto damageResult = worldp->damageTiles(List<Vec2I>{brushArea}, layer, ownerp->position(), {TileDamageType::Beamish, m_tileDamage, m_harvestLevel}, ownerp->entityId());
      used = damageResult != TileDamageResult::None;

      if (damageResult == TileDamageResult::Protected) {
        blockSound = Root::singleton().assets()->json("/client.config:defaultDingSound").toString();
      }

      if (!used && m_canCollectLiquid && layer == TileLayer::Foreground && worldp->material(aimPosition, TileLayer::Foreground) == EmptyMaterialId) {
        auto targetLiquid = worldp->liquidLevel(aimPosition).liquid;
        List<Vec2I> drainTiles;
        float totalLiquid = 0;
        for (auto pos : brushArea) {
          if (worldp->isTileProtected(pos))
            continue;

          auto liquid = worldp->liquidLevel(pos);
          if (liquid.liquid != EmptyLiquidId) {
            if (targetLiquid == EmptyLiquidId)
              targetLiquid = liquid.liquid;

            if (liquid.liquid == targetLiquid) {
              totalLiquid += liquid.level;
              drainTiles.append(pos);
            }
          }
        }

        float bucketSize = Root::singleton().assets()->json("/items/defaultParameters.config:liquidItems.bucketSize").toUInt();
        if (totalLiquid >= bucketSize) {
          if (auto clientWorld = as<WorldClient>(worldp))
            clientWorld->collectLiquid(drainTiles, targetLiquid);

          blockSound = Root::singleton().assets()->json("/items/defaultParameters.config:liquidBlockSound").toString();

          used = true;
        }
      }
    }

    if (used) {
      ownerp->addSound(Random::randValueFrom(m_strikeSounds), m_toolVolume);
      ownerp->addSound(blockSound, m_blockVolume);
      List<Particle> miningParticles;
      for (auto pos : brushArea) {
        if (auto miningParticleConfig = materialDatabase->miningParticle(worldp->material(pos, layer), worldp->mod(pos, layer))) {
          auto miningParticle = miningParticleConfig->instance();
          miningParticle.position += (Vec2F)pos;
          miningParticles.append(miningParticle);
        }
      }
      ownerp->addParticles(miningParticles);
      FireableItem::fire(mode, shifting, edgeTriggered);
    }
  }
}

float BeamMiningTool::getAngle(float angle) {
  return BeamItem::getAngle(angle);
}

TillingTool::TillingTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), SwingableItem(config) {
  auto assets = Root::singleton().assets();

  m_image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_frames = instanceValue("frames", 1).toInt();
  m_frameCycle = instanceValue("animationCycle", 1.0f).toFloat();
  for (size_t i = 0; i < (size_t)m_frames; i++)
    m_animationFrame.append(m_image.replace("{frame}", toString(i)));
  m_idleFrame = m_image.replace("{frame}", "idle");

  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_toolVolume = assets->json("/sfx.config:harvestToolVolume").toFloat();
  m_frameTiming = 0;
}

ItemPtr TillingTool::clone() const {
  return make_shared<TillingTool>(*this);
}

List<Drawable> TillingTool::drawables() const {
  if (m_frameTiming == 0)
    return {Drawable::makeImage(m_idleFrame, 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  else {
    int frame = std::max(0, std::min(m_frames - 1, (int)std::floor((m_frameTiming / m_frameCycle) * m_frames)));
    return {Drawable::makeImage(m_animationFrame[frame], 1.0f / TilePixels, true, -handPosition() / TilePixels)};
  }
}

Vec2F TillingTool::handPosition() const {
  return m_handPosition;
}

void TillingTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  auto strikeSound = Random::randValueFrom(m_strikeSounds);

  if (owner() && world()) {
    auto materialDatabase = Root::singleton().materialDatabase();
    Vec2I pos(owner()->aimPosition().floor());

    if (world()->material(pos + Vec2I(0, 1), TileLayer::Foreground) != EmptyMaterialId)
      return;

    bool used = false;
    for (auto layer : {TileLayer::Foreground, TileLayer::Background}) {
      if (world()->material(pos, layer) == EmptyMaterialId)
        pos = pos - Vec2I(0, 1);

      if ((layer == TileLayer::Background)
          && world()->material(pos + Vec2I(0, 1), TileLayer::Background) != EmptyMaterialId)
        continue;

      if (owner()->isAdmin() || owner()->inToolRange()) {
        auto currentMod = world()->mod(pos, layer);
        auto material = world()->material(pos, layer);
        auto tilledMod = materialDatabase->tilledModFor(material);

        if (tilledMod != NoModId && currentMod == NoModId) {
          if (world()->modifyTile(pos, PlaceMod{layer, tilledMod, MaterialHue()}, true))
            used = true;
        } else if (currentMod != tilledMod) {
          auto damageResult = world()->damageTile(pos, layer, owner()->position(), {TileDamageType::Tilling, 1.0f});
          used = damageResult != TileDamageResult::None;
          if (damageResult == TileDamageResult::Protected) {
            strikeSound = Root::singleton().assets()->json("/client.config:defaultDingSound").toString();
          }
        }
      }
    }

    if (used) {
      owner()->addSound(strikeSound, m_toolVolume);
      SwingableItem::fire(mode, shifting, edgeTriggered);
    }
  }
}

void TillingTool::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  SwingableItem::update(dt, fireMode, shifting, moves);

  if (!ready() && !coolingDown())
    m_frameTiming = std::fmod((m_frameTiming + dt), m_frameCycle);
  else
    m_frameTiming = 0;
}

float TillingTool::getAngle(float aimAngle) {
  if (!ready() && !coolingDown())
    return SwingableItem::getAngle(aimAngle);
  return aimAngle;
}

PaintingBeamTool::PaintingBeamTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), FireableItem(config), BeamItem(config) {
  auto assets = Root::singleton().assets();

  m_blockRadius = instanceValue("blockRadius").toFloat();
  m_altBlockRadius = instanceValue("altBlockRadius").toFloat();
  m_strikeSounds = jsonToStringList(instanceValue("strikeSounds"));
  m_toolVolume = assets->json("/sfx.config:miningToolVolume").toFloat();
  m_blockVolume = assets->json("/sfx.config:miningBlockVolume").toFloat();
  m_endType = EndType::Object;

  for (auto color : instanceValue("colorNumbers").toArray())
    m_colors.append(jsonToColor(color));

  m_colorKeys = jsonToStringList(instanceValue("colorKeys"));

  m_colorIndex = instanceValue("colorIndex", 0).toInt();
  m_color = m_colors[m_colorIndex].toRgba();
}

ItemPtr PaintingBeamTool::clone() const {
  return make_shared<PaintingBeamTool>(*this);
}

List<Drawable> PaintingBeamTool::drawables() const {
  auto result = BeamItem::drawables();
  for (auto& entry : result) {
    if (entry.isImage())
      entry.imagePart().image.directives += m_colorKeys[m_colorIndex];
  }
  return result;
}

void PaintingBeamTool::setEnd(EndType type) {
  _unused(type);
  m_endType = EndType::Object;
}

void PaintingBeamTool::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  BeamItem::update(dt, fireMode, shifting, moves);
  FireableItem::update(dt, fireMode, shifting, moves);
}

List<PreviewTile> PaintingBeamTool::preview(bool shifting) const {
  List<PreviewTile> result;
  auto ownerp = owner();
  auto worldp = world();
  if (ownerp && worldp) {
    Vec3B light = Color::White.toRgb();

    if (ownerp->isAdmin() || ownerp->inToolRange()) {
      int radius = !shifting ? m_blockRadius : m_altBlockRadius;

      for (auto pos : tileAreaBrush(radius, ownerp->aimPosition(), true)) {
        if (worldp->canModifyTile(pos, PlaceMaterialColor{TileLayer::Foreground, (MaterialColorVariant)m_colorIndex}, true)) {
          result.append({pos, true, NullMaterialId, MaterialHue(), false, light, true, (MaterialColorVariant)m_colorIndex});
        } else if (worldp->canModifyTile(pos, PlaceMaterialColor{TileLayer::Background, (MaterialColorVariant)m_colorIndex}, true)) {
          result.append({pos, false, NullMaterialId, MaterialHue(), false, light, true, (MaterialColorVariant)m_colorIndex});
        } else if (worldp->canModifyTile(pos, PlaceMaterialColor{TileLayer::Foreground, DefaultMaterialColorVariant}, true)) {
          result.append({pos, true, NullMaterialId, MaterialHue(), false, light, true, DefaultMaterialColorVariant});
        } else if (worldp->canModifyTile(pos, PlaceMaterialColor{TileLayer::Background, DefaultMaterialColorVariant}, true)) {
          result.append({pos, false, NullMaterialId, MaterialHue(), false, light, true, DefaultMaterialColorVariant});
        }
      }
    }
  }

  return result;
}

void PaintingBeamTool::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  BeamItem::init(owner, hand);
  m_color = m_colors[m_colorIndex].toRgba();
}

List<Drawable> PaintingBeamTool::nonRotatedDrawables() const {
  if (!coolingDown())
    return BeamItem::nonRotatedDrawables();
  return {};
}

void PaintingBeamTool::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  if (mode == FireMode::Alt && edgeTriggered) {
    m_colorIndex = (m_colorIndex + 1) % m_colors.size();
    m_color = m_colors[m_colorIndex].toRgba();
    setInstanceValue("colorIndex", m_colorIndex);
    return;
  }

  if (mode == FireMode::Primary) {
    auto worldp = world();
    auto ownerp = owner();
    if (ownerp && worldp) {
      bool used = false;
      int radius = !shifting ? m_blockRadius : m_altBlockRadius;

      if (ownerp->isAdmin() || ownerp->inToolRange()) {
        for (auto pos : tileAreaBrush(radius, ownerp->aimPosition(), true)) {
          TileModificationList modifications = {
            {pos, PlaceMaterialColor{TileLayer::Foreground, (MaterialColorVariant)m_colorIndex}},
            {pos, PlaceMaterialColor{TileLayer::Background, (MaterialColorVariant)m_colorIndex}}
          };
          auto failed = worldp->applyTileModifications(modifications, true);
          if (failed.count() < 2)
            used = true;
        }
      }

      if (used) {
        ownerp->addSound(Random::randValueFrom(m_strikeSounds), m_toolVolume);
        FireableItem::fire(mode, shifting, edgeTriggered);
      }
    }
  }
}

float PaintingBeamTool::getAngle(float angle) {
  return BeamItem::getAngle(angle);
}

}
