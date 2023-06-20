#include "StarFireableItem.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarItemLuaBindings.hpp"
#include "StarFireableItemLuaBindings.hpp"
#include "StarItem.hpp"
#include "StarWorld.hpp"

namespace Star {

FireableItem::FireableItem()
  : m_fireTimer(0),
    m_cooldownTime(10),
    m_windupTime(0),
    m_fireWhenReady(false),
    m_startWhenReady(false),
    m_cooldown(false),
    m_alreadyInit(false),
    m_requireEdgeTrigger(false),
    m_attemptedFire(false),
    m_fireOnRelease(false),
    m_timeFiring(0.0f),
    m_startTimingFire(false),
    m_inUse(false),
    m_walkWhileFiring(false),
    m_stopWhileFiring(false),
    m_mode(FireMode::None) {}

FireableItem::FireableItem(Json const& params) : FireableItem() {
  setParams(params);
  m_fireableParams = params;
}

FireableItem::FireableItem(FireableItem const& rhs) : ToolUserItem(rhs), StatusEffectItem(rhs) {
  m_fireTimer = rhs.m_fireTimer;
  m_cooldownTime = rhs.m_cooldownTime;
  m_windupTime = rhs.m_windupTime;
  m_fireWhenReady = rhs.m_fireWhenReady;
  m_startWhenReady = rhs.m_startWhenReady;
  m_cooldown = rhs.m_cooldown;
  m_alreadyInit = rhs.m_alreadyInit;
  m_requireEdgeTrigger = rhs.m_requireEdgeTrigger;
  m_attemptedFire = rhs.m_attemptedFire;
  m_fireOnRelease = rhs.m_fireOnRelease;
  m_timeFiring = rhs.m_timeFiring;
  m_startTimingFire = rhs.m_startTimingFire;
  m_inUse = rhs.m_inUse;
  m_walkWhileFiring = rhs.m_walkWhileFiring;
  m_stopWhileFiring = rhs.m_stopWhileFiring;
  m_fireableParams = rhs.m_fireableParams;
  m_handPosition = rhs.m_handPosition;
  m_mode = rhs.m_mode;
}

void FireableItem::init(ToolUserEntity* owner, ToolHand hand) {
  ToolUserItem::init(owner, hand);

  m_fireWhenReady = false;
  m_startWhenReady = false;

  auto scripts = m_fireableParams.opt("scripts").apply(jsonToStringList);
  if (entityMode() == EntityMode::Master && scripts) {
    if (!m_scriptComponent) {
      m_scriptComponent.emplace();
      m_scriptComponent->setScripts(*scripts);
    }
    m_scriptComponent->addCallbacks(
        "config", LuaBindings::makeConfigCallbacks(bind(&Item::instanceValue, as<Item>(this), _1, _2)));
    m_scriptComponent->addCallbacks("fireableItem", LuaBindings::makeFireableItemCallbacks(this));
    m_scriptComponent->addCallbacks("item", LuaBindings::makeItemCallbacks(as<Item>(this)));
    m_scriptComponent->init(world());
  }
}

void FireableItem::uninit() {
  if (m_scriptComponent) {
    m_scriptComponent->uninit();
    m_scriptComponent->removeCallbacks("config");
    m_scriptComponent->removeCallbacks("fireableItem");
    m_scriptComponent->removeCallbacks("item");
  }

  ToolUserItem::uninit();
}

void FireableItem::fire(FireMode mode, bool, bool edgeTriggered) {
  m_attemptedFire = true;
  if (ready()) {
    m_inUse = true;
    m_startTimingFire = true;
    m_mode = mode;
    if (!m_requireEdgeTrigger || edgeTriggered) {
      setFireTimer(windupTime() + cooldownTime());
      if (!m_fireOnRelease) {
        m_fireWhenReady = true;
        m_startWhenReady = true;
      }
    }
  }

  if (m_scriptComponent)
    m_scriptComponent->invoke("attemptedFire");
}

void FireableItem::endFire(FireMode mode, bool) {
  if (m_scriptComponent)
    m_scriptComponent->invoke("endFire");

  m_attemptedFire = false;
  if (m_fireOnRelease && m_timeFiring) {
    m_mode = mode;
    triggerCooldown();
    fireTriggered();
  }
}

FireMode FireableItem::fireMode() const {
  return m_mode;
}

float FireableItem::cooldownTime() const {
  return m_cooldownTime;
}

void FireableItem::setCooldownTime(float cooldownTime) {
  m_cooldownTime = cooldownTime;
}

float FireableItem::fireTimer() const {
  return m_fireTimer;
}

void FireableItem::setFireTimer(float fireTimer) {
  m_fireTimer = fireTimer;
}

bool FireableItem::ready() const {
  return fireTimer() <= 0;
}

bool FireableItem::firing() const {
  return m_timeFiring > 0;
}

bool FireableItem::inUse() const {
  return m_inUse;
}

bool FireableItem::walkWhileFiring() const {
  return m_walkWhileFiring;
}

bool FireableItem::stopWhileFiring() const {
  return m_stopWhileFiring;
}

bool FireableItem::windup() const {
  if (ready())
    return false;

  if (m_scriptComponent)
    m_scriptComponent->invoke("triggerWindup");

  return fireTimer() > cooldownTime();
}

void FireableItem::update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const&) {
  if (m_scriptComponent)
    m_scriptComponent->invoke("update", WorldTimestep, FireModeNames.getRight(fireMode), shifting);

  if (m_attemptedFire) {
    if (m_startTimingFire) {
      m_timeFiring += WorldTimestep;
      if (m_scriptComponent)
        m_scriptComponent->invoke("continueFire", WorldTimestep);
    }
  } else {
    m_timeFiring = 0.0f;
    m_startTimingFire = false;
  }
  m_attemptedFire = false;

  if (entityMode() == EntityMode::Master) {
    if (fireTimer() > 0.0f) {
      setFireTimer(fireTimer() - WorldTimestep);
      if (fireTimer() < 0.0f) {
        setFireTimer(0.0f);
        m_inUse = false;
      }
    }
    if (fireTimer() <= 0) {
      m_cooldown = false;
    }
    if (m_startWhenReady) {
      m_startWhenReady = false;
      startTriggered();
    }
    if (m_fireWhenReady) {
      if (fireTimer() <= cooldownTime()) {
        m_fireWhenReady = false;
        fireTriggered();
      }
    }
  }
}

void FireableItem::triggerCooldown() {
  setFireTimer(cooldownTime());
  m_cooldown = true;
  if (m_scriptComponent)
    m_scriptComponent->invoke("triggerCooldown");
}

bool FireableItem::coolingDown() const {
  return m_cooldown;
}

void FireableItem::setCoolingDown(bool coolingdown) {
  m_cooldown = coolingdown;
}

float FireableItem::timeFiring() const {
  return m_timeFiring;
}

void FireableItem::setTimeFiring(float timeFiring) {
  m_timeFiring = timeFiring;
}

Vec2F FireableItem::handPosition() const {
  return m_handPosition;
}

Vec2F FireableItem::firePosition() const {
  return Vec2F();
}

Json FireableItem::fireableParam(String const& key) const {
  return m_fireableParams.get(key);
}

Json FireableItem::fireableParam(String const& key, Json const& defaultVal) const {
  return m_fireableParams.get(key, defaultVal);
}

bool FireableItem::validAimPos(Vec2F const&) {
  return true;
}

void FireableItem::setParams(Json const& params) {
  if (!m_alreadyInit) {
    // cannot use setWindupTime or setCooldownTime here, because object is not fully constructed
    m_windupTime = params.getFloat("windupTime", 0.0f);
    m_cooldownTime = params.getFloat("cooldown", params.getFloat("fireTime", 0.15f) - m_windupTime);
    if (params.contains("handPosition")) {
      m_handPosition = jsonToVec2F(params.get("handPosition"));
    }
    m_requireEdgeTrigger = params.getBool("edgeTrigger", false);
    m_fireOnRelease = params.getBool("fireOnRelease", false);
    m_walkWhileFiring = params.getBool("walkWhileFiring", false);
    m_stopWhileFiring = params.getBool("stopWhileFiring", false);
    m_alreadyInit = true;
  }
}

void FireableItem::setFireableParam(String const& key, Json const& value) {
  m_fireableParams = m_fireableParams.set(key, value);
}

void FireableItem::startTriggered() {
  if (m_scriptComponent)
    m_scriptComponent->invoke("startTriggered");
}

void FireableItem::fireTriggered() {
  if (m_scriptComponent)
    m_scriptComponent->invoke("fireTriggered");
}

Vec2F FireableItem::ownerFirePosition() const {
  if (!initialized())
    throw StarException("FireableItem uninitialized in ownerFirePosition");

  return owner()->handPosition(hand(), (this->firePosition() - handPosition()) / TilePixels);
}

float FireableItem::windupTime() const {
  return m_windupTime;
}

void FireableItem::setWindupTime(float time) {
  m_windupTime = time;
}

List<PersistentStatusEffect> FireableItem::statusEffects() const {
  return {};
}

}
