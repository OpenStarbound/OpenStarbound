#pragma once

#include "StarJson.hpp"

namespace Star {

class InterpolationTracker {
public:
  InterpolationTracker(Json config = Json());

  // Should interpolation be enabled on entities at all?  If this is false,
  // extrapolationHint and interpolationLead will always return 0.
  bool interpolationEnabled() const;
  unsigned extrapolationHint() const;

  // Steps in between entity updates
  unsigned entityUpdateDelta() const;

  void receiveStepUpdate(double remoteStep);
  void update(double newLocalStep);

  // Lead steps that incoming interpolated data as of this moment should be
  // marked for.  If interpolation is disabled, this is always 0.0
  float interpolationLeadSteps() const;

private:
  bool m_interpolationEnabled;
  unsigned m_entityUpdateDelta;
  unsigned m_stepLead;
  unsigned m_extrapolationHint;
  double m_stepTrackFactor;
  double m_stepMaxDistance;

  double m_currentStep;
  Maybe<double> m_lastStepUpdate;
  Maybe<double> m_predictedStep;
};

}
