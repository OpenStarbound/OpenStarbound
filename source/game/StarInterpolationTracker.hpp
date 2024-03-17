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

  // Time in-between entity updates
  float entityUpdateDelta() const;

  void receiveTimeUpdate(double remoteTime);
  void update(double newLocalTime);

  // Lead time that incoming interpolated data as of this moment should be
  // marked for.  If interpolation is disabled, this is always 0.0
  float interpolationLeadTime() const;

private:
  bool m_interpolationEnabled;
  float m_entityUpdateDelta;
  double m_timeLead;
  unsigned m_extrapolationHint;
  double m_timeTrackFactor;
  double m_timeMaxDistance;

  double m_currentTime;
  Maybe<double> m_lastTimeUpdate;
  Maybe<double> m_predictedTime;
};

}
