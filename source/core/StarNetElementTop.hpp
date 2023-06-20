#ifndef STAR_NET_ELEMENT_TOP_HPP
#define STAR_NET_ELEMENT_TOP_HPP

#include "StarNetElement.hpp"

namespace Star {

// Mixin for the NetElement that should be the top element for a network, wraps
// any NetElement class and manages the NetElementVersion.
template <typename BaseNetElement>
class NetElementTop : public BaseNetElement {
public:
  NetElementTop();

  // Returns the state update, combined with the version code that should be
  // passed to the next call to writeState.  If 'fromVersion' is 0, then this
  // is a full write for an initial read of a slave NetElementTop.
  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0);
  // Reads a state produced by a call to writeState, optionally with the
  // interpolation delay time for the data contained in this state update.  If
  // the state is a full update rather than a delta, the interoplation delay
  // will be ignored.  Blank updates are not necessary to send to be read by
  // readState, *unless* extrapolation is enabled.  If extrapolation is
  // enabled, reading a blank update calls 'blankNetDelta' which is necessary
  // to not improperly extrapolate past the end of incoming deltas.
  void readNetState(ByteArray data, float interpolationTime = 0.0);

private:
  using BaseNetElement::initNetVersion;
  using BaseNetElement::netStore;
  using BaseNetElement::netLoad;
  using BaseNetElement::writeNetDelta;
  using BaseNetElement::readNetDelta;
  using BaseNetElement::blankNetDelta;

  NetElementVersion m_netVersion;
};

template <typename BaseNetElement>
NetElementTop<BaseNetElement>::NetElementTop() {
  BaseNetElement::initNetVersion(&m_netVersion);
}

template <typename BaseNetElement>
pair<ByteArray, uint64_t> NetElementTop<BaseNetElement>::writeNetState(uint64_t fromVersion) {
  if (fromVersion == 0) {
    DataStreamBuffer ds;
    ds.write<bool>(true);
    BaseNetElement::netStore(ds);
    m_netVersion.increment();
    return {ds.takeData(), m_netVersion.current()};

  } else {
    DataStreamBuffer ds;
    ds.write<bool>(false);
    if (!BaseNetElement::writeNetDelta(ds, fromVersion)) {
      return {ByteArray(), m_netVersion.current()};
    } else {
      m_netVersion.increment();
      return {ds.takeData(), m_netVersion.current()};
    }
  }
}

template <typename BaseNetElement>
void NetElementTop<BaseNetElement>::readNetState(ByteArray data, float interpolationTime) {
  if (data.empty()) {
    BaseNetElement::blankNetDelta(interpolationTime);

  } else {
    DataStreamBuffer ds(move(data));

    if (ds.read<bool>())
      BaseNetElement::netLoad(ds);
    else
      BaseNetElement::readNetDelta(ds, interpolationTime);
  }
}

}

#endif
