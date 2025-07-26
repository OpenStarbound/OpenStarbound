#include "StarLoungeableObject.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarObjectDatabase.hpp"

namespace Star {

LoungeableObject::LoungeableObject(ObjectConfigConstPtr config, Json const& parameters) : Object(config, parameters) {
  m_interactive.set(true);

  if (configValue("loungePositions", Json()).isType(Json::Type::Object)) {
    m_useLoungePositions = true;
    setupLoungePositions(
      configValue("slaveControlTimeout").toFloat(),
      configValue("slaveControlHeartbeat").toFloat(),
      configValue("loungePositions", JsonObject()).toObject(),
      configValue("receiveExtraControls", false).toBool()
    );
    setupLoungeNetStates(&m_netGroup, 10);
  }
}

void LoungeableObject::render(RenderCallback* renderCallback) {
  setupLoungingDrawables();
  Object::render(renderCallback);

  if (!m_sitCoverImage.empty()) {
    if (!entitiesLounging().empty()) {
      if (auto orientation = currentOrientation()) {
        Drawable drawable =
            Drawable::makeImage(m_sitCoverImage, 1.0f / TilePixels, false, position() + orientation->imagePosition);
        if (m_flipImages)
          drawable.scale(Vec2F(-1, 1), drawable.boundBox(false).center());
        renderCallback->addDrawable(std::move(drawable), RenderLayerObject + 2);
      }
    }
  }
}

InteractAction LoungeableObject::interact(InteractRequest const& request) {
  auto res = Object::interact(request);
  if (m_useLoungePositions) {
    Maybe<size_t> index = loungeInteract(request);
    if (index)
      return InteractAction(InteractActionType::SitDown, entityId(), *index);
    return res;
  } else if (res.type == InteractActionType::None && !m_sitPositions.empty() ) {
    Maybe<size_t> index;
    Vec2F interactOffset =
        direction() == Direction::Right ? position() - request.interactPosition : request.interactPosition - position();
    for (size_t i = 0; i < m_sitPositions.size(); ++i) {
      if (!index || vmag(m_sitPositions[i] + interactOffset) < vmag(m_sitPositions[*index] + interactOffset))
        index = i;
    }
    return InteractAction(InteractActionType::SitDown, entityId(), *index);
  } else {
    return res;
  }
}

size_t LoungeableObject::anchorCount() const {
  if (m_useLoungePositions)
    return LoungeableEntity::anchorCount();
  return m_sitPositions.size();
}

LoungeAnchorConstPtr LoungeableObject::loungeAnchor(size_t positionIndex) const {
  if (m_useLoungePositions)
    return LoungeableEntity::loungeAnchor(positionIndex);
  if (positionIndex >= m_sitPositions.size())
    return {};

  auto loungeAnchor = make_shared<LoungeAnchor>();

  loungeAnchor->suppressTools = false;
  loungeAnchor->controllable = false;
  loungeAnchor->direction = m_sitFlipDirection ? -direction() : direction();

  loungeAnchor->position = m_sitPositions.at(positionIndex);
  if (loungeAnchor->direction == Direction::Left)
    loungeAnchor->position[0] *= -1;
  loungeAnchor->position += position();

  loungeAnchor->exitBottomPosition = Vec2F(loungeAnchor->position[0], position()[1] + volume().boundBox().min()[1]);

  loungeAnchor->angle = m_sitAngle;
  if (loungeAnchor->direction == Direction::Left)
    loungeAnchor->angle *= -1;

  loungeAnchor->orientation = m_sitOrientation;
  // Layer all anchored entities one above the object layer, in top to bottom
  // order based on the anchor index.
  loungeAnchor->loungeRenderLayer = RenderLayerObject + m_sitPositions.size() - positionIndex;

  loungeAnchor->statusEffects = m_sitStatusEffects;
  loungeAnchor->effectEmitters = m_sitEffectEmitters;
  loungeAnchor->emote = m_sitEmote;
  loungeAnchor->dance = m_sitDance;
  loungeAnchor->armorCosmeticOverrides = m_sitArmorCosmeticOverrides;
  loungeAnchor->cursorOverride = m_sitCursorOverride;
  return loungeAnchor;
}

void LoungeableObject::init(World* world, EntityId entityId, EntityMode mode) {
  Object::init(world, entityId, mode);
  if (m_useLoungePositions){
    loungeInit();
  }
}

void LoungeableObject::uninit() {
  Object::uninit();
}

void LoungeableObject::update(float dt, uint64_t currentStep) {
  Object::update(dt, currentStep);
  if (isMaster()) {
    loungeTickMaster(dt);
  } else {
    loungeTickSlave(dt);
  }
}

void LoungeableObject::loungeControl(size_t anchorPositionIndex, LoungeControl loungeControl) {
  if (m_useLoungePositions)
    LoungeableEntity::loungeControl(anchorPositionIndex, loungeControl);
}

void LoungeableObject::loungeAim(size_t anchorPositionIndex, Vec2F const& aimPosition) {
  if (m_useLoungePositions)
    LoungeableEntity::loungeAim(anchorPositionIndex, aimPosition);
}

EntityRenderLayer LoungeableObject::loungeRenderLayer(size_t anchorPositionIndex) const {
  return RenderLayerObject + anchorCount() - anchorPositionIndex;
}

NetworkedAnimator const* LoungeableObject::networkedAnimator() const {
  return Object::networkedAnimator();
}
NetworkedAnimator * LoungeableObject::networkedAnimator()  {
  return Object::networkedAnimator();
}

Maybe<Json> LoungeableObject::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  if (m_useLoungePositions)
    if (receiveLoungeMessage(sendingConnection, message, args).isValid())
      return Json();
  return Object::receiveMessage(sendingConnection, message, args);
}

LuaCallbacks LoungeableObject::makeObjectCallbacks() {
  if (m_useLoungePositions)
    return addLoungeableCallbacks(Object::makeObjectCallbacks());
  return Object::makeObjectCallbacks();
}

void LoungeableObject::setOrientationIndex(size_t orientationIndex) {
  Object::setOrientationIndex(orientationIndex);
  if (orientationIndex != NPos) {
    if (auto sp = configValue("sitPosition")) {
      m_sitPositions = {jsonToVec2F(configValue("sitPosition")) / TilePixels};
    } else if (auto sps = configValue("sitPositions")) {
      m_sitPositions.clear();
      for (auto const& sp : sps.toArray())
        m_sitPositions.append(jsonToVec2F(sp) / TilePixels);
    }
    m_sitFlipDirection = configValue("sitFlipDirection", false).toBool();
    m_sitOrientation = LoungeOrientationNames.getLeft(configValue("sitOrientation", "sit").toString());
    m_sitAngle = configValue("sitAngle", 0).toFloat() * Constants::pi / 180.0f;
    m_sitCoverImage = configValue("sitCoverImage", "").toString();
    m_flipImages = configValue("flipImages", false).toBool();
    m_sitStatusEffects = configValue("sitStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
    m_sitEffectEmitters = jsonToStringSet(configValue("sitEffectEmitters", JsonArray()));
    m_sitEmote = configValue("sitEmote").optString();
    m_sitDance = configValue("sitDance").optString();
    m_sitArmorCosmeticOverrides = configValue("sitArmorCosmeticOverrides", JsonObject()).toObject();
    m_sitCursorOverride = configValue("sitCursorOverride").optString();
  }
}

LoungeableEntity::LoungePositions * LoungeableObject::loungePositions(){
  return &m_loungePositions;
}

LoungeableEntity::LoungePositions const* LoungeableObject::loungePositions() const {
  return &m_loungePositions;
}

}
