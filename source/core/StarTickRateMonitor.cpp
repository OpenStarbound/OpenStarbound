#include "StarTickRateMonitor.hpp"
#include "StarTime.hpp"

namespace Star {

TickRateMonitor::TickRateMonitor(double window) : m_window(window) {
  reset();
}

double TickRateMonitor::window() const {
  return m_window;
}

void TickRateMonitor::reset() {
  m_lastTick = Time::monotonicTime() - m_window;
  m_ticks = 0;
}

double TickRateMonitor::tick(unsigned count) {
  double currentTime = Time::monotonicTime();

  if (m_lastTick > currentTime) {
    m_lastTick = currentTime - m_window;
    m_ticks = 0;
  } else if (m_lastTick < currentTime) {
    double timePast = currentTime - m_lastTick;
    double rate = m_ticks / m_window;
    m_ticks = max(0.0, m_ticks - timePast * rate);
    m_lastTick = currentTime;
  }

  m_ticks += count;

  return m_ticks / m_window;
}

double TickRateMonitor::rate() const {
  return TickRateMonitor(*this).tick(0);
}

TickRateApproacher::TickRateApproacher(double targetTickRate, double window)
  : m_tickRateMonitor(window), m_targetTickRate(targetTickRate) {}

double TickRateApproacher::window() const {
  return m_tickRateMonitor.window();
}

void TickRateApproacher::setWindow(double window) {
  if (window != m_tickRateMonitor.window()) {
    m_tickRateMonitor = TickRateMonitor(window);
    tick(m_targetTickRate * window);
  }
}

double TickRateApproacher::targetTickRate() const {
  return m_targetTickRate;
}

void TickRateApproacher::setTargetTickRate(double targetTickRate) {
  m_targetTickRate = targetTickRate;
}

void TickRateApproacher::reset() {
  setWindow(window());
}

double TickRateApproacher::tick(unsigned count) {
  return m_tickRateMonitor.tick(count);
}

double TickRateApproacher::rate() const {
  return m_tickRateMonitor.rate();
}

double TickRateApproacher::ticksBehind() {
  return (m_targetTickRate - m_tickRateMonitor.rate()) * window();
}

double TickRateApproacher::ticksAhead() {
  return -ticksBehind();
}

double TickRateApproacher::spareTime() {
  return ticksAhead() / m_targetTickRate;
}

}
