#include "StarNetElementSyncGroup.hpp"

namespace Star {

void NetElementSyncGroup::enableNetInterpolation(float extrapolationHint) {
  NetElementGroup::enableNetInterpolation(extrapolationHint);
  if (m_hasRecentChanges)
    netElementsNeedLoad(false);
}

void NetElementSyncGroup::disableNetInterpolation() {
  NetElementGroup::disableNetInterpolation();
  if (m_hasRecentChanges)
    netElementsNeedLoad(false);
}

void NetElementSyncGroup::tickNetInterpolation(float dt) {
  NetElementGroup::tickNetInterpolation(dt);
  if (m_hasRecentChanges) {
    m_recentDeltaTime -= dt;
    if (netInterpolationEnabled())
      netElementsNeedLoad(false);

    if (m_recentDeltaTime < 0.0f && m_recentDeltaWasBlank)
      m_hasRecentChanges = false;
  }
}

void NetElementSyncGroup::netStore(DataStream& ds, NetCompatibilityRules rules) const {
  if (!checkWithRules(rules)) return;
  const_cast<NetElementSyncGroup*>(this)->netElementsNeedStore();
  return NetElementGroup::netStore(ds, rules);
}

void NetElementSyncGroup::netLoad(DataStream& ds, NetCompatibilityRules rules) {
  if (!checkWithRules(rules)) return;
  NetElementGroup::netLoad(ds, rules);
  netElementsNeedLoad(true);
}

bool NetElementSyncGroup::writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules) const {
  if (!checkWithRules(rules)) return false;
  const_cast<NetElementSyncGroup*>(this)->netElementsNeedStore();
  return NetElementGroup::writeNetDelta(ds, fromVersion, rules);
}

void NetElementSyncGroup::readNetDelta(DataStream& ds, float interpolationTime, NetCompatibilityRules rules) {
  if (!checkWithRules(rules)) return;
  NetElementGroup::readNetDelta(ds, interpolationTime, rules);

  m_hasRecentChanges = true;
  m_recentDeltaTime = interpolationTime;
  m_recentDeltaWasBlank = false;

  netElementsNeedLoad(false);
}

void NetElementSyncGroup::blankNetDelta(float interpolationTime) {
  NetElementGroup::blankNetDelta(interpolationTime);

  if (!m_recentDeltaWasBlank) {
    m_recentDeltaTime = interpolationTime;
    m_recentDeltaWasBlank = true;
  }

  if (m_hasRecentChanges && netInterpolationEnabled())
    netElementsNeedLoad(false);
}

void NetElementSyncGroup::netElementsNeedLoad(bool) {}

void NetElementSyncGroup::netElementsNeedStore() {}

void NetElementCallbackGroup::setNeedsLoadCallback(function<void(bool)> needsLoadCallback) {
  m_netElementsNeedLoad = std::move(needsLoadCallback);
}

void NetElementCallbackGroup::setNeedsStoreCallback(function<void()> needsStoreCallback) {
  m_netElementsNeedStore = std::move(needsStoreCallback);
}

void NetElementCallbackGroup::netElementsNeedLoad(bool load) {
  if (m_netElementsNeedLoad)
    m_netElementsNeedLoad(load);
}

void NetElementCallbackGroup::netElementsNeedStore() {
  if (m_netElementsNeedStore)
    m_netElementsNeedStore();
}

}
