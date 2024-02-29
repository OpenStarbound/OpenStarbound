#include "StarInstrumentItem.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

InstrumentItem::InstrumentItem(Json const& config, String const& directory, Json const& data) : Item(config, directory, data) {
  m_activeCooldown = 0;

  auto image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  Vec2F position = jsonToVec2F(instanceValue("handPosition", JsonArray{0, 0}));
  m_drawables.append(Drawable::makeImage(image, 1.0f / TilePixels, true, position));

  image = AssetPath::relativeTo(directory, instanceValue("activeImage").toString());
  position = jsonToVec2F(instanceValue("activeHandPosition", JsonArray{0, 0}));
  m_activeDrawables.append(Drawable::makeImage(image, 1.0f / TilePixels, true, position));

  m_activeAngle = (instanceValue("activeAngle").toFloat() / 180.0f) * Constants::pi;

  m_activeStatusEffects = instanceValue("activeStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  m_inactiveStatusEffects = instanceValue("inactiveStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  m_activeEffectSources = jsonToStringSet(instanceValue("activeEffectSources", JsonArray()));
  m_inactiveEffectSources = jsonToStringSet(instanceValue("inactiveEffectSources", JsonArray()));

  m_kind = instanceValue("kind").toString();
}

ItemPtr InstrumentItem::clone() const {
  return make_shared<InstrumentItem>(*this);
}

List<PersistentStatusEffect> InstrumentItem::statusEffects() const {
  if (active())
    return m_activeStatusEffects;
  return m_inactiveStatusEffects;
}

StringSet InstrumentItem::effectSources() const {
  if (active())
    return m_activeEffectSources;
  return m_inactiveEffectSources;
}

void InstrumentItem::update(float, FireMode, bool, HashSet<MoveControlType> const&) {
  if (entityMode() == EntityMode::Master) {
    if (active()) {
      m_activeCooldown--;
      owner()->addEffectEmitters({"music"});
    }
  }
  owner()->instrumentEquipped(m_kind);
}

bool InstrumentItem::active() const {
  if (!initialized())
    return false;
  return (m_activeCooldown > 0) || owner()->instrumentPlaying();
}

void InstrumentItem::setActive(bool active) {
  if (active)
    m_activeCooldown = 3;
  else
    m_activeCooldown = 0;
}

bool InstrumentItem::usable() const {
  return true;
}

void InstrumentItem::activate() {
  owner()->interact(InteractAction{InteractActionType::OpenSongbookInterface, owner()->entityId(), {}});
}

List<Drawable> InstrumentItem::drawables() const {
  if (active())
    return m_activeDrawables;
  return m_drawables;
}

float InstrumentItem::getAngle(float angle) {
  if (active())
    return m_activeAngle;
  return angle;
}

}
