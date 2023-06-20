#include "StarInterpolationTracker.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

InterpolationTracker::InterpolationTracker(Json config) {
  if (config.isNull()) {
    config = JsonObject();
  } else if (config.type() == Json::Type::String) {
    auto assets = Root::singleton().assets();
    config = assets->json(config.toString());
  }

  m_interpolationEnabled = config.getBool("interpolationEnabled", false);
  m_entityUpdateDelta = config.getUInt("enittyUpdateDelta", 3);
  m_stepLead = config.getUInt("stepLead", 0);
  m_extrapolationHint = config.getUInt("extrapolationHint", 0);
  m_stepTrackFactor = config.getDouble("stepTrackFactor", 1.0);
  m_stepMaxDistance = config.getDouble("stepMaxDistance", 0.0);

  m_currentStep = 0.0;
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

unsigned InterpolationTracker::entityUpdateDelta() const {
  return m_entityUpdateDelta;
}

void InterpolationTracker::receiveStepUpdate(double remoteStep) {
  m_lastStepUpdate = remoteStep;
}

void InterpolationTracker::update(double newLocalStep) {
  double dt = newLocalStep - m_currentStep;
  m_currentStep = newLocalStep;
  if (!m_predictedStep || !m_lastStepUpdate || dt < 0.0f) {
    m_predictedStep = m_lastStepUpdate;
  } else {
    *m_lastStepUpdate += dt;
    *m_predictedStep += dt;
    *m_predictedStep += (*m_lastStepUpdate - *m_predictedStep) * m_stepTrackFactor;
    m_predictedStep = clamp<double>(*m_predictedStep, *m_lastStepUpdate - m_stepMaxDistance, *m_lastStepUpdate + m_stepMaxDistance);
  }
}

float InterpolationTracker::interpolationLeadSteps() const {
  if (!m_interpolationEnabled || !m_predictedStep || !m_lastStepUpdate)
    return 0.0f;
  return *m_lastStepUpdate - *m_predictedStep + m_stepLead;
}

}
