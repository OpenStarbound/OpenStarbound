#pragma once

#include "StarNetElement.hpp"

namespace Star {

template <typename BaseNetElement>
class NetElementOverride : public BaseNetElement {
public:
  void netStore(DataStream& ds, NetCompatibilityRules rules = {}) const override;
  void netLoad(DataStream& ds, NetCompatibilityRules rules) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules = {}) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f, NetCompatibilityRules rules = {}) override;

  typedef std::function<void(DataStream&, NetCompatibilityRules)> NetStorer;
  typedef std::function<void(DataStream&, NetCompatibilityRules)> NetLoader;
  typedef std::function<bool(DataStream&, uint64_t, NetCompatibilityRules)> NetDeltaWriter;
  typedef std::function<void(DataStream&, float, NetCompatibilityRules)> NetDeltaReader;

  void setNetStorer(NetStorer);
  void setNetLoader(NetLoader);
  void setNetDeltaWriter(NetDeltaWriter);
  void setNetDeltaReader(NetDeltaReader);
  void setOverrides(NetStorer netStorer, NetLoader netLoader,
    NetDeltaWriter netDeltaWriter, NetDeltaReader netDeltaReader);

private:
  NetStorer m_netStorer;
  NetLoader m_netLoader;
  
  NetDeltaWriter m_netDeltaWriter;
  NetDeltaReader m_netDeltaReader;
};

template <typename BaseNetElement>
void NetElementOverride<BaseNetElement>::netStore(DataStream& ds, NetCompatibilityRules rules) const {
  if (m_netStorer)
    m_netStorer(ds, rules);
  else
    BaseNetElement::netStore(ds, rules);
}

template <typename BaseNetElement>
void NetElementOverride<BaseNetElement>::netLoad(DataStream& ds, NetCompatibilityRules rules) {
  if (m_netLoader)
    m_netLoader(ds, rules);
  else
    BaseNetElement::netLoad(ds, rules);
}

template <typename BaseNetElement>
bool NetElementOverride<BaseNetElement>::writeNetDelta(DataStream& ds, uint64_t fromVersion, NetCompatibilityRules rules) const {
  if (m_netDeltaWriter)
    return m_netDeltaWriter(ds, fromVersion, rules);
  else
    return BaseNetElement::writeNetDelta(ds, fromVersion, rules);
}

template <typename BaseNetElement>
void NetElementOverride<BaseNetElement>::readNetDelta(DataStream& ds, float interpolationTime, NetCompatibilityRules rules) {
  if (m_netDeltaReader)
    m_netDeltaReader(ds, interpolationTime, rules);
  else
    BaseNetElement::readNetDelta(ds, interpolationTime, rules);
}

template <typename BaseNetElement>
inline void NetElementOverride<BaseNetElement>::setNetStorer(NetStorer f) { m_netStorer = std::move(f); }
template <typename BaseNetElement>
inline void NetElementOverride<BaseNetElement>::setNetLoader(NetLoader f) { m_netLoader = std::move(f); }
template <typename BaseNetElement>
inline void NetElementOverride<BaseNetElement>::setNetDeltaWriter(NetDeltaWriter f) { m_netDeltaWriter = std::move(f); }
template <typename BaseNetElement>
inline void NetElementOverride<BaseNetElement>::setNetDeltaReader(NetDeltaReader f) { m_netDeltaReader = std::move(f); }

template <typename BaseNetElement>
inline void NetElementOverride<BaseNetElement>::setOverrides(NetStorer netStorer, NetLoader netLoader, NetDeltaWriter netDeltaWriter, NetDeltaReader netDeltaReader) {
  m_netStorer = std::move(netStorer);
  m_netLoader = std::move(netLoader);
  m_netDeltaWriter = std::move(netDeltaWriter);
  m_netDeltaReader = std::move(netDeltaReader);
}

}