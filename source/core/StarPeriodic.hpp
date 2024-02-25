#pragma once

#include "StarMathCommon.hpp"
#include "StarRandom.hpp"

namespace Star {

// Perform some action every X ticks.  Setting the tick count to 0 means never
// perform the action, 1 performs the action every call, 2 performs the action
// every other call, and so forth.
class Periodic {
public:
  Periodic(unsigned everyXSteps = 1)
    : m_counter(0), m_everyXSteps(everyXSteps) {}

  unsigned stepCount() const {
    return m_everyXSteps;
  }

  void setStepCount(unsigned everyXSteps) {
    m_everyXSteps = everyXSteps;
    if (everyXSteps != 0)
      m_counter = clamp<unsigned>(m_counter, 0, m_everyXSteps - 1);
    else
      m_counter = 0;
  }

  // Will the next tick() return true?
  bool ready() const {
    return m_everyXSteps != 0 && m_counter == 0;
  }

  bool tick() {
    if (m_everyXSteps == 0)
      return false;

    if (m_counter == 0) {
      m_counter = m_everyXSteps - 1;
      return true;
    } else {
      --m_counter;
      return false;
    }
  }

  template <typename Function>
  void tick(Function&& function) {
    if (tick())
      function();
  }

private:
  unsigned m_counter;
  unsigned m_everyXSteps;
};

// Perform some action with a given period over an amount of some value (like
// time) with optional randomness.
class RatePeriodic {
public:
  RatePeriodic(double period = 1, double noise = 0)
    : m_period(period), m_noise(noise), m_counter(period + Random::randf(-noise, noise)), m_elapsed(0.0) {}

  double period() const {
    return m_period;
  }

  double noise() const {
    return m_noise;
  }

  template <typename Function>
  void update(double amount, Function&& function) {
    double subAmount = min(amount, m_counter);
    m_counter -= subAmount;
    amount -= subAmount;
    m_elapsed += subAmount;

    if (m_counter <= 0.0) {
      m_counter = m_period + Random::randf(-m_noise, m_noise);
      function(m_elapsed);
      m_elapsed = 0.0;
      if (amount > 0.0)
        update(amount, forward<Function>(function));
    }
  }

private:
  double m_period;
  double m_noise;
  double m_counter;
  double m_elapsed;
};

}
