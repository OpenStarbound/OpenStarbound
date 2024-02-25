#pragma once

#include "StarNetElementGroup.hpp"

namespace Star {

// NetElementGroup class that works with NetElements that are not automatically
// kept up to date with working data, and users need to be notified when to
// synchronize with working data.
class NetElementSyncGroup : public NetElementGroup {
public:
  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  void netStore(DataStream& ds) const override;
  void netLoad(DataStream& ds) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromStep) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f) override;
  void blankNetDelta(float interpolationTime = 0.0f) override;

protected:
  // Notifies when data needs to be pulled from NetElements, load is true if
  // this is due to a netLoad call
  virtual void netElementsNeedLoad(bool load);
  // Notifies when data needs to be pushed to NetElements
  virtual void netElementsNeedStore();

private:
  bool m_hasRecentChanges = false;
  float m_recentDeltaTime = 0.0f;
  bool m_recentDeltaWasBlank = false;
};

// Same as a NetElementSyncGroup, except instead of protected methods, calls
// optional callback functions.
class NetElementCallbackGroup : public NetElementSyncGroup {
public:
  void setNeedsLoadCallback(function<void(bool)> needsLoadCallback);
  void setNeedsStoreCallback(function<void()> needsStoreCallback);

private:
  void netElementsNeedLoad(bool load) override;
  void netElementsNeedStore() override;

  function<void(bool)> m_netElementsNeedLoad;
  function<void()> m_netElementsNeedStore;
};

}
