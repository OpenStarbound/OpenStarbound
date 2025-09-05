#pragma once

#include "StarSet.hpp"
#include "StarNetElement.hpp"
#include "StarDataStreamDevices.hpp"

namespace Star {

// A static group of NetElements that itself is a NetElement and serializes
// changes based on the order in which elements are added.  All participants
// must externally add elements of the correct type in the correct order.
class NetElementGroup : public NetElement {
public:
  NetElementGroup() = default;

  NetElementGroup(NetElementGroup const&) = delete;
  NetElementGroup& operator=(NetElementGroup const&) = delete;

  // Add an element to the group.
  void addNetElement(NetElement* element, bool propagateInterpolation = true);

  // Removes all previously added elements
  void clearNetElements();

  void initNetVersion(NetElementVersion const* version = nullptr) override;

  void netStore(DataStream& ds, NetCompatibilityRules rules = {}) const override;
  void netLoad(DataStream& ds, NetCompatibilityRules rules) override;

  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules = {}) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;
  void blankNetDelta(float interpolationTime) override;

  NetElementVersion const* netVersion() const;
  bool netInterpolationEnabled() const;
  float netExtrapolationHint() const;

private:
  List<pair<NetElement*, bool>> m_elements;
  NetElementVersion const* m_version = nullptr;
  bool m_interpolationEnabled = false;
  float m_extrapolationHint = 0.0f;

  HashMap<VersionNumber, size_t> m_elementCounts;

  mutable DataStreamBuffer m_buffer;
};

inline NetElementVersion const* NetElementGroup::netVersion() const {
  return m_version;
}

inline bool NetElementGroup::netInterpolationEnabled() const {
  return m_interpolationEnabled;
}

inline float NetElementGroup::netExtrapolationHint() const {
  return m_extrapolationHint;
}

}
