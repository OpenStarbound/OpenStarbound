#ifndef STAR_SKY_HPP
#define STAR_SKY_HPP

#include "StarEither.hpp"
#include "StarNetElementSystem.hpp"
#include "StarCelestialParameters.hpp"
#include "StarSkyParameters.hpp"
#include "StarSkyRenderData.hpp"

namespace Star {

STAR_CLASS(Clock);
STAR_CLASS(AudioInstance);

STAR_CLASS(Sky);

// Sky objects, such as stars and orbiters, are given in a pseudo screen space,
// "view space", that does not take the pixel ratio into account.  "viewSize"
// is the size of this space,  expected to be the size of the screen *after*
// dividing by the pixel ratio.
class Sky {
public:
  Sky();
  Sky(SkyParameters const& skyParameters, bool inOrbit);

  // Controls the space sky "flight" system
  void startFlying(bool enterHyperspace, bool startInWarp);
  // Stops flying animation copying the new pertinant sky data from the given
  // sky, as though the sky as moved to a new world.
  void stopFlyingAt(Maybe<SkyParameters> SkyParameters);

  void jumpTo(SkyParameters SkyParameters);

  pair<ByteArray, uint64_t> writeUpdate(uint64_t fromVersion = 0);
  void readUpdate(ByteArray data);
  
  // handles flying and warp state transitions
  void stateUpdate();
  void update(double dt);

  void setType(SkyType type);
  SkyType type() const;

  bool inSpace() const;

  uint64_t seed() const;

  float dayLength() const;
  uint32_t day() const;
  float timeOfDay() const;

  // Total time since the 0th day for this world.
  double epochTime() const;
  void setEpochTime(double epochTime);

  // Altitude is used to determine, in Atmospheric skies, the percentage of the
  // atmosphere to draw and how much like space it should appear.
  float altitude() const;
  void setAltitude(float altitude);

  // If a reference clock is set, then the epoch time is driven by the
  // reference clock rather than an internal timer
  void setReferenceClock(ClockConstPtr const& referenceClock);
  ClockConstPtr referenceClock() const;

  String ambientNoise() const;
  List<AudioInstancePtr> pullSounds();

  // How close is the atmosphere to space?
  float spaceLevel() const;

  float orbitAngle() const;
  bool isDayTime() const;

  // Ranges from 0.0 to 1.0  Blended periodic curve with a period of
  // clock.dayLength, and the blend region size is determined by
  // the variant asset "dayTransitionTime"
  float dayLevel() const;

  // Returns a value that cycles through the range [0.0, 4.0).  0.0 / 4.0 is
  // mid-morning, 1.0 is mid-day, 2.0 is mid-evening, and 3.0 is mid-night.
  // Does not cycle through evenly, the value will "stick" to mid-day and
  // mid-night based on the value of the variant asset "dayTransitionTime"
  float dayCycle() const;

  float skyAlpha() const;

  Color environmentLight() const;
  Color mainSkyColor() const;

  // Base sky rect colors, top and bottom, includes calculation based on day /
  // night alpha
  pair<Color, Color> skyRectColors() const;
  Color skyFlashColor() const;

  bool flying() const;
  FlyingType flyingType() const;
  float warpProgress() const;
  WarpPhase warpPhase() const;
  bool inHyperspace() const;

  SkyRenderData renderData() const;

private:
  // TODO: This needs to be more explicit/handled better
  static constexpr float DefaultDayLength = 1000.0f;

  void writeNetStates();
  void readNetStates();

  void enterHyperspace();
  void exitHyperspace();
  bool controlledMovement(JsonArray const& path, Json const& origin, float timeOffset);
  Vec2F getStarOffset() const;
  float getStarRotation() const;
  Vec2F getWorldOffset() const;
  float getWorldRotation() const;
  float speedupTime() const;
  float slowdownTime() const;

  Json m_settings;
  SkyParameters m_skyParameters;
  bool m_skyParametersUpdated;

  SkyType m_skyType = SkyType::Orbital;

  double m_time = 0.0;

  ClockConstPtr m_referenceClock;
  Maybe<double> m_clockTrackingTime;

  float m_altitude = 0.0f;

  FlyingType m_flyingType = FlyingType::None;
  FlyingType m_lastFlyingType = FlyingType::None;
  double m_flyingTimer = 0.0;

  bool m_enterHyperspace = false;
  bool m_startInWarp = false;

  WarpPhase m_warpPhase = WarpPhase::Maintain;
  WarpPhase m_lastWarpPhase = WarpPhase::Maintain;

  double m_flashTimer = 0.0;

  // The star and world offsets and rotations must be different for two
  // reasons: #1, the stars rotate over time, meaning that if they're not
  // different then the world will fly off in a random direction when we leave
  // #2, the stars move at a different, slower rate, controlled by JSON
  // "starVelocityFactor", because they're farther away
  Vec2F m_starOffset;
  double m_starRotation = 0.0;
  Vec2F m_starMoveOffset;

  Vec2F m_worldOffset;
  float m_worldRotation = 0.0f;
  Vec2F m_worldMoveOffset;

  // Finally, these are the offsets for the disembark and arrival paths they
  // are applied to BOTH world and star offsets governed by the
  // starVelocityFactor in the latter case

  Vec2F m_pathOffset;
  float m_pathRotation = 0.0f;

  size_t m_starFrames = 0;
  StringList m_starList;
  StringList m_hyperStarList;

  bool m_sentSFX = false;

  Maybe<SkyParameters> m_destWorld;

  bool m_netInit;
  NetElementTopGroup m_netGroup;

  NetElementBytes m_skyParametersNetState;
  NetElementInt m_skyTypeNetState;
  NetElementDouble m_timeNetState;
  NetElementUInt m_flyingTypeNetState;
  NetElementBool m_enterHyperspaceNetState;
  NetElementBool m_startInWarpNetState;
  NetElementInt m_warpPhaseNetState;
  NetElementData<Vec2F> m_worldMoveNetState;
  NetElementData<Vec2F> m_starMoveNetState;
  NetElementFloat m_flyingTimerNetState;
};

}

#endif
