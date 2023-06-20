#include "StarGameTimers.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

GameTimer::GameTimer() : time(), timer() {}

GameTimer::GameTimer(float time) : time(time) {
  reset();
}

bool GameTimer::tick(float dt) {
  timer = approach(0.0f, timer, dt);
  return timer == 0.0f;
}

bool GameTimer::ready() const {
  return timer == 0.0f;
}

bool GameTimer::wrapTick(float dt) {
  auto res = tick(dt);
  if (res)
    reset();
  return res;
}

void GameTimer::reset() {
  timer = time;
}

void GameTimer::setDone() {
  timer = 0.0f;
}

void GameTimer::invert() {
  timer = time - timer;
}

float GameTimer::percent() const {
  if (time)
    return timer / time;
  else
    return 0.0f;
}

DataStream& operator>>(DataStream& ds, GameTimer& gt) {
  ds >> gt.time;
  ds >> gt.timer;
  return ds;
}

DataStream& operator<<(DataStream& ds, GameTimer const& gt) {
  ds << gt.time;
  ds << gt.timer;
  return ds;
}

SlidingWindow::SlidingWindow() : windowSize(1.0f), resolution(1) {}

SlidingWindow::SlidingWindow(float windowSize, size_t resolution, float initialValue)
  : windowSize(windowSize), resolution(resolution) {
  sampleTimer = GameTimer(windowSize / resolution);
  window = std::vector<float>(resolution);
  reset(initialValue);
}

void SlidingWindow::reset(float initialValue) {
  sampleTimer.reset();
  currentIndex = 0;
  currentMin = initialValue;
  currentMax = initialValue;
  currentAverage = initialValue;
  std::fill(window.begin(), window.end(), initialValue);
}

void SlidingWindow::update(function<float()> sampleFunction) {
  if (sampleTimer.wrapTick()) {
    processUpdate(sampleFunction());
  }
}

void SlidingWindow::update(float newValue) {
  if (sampleTimer.wrapTick()) {
    processUpdate(newValue);
  }
}

void SlidingWindow::processUpdate(float newValue) {
  ++currentIndex;
  currentIndex = currentIndex % resolution;
  window[currentIndex] = newValue;

  currentMin = std::numeric_limits<float>::max();
  currentMax = 0;
  float total = 0;
  for (float v : window) {
    total += v;
    currentMin = std::min(currentMin, v);
    currentMax = std::max(currentMax, v);
  }
  currentAverage = total / resolution;
}

float SlidingWindow::min() {
  return currentMin;
}

float SlidingWindow::max() {
  return currentMax;
}

float SlidingWindow::average() {
  return currentAverage;
}

EpochTimer::EpochTimer() : m_elapsedTime(0.0) {}

EpochTimer::EpochTimer(Json json) {
  m_lastSeenEpochTime = json.get("lastEpochTime").optDouble();
  m_elapsedTime = json.getDouble("elapsedTime");
}

Json EpochTimer::toJson() const {
  return JsonObject{{"lastEpochTime", jsonFromMaybe(m_lastSeenEpochTime)}, {"elapsedTime", m_elapsedTime}};
}

void EpochTimer::update(double newEpochTime) {
  if (!m_lastSeenEpochTime) {
    m_lastSeenEpochTime = newEpochTime;
  } else {
    // Don't allow elapsed time to go backwards in the case of the epoch time
    // being lost or wrong.
    double difference = newEpochTime - *m_lastSeenEpochTime;
    if (difference > 0)
      m_elapsedTime += difference;
    m_lastSeenEpochTime = newEpochTime;
  }
}

double EpochTimer::elapsedTime() const {
  return m_elapsedTime;
}

void EpochTimer::setElapsedTime(double elapsedTime) {
  m_elapsedTime = elapsedTime;
}

DataStream& operator>>(DataStream& ds, EpochTimer& et) {
  ds >> et.m_lastSeenEpochTime;
  ds >> et.m_elapsedTime;
  return ds;
}

DataStream& operator<<(DataStream& ds, EpochTimer const& et) {
  ds << et.m_lastSeenEpochTime;
  ds << et.m_elapsedTime;
  return ds;
}

}
