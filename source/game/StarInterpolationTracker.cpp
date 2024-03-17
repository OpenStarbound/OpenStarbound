#include "StarInterpolationTracker.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

constexpr double VanillaStepsPerSecond = 60.0;

InterpolationTracker::InterpolationTracker(Json config) {
  if (config.isNull()) {
    config = JsonObject();
  } else if (config.type() == Json::Type::String) {
    auto assets = Root::singleton().assets();
    config = assets->json(config.toString());
  }

  m_interpolationEnabled = config.getBool("interpolationEnabled", false);
  m_entityUpdateDelta = config.getDouble("entityUpdateDelta", 3) / VanillaStepsPerSecond;
  m_timeLead = config.getDouble("stepLead", 0) / VanillaStepsPerSecond;
  m_extrapolationHint = config.getUInt("extrapolationHint", 0);
  m_timeTrackFactor = config.getDouble("stepTrackFactor", 1.0);
  m_timeMaxDistance = config.getDouble("stepMaxDistance", 0.0) / VanillaStepsPerSecond;

  m_currentTime = 0.0;
}

bool InterpolationTracker::interpolationEnabled() const {
  return m_interpolationEnabled;
}

unsigned InterpolationTracker::extrapolationHint() const {
  if (m_interpolationEnabled)
    return m_extrapolationHint;
  else
    return 0;
}

float InterpolationTracker::entityUpdateDelta() const {
  return m_entityUpdateDelta;
}

void InterpolationTracker::receiveTimeUpdate(double remoteStep) {
  m_lastTimeUpdate = remoteStep;
}

void InterpolationTracker::update(double newLocalTime) {
  double dt = newLocalTime - m_currentTime;
  m_currentTime = newLocalTime;
  if (!m_predictedTime || !m_lastTimeUpdate || dt < 0.0f) {
    m_predictedTime = m_lastTimeUpdate;
  } else {
    *m_lastTimeUpdate += dt;
    *m_predictedTime += dt;
    *m_predictedTime += (*m_lastTimeUpdate - *m_predictedTime) * m_timeTrackFactor;
    m_predictedTime = clamp<double>(*m_predictedTime, *m_lastTimeUpdate - m_timeMaxDistance, *m_lastTimeUpdate + m_timeMaxDistance);
  }
}

float InterpolationTracker::interpolationLeadTime() const {
  if (!m_interpolationEnabled || !m_predictedTime || !m_lastTimeUpdate)
    return 0.0f;
  return *m_lastTimeUpdate - *m_predictedTime + m_timeLead;
}

}
