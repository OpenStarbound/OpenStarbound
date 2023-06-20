#ifndef STAR_FIREABLE_ITEM_HPP
#define STAR_FIREABLE_ITEM_HPP

#include "StarToolUserItem.hpp"
#include "StarStatusEffectItem.hpp"
#include "StarLuaComponents.hpp"

namespace Star {

STAR_CLASS(FireableItem);

class FireableItem : public virtual ToolUserItem, public virtual StatusEffectItem {
public:
  FireableItem();
  FireableItem(Json const& params);
  virtual ~FireableItem() {}

  FireableItem(FireableItem const& fireableItem);

  virtual void fire(FireMode mode, bool shifting, bool edgeTriggered);
  virtual void endFire(FireMode mode, bool shifting);
  virtual FireMode fireMode() const;
  virtual float fireTimer() const;
  virtual void setFireTimer(float fireTimer);
  virtual float cooldownTime() const;
  virtual void setCooldownTime(float cooldownTime);
  virtual float windupTime() const;
  virtual void setWindupTime(float time);
  virtual bool ready() const;
  virtual bool firing() const;
  virtual bool inUse() const;
  virtual bool walkWhileFiring() const;
  virtual bool stopWhileFiring() const;
  virtual bool windup() const;
  virtual void triggerCooldown();
  virtual bool coolingDown() const;
  virtual void setCoolingDown(bool coolingdown);
  virtual float timeFiring() const;
  virtual void setTimeFiring(float timeFiring);
  virtual Vec2F firePosition() const;
  virtual Vec2F handPosition() const;

  virtual void init(ToolUserEntity* owner, ToolHand hand) override;
  virtual void uninit() override;
  virtual void update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  virtual List<PersistentStatusEffect> statusEffects() const override;

  virtual bool validAimPos(Vec2F const& aimPos);

  Json fireableParam(String const& key) const;
  Json fireableParam(String const& key, Json const& defaultVal) const;

protected:
  void setParams(Json const& params);
  void setFireableParam(String const& key, Json const& value);
  virtual void startTriggered();
  virtual void fireTriggered();

  // firePosition translated by the hand in the owner's space
  Vec2F ownerFirePosition() const;

  float m_fireTimer;
  float m_cooldownTime;
  float m_windupTime;
  bool m_fireWhenReady;
  bool m_startWhenReady;
  bool m_cooldown;
  bool m_alreadyInit;
  bool m_requireEdgeTrigger;

  bool m_attemptedFire;
  bool m_fireOnRelease;
  float m_timeFiring;
  bool m_startTimingFire;
  bool m_inUse;
  bool m_walkWhileFiring;
  bool m_stopWhileFiring;

  mutable Maybe<LuaWorldComponent<LuaBaseComponent>> m_scriptComponent;

  Json m_fireableParams;

  Vec2F m_handPosition;

  FireMode m_mode;
};

}

#endif
