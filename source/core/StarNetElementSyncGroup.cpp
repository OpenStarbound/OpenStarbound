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

void NetElementSyncGroup::netStore(DataStream& ds) const {
  const_cast<NetElementSyncGroup*>(this)->netElementsNeedStore();
  return NetElementGroup::netStore(ds);
}

void NetElementSyncGroup::netLoad(DataStream& ds) {
  NetElementGroup::netLoad(ds);
  netElementsNeedLoad(true);
}

bool NetElementSyncGroup::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  const_cast<NetElementSyncGroup*>(this)->netElementsNeedStore();
  return NetElementGroup::writeNetDelta(ds, fromVersion);
}

void NetElementSyncGroup::readNetDelta(DataStream& ds, float interpolationTime) {
  NetElementGroup::readNetDelta(ds, interpolationTime);

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
  m_netElementsNeedLoad = move(needsLoadCallback);
}

void NetElementCallbackGroup::setNeedsStoreCallback(function<void()> needsStoreCallback) {
  m_netElementsNeedStore = move(needsStoreCallback);
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
