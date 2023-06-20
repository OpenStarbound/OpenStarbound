#ifndef STAR_NET_ELEMENT_HPP
#define STAR_NET_ELEMENT_HPP

#include "StarDataStream.hpp"

namespace Star {

// Monotonically increasing NetElementVersion shared between all NetElements in
// a network.
class NetElementVersion {
public:
  uint64_t current() const;
  void increment();

private:
  uint64_t m_version = 0;
};

// Primary interface for the composable network synchronizable element system.
class NetElement {
public:
  virtual ~NetElement() = default;

  // A network of NetElements will have a shared monotinically increasing
  // NetElementVersion.  When elements are updated, they will mark the version
  // number at the time they are updated so that a delta can be constructed
  // that contains only changes since any past version.
  virtual void initNetVersion(NetElementVersion const* version = nullptr) = 0;

  // Full store / load of the entire element.
  virtual void netStore(DataStream& ds) const = 0;
  virtual void netLoad(DataStream& ds) = 0;

  // Enables interpolation mode.  If interpolation mode is enabled, then
  // NetElements will delay presenting incoming delta data for the
  // 'interpolationTime' parameter given in readNetDelta, and smooth between
  // received values.  When interpolation is enabled, tickNetInterpolation must
  // be periodically called to smooth values forward in time.  If
  // extrapolationHint is given, this may be used as a hint for the amount of
  // time to extrapolate forward if no deltas are received.
  virtual void enableNetInterpolation(float extrapolationHint = 0.0f);
  virtual void disableNetInterpolation();
  virtual void tickNetInterpolation(float dt);

  // Write all the state changes that have happened since (and including)
  // fromVersion.  The normal way to use this would be to call writeDelta with
  // the version at the time of the *last* call to writeDelta, + 1.  If
  // fromVersion is 0, this will always write the full state.  Should return
  // true if a delta was needed and was written to DataStream, false otherwise.
  virtual bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const = 0;
  // Read a delta written by writeNetDelta.  'interpolationTime' is the time in
  // the future that data from this delta should be delayed and smoothed into,
  // if interpolation is enabled.
  virtual void readNetDelta(DataStream& ds, float interpolationTime = 0.0) = 0;
  // When extrapolating, it is important to notify when a delta WOULD have been
  // received even if no deltas are produced, so no extrapolation takes place.
  virtual void blankNetDelta(float interpolationTime);
};

}

#endif
