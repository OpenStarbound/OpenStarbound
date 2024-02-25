#pragma once

#include <type_traits>

#include "StarNetElement.hpp"
#include "StarInterpolation.hpp"

namespace Star {

STAR_EXCEPTION(StepStreamException, StarException);

template <typename T>
class NetElementFloating : public NetElement {
public:
  T get() const;
  void set(T value);

  // If a fixed point base is given, then instead of transmitting the value as
  // a float, it is transmitted as a VLQ of the value divided by the fixed
  // point base.  Any NetElementFloating that is transmitted to must also have
  // the same fixed point base set.
  void setFixedPointBase(Maybe<T> fixedPointBase = {});

  // If interpolation is enabled on the NetStepStates parent, and an
  // interpolator is set, then on steps in between data points this will be
  // used to interpolate this value.  It is not necessary that senders and
  // receivers both have matching interpolation functions, or any interpolation
  // functions at all.
  void setInterpolator(function<T(T, T, T)> interpolator);

  void initNetVersion(NetElementVersion const* version = nullptr) override;

  // Values are never interpolated, but they will be delayed for the given
  // interpolationTime.
  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  void netStore(DataStream& ds) const override;
  void netLoad(DataStream& ds) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f) override;
  void blankNetDelta(float interpolationTime = 0.0f) override;

private:
  void writeValue(DataStream& ds, T t) const;
  T readValue(DataStream& ds) const;

  T interpolate() const;

  Maybe<T> m_fixedPointBase;
  NetElementVersion const* m_netVersion = nullptr;
  uint64_t m_latestUpdateVersion = 0;
  T m_value = T();

  function<T(T, T, T)> m_interpolator;
  float m_extrapolation = 0.0f;
  Maybe<Deque<pair<float, T>>> m_interpolationDataPoints;
};

typedef NetElementFloating<float> NetElementFloat;
typedef NetElementFloating<double> NetElementDouble;

template <typename T>
T NetElementFloating<T>::get() const {
  return m_value;
}

template <typename T>
void NetElementFloating<T>::set(T value) {
  if (m_value != value) {
    // Only mark the step as updated here if it actually would change the
    // transmitted value.
    if (!m_fixedPointBase || round(m_value / *m_fixedPointBase) != round(value / *m_fixedPointBase))
      m_latestUpdateVersion = m_netVersion ? m_netVersion->current() : 0;

    m_value = value;

    if (m_interpolationDataPoints) {
      m_interpolationDataPoints->clear();
      m_interpolationDataPoints->append({0.0f, m_value});
    }
  }
}

template <typename T>
void NetElementFloating<T>::setFixedPointBase(Maybe<T> fixedPointBase) {
  m_fixedPointBase = fixedPointBase;
}

template <typename T>
void NetElementFloating<T>::setInterpolator(function<T(T, T, T)> interpolator) {
  m_interpolator = std::move(interpolator);
}

template <typename T>
void NetElementFloating<T>::initNetVersion(NetElementVersion const* version) {
  m_netVersion = version;
  m_latestUpdateVersion = 0;
}

template <typename T>
void NetElementFloating<T>::enableNetInterpolation(float extrapolationHint) {
  m_extrapolation = extrapolationHint;
  if (!m_interpolationDataPoints) {
    m_interpolationDataPoints.emplace();
    m_interpolationDataPoints->append({0.0f, m_value});
  }
}

template <typename T>
void NetElementFloating<T>::disableNetInterpolation() {
  if (m_interpolationDataPoints) {
    m_value = m_interpolationDataPoints->last().second;
    m_interpolationDataPoints.reset();
  }
}

template <typename T>
void NetElementFloating<T>::tickNetInterpolation(float dt) {
  if (m_interpolationDataPoints) {
    for (auto& p : *m_interpolationDataPoints)
      p.first -= dt;

    while (m_interpolationDataPoints->size() > 2 && (*m_interpolationDataPoints)[1].first <= 0.0f)
      m_interpolationDataPoints->removeFirst();

    m_value = interpolate();
  }
}

template <typename T>
void NetElementFloating<T>::netStore(DataStream& ds) const {
  if (m_interpolationDataPoints)
    writeValue(ds, m_interpolationDataPoints->last().second);
  else
    writeValue(ds, m_value);
}

template <typename T>
void NetElementFloating<T>::netLoad(DataStream& ds) {
  m_value = readValue(ds);
  m_latestUpdateVersion = m_netVersion ? m_netVersion->current() : 0;
  if (m_interpolationDataPoints) {
    m_interpolationDataPoints->clear();
    m_interpolationDataPoints->append({0.0f, m_value});
  }
}

template <typename T>
bool NetElementFloating<T>::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  if (m_latestUpdateVersion < fromVersion)
    return false;

  if (m_interpolationDataPoints)
    writeValue(ds, m_interpolationDataPoints->last().second);
  else
    writeValue(ds, m_value);

  return true;
}

template <typename T>
void NetElementFloating<T>::readNetDelta(DataStream& ds, float interpolationTime) {
  T t = readValue(ds);

  m_latestUpdateVersion = m_netVersion ? m_netVersion->current() : 0;
  if (m_interpolationDataPoints) {
    if (interpolationTime < m_interpolationDataPoints->last().first)
      m_interpolationDataPoints->clear();
    m_interpolationDataPoints->append({interpolationTime, t});
    m_value = interpolate();
  } else {
    m_value = t;
  }
}

template <typename T>
void NetElementFloating<T>::blankNetDelta(float interpolationTime) {
  if (m_interpolationDataPoints) {
    auto lastPoint = m_interpolationDataPoints->last();
    float lastTime = lastPoint.first;
    lastPoint.first = interpolationTime;
    if (interpolationTime < lastTime)
      *m_interpolationDataPoints = {lastPoint};
    else
      m_interpolationDataPoints->append(lastPoint);

    m_value = interpolate();
  }
}

template <typename T>
void NetElementFloating<T>::writeValue(DataStream& ds, T t) const {
  if (m_fixedPointBase)
    ds.writeVlqI(round(t / *m_fixedPointBase));
  else
    ds.write(t);
}

template <typename T>
T NetElementFloating<T>::readValue(DataStream& ds) const {
  T t;
  if (m_fixedPointBase)
    t = ds.readVlqI() * *m_fixedPointBase;
  else
    ds.read(t);
  return t;
}

template <typename T>
T NetElementFloating<T>::interpolate() const {
  auto& dataPoints = *m_interpolationDataPoints;

  float ipos = inverseLinearInterpolateUpper(dataPoints.begin(), dataPoints.end(), 0.0f,
      [](float lhs, auto const& rhs) {
        return lhs < rhs.first;
      }, [](auto const& dataPoint) {
        return dataPoint.first;
      });
  auto bound = getBound2(ipos, dataPoints.size(), BoundMode::Extrapolate);

  if (m_interpolator) {
    auto const& minPoint = dataPoints[bound.i0];
    auto const& maxPoint = dataPoints[bound.i1];

    // If step separation is less than 1.0, don't normalize extrapolation to
    // the very small step difference, because this can result in large jumps
    // during jitter.
    float stepDist = max(maxPoint.first - minPoint.first, 1.0f);
    float offset = clamp<float>(bound.offset, 0.0f, 1.0f + m_extrapolation / stepDist);
    return m_interpolator(offset, minPoint.second, maxPoint.second);

  } else {
    if (bound.offset < 1.0f)
      return dataPoints[bound.i0].second;
    else
      return dataPoints[bound.i1].second;
  }
}

}
