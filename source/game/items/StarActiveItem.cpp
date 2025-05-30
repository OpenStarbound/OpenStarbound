#include "StarActiveItem.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarItemLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarPlayer.hpp"
#include "StarEmoteEntity.hpp"

namespace Star {

ActiveItem::ActiveItem(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters) {
  auto assets = Root::singleton().assets();
  auto animationConfig = assets->fetchJson(instanceValue("animation"), directory);
  if (auto customConfig = instanceValue("animationCustom"))
    animationConfig = jsonMerge(animationConfig, customConfig);
  m_itemAnimator = NetworkedAnimator(animationConfig, directory);
  for (auto const& pair : instanceValue("animationParts", JsonObject()).iterateObject())
    m_itemAnimator.setPartTag(pair.first, "partImage", pair.second.toString());
  m_scriptedAnimationParameters.reset(config.getObject("scriptedAnimationParameters", {}));

  addNetElement(&m_itemAnimator);
  addNetElement(&m_holdingItem);
  addNetElement(&m_backArmFrame);
  addNetElement(&m_frontArmFrame);
  addNetElement(&m_twoHandedGrip);
  addNetElement(&m_recoil);
  addNetElement(&m_outsideOfHand);
  addNetElement(&m_armAngle);
  addNetElement(&m_facingDirection);
  addNetElement(&m_damageSources);
  addNetElement(&m_itemDamageSources);
  addNetElement(&m_shieldPolys);
  addNetElement(&m_itemShieldPolys);
  addNetElement(&m_forceRegions);
  addNetElement(&m_itemForceRegions);

  // don't interpolate scripted animation parameters
  addNetElement(&m_scriptedAnimationParameters, false);

  m_holdingItem.set(true);
  m_armAngle.setFixedPointBase(0.01f);
}

ActiveItem::ActiveItem(ActiveItem const& rhs) : ActiveItem(rhs.config(), rhs.directory(), rhs.parameters()) {}

ItemPtr ActiveItem::clone() const {
  return make_shared<ActiveItem>(*this);
}

void ActiveItem::init(ToolUserEntity* owner, ToolHand hand) {
  ToolUserItem::init(owner, hand);
  if (entityMode() == EntityMode::Master) {
    m_script.setScripts(jsonToStringList(instanceValue("scripts")).transformed(bind(&AssetPath::relativeTo, directory(), _1)));
    m_script.setUpdateDelta(instanceValue("scriptDelta", 1).toUInt());
    m_twoHandedGrip.set(twoHanded());

    if (auto previousStorage = instanceValue("scriptStorage"))
      m_script.setScriptStorage(previousStorage.toObject());

    m_script.addCallbacks("activeItem", makeActiveItemCallbacks());
    m_script.addCallbacks("item", LuaBindings::makeItemCallbacks(this));
    m_script.addCallbacks("config", LuaBindings::makeConfigCallbacks(bind(&Item::instanceValue, as<Item>(this), _1, _2)));
    m_script.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&m_itemAnimator));
    m_script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(owner->statusController()));
    m_script.addActorMovementCallbacks(owner->movementController());
    if (auto player = as<Player>(owner))
      m_script.addCallbacks("player", LuaBindings::makePlayerCallbacks(player));
    m_script.addCallbacks("entity", LuaBindings::makeEntityCallbacks(as<Entity>(owner)));
    m_script.init(world());
    m_currentFireMode = FireMode::None;
  }
  if (world()->isClient()) {
    if (auto animationScripts = instanceValue("animationScripts")) {
      m_scriptedAnimator.setScripts(jsonToStringList(animationScripts).transformed(bind(&AssetPath::relativeTo, directory(), _1)));
      m_scriptedAnimator.setUpdateDelta(instanceValue("animationDelta", 1).toUInt());

      m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(&m_itemAnimator,
        [this](String const& name, Json const& defaultValue) -> Json {
          return m_scriptedAnimationParameters.value(name, defaultValue);
        }));
      m_scriptedAnimator.addCallbacks("activeItemAnimation", makeScriptedAnimationCallbacks());
      m_scriptedAnimator.addCallbacks("config", LuaBindings::makeConfigCallbacks(bind(&Item::instanceValue, as<Item>(this), _1, _2)));
      m_scriptedAnimator.init(world());
    }
  }
}

void ActiveItem::uninit() {
  if (entityMode() == EntityMode::Master) {
    m_script.uninit();
    m_script.removeCallbacks("activeItem");
    m_script.removeCallbacks("item");
    m_script.removeCallbacks("config");
    m_script.removeCallbacks("animator");
    m_script.removeCallbacks("status");
    m_script.removeActorMovementCallbacks();
    m_script.removeCallbacks("player");
    m_script.removeCallbacks("entity");
  }
  if (world()->isClient()) {
    if (auto animationScripts = instanceValue("animationScripts")) {
      m_scriptedAnimator.uninit();
      m_scriptedAnimator.removeCallbacks("animationConfig");
      m_scriptedAnimator.removeCallbacks("activeItemAnimation");
      m_scriptedAnimator.removeCallbacks("config");
    }
  }

  m_itemAnimatorDynamicTarget.stopAudio();
  ToolUserItem::uninit();
  m_activeAudio.clear();
}

void ActiveItem::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  StringMap<bool> moveMap;
  for (auto m : moves)
    moveMap[MoveControlTypeNames.getRight(m)] = true;

  if (entityMode() == EntityMode::Master) {
    if (fireMode != m_currentFireMode) {
      m_currentFireMode = fireMode;
      if (fireMode != FireMode::None)
        m_script.invoke("activate", FireModeNames.getRight(fireMode), shifting, moveMap);
    }
    m_script.update(m_script.updateDt(dt), FireModeNames.getRight(fireMode), shifting, moveMap);

    if (instanceValue("retainScriptStorageInItem", false).toBool()) {
      setInstanceValue("scriptStorage", m_script.getScriptStorage());
    }
  }

  bool isClient = world()->isClient();
  if (isClient) {
    m_itemAnimator.update(dt, &m_itemAnimatorDynamicTarget);
    m_scriptedAnimator.update(m_scriptedAnimator.updateDt(dt));
  } else {
    m_itemAnimator.update(dt, nullptr);
  }

  eraseWhere(m_activeAudio, [this](pair<AudioInstancePtr const, Vec2F> const& a) {
      a.first->setPosition(owner()->position() + handPosition(a.second));
      return a.first->finished();
    });

  for (auto shieldPoly : shieldPolys()) {
    shieldPoly.translate(owner()->position());
    if (isClient)
      SpatialLogger::logPoly("world", shieldPoly, {255, 255, 0, 255});
  }

  if (isClient) {
    for (auto forceRegion : forceRegions()) {
      if (auto dfr = forceRegion.ptr<DirectionalForceRegion>())
        SpatialLogger::logPoly("world", dfr->region, { 155, 0, 255, 255 });
      else if (auto rfr = forceRegion.ptr<RadialForceRegion>())
        SpatialLogger::logPoint("world", rfr->center, { 155, 0, 255, 255 });
    }
  }
}

List<DamageSource> ActiveItem::damageSources() const {
  List<DamageSource> damageSources = m_damageSources.get();
  for (auto ds : m_itemDamageSources.get()) {
    if (ds.damageArea.is<PolyF>()) {
      auto& poly = ds.damageArea.get<PolyF>();
      poly.rotate(m_armAngle.get());
      poly.scale(owner()->movementController()->getScale());
      if (owner()->facingDirection() == Direction::Left)
        poly.flipHorizontal(0.0f);
      poly.translate(handPosition(Vec2F()));
    } else if (ds.damageArea.is<Line2F>()) {
      auto& line = ds.damageArea.get<Line2F>();
      line.rotate(m_armAngle.get());
      line.scale(owner()->movementController()->getScale());
      if (owner()->facingDirection() == Direction::Left)
        line.flipHorizontal(0.0f);
      line.translate(handPosition(Vec2F()));
    }
    damageSources.append(std::move(ds));
  }
  return damageSources;
}

List<PolyF> ActiveItem::shieldPolys() const {
  List<PolyF> shieldPolys = m_shieldPolys.get();
  for (auto sp : m_itemShieldPolys.get()) {
    sp.rotate(m_armAngle.get());
    sp.scale(owner()->movementController()->getScale());
    if (owner()->facingDirection() == Direction::Left)
      sp.flipHorizontal(0.0f);
    sp.translate(handPosition(Vec2F()));
    shieldPolys.append(std::move(sp));
  }
  return shieldPolys;
}

List<PhysicsForceRegion> ActiveItem::forceRegions() const {
  List<PhysicsForceRegion> forceRegions = m_forceRegions.get();
  for (auto fr : m_itemForceRegions.get()) {
    if (auto dfr = fr.ptr<DirectionalForceRegion>()) {
      dfr->region.rotate(m_armAngle.get());
      dfr->region.scale(owner()->movementController()->getScale());
      if (owner()->facingDirection() == Direction::Left)
        dfr->region.flipHorizontal(0.0f);
      dfr->region.translate(owner()->position() + handPosition(Vec2F()));
    } else if (auto rfr = fr.ptr<RadialForceRegion>()) {
      rfr->center = rfr->center.rotate(m_armAngle.get());
      rfr->innerRadius *= owner()->movementController()->getScale();
      rfr->outerRadius *= owner()->movementController()->getScale();
      if (owner()->facingDirection() == Direction::Left)
        rfr->center[0] *= -1;
      rfr->center += owner()->position() + handPosition(Vec2F());
    }
    forceRegions.append(std::move(fr));
  }
  return forceRegions;
}

bool ActiveItem::holdingItem() const {
  return m_holdingItem.get();
}

Maybe<String> ActiveItem::backArmFrame() const {
  return m_backArmFrame.get();
}

Maybe<String> ActiveItem::frontArmFrame() const {
  return m_frontArmFrame.get();
}

bool ActiveItem::twoHandedGrip() const {
  return m_twoHandedGrip.get();
}

bool ActiveItem::recoil() const {
  return m_recoil.get();
}

bool ActiveItem::outsideOfHand() const {
  return m_outsideOfHand.get();
}

float ActiveItem::armAngle() const {
  return m_armAngle.get();
}

Maybe<Direction> ActiveItem::facingDirection() const {
  return m_facingDirection.get();
}

List<Drawable> ActiveItem::handDrawables() const {
  if (m_itemAnimator.constParts().empty()) {
    auto drawables = Item::iconDrawables();
    Drawable::scaleAll(drawables, 1.0f / TilePixels);
    return drawables;
  } else {
    return m_itemAnimator.drawables();
  }
}

List<pair<Drawable, Maybe<EntityRenderLayer>>> ActiveItem::entityDrawables() const {
  return m_scriptedAnimator.drawables();
}

List<LightSource> ActiveItem::lights() const {
  // Same as pullNewAudios, we translate and flip ourselves.
  List<LightSource> result;
  for (auto& light : m_itemAnimator.lightSources()) {
    light.position = owner()->position() + handPosition(light.position);
    light.beamAngle += m_armAngle.get();
    if (owner()->facingDirection() == Direction::Left) {
      if (light.beamAngle > 0)
        light.beamAngle = Constants::pi / 2 + constrainAngle(Constants::pi / 2 - light.beamAngle);
      else
        light.beamAngle = -Constants::pi / 2 - constrainAngle(light.beamAngle + Constants::pi / 2);
    }
    result.append(std::move(light));
  }
  result.appendAll(m_scriptedAnimator.lightSources());
  return result;
}

List<AudioInstancePtr> ActiveItem::pullNewAudios() {
  // Because the item animator is in hand-space, and Humanoid does all the
  // translation *and flipping*, we cannot use NetworkedAnimator's built-in
  // functionality to rotate and flip, and instead must do it manually.  We do
  // not call animatorTarget.setPosition, and keep track of running audio
  // ourselves.  It would be easier if (0, 0) for the NetworkedAnimator was,
  // say, the shoulder and un-rotated, but it gets a bit weird with Humanoid
  // modifications.
  List<AudioInstancePtr> result;
  for (auto& audio : m_itemAnimatorDynamicTarget.pullNewAudios()) {
    m_activeAudio[audio] = *audio->position();
    audio->setPosition(owner()->position() + handPosition(*audio->position()));
    result.append(std::move(audio));
  }
  result.appendAll(m_scriptedAnimator.pullNewAudios());
  return result;
}

List<Particle> ActiveItem::pullNewParticles() {
  // Same as pullNewAudios, we translate, rotate, and flip ourselves
  List<Particle> result;
  for (auto& particle : m_itemAnimatorDynamicTarget.pullNewParticles()) {
    particle.position = owner()->position() + handPosition(particle.position);
    particle.velocity = particle.velocity.rotate(m_armAngle.get());
    if (owner()->facingDirection() == Direction::Left) {
      particle.velocity[0] *= -1;
      particle.flip = !particle.flip;
    }
    result.append(std::move(particle));
  }
  result.appendAll(m_scriptedAnimator.pullNewParticles());
  return result;
}

Maybe<String> ActiveItem::cursor() const {
  return m_cursor;
}

Maybe<Json> ActiveItem::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  return m_script.handleMessage(message, localMessage, args);
}

float ActiveItem::durabilityStatus() {
  auto durability = instanceValue("durability").optFloat();
  if (durability) {
    auto durabilityHit = instanceValue("durabilityHit").optFloat().value(*durability);
    return durabilityHit / *durability;
  }
  return 1.0;
}

Vec2F ActiveItem::armPosition(Vec2F const& offset) const {
  return owner()->armPosition(hand(), owner()->facingDirection(), m_armAngle.get(), offset);
}

Vec2F ActiveItem::handPosition(Vec2F const& offset) const {
  return armPosition(offset + owner()->handOffset(hand(), owner()->facingDirection()));
}

LuaCallbacks ActiveItem::makeActiveItemCallbacks() {
  LuaCallbacks callbacks;
  callbacks.registerCallback("ownerEntityId", [this]() {
      return owner()->entityId();
    });
  callbacks.registerCallback("ownerTeam", [this]() {
      return owner()->getTeam().toJson();
    });
  callbacks.registerCallback("ownerAimPosition", [this]() {
      return owner()->aimPosition();
    });
  callbacks.registerCallback("ownerPowerMultiplier", [this]() {
      return owner()->powerMultiplier();
    });
  callbacks.registerCallback("fireMode", [this]() {
      return FireModeNames.getRight(m_currentFireMode);
    });
  callbacks.registerCallback("hand", [this]() {
      return ToolHandNames.getRight(hand());
    });
  callbacks.registerCallback("handPosition", [this](Maybe<Vec2F> offset) {
      return handPosition(offset.value());
    });

  // Gets the required aim angle to aim a "barrel" of the item that has the given
  // vertical offset from the hand at the given target.  The line that is aimed
  // at the target is the horizontal line going through the aimVerticalOffset.
  callbacks.registerCallback("aimAngleAndDirection", [this](float aimVerticalOffset, Vec2F targetPosition) {
      // This was figured out using pencil and paper geometry from the hand
      // rotation center, the target position, and the 90 deg vertical offset of
      // the "barrel".

      Vec2F handRotationCenter = owner()->armPosition(hand(), owner()->facingDirection(), 0.0f, Vec2F());
      Vec2F ownerPosition = owner()->position();

      // Vector in owner entity space from hand rotation center to target.
      Vec2F toTarget = owner()->world()->geometry().diff(targetPosition, (ownerPosition + handRotationCenter));
      float toTargetDist = toTarget.magnitude();

      // If the aim position is inside the circle formed by the barrel line as it
      // goes around (aimVerticalOffset <= toTargetDist) absolutely no angle will
      // give you an intersect, so we just bail out and assume the target is at the
      // edge of the circle to retain continuity.
      float angleAdjust = -std::asin(clamp(aimVerticalOffset / toTargetDist, -1.0f, 1.0f));
      auto angleSide = getAngleSide(toTarget.angle());
      return luaTupleReturn(angleSide.first + angleAdjust, numericalDirection(angleSide.second));
    });

  // Similar to aimAngleAndDirection, but only provides the offset-adjusted aimAngle for the current facing direction
  callbacks.registerCallback("aimAngle", [this](float aimVerticalOffset, Vec2F targetPosition) {
      Vec2F handRotationCenter = owner()->armPosition(hand(), owner()->facingDirection(), 0.0f, Vec2F());
      Vec2F ownerPosition = owner()->position();
      Vec2F toTarget = owner()->world()->geometry().diff(targetPosition, (ownerPosition + handRotationCenter));
      float toTargetDist = toTarget.magnitude();
      float angleAdjust = -std::asin(clamp(aimVerticalOffset / toTargetDist, -1.0f, 1.0f));
      return toTarget.angle() + angleAdjust;
    });

  callbacks.registerCallback("setHoldingItem", [this](bool holdingItem) {
      m_holdingItem.set(holdingItem);
    });

  callbacks.registerCallback("setBackArmFrame", [this](Maybe<String> armFrame) {
      m_backArmFrame.set(armFrame);
    });

  callbacks.registerCallback("setFrontArmFrame", [this](Maybe<String> armFrame) {
      m_frontArmFrame.set(armFrame);
    });

  callbacks.registerCallback("setTwoHandedGrip", [this](bool twoHandedGrip) {
      m_twoHandedGrip.set(twoHandedGrip);
    });

  callbacks.registerCallback("setRecoil", [this](bool recoil) {
      m_recoil.set(recoil);
    });

  callbacks.registerCallback("setOutsideOfHand", [this](bool outsideOfHand) {
      m_outsideOfHand.set(outsideOfHand);
    });

  callbacks.registerCallback("setArmAngle", [this](float armAngle) {
      m_armAngle.set(armAngle);
    });

  callbacks.registerCallback("setFacingDirection", [this](float direction) {
      m_facingDirection.set(directionOf(direction));
    });

  callbacks.registerCallback("setDamageSources", [this](Maybe<JsonArray> const& damageSources) {
      m_damageSources.set(damageSources.value().transformed(construct<DamageSource>()));
    });

  callbacks.registerCallback("setItemDamageSources", [this](Maybe<JsonArray> const& damageSources) {
      m_itemDamageSources.set(damageSources.value().transformed(construct<DamageSource>()));
    });

  callbacks.registerCallback("setShieldPolys", [this](Maybe<List<PolyF>> const& shieldPolys) {
      m_shieldPolys.set(shieldPolys.value());
    });

  callbacks.registerCallback("setItemShieldPolys", [this](Maybe<List<PolyF>> const& shieldPolys) {
      m_itemShieldPolys.set(shieldPolys.value());
    });

  callbacks.registerCallback("setForceRegions", [this](Maybe<JsonArray> const& forceRegions) {
      if (forceRegions)
        m_forceRegions.set(forceRegions->transformed(jsonToPhysicsForceRegion));
      else
        m_forceRegions.set({});
    });

  callbacks.registerCallback("setItemForceRegions", [this](Maybe<JsonArray> const& forceRegions) {
      if (forceRegions)
        m_itemForceRegions.set(forceRegions->transformed(jsonToPhysicsForceRegion));
      else
        m_itemForceRegions.set({});
    });

  callbacks.registerCallback("setCursor", [this](Maybe<String> cursor) {
      m_cursor = std::move(cursor);
    });

  callbacks.registerCallback("setScriptedAnimationParameter", [this](String name, Json value) {
      m_scriptedAnimationParameters.set(std::move(name), std::move(value));
    });

  callbacks.registerCallback("setInventoryIcon", [this](String image) {
      setInstanceValue("inventoryIcon", image);
      setIconDrawables({Drawable::makeImage(std::move(image), 1.0f, true, Vec2F())});
    });

  callbacks.registerCallback("setInstanceValue", [this](String name, Json val) {
      setInstanceValue(std::move(name), std::move(val));
    });

  callbacks.registerCallback("callOtherHandScript", [this](String const& func, LuaVariadic<LuaValue> const& args) {
      if (auto otherHandItem = owner()->handItem(hand() == ToolHand::Primary ? ToolHand::Alt : ToolHand::Primary)) {
        if (auto otherActiveItem = as<ActiveItem>(otherHandItem))
          return otherActiveItem->m_script.invoke(func, args).value();
      }
      return LuaValue();
    });

  callbacks.registerCallback("interact", [this](String const& type, Json const& configData, Maybe<EntityId> const& sourceEntityId) {
      owner()->interact(InteractAction(type, sourceEntityId.value(NullEntityId), configData));
    });

  callbacks.registerCallback("emote", [this](String const& emoteName) {
      auto emote = HumanoidEmoteNames.getLeft(emoteName);
      if (auto entity = as<EmoteEntity>(owner()))
        entity->playEmote(emote);
    });

  callbacks.registerCallback("setCameraFocusEntity", [this](Maybe<EntityId> const& cameraFocusEntity) {
      owner()->setCameraFocusEntity(cameraFocusEntity);
    });

  return callbacks;
}

LuaCallbacks ActiveItem::makeScriptedAnimationCallbacks() {
  LuaCallbacks callbacks;
  callbacks.registerCallback("ownerPosition", [this]() {
      return owner()->position();
    });
  callbacks.registerCallback("ownerAimPosition", [this]() {
      return owner()->aimPosition();
    });
  callbacks.registerCallback("ownerArmAngle", [this]() {
      return m_armAngle.get();
    });
  callbacks.registerCallback("ownerFacingDirection", [this]() {
      return numericalDirection(owner()->facingDirection());
    });
  callbacks.registerCallback("handPosition", [this](Maybe<Vec2F> offset) {
      return handPosition(offset.value());
    });
  return callbacks;
}

}
