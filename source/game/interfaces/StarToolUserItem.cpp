#include "StarToolUserItem.hpp"

namespace Star {

ToolUserItem::ToolUserItem() : m_owner(nullptr) {}

void ToolUserItem::init(ToolUserEntity* owner, ToolHand hand) {
  m_owner = owner;
  m_hand = hand;
}

void ToolUserItem::uninit() {
  m_owner = nullptr;
  m_hand = {};
}

void ToolUserItem::update(FireMode, bool, HashSet<MoveControlType> const&) {}

bool ToolUserItem::initialized() const {
  return (bool)m_owner;
}

ToolUserEntity* ToolUserItem::owner() const {
  if (!m_owner)
    throw ToolUserItemException("Not initialized in ToolUserItem::owner");
  return m_owner;
}

EntityMode ToolUserItem::entityMode() const {
  if (!m_owner)
    throw ToolUserItemException("Not initialized in ToolUserItem::entityMode");
  return *m_owner->entityMode();
}

ToolHand ToolUserItem::hand() const {
  if (!m_owner)
    throw ToolUserItemException("Not initialized in ToolUserItem::hand");
  return *m_hand;
}

World* ToolUserItem::world() const {
  if (!m_owner)
    throw ToolUserItemException("Not initialized in ToolUserItem::world");
  return m_owner->world();
}

List<DamageSource> ToolUserItem::damageSources() const {
  return {};
}

List<PolyF> ToolUserItem::shieldPolys() const {
  return {};
}

List<PhysicsForceRegion> ToolUserItem::forceRegions() const {
  return {};
}

}
