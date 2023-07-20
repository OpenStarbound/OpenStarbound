#include "StarToolUser.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarArmors.hpp"
#include "StarCasting.hpp"
#include "StarImageProcessing.hpp"
#include "StarLiquidItem.hpp"
#include "StarMaterialItem.hpp"
#include "StarObject.hpp"
#include "StarTools.hpp"
#include "StarActivatableItem.hpp"
#include "StarObjectItem.hpp"
#include "StarAssets.hpp"
#include "StarObjectDatabase.hpp"
#include "StarWorld.hpp"
#include "StarActiveItem.hpp"
#include "StarStatusController.hpp"
#include "StarInspectionTool.hpp"

namespace Star {

ToolUser::ToolUser()
  : m_beamGunRadius(), m_beamGunGlowBorder(), m_objectPreviewInnerAlpha(), m_objectPreviewOuterAlpha(), m_user(nullptr),
    m_fireMain(), m_fireAlt(), m_edgeTriggeredMain(), m_edgeTriggeredAlt(), m_edgeSuppressedMain(), m_edgeSuppressedAlt(),
    m_suppress() {
  auto assets = Root::singleton().assets();
  m_beamGunRadius = assets->json("/player.config:initialBeamGunRadius").toFloat();
  m_beamGunGlowBorder = assets->json("/player.config:previewGlowBorder").toInt();
  m_objectPreviewInnerAlpha = assets->json("/player.config:objectPreviewInnerAlpha").toFloat();
  m_objectPreviewOuterAlpha = assets->json("/player.config:objectPreviewOuterAlpha").toFloat();

  addNetElement(&m_primaryHandItem);
  addNetElement(&m_altHandItem);
  addNetElement(&m_primaryFireTimerNetState);
  addNetElement(&m_altFireTimerNetState);
  addNetElement(&m_primaryTimeFiringNetState);
  addNetElement(&m_altTimeFiringNetState);
  addNetElement(&m_primaryItemActiveNetState);
  addNetElement(&m_altItemActiveNetState);

  float fixedPointBase = 1.f / 60.f;
  m_primaryFireTimerNetState.setFixedPointBase(fixedPointBase);
  m_altFireTimerNetState.setFixedPointBase(fixedPointBase);
  m_primaryTimeFiringNetState.setFixedPointBase(fixedPointBase);
  m_altTimeFiringNetState.setFixedPointBase(fixedPointBase);

  auto interpolateTimer = [](double offset, double min, double max) -> double {
    if (max > min)
      return min + offset * (max - min);
    else
      return max;
  };

  m_primaryFireTimerNetState.setInterpolator(interpolateTimer);
  m_altFireTimerNetState.setInterpolator(interpolateTimer);
  m_primaryTimeFiringNetState.setInterpolator(interpolateTimer);
  m_altTimeFiringNetState.setInterpolator(interpolateTimer);
}

Json ToolUser::diskStore() const {
  JsonObject res;
  if (m_primaryHandItem.get())
    res["primaryHandItem"] = m_primaryHandItem.get()->descriptor().diskStore();
  if (m_altHandItem.get())
    res["altHandItem"] = m_altHandItem.get()->descriptor().diskStore();

  return res;
}

void ToolUser::diskLoad(Json const& diskStore) {
  auto itemDb = Root::singleton().itemDatabase();
  m_primaryHandItem.set(itemDb->diskLoad(diskStore.get("primaryHandItem", {})));
  m_altHandItem.set(itemDb->diskLoad(diskStore.get("altHandItem", {})));
}

ItemPtr ToolUser::primaryHandItem() const {
  return m_primaryHandItem.get();
}

ItemPtr ToolUser::altHandItem() const {
  return m_altHandItem.get();
}

ItemDescriptor ToolUser::primaryHandItemDescriptor() const {
  if (m_primaryHandItem.get())
    return m_primaryHandItem.get()->descriptor();
  return {};
}

ItemDescriptor ToolUser::altHandItemDescriptor() const {
  if (m_altHandItem.get())
    return m_altHandItem.get()->descriptor();
  return {};
}

void ToolUser::init(ToolUserEntity* user) {
  m_user = user;

  initPrimaryHandItem();
  if (!itemSafeTwoHanded(m_primaryHandItem.get()))
    initAltHandItem();
}

void ToolUser::uninit() {
  m_user = nullptr;
  uninitItem(m_primaryHandItem.get());
  uninitItem(m_altHandItem.get());
}

List<LightSource> ToolUser::lightSources() const {
  if (m_suppress.get() || !m_user)
    return {};

  List<LightSource> lights;
  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto activeItem = as<ActiveItem>(item))
      lights.appendAll(activeItem->lights());

    if (auto flashlight = as<Flashlight>(item))
      lights.appendAll(flashlight->lightSources());

    if (auto inspectionTool = as<InspectionTool>(item))
      lights.appendAll(inspectionTool->lightSources());
  }

  return lights;
}

void ToolUser::effects(EffectEmitter& emitter) const {
  if (m_suppress.get())
    return;

  if (auto item = as<EffectSourceItem>(m_primaryHandItem.get()))
    emitter.addEffectSources("primary", item->effectSources());
  if (auto item = as<EffectSourceItem>(m_altHandItem.get()))
    emitter.addEffectSources("alt", item->effectSources());
}

List<PersistentStatusEffect> ToolUser::statusEffects() const {
  if (m_suppress.get())
    return {};

  List<PersistentStatusEffect> statusEffects;
  auto addStatusFromItem = [&](ItemPtr const& item) {
    if (auto effectItem = as<StatusEffectItem>(item))
      statusEffects.appendAll(effectItem->statusEffects());
  };

  if (!is<ArmorItem>(m_primaryHandItem.get()))
    addStatusFromItem(m_primaryHandItem.get());

  if (!is<ArmorItem>(m_altHandItem.get()))
    addStatusFromItem(m_altHandItem.get());

  return statusEffects;
}

Maybe<float> ToolUser::toolRadius() const {
  if (m_suppress.get())
    return {};
  else if (is<BeamItem>(m_primaryHandItem.get()) || is<BeamItem>(m_altHandItem.get()))
    return beamGunRadius();
  else if (is<WireTool>(m_primaryHandItem.get()) || is<WireTool>(m_altHandItem.get()))
    return beamGunRadius();
  return {};
}

List<Drawable> ToolUser::renderObjectPreviews(Vec2F aimPosition, Direction walkingDirection, bool inToolRange, Vec4B favoriteColor) const {
  if (m_suppress.get() || !m_user)
    return {};

  auto generate = [&](ObjectItemPtr item) -> List<Drawable> {
    auto objectDatabase = Root::singleton().objectDatabase();

    Vec2I aimPos(aimPosition.floor());
    auto drawables = objectDatabase->cursorHintDrawables(m_user->world(), item->objectName(),
        aimPos, walkingDirection, item->objectParameters());

    Color opacityMask = Color::White;
    opacityMask.setAlphaF(item->getAppropriateOpacity());
    Vec4B favoriteColorTrans;
    if (inToolRange && objectDatabase->canPlaceObject(m_user->world(), aimPos, item->objectName())) {
      favoriteColorTrans = favoriteColor;
    } else {
      Color color = Color::rgba(favoriteColor);
      color.setHue(color.hue() + 120);
      favoriteColorTrans = color.toRgba();
    }

    favoriteColorTrans[3] = m_objectPreviewOuterAlpha * 255;
    Color nearWhite = Color::rgba(favoriteColorTrans);
    nearWhite.setValue(1 - (1 - nearWhite.value()) / 5);
    nearWhite.setSaturation(nearWhite.saturation() / 5);
    nearWhite.setAlphaF(m_objectPreviewInnerAlpha);
    ImageOperation op = BorderImageOperation{m_beamGunGlowBorder, nearWhite.toRgba(), favoriteColorTrans, false, false};

    for (Drawable& drawable : drawables) {
      if (drawable.isImage())
        drawable.imagePart().addDirectives(imageOperationToString(op), true);
      drawable.color = opacityMask;
    }
    return drawables;
  };

  if (auto pri = as<ObjectItem>(m_primaryHandItem.get()))
    return generate(pri);
  else if (auto alt = as<ObjectItem>(m_altHandItem.get()))
    return generate(alt);
  else
    return {};
}

Maybe<Direction> ToolUser::setupHumanoidHandItems(Humanoid& humanoid, Vec2F position, Vec2F aimPosition) const {
  if (m_suppress.get() || !m_user) {
    humanoid.setPrimaryHandParameters(false, 0.0f, 0.0f, false, false, false);
    humanoid.setAltHandParameters(false, 0.0f, 0.0f, false, false);
    return {};
  }

  auto inner = [&](bool primary) -> Maybe<Direction> {
    Maybe<Direction> overrideFacingDirection;

    auto setRotation = [&](bool holdingItem, float angle, float itemAngle, bool twoHanded, bool recoil, bool outsideOfHand) {
      if (primary || twoHanded)
        humanoid.setPrimaryHandParameters(holdingItem, angle, itemAngle, twoHanded, recoil, outsideOfHand);
      else
        humanoid.setAltHandParameters(holdingItem, angle, itemAngle, recoil, outsideOfHand);
    };

    ItemPtr handItem = primary ? m_primaryHandItem.get() : m_altHandItem.get();

    auto angleSide = getAngleSide(m_user->world()->geometry().diff(aimPosition, position).angle());

    if (auto swingItem = as<SwingableItem>(handItem)) {
      float angle = swingItem->getAngleDir(angleSide.first, angleSide.second);
      bool handedness = handItem->twoHanded();
      setRotation(true, angle, swingItem->getItemAngle(angleSide.first), handedness, false, false);
      overrideFacingDirection = angleSide.second;

    } else if (auto pointableItem = as<PointableItem>(handItem)) {
      float angle = pointableItem->getAngleDir(angleSide.first, angleSide.second);
      setRotation(true, angle, angle, handItem->twoHanded(), false, false);
      overrideFacingDirection = angleSide.second;

    } else if (auto activeItem = as<ActiveItem>(handItem)) {
      setRotation(activeItem->holdingItem(), activeItem->armAngle(), activeItem->armAngle(),
          activeItem->twoHandedGrip(), activeItem->recoil(), activeItem->outsideOfHand());
      if (auto fd = activeItem->facingDirection())
        overrideFacingDirection = *fd;

    } else if (auto beamItem = as<BeamItem>(handItem)) {
      float angle = beamItem->getAngle(angleSide.first);
      setRotation(true, angle, angle, false, false, false);
      overrideFacingDirection = angleSide.second;

    } else {
      setRotation(false, 0.0f, 0.0f, false, false, false);
    }

    return overrideFacingDirection;
  };

  Maybe<Direction> overrideFacingDirection;
  overrideFacingDirection = overrideFacingDirection.orMaybe(inner(true));
  if (itemSafeTwoHanded(m_primaryHandItem.get()))
    humanoid.setAltHandParameters(false, 0.0f, 0.0f, false, false);
  else
    overrideFacingDirection = overrideFacingDirection.orMaybe(inner(false));

  return overrideFacingDirection;
}

void ToolUser::setupHumanoidHandItemDrawables(Humanoid& humanoid) const {
  if (m_suppress.get() || !m_user) {
    humanoid.setPrimaryHandFrameOverrides("", "");
    humanoid.setAltHandFrameOverrides("", "");
    humanoid.setPrimaryHandDrawables({});
    humanoid.setAltHandDrawables({});
    humanoid.setPrimaryHandNonRotatedDrawables({});
    humanoid.setAltHandNonRotatedDrawables({});
    return;
  }

  auto inner = [&](bool primary) {
    auto setRotated = [&](String const& backFrameOverride, String const& frontFrameOverride, List<Drawable> drawables, bool twoHanded) {
      if (primary || twoHanded) {
        humanoid.setPrimaryHandFrameOverrides(backFrameOverride, frontFrameOverride);
        humanoid.setPrimaryHandDrawables(move(drawables));
      } else {
        humanoid.setAltHandFrameOverrides(backFrameOverride, frontFrameOverride);
        humanoid.setAltHandDrawables(move(drawables));
      }
    };

    auto setNonRotated = [&](List<Drawable> drawables) {
      if (primary)
        humanoid.setPrimaryHandNonRotatedDrawables(move(drawables));
      else
        humanoid.setAltHandNonRotatedDrawables(move(drawables));
    };

    ItemPtr handItem = primary ? m_primaryHandItem.get() : m_altHandItem.get();

    if (auto swingItem = as<SwingableItem>(handItem)) {
      setRotated(swingItem->getArmFrame(), swingItem->getArmFrame(), swingItem->drawables(), handItem->twoHanded());

    } else if (auto pointableItem = as<PointableItem>(handItem)) {
      setRotated("", "", pointableItem->drawables(), handItem->twoHanded());

    } else if (auto activeItem = as<ActiveItem>(handItem)) {
      setRotated(activeItem->backArmFrame().value(), activeItem->frontArmFrame().value(),
          activeItem->handDrawables(), activeItem->twoHandedGrip());

    } else if (auto beamItem = as<BeamItem>(handItem)) {
      setRotated("", "", beamItem->drawables(), false);

    } else {
      setRotated("", "", {}, false);
    }

    if (auto drawItem = as<NonRotatedDrawablesItem>(handItem))
      setNonRotated(drawItem->nonRotatedDrawables());
    else
      setNonRotated({});
  };

  inner(true);
  if (itemSafeTwoHanded(m_primaryHandItem.get())) {
    humanoid.setAltHandFrameOverrides("", "");
    humanoid.setAltHandDrawables({});
    humanoid.setAltHandNonRotatedDrawables({});
  } else {
    inner(false);
  }
}

Vec2F ToolUser::armPosition(Humanoid const& humanoid, ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const {
  if (hand == ToolHand::Primary)
    return humanoid.primaryArmPosition(facingDirection, armAngle, offset);
  else
    return humanoid.altArmPosition(facingDirection, armAngle, offset);
}

Vec2F ToolUser::handOffset(Humanoid const& humanoid, ToolHand hand, Direction direction) const {
  if (hand == ToolHand::Primary)
    return humanoid.primaryHandOffset(direction);
  else
    return humanoid.altHandOffset(direction);
}

Vec2F ToolUser::handPosition(ToolHand hand, Humanoid const& humanoid, Vec2F const& handOffset) const {
  if (hand == ToolHand::Primary)
    return humanoid.primaryHandPosition(handOffset);
  else
    return humanoid.altHandPosition(handOffset);
}

bool ToolUser::queryShieldHit(DamageSource const& source) const {
  if (m_suppress.get() || !m_user)
    return false;

  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto tool = as<ToolUserItem>(item))
      for (auto poly : tool->shieldPolys()) {
        poly.translate(m_user->position());
        if (source.intersectsWithPoly(m_user->world()->geometry(), poly))
          return true;
      }
  }
  return false;
}

void ToolUser::tick(float dt, bool shifting, HashSet<MoveControlType> const& moves) {
  if (auto toolUserItem = as<ToolUserItem>(m_primaryHandItem.get())) {
    FireMode fireMode = FireMode::None;
    if (!m_suppress.get()) {
      if (m_fireMain)
        fireMode = FireMode::Primary;
      else if (m_fireAlt && m_primaryHandItem.get()->twoHanded())
        fireMode = FireMode::Alt;
    }
    toolUserItem->update(dt, fireMode, shifting, moves);
  }

  if (!m_primaryHandItem.get() || !m_primaryHandItem.get()->twoHanded()) {
    if (auto toolUserItem = as<ToolUserItem>(m_altHandItem.get())) {
      FireMode fireMode = FireMode::None;
      if (!m_suppress.get() && m_fireAlt)
        fireMode = FireMode::Primary;
      toolUserItem->update(dt, fireMode, shifting, moves);
    }
  }

  bool edgeTriggeredMain = m_edgeTriggeredMain;
  m_edgeTriggeredMain = false;
  bool edgeTriggeredAlt = m_edgeTriggeredAlt;
  m_edgeTriggeredAlt = false;

  bool edgeSuppressedMain = m_edgeSuppressedMain;
  m_edgeSuppressedMain = false;
  bool edgeSuppressedAlt = m_edgeSuppressedAlt;
  m_edgeSuppressedAlt = false;

  if (!m_suppress.get()) {
    if (itemSafeTwoHanded(m_primaryHandItem.get()) && (m_fireMain || m_fireAlt)) {
      FireableItemPtr fireableItem = as<FireableItem>(m_primaryHandItem.get());
      if (fireableItem) {
        FireMode mode;
        if (m_fireMain)
          mode = FireMode::Primary;
        else
          mode = FireMode::Alt;
        fireableItem->fire(mode, shifting, edgeTriggeredMain || edgeTriggeredAlt);
      }
      ActivatableItemPtr activatableItem = as<ActivatableItem>(m_primaryHandItem.get());
      if (activatableItem && activatableItem->usable())
        activatableItem->activate();
    } else if (edgeSuppressedMain || (itemSafeTwoHanded(m_primaryHandItem.get()) && edgeSuppressedAlt)) {
      FireableItemPtr fireableItem = as<FireableItem>(m_primaryHandItem.get());
      if (fireableItem) {
        FireMode mode = FireMode::Primary;
        fireableItem->endFire(mode, shifting);
      }

      if (m_fireAlt) {
        FireableItemPtr fireableItem = as<FireableItem>(m_altHandItem.get());
        if (fireableItem)
          fireableItem->fire(FireMode::Alt, shifting, edgeTriggeredAlt);
        ActivatableItemPtr activatableItem = as<ActivatableItem>(m_altHandItem.get());
        if (activatableItem && activatableItem->usable())
          activatableItem->activate();
      }

    } else if (edgeSuppressedAlt) {
      FireableItemPtr fireableItem = as<FireableItem>(m_altHandItem.get());
      if (fireableItem) {
        FireMode mode = FireMode::Alt;
        fireableItem->endFire(mode, shifting);
      }

      if (m_fireMain) {
        FireableItemPtr fireableItem = as<FireableItem>(m_primaryHandItem.get());
        if (fireableItem)
          fireableItem->fire(FireMode::Primary, shifting, edgeTriggeredMain);
        ActivatableItemPtr activatableItem = as<ActivatableItem>(m_primaryHandItem.get());
        if (activatableItem && activatableItem->usable())
          activatableItem->activate();
      }

    } else {
      if (m_fireMain) {
        FireableItemPtr fireableItem = as<FireableItem>(m_primaryHandItem.get());
        if (fireableItem)
          fireableItem->fire(FireMode::Primary, shifting, edgeTriggeredMain);
        ActivatableItemPtr activatableItem = as<ActivatableItem>(m_primaryHandItem.get());
        if (activatableItem && activatableItem->usable())
          activatableItem->activate();
      }
      if (m_fireAlt) {
        FireableItemPtr fireableItem = as<FireableItem>(m_altHandItem.get());
        if (fireableItem)
          fireableItem->fire(FireMode::Alt, shifting, edgeTriggeredAlt);
        ActivatableItemPtr activatableItem = as<ActivatableItem>(m_altHandItem.get());
        if (activatableItem && activatableItem->usable())
          activatableItem->activate();
      }
    }
  }
}

void ToolUser::beginPrimaryFire() {
  if (!m_fireMain)
    m_edgeTriggeredMain = true;
  m_fireMain = true;
}

void ToolUser::beginAltFire() {
  if (!m_fireAlt)
    m_edgeTriggeredAlt = true;
  m_fireAlt = true;
}

void ToolUser::endPrimaryFire() {
  if (m_fireMain)
    m_edgeSuppressedMain = true;
  m_fireMain = false;
}

void ToolUser::endAltFire() {
  if (m_fireAlt)
    m_edgeSuppressedAlt = true;
  m_fireAlt = false;
}

bool ToolUser::firingPrimary() const {
  return m_fireMain;
}

bool ToolUser::firingAlt() const {
  return m_fireAlt;
}

List<DamageSource> ToolUser::damageSources() const {
  if (m_suppress.get())
    return {};

  List<DamageSource> ds;
  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto toolItem = as<ToolUserItem>(item))
      ds.appendAll(toolItem->damageSources());
  }
  return ds;
}

List<PhysicsForceRegion> ToolUser::forceRegions() const {
  if (m_suppress.get())
    return {};

  List<PhysicsForceRegion> ds;
  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto toolItem = as<ToolUserItem>(item))
      ds.appendAll(toolItem->forceRegions());
  }
  return ds;
}

void ToolUser::render(RenderCallback* renderCallback, bool inToolRange, bool shifting, EntityRenderLayer renderLayer) {
  if (m_suppress.get()) {
    for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
      if (auto activeItem = as<ActiveItem>(item)) {
        activeItem->pullNewAudios();
        activeItem->pullNewParticles();
      }
    }
    return;
  }

  // FIXME: Why isn't material item a PreviewTileTool, why is inToolRange
  // passed in again, what is the difference here between the owner's tool
  // range, can't MaterialItem figure this out?
  if (inToolRange) {
    if (auto materialItem = as<MaterialItem>(m_primaryHandItem.get()))
      renderCallback->addTilePreviews(materialItem->preview(shifting));
    else if (auto liquidItem = as<LiquidItem>(m_primaryHandItem.get()))
      renderCallback->addTilePreviews(liquidItem->preview(shifting));
  }

  if (auto pri = as<PreviewTileTool>(m_primaryHandItem.get()))
    renderCallback->addTilePreviews(pri->preview(shifting));
  else if (auto alt = as<PreviewTileTool>(m_altHandItem.get()))
    renderCallback->addTilePreviews(alt->preview(shifting));

  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto activeItem = as<ActiveItem>(item)) {
      for (auto drawablePair : activeItem->entityDrawables())
        renderCallback->addDrawable(drawablePair.first, drawablePair.second.value(renderLayer));
      renderCallback->addAudios(activeItem->pullNewAudios());
      renderCallback->addParticles(activeItem->pullNewParticles());
    }
  }
}

void ToolUser::setItems(ItemPtr newPrimaryHandItem, ItemPtr newAltHandItem) {
  if (itemSafeTwoHanded(newPrimaryHandItem))
    newAltHandItem = {};

  if (m_suppress.get()) {
    newPrimaryHandItem = {};
    newAltHandItem = {};
  }

  // Only skip if BOTH items match, to easily handle the edge cases where the
  // primary and alt hands are swapped or share a pointer, to make sure both
  // items end up initialized at the end.
  if (newPrimaryHandItem == m_primaryHandItem.get() && newAltHandItem == m_altHandItem.get())
    return;

  uninitItem(m_primaryHandItem.get());
  uninitItem(m_altHandItem.get());

  // Cancel held fire if we switch primary / alt hand items, to prevent
  // accidentally triggering a switched item without a new edge trigger.

  if (m_primaryHandItem.get() != newPrimaryHandItem) {
    m_fireMain = false;
    m_fireAlt = false;
  }

  if (m_altHandItem.get() != newAltHandItem)
    m_fireAlt = false;

  m_primaryHandItem.set(move(newPrimaryHandItem));
  m_altHandItem.set(move(newAltHandItem));

  initPrimaryHandItem();
  initAltHandItem();
}

void ToolUser::suppressItems(bool suppress) {
  m_suppress.set(suppress);
}

Maybe<Json> ToolUser::receiveMessage(String const& message, bool localMessage, JsonArray const& args) {
  Maybe<Json> result;
  for (auto item : {m_primaryHandItem.get(), m_altHandItem.get()}) {
    if (auto activeItem = as<ActiveItem>(item))
      result = activeItem->receiveMessage(message, localMessage, args);
    if (result)
      break;
  }
  return result;
}

float ToolUser::beamGunRadius() const {
  return m_beamGunRadius + m_user->statusController()->statusProperty("bonusBeamGunRadius", 0).toFloat();
}

void ToolUser::NetItem::initNetVersion(NetElementVersion const* version) {
  m_netVersion = version;
  m_itemDescriptor.initNetVersion(m_netVersion);
  if (auto netItem = as<NetElement>(m_item.get()))
    netItem->initNetVersion(m_netVersion);
}

void ToolUser::NetItem::netStore(DataStream& ds) const {
  const_cast<NetItem*>(this)->updateItemDescriptor();
  m_itemDescriptor.netStore(ds);
  if (auto netItem = as<NetElement>(m_item.get()))
    netItem->netStore(ds);
}

void ToolUser::NetItem::netLoad(DataStream& ds) {
  m_itemDescriptor.netLoad(ds);

  auto itemDatabase = Root::singleton().itemDatabase();
  if (itemDatabase->loadItem(m_itemDescriptor.get(), m_item)) {
    m_newItem = true;
    if (auto netItem = as<NetElement>(m_item.get())) {
      netItem->initNetVersion(m_netVersion);
      if (m_netInterpolationEnabled)
        netItem->enableNetInterpolation(m_netExtrapolationHint);
    }
  }

  if (auto netItem = as<NetElement>(m_item.get()))
    netItem->netLoad(ds);
}

void ToolUser::NetItem::enableNetInterpolation(float extrapolationHint) {
  m_netInterpolationEnabled = true;
  m_netExtrapolationHint = extrapolationHint;
  if (auto netItem = as<NetElement>(m_item.get()))
    netItem->enableNetInterpolation(extrapolationHint);
}

void ToolUser::NetItem::disableNetInterpolation() {
  m_netInterpolationEnabled = false;
  m_netExtrapolationHint = 0;
  if (auto netItem = as<NetElement>(m_item.get()))
    netItem->disableNetInterpolation();
}

void ToolUser::NetItem::tickNetInterpolation(float dt) {
  if (m_netInterpolationEnabled) {
    if (auto netItem = as<NetElement>(m_item.get()))
      netItem->tickNetInterpolation(dt);
  }
}

bool ToolUser::NetItem::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  bool deltaWritten = false;
  const_cast<NetItem*>(this)->updateItemDescriptor();
  m_buffer.clear();
  if (m_itemDescriptor.writeNetDelta(m_buffer, fromVersion)) {
    deltaWritten = true;
    ds.write<uint8_t>(1);
    ds.writeBytes(m_buffer.data());
    if (auto netItem = as<NetElement>(m_item.get())) {
      ds.write<uint8_t>(2);
      netItem->netStore(ds);
    }
  }

  if (auto netItem = as<NetElement>(m_item.get())) {
    m_buffer.clear();
    if (netItem->writeNetDelta(m_buffer, fromVersion)) {
      deltaWritten = true;
      ds.write<uint8_t>(3);
      ds.writeBytes(m_buffer.data());
    }
  }

  if (deltaWritten)
    ds.write<uint8_t>(0);
  return deltaWritten;
}

void ToolUser::NetItem::readNetDelta(DataStream& ds, float interpolationTime) {
  while (true) {
    uint8_t code = ds.read<uint8_t>();
    if (code == 0) {
      break;
    } else if (code == 1) {
      m_itemDescriptor.readNetDelta(ds);
      if (!m_item || !m_item->matches(m_itemDescriptor.get(), true)) {
        auto itemDatabase = Root::singleton().itemDatabase();
        if (itemDatabase->loadItem(m_itemDescriptor.get(), m_item)) {
          m_newItem = true;
          if (auto netItem = as<NetElement>(m_item.get())) {
            netItem->initNetVersion(m_netVersion);
            if (m_netInterpolationEnabled)
              netItem->enableNetInterpolation(m_netExtrapolationHint);
          }
        }
      }
    } else if (code == 2) {
      if (auto netItem = as<NetElement>(m_item.get()))
        netItem->netLoad(ds);
      else
        throw IOException("Server/Client disagreement about whether an Item is a NetElement in NetItem::readNetDelta");
    } else if (code == 3) {
      if (auto netItem = as<NetElement>(m_item.get()))
        netItem->readNetDelta(ds, interpolationTime);
      else
        throw IOException("Server/Client disagreement about whether an Item is a NetElement in NetItem::readNetDelta");
    } else {
      throw IOException("Improper code received in NetItem::readDelta");
    }
  }
}

void ToolUser::NetItem::blankNetDelta(float interpolationTime) {
  if (m_netInterpolationEnabled) {
    if (auto netItem = as<NetElement>(m_item.get()))
      netItem->blankNetDelta(interpolationTime);
  }
}

ItemPtr const& ToolUser::NetItem::get() const {
  return m_item;
}

void ToolUser::NetItem::set(ItemPtr item) {
  if (m_item != item) {
    m_item = move(item);
    m_newItem = true;
    if (auto netItem = as<NetElement>(m_item.get())) {
      netItem->initNetVersion(m_netVersion);
      if (m_netInterpolationEnabled)
        netItem->enableNetInterpolation(m_netExtrapolationHint);
      else
        netItem->disableNetInterpolation();
    }
    updateItemDescriptor();
  }
}

bool ToolUser::NetItem::pullNewItem() {
  return take(m_newItem);
}

void ToolUser::NetItem::updateItemDescriptor() {
  if (m_item)
    m_itemDescriptor.set(m_item->descriptor());
  else
    m_itemDescriptor.set(ItemDescriptor());
}

void ToolUser::initPrimaryHandItem() {
  if (m_user && m_primaryHandItem.get()) {
    if (auto toolUserItem = as<ToolUserItem>(m_primaryHandItem.get()))
      toolUserItem->init(m_user, ToolHand::Primary);

    if (auto fireable = as<FireableItem>(m_primaryHandItem.get()))
      fireable->triggerCooldown();
  }
}

void ToolUser::initAltHandItem() {
  if (m_altHandItem.get() == m_primaryHandItem.get())
    m_altHandItem.set({});

  if (m_user && m_altHandItem.get()) {
    if (auto toolUserItem = as<ToolUserItem>(m_altHandItem.get()))
      toolUserItem->init(m_user, ToolHand::Alt);

    if (auto fireable = as<FireableItem>(m_altHandItem.get()))
      fireable->triggerCooldown();
  }
}

void ToolUser::uninitItem(ItemPtr const& item) {
  if (auto toolUserItem = as<ToolUserItem>(item))
    toolUserItem->uninit();
}

void ToolUser::netElementsNeedLoad(bool) {
  if (m_primaryHandItem.pullNewItem())
    initPrimaryHandItem();

  if (m_altHandItem.pullNewItem())
    initAltHandItem();

  if (auto fireableItem = as<FireableItem>(m_primaryHandItem.get())) {
    auto fireTime = m_primaryFireTimerNetState.get();
    fireableItem->setCoolingDown(fireTime < 0);
    fireableItem->setFireTimer(abs(fireTime));

    auto timeFiring = m_primaryTimeFiringNetState.get();
    fireableItem->setTimeFiring(timeFiring);
  }
  if (auto fireableItem = as<FireableItem>(m_altHandItem.get())) {
    auto fireTime = m_altFireTimerNetState.get();
    fireableItem->setCoolingDown(fireTime < 0);
    fireableItem->setFireTimer(abs(fireTime));

    auto timeFiring = m_altTimeFiringNetState.get();
    fireableItem->setTimeFiring(timeFiring);
  }

  if (auto activatableItem = as<ActivatableItem>(m_primaryHandItem.get()))
    activatableItem->setActive(m_primaryItemActiveNetState.get());
  if (auto activatableItem = as<ActivatableItem>(m_altHandItem.get()))
    activatableItem->setActive(m_altItemActiveNetState.get());
}

void ToolUser::netElementsNeedStore() {
  if (auto fireableItem = as<FireableItem>(m_primaryHandItem.get())) {
    auto t = std::max(0.0f, fireableItem->fireTimer());
    if (fireableItem->coolingDown())
      t *= -1;
    m_primaryFireTimerNetState.set(t);
    m_primaryTimeFiringNetState.set(fireableItem->timeFiring());
  } else {
    m_primaryFireTimerNetState.set(0.0);
    m_primaryTimeFiringNetState.set(0.0);
  }
  if (auto fireableItem = as<FireableItem>(m_altHandItem.get())) {
    auto t = std::max(0.0f, fireableItem->fireTimer());
    if (fireableItem->coolingDown())
      t *= -1;
    m_altFireTimerNetState.set(t);
    m_altTimeFiringNetState.set(fireableItem->timeFiring());
  } else {
    m_altFireTimerNetState.set(0.0);
    m_altTimeFiringNetState.set(0.0);
  }

  if (auto activatableItem = as<ActivatableItem>(m_primaryHandItem.get()))
    m_primaryItemActiveNetState.set(activatableItem->active());
  else
    m_primaryItemActiveNetState.set(false);
  if (auto activatableItem = as<ActivatableItem>(m_altHandItem.get()))
    m_altItemActiveNetState.set(activatableItem->active());
  else
    m_altItemActiveNetState.set(false);
}

}
