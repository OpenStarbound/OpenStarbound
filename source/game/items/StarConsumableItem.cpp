#include "StarConsumableItem.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarStatusController.hpp"

namespace Star {

ConsumableItem::ConsumableItem(Json const& config, String const& directory, Json const& data)
  : Item(config, directory, data), SwingableItem(config) {
  setWindupTime(0);
  setCooldownTime(0.25f);
  m_requireEdgeTrigger = true;
  m_swingStart = config.getFloat("swingStart", -60) * Constants::pi / 180;
  m_swingFinish = config.getFloat("swingFinish", 40) * Constants::pi / 180;
  m_swingAimFactor = config.getFloat("swingAimFactor", 0.2f);
  m_blockingEffects = jsonToStringSet(instanceValue("blockingEffects", JsonArray()));
  if (auto foodValue = instanceValue("foodValue")) {
    m_foodValue = foodValue.toFloat();
    m_blockingEffects.add("wellfed");
  }
  m_emitters = jsonToStringSet(instanceValue("emitters", JsonArray{"eating"}));
  m_emote = instanceValue("emote", "eat").toString();
  m_consuming = false;
}

ItemPtr ConsumableItem::clone() const {
  return make_shared<ConsumableItem>(*this);
}

List<Drawable> ConsumableItem::drawables() const {
  auto drawables = iconDrawables();
  Drawable::scaleAll(drawables, 1.0f / TilePixels);
  Drawable::translateAll(drawables, -handPosition() / TilePixels);
  return drawables;
}

void ConsumableItem::update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  SwingableItem::update(fireMode, shifting, moves);

  if (entityMode() == EntityMode::Master) {
    if (m_consuming)
      owner()->addEffectEmitters(m_emitters);
    if (ready())
      maybeConsume();
  }
}

void ConsumableItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (canUse())
    FireableItem::fire(mode, shifting, edgeTriggered);
}

void ConsumableItem::fireTriggered() {
  if (canUse()) {
    triggerEffects();
    FireableItem::fireTriggered();
  }
}

void ConsumableItem::uninit() {
  maybeConsume();
  FireableItem::uninit();
}

bool ConsumableItem::canUse() const {
  if (!count() || m_consuming)
    return false;

  for (auto pair : owner()->statusController()->activeUniqueStatusEffectSummary()) {
    if (m_blockingEffects.contains(pair.first))
      return false;
  }
  return true;
}

void ConsumableItem::triggerEffects() {
  auto options = instanceValue("effects", JsonArray()).toArray();
  if (options.size()) {
    auto option = Random::randFrom(options).toArray().transformed(jsonToEphemeralStatusEffect);
    owner()->statusController()->addEphemeralEffects(option);
  }

  if (m_foodValue) {
    owner()->statusController()->giveResource("food", *m_foodValue);
    if (owner()->statusController()->resourcePercentage("food") == 1.0f)
      owner()->statusController()->addEphemeralEffect(EphemeralStatusEffect{UniqueStatusEffect("wellfed"), {}});
  }

  if (!m_emote.empty())
    owner()->requestEmote(m_emote);

  m_consuming = true;
}

void ConsumableItem::maybeConsume() {
  if (m_consuming) {
    m_consuming = false;

    world()->sendEntityMessage(owner()->entityId(), "recordEvent", {"useItem", JsonObject {
      {"itemType", name()}
    }});
    if (count())
      setCount(count() - 1);
    else
      setCount(0);
  }
}

}
