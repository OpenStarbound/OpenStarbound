#include "StarNetElementGroup.hpp"

namespace Star {

void NetElementGroup::addNetElement(NetElement* element, bool propagateInterpolation) {
  starAssert(!m_elements.any([element](auto p) { return p.first == element; }));

  element->initNetVersion(m_version);
  if (m_interpolationEnabled && propagateInterpolation)
    element->enableNetInterpolation(m_extrapolationHint);
  m_elements.append(pair<NetElement*, bool>(element, propagateInterpolation));
}

void NetElementGroup::clearNetElements() {
  m_elements.clear();
}

void NetElementGroup::initNetVersion(NetElementVersion const* version) {
  m_version = version;
  for (auto& p : m_elements)
    p.first->initNetVersion(m_version);
}

void NetElementGroup::netStore(DataStream& ds, NetCompatibilityRules rules) const {
  if (!checkWithRules(rules)) return;
  for (auto& p : m_elements)
    if (p.first->checkWithRules(rules))
      p.first->netStore(ds, rules);
}

void NetElementGroup::netLoad(DataStream& ds, NetCompatibilityRules rules) {
  if (!checkWithRules(rules)) return;
  for (auto& p : m_elements)
    if (p.first->checkWithRules(rules))
      p.first->netLoad(ds, rules);
}

void NetElementGroup::enableNetInterpolation(float extrapolationHint) {
  m_interpolationEnabled = true;
  m_extrapolationHint = extrapolationHint;
  for (auto& p : m_elements) {
    if (p.second)
      p.first->enableNetInterpolation(extrapolationHint);
  }
}

void NetElementGroup::disableNetInterpolation() {
  m_interpolationEnabled = false;
  m_extrapolationHint = 0;
  for (auto& p : m_elements) {
    if (p.second)
      p.first->disableNetInterpolation();
  }
}

void NetElementGroup::tickNetInterpolation(float dt) {
  if (m_interpolationEnabled) {
    for (auto& p : m_elements)
      p.first->tickNetInterpolation(dt);
  }
}

bool NetElementGroup::writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules) const {
  if (!checkWithRules(rules)) return false;
  if (m_elements.size() == 0) {
    return false;
  } else if (m_elements.size() == 1) {
    return m_elements[0].first->writeNetDelta(ds, fromVersion, rules);
  } else {
    bool deltaWritten = false;
    uint64_t i = 0;
    m_buffer.setStreamCompatibilityVersion(rules);
    for (auto& element : m_elements) {
      if (!element.first->checkWithRules(rules))
        continue;
      ++i;
      if (element.first->writeNetDelta(m_buffer, fromVersion, rules)) {
        deltaWritten = true;
        ds.writeVlqU(i);
        ds.writeBytes(m_buffer.data());
        m_buffer.clear();
      }
    }
    if (deltaWritten)
      ds.writeVlqU(0);
    return deltaWritten;
  }
}

void NetElementGroup::readNetDelta(DataStream& ds, float interpolationTime, NetCompatibilityRules rules) {
  if (!checkWithRules(rules))
    return;
  if (m_elements.size() == 0) {
    throw IOException("readNetDelta called on empty NetElementGroup");
  } else if (m_elements.size() == 1) {
    m_elements[0].first->readNetDelta(ds, interpolationTime, rules);
  } else {
    uint64_t readIndex = ds.readVlqU();
    uint64_t i = 0;
    uint64_t offset = 0;
    for (auto& element : m_elements) {
      if (!element.first->checkWithRules(rules)) {
        offset++;
        continue;
      }
      if (readIndex == 0 || readIndex - 1 > i) {
        if (m_interpolationEnabled)
          m_elements[i + offset].first->blankNetDelta(interpolationTime);
      } else if (readIndex - 1 == i) {
        m_elements[i + offset].first->readNetDelta(ds, interpolationTime, rules);
        readIndex = ds.readVlqU();
      } else {
        throw IOException("group indexes out of order in NetElementGroup::readNetDelta");
      }
      ++i;
    }
  }
}

void NetElementGroup::blankNetDelta(float interpolationTime) {
  if (m_interpolationEnabled) {
    for (auto& p : m_elements)
      p.first->blankNetDelta(interpolationTime);
  }
}

}
