#pragma once

#include "StarInterpolation.hpp"
#include "StarRandom.hpp"

namespace Star {

// Repeating, periodic function with optional period and magnitude variance.
// Each cycle of the function will randomize the min and max values of the
// function by the magnitude variance, and the period by the period variance.
// Can approximate a randomized sin wave, triangle wave, square wave, etc based
// on the weight operator provided to the value method.
template <typename Float>
class PeriodicFunction {
public:
  PeriodicFunction(
      Float period = 1, Float min = 0, Float max = 1, Float periodVariance = 0, Float magnitudeVariance = 0);

  void update(Float delta);

  template <typename WeightOperator>
  Float value(WeightOperator weightOperator) const;

private:
  Float m_halfPeriod;
  Float m_min;
  Float m_max;
  Float m_halfPeriodVariance;
  Float m_magnitudeVariance;

  Float m_timerMax;
  Float m_timer;
  Float m_source;
  Float m_target;
  bool m_targetMode;
};

template <typename Float>
PeriodicFunction<Float>::PeriodicFunction(
    Float period, Float min, Float max, Float periodVariance, Float magnitudeVariance) {
  m_halfPeriod = period / 2;
  m_min = min;
  m_max = max;
  m_halfPeriodVariance = periodVariance / 2;
  m_magnitudeVariance = magnitudeVariance;

  m_timerMax = m_halfPeriod;
  m_timer = 0;
  m_source = m_max + Random::randf(-1, 1) * m_magnitudeVariance;
  m_target = m_min + Random::randf(-1, 1) * m_magnitudeVariance;
  m_targetMode = true;
}

template <typename Float>
void PeriodicFunction<Float>::update(Float delta) {
  m_timer -= delta;

  // Only bring the timer forward once, rather than doing this in a loop.  This
  // makes the function behave somewhat differently than it would for deltas
  // which are greater than the period, but it avoids infinite looping
  if (m_timer <= 0.0f) {
    m_source = m_target;
    m_target = (m_targetMode ? m_max : m_min) + Random::randf(-1, 1) * m_magnitudeVariance;
    m_targetMode = !m_targetMode;
    m_timerMax = m_halfPeriod + Random::randf(-1, 1) * m_halfPeriodVariance;
    m_timer = max(0.0f, m_timer + m_timerMax);
  }
}

template <typename Float>
template <typename WeightOperator>
Float PeriodicFunction<Float>::value(WeightOperator weightOperator) const {
  // This is inverted, m_timer goes from m_timerMax to 0 as the value should go
  // from m_source to m_target
  auto wvec = weightOperator(m_timer / m_timerMax);
  return m_target * wvec[0] + m_source * wvec[1];
}

}
