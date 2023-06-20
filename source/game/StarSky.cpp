#include "StarSky.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarCelestialGraphics.hpp"
#include "StarAssets.hpp"
#include "StarTime.hpp"
#include "StarRandomPoint.hpp"
#include "StarMixer.hpp"
#include "StarCompression.hpp"

namespace Star {

Sky::Sky() {
  m_settings = Root::singleton().assets()->json("/sky.config");

  m_starFrames = m_settings.queryInt("stars.frames");
  m_starList = jsonToStringList(m_settings.query("stars.list"));
  m_hyperStarList = jsonToStringList(m_settings.query("stars.hyperlist"));

  m_netInit = false;

  m_netGroup.addNetElement(&m_skyParametersNetState);
  m_netGroup.addNetElement(&m_skyTypeNetState);
  m_netGroup.addNetElement(&m_timeNetState);
  m_netGroup.addNetElement(&m_flyingTypeNetState);
  m_netGroup.addNetElement(&m_enterHyperspaceNetState);
  m_netGroup.addNetElement(&m_startInWarpNetState);
  m_netGroup.addNetElement(&m_worldMoveNetState);
  m_netGroup.addNetElement(&m_starMoveNetState);
  m_netGroup.addNetElement(&m_warpPhaseNetState);
  m_netGroup.addNetElement(&m_flyingTimerNetState);

  m_netGroup.setNeedsLoadCallback(bind(&Sky::readNetStates, this));
  m_netGroup.setNeedsStoreCallback(bind(&Sky::writeNetStates, this));
}

Sky::Sky(SkyParameters const& skyParameters, bool inOrbit) : Sky() {
  m_skyParameters = skyParameters;
  m_skyParametersUpdated = true;

  if (inOrbit)
    m_skyType = SkyType::Orbital;
  else
    m_skyType = m_skyParameters.skyType;
}

void Sky::startFlying(bool enterHyperspace, bool startInWarp) {
  if (startInWarp)
    m_flyingType = FlyingType::Warp;
  else
    m_flyingType = FlyingType::Disembarking;

  m_flyingTimer = 0;
  m_enterHyperspace = enterHyperspace;
  m_startInWarp = startInWarp;
}

void Sky::stopFlyingAt(Maybe<SkyParameters> dest) {
  m_destWorld = dest;
}

void Sky::jumpTo(SkyParameters skyParameters) {
  m_skyParameters = skyParameters;
  m_skyParametersUpdated = true;
}

pair<ByteArray, uint64_t> Sky::writeUpdate(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Sky::readUpdate(ByteArray data) {
  m_netGroup.readNetState(move(data));
}

void Sky::stateUpdate() {
  if (m_lastFlyingType != m_flyingType) {
    m_flyingTimer = 0.0f;

    if (m_flyingType == FlyingType::Warp) {
      m_warpPhase = WarpPhase::SpeedingUp;
      if (m_startInWarp) {
        if (m_enterHyperspace)
          m_warpPhase = WarpPhase::Maintain;
        else
          m_flyingTimer = speedupTime();

        m_lastWarpPhase = m_warpPhase;
      }

      float maxVelocity = m_settings.queryFloat("flyMaxVelocity");
      m_worldMoveOffset = Vec2F::withAngle(m_pathRotation, maxVelocity / 2.0 * speedupTime());
      m_starMoveOffset = Vec2F::withAngle(0, (maxVelocity * m_settings.queryFloat("starVelocityFactor")) / 2.0 * speedupTime());
    } else if (m_flyingType == FlyingType::Arriving) {
      m_sentSFX = false;

      m_worldOffset = {};
      m_starOffset = {};
    }
  }

  if (m_lastWarpPhase != m_warpPhase) {
    m_flyingTimer = 0.0f;

    if (m_warpPhase == WarpPhase::SpeedingUp)
      m_sentSFX = false;
    else if (m_warpPhase == WarpPhase::Maintain)
      enterHyperspace();
    else if (m_warpPhase == WarpPhase::SlowingDown)
      exitHyperspace();
  }

  m_lastFlyingType = m_flyingType;
  m_lastWarpPhase = m_warpPhase;
}

void Sky::update() {
  double dt = WorldTimestep;
  if (m_referenceClock) {
    m_time = m_referenceClock->time();
    if (!m_clockTrackingTime) {
      m_clockTrackingTime = m_time;
    } else {
      // If our reference clock is set, and we have a valid tracking time, then
      // the dt should be driven by the reference clock.
      dt = m_time - *m_clockTrackingTime;
      m_clockTrackingTime = m_time;
    }
  } else {
    m_time += dt;
  }

  m_flashTimer = std::max(0.0, m_flashTimer - dt);

  if (flying()) {
    m_flyingTimer += dt;

    if (m_flyingType == FlyingType::Disembarking) {
      bool finished;
      if (m_skyParameters.skyType == SkyType::Space)
        finished = controlledMovement(m_settings.getArray("spaceDisembarkPath"), m_settings.get("spaceDisembarkOrigin"), m_flyingTimer);
      else
        finished = controlledMovement(m_settings.getArray("disembarkPath"), m_settings.get("disembarkOrigin"), m_flyingTimer);

      if (finished) {
        m_flyingType = FlyingType::Warp;
      }
    } else if (m_flyingType == FlyingType::Arriving) {
      bool finished;
      if (m_skyParameters.skyType == SkyType::Space)
        finished = controlledMovement(m_settings.getArray("spaceArrivalPath"), m_settings.get("spaceArrivalOrigin"), m_flyingTimer);
      else
        finished = controlledMovement(m_settings.getArray("arrivalPath"), m_settings.get("arrivalOrigin"), m_flyingTimer);

      if (finished) {
        m_flyingType = FlyingType::None;
      }

      m_starOffset -= m_starOffset * m_settings.queryFloat("correctionPower");
      m_worldOffset -= m_worldOffset * m_settings.queryFloat("correctionPower");
    } else if (m_flyingType == FlyingType::Warp) {
      float percentage = 0.0;
      float dir = (int)m_warpPhase < 0 ? -1.0 : 1.0;
      if (m_warpPhase == WarpPhase::SpeedingUp)
        percentage = powf(m_flyingTimer / speedupTime(), 2.0);
      else if (m_warpPhase == WarpPhase::Maintain)
        percentage = 1.0;
      else if (m_warpPhase == WarpPhase::SlowingDown)
        percentage = powf(1 - m_flyingTimer / slowdownTime(), 2.0);

      if (percentage < 1.0) {
        m_starOffset = (dir * m_starMoveOffset * percentage).rotate(-getStarRotation());
        m_worldOffset = (dir * m_worldMoveOffset * percentage).rotate(-getWorldRotation());
      } else {
        m_starOffset += Vec2F::withAngle(-getStarRotation(), m_settings.queryFloat("flyMaxVelocity") * dt * m_settings.queryFloat("starVelocityFactor"));
        m_worldOffset = m_worldMoveOffset;
      }

      if (m_warpPhase == WarpPhase::SpeedingUp && m_flyingTimer >= speedupTime()
          && !m_enterHyperspace
          && m_destWorld) {
        jumpTo(m_destWorld.take());
        m_warpPhase = WarpPhase::SlowingDown;
      } else if (m_warpPhase == WarpPhase::SpeedingUp && m_flyingTimer >= speedupTime() && m_enterHyperspace) {
        m_warpPhase = WarpPhase::Maintain;
      } else if (m_warpPhase == WarpPhase::Maintain && m_flyingTimer >= m_settings.queryFloat("flyingTimer")
          && m_destWorld) {
        jumpTo(m_destWorld.take());
        m_warpPhase = WarpPhase::SlowingDown;
      } else if (m_warpPhase == WarpPhase::SlowingDown && m_flyingTimer >= slowdownTime()) {
        m_flyingType = FlyingType::Arriving;
      }
    }
  } else {
    m_starOffset = {};
    m_worldOffset = {};
    m_pathOffset = {};
    m_worldRotation = m_pathRotation = 0;
  }

  stateUpdate();

  if (!flying())
    m_starRotation = constrainAngle(m_starRotation + dt / dayLength() * 2 * Constants::pi);
  else
    m_starRotation = 0;
}

void Sky::setType(SkyType skyType) {
  m_skyType = skyType;
}

SkyType Sky::type() const {
  return m_skyType;
}

bool Sky::inSpace() const {
  return m_skyType == SkyType::Orbital || m_skyType == SkyType::Warp || m_skyType == SkyType::Space;
}

uint64_t Sky::seed() const {
  return m_skyParameters.seed;
}

float Sky::dayLength() const {
  return m_skyParameters.dayLength.value(DefaultDayLength);
}

uint32_t Sky::day() const {
  if (!m_skyParameters.dayLength)
    return 0;
  return floor(epochTime() / dayLength());
}

float Sky::timeOfDay() const {
  if (!m_skyParameters.dayLength)
    return 0;
  return fmod(epochTime(), (double)dayLength());
}

double Sky::epochTime() const {
  return m_time;
}

void Sky::setEpochTime(double epochTime) {
  m_time = epochTime;
}

float Sky::altitude() const {
  return m_altitude;
}

void Sky::setAltitude(float altitude) {
  m_altitude = altitude;
}

void Sky::setReferenceClock(ClockConstPtr const& referenceClock) {
  m_referenceClock = referenceClock;
  m_time = m_referenceClock->time();
  m_clockTrackingTime = {};
}

ClockConstPtr Sky::referenceClock() const {
  return m_referenceClock;
}

String Sky::ambientNoise() const {
  if (flying()) {
    if (m_flyingType == FlyingType::Warp && m_warpPhase == WarpPhase::Maintain) {
      return m_settings.queryString("hyperspaceAudio");
    } else {
      return m_settings.queryString("engineAudio");
    }
  }

  return "";
}

List<AudioInstancePtr> Sky::pullSounds() {
  List<AudioInstancePtr> res;
  if (m_flyingType == FlyingType::Warp) {
    if (m_warpPhase == WarpPhase::SpeedingUp && !m_sentSFX) {
      float triggerTime = speedupTime() - m_settings.queryFloat("enterHyperspaceAudioLeadIn");
      if (triggerTime < 0 || !m_enterHyperspace) {
        m_sentSFX = true;
        return res;
      }

      if (m_flyingTimer >= triggerTime) {
        auto assets = Root::singleton().assets();
        auto SFX = assets->audio(m_settings.queryString("enterHyperspaceAudio"));
        m_sentSFX = true;
        res.append(make_shared<AudioInstance>(*SFX));
        return res;
      }
    } else if (m_warpPhase == WarpPhase::Maintain && !m_sentSFX) {
      float triggerTime = m_settings.queryFloat("flyingTimer") - m_settings.queryFloat("exitHyperspaceAudioLeadIn");
      if (triggerTime < 0) {
        m_sentSFX = true;
        return res;
      }

      if (m_flyingTimer >= triggerTime) {
        auto assets = Root::singleton().assets();
        auto SFX = assets->audio(m_settings.queryString("exitHyperspaceAudio"));
        m_sentSFX = true;
        res.append(make_shared<AudioInstance>(*SFX));
        return res;
      }
    }
  } else if (m_flyingType == FlyingType::Arriving) {
    if (!m_sentSFX) {
      auto assets = Root::singleton().assets();
      auto SFX = assets->audio(m_settings.queryString("arrivalAudio"));
      m_sentSFX = true;
      res.append(make_shared<AudioInstance>(*SFX));
      return res;
    }
  }
  return res;
}

float Sky::spaceLevel() const {
  if (type() == SkyType::Atmospheric && m_skyParameters.spaceLevel && m_skyParameters.surfaceLevel) {
    float altitudeRange = *m_skyParameters.spaceLevel - *m_skyParameters.surfaceLevel;
    float relativeAltitude = m_altitude - *m_skyParameters.surfaceLevel;
    return clamp(relativeAltitude / altitudeRange, 0.0f, 1.0f);
  }
  return 1.0f;
}

float Sky::orbitAngle() const {
  // TODO: What should we do here?  Used to divide by zero.  Now is saved by
  // DefaultDayLength, but it's a hack
  return 2 * Constants::pi * timeOfDay() / dayLength();
}

bool Sky::isDayTime() const {
  return dayLevel() >= 0.5;
}

float Sky::dayLevel() const {
  // Turn the dayCycle value into a value that blends evenly between 0.0 at
  // mid-night and 1.0 at mid-day and then back again.

  float dayCycle = Sky::dayCycle();
  if (dayCycle < 1.0)
    return dayCycle / 2 + 0.5f;
  else if (dayCycle > 3.0)
    return (dayCycle - 3) / 2;
  else
    return 1.0 - (dayCycle - 1.0) / 2;
}

float Sky::dayCycle() const {
  // Always middle of the night in orbit or warp space.
  if (type() == SkyType::Orbital || type() == SkyType::Warp)
    return 3.0f;

  // This will misbehave badly if dayTransitionTime is greater than dayLength /
  // 2

  float transitionTime = m_settings.queryFloat("dayTransitionTime") / 2;
  float dayLength = Sky::dayLength();
  float timeOfDay = Sky::timeOfDay();

  // timeOfDay() is defined such that 0.0 is mid-dawn.  For convenience, shift
  // the time of day forwards such that 0.0 is the beginning of the morning.
  float shiftedTime = pfmod(timeOfDay + transitionTime / 2, dayLength);

  // There are 5 times here, beginning of the morning, end of the morning,
  // beginning of the evening, end of the evening, and then the beginning of
  // the morning again (wrapping around).
  Array<float, 5> transitionPositions = {
      0.0f, transitionTime, dayLength / 2, dayLength / 2 + transitionTime, dayLength};
  // The values here are mid-night, mid-day, mid-day, mid-night, mid-night.
  Array<float, 5> transitionValues = {-1.0f, 1.0f, 1.0f, 3.0f, 3.0f};

  return pfmod(parametricInterpolate2(
                   transitionPositions, transitionValues, shiftedTime, SinWeightOperator<float>(), BoundMode::Clamp),
      4.0f);
}

float Sky::skyAlpha() const {
  if (m_skyType != SkyType::Atmospheric) {
    return 0.0f;
  } else {
    float skyLevel = 1.0f - spaceLevel();
    return clamp(pow(skyLevel, m_settings.getFloat("skyLevelExponent")), 0.0f, 1.0f);
  }
}

Color Sky::environmentLight() const {
  if (m_skyType == SkyType::Orbital || m_skyType == SkyType::Warp)
    return Color::Black;

  if (m_skyParameters.skyColoring.isLeft()) {
    auto skyColoring = m_skyParameters.skyColoring.left();

    Array<Vec4F, 4> colors;
    colors[0] = skyColoring.morningLightColor.toRgbaF();
    colors[1] = skyColoring.dayLightColor.toRgbaF();
    colors[2] = skyColoring.eveningLightColor.toRgbaF();
    colors[3] = skyColoring.nightLightColor.toRgbaF();

    return Color::rgbaf(listInterpolate2(colors, dayCycle(), SinWeightOperator<float>(), BoundMode::Wrap));
  } else {
    return m_skyParameters.skyColoring.right();
  }
}

Color Sky::mainSkyColor() const {
  if (m_skyParameters.skyColoring.isLeft())
    return m_skyParameters.skyColoring.left().mainColor;

  return Color::Black;
}

pair<Color, Color> Sky::skyRectColors() const {
  if (m_skyParameters.skyColoring.isLeft()) {
    auto skyColoring = m_skyParameters.skyColoring.left();

    Array<Vec4F, 4> topColorList;
    topColorList[0] = skyColoring.morningColors.first.toRgbaF();
    topColorList[1] = skyColoring.dayColors.first.toRgbaF();
    topColorList[2] = skyColoring.eveningColors.first.toRgbaF();
    topColorList[3] = skyColoring.nightColors.first.toRgbaF();

    Array<Vec4F, 4> bottomColorList;
    bottomColorList[0] = skyColoring.morningColors.second.toRgbaF();
    bottomColorList[1] = skyColoring.dayColors.second.toRgbaF();
    bottomColorList[2] = skyColoring.eveningColors.second.toRgbaF();
    bottomColorList[3] = skyColoring.nightColors.second.toRgbaF();

    float cycle = dayCycle();
    auto topColor = Color::rgbaf(listInterpolate2(topColorList, cycle, SinWeightOperator<float>(), BoundMode::Wrap));
    auto bottomColor = Color::rgbaf(listInterpolate2(bottomColorList, cycle, SinWeightOperator<float>(), BoundMode::Wrap));

    float skyAlpha = Sky::skyAlpha();
    topColor.setAlpha(topColor.alpha() * skyAlpha);
    bottomColor.setAlpha(bottomColor.alpha() * skyAlpha);

    return {topColor, bottomColor};
  }

  return {Color::Clear, Color::Clear};
}

Color Sky::skyFlashColor() const {
  Color res = Color::White;
  res.setAlphaF(m_flashTimer / m_settings.queryFloat("flashTimer"));
  return res;
}

bool Sky::flying() const {
  if (m_flyingType == FlyingType::None)
    return false;
  return true;
}

FlyingType Sky::flyingType() const {
  return m_flyingType;
}

float Sky::warpProgress() const {
  if (m_flyingType == FlyingType::Warp) {
    auto warpTime = speedupTime() + m_settings.queryFloat("flyingTimer") + slowdownTime();
    auto timer = m_flyingTimer;
    if (m_warpPhase < WarpPhase::SpeedingUp)
      timer += speedupTime();
    if (m_warpPhase < WarpPhase::Maintain)
      timer += m_settings.queryFloat("flyingTimer");
    return timer / warpTime;
  }
  return 0.0;
}

WarpPhase Sky::warpPhase() const {
  return m_warpPhase;
}

bool Sky::inHyperspace() const {
  return m_flyingType == FlyingType::Warp && m_enterHyperspace;
}

SkyRenderData Sky::renderData() const {
  SkyRenderData renderData;
  renderData.settings = m_settings;
  renderData.skyParameters = m_skyParameters;
  renderData.type = m_skyType;
  renderData.dayLevel = dayLevel();
  renderData.skyAlpha = skyAlpha();

  renderData.dayLength = dayLength();
  renderData.timeOfDay = timeOfDay();
  renderData.epochTime = epochTime();

  renderData.starOffset = getStarOffset();
  renderData.starRotation = getStarRotation();
  renderData.worldOffset = getWorldOffset();
  renderData.worldRotation = getWorldRotation();
  renderData.orbitAngle = orbitAngle();

  renderData.starFrames = m_starFrames;
  renderData.starList = m_starList;
  renderData.hyperStarList = m_hyperStarList;

  renderData.environmentLight = environmentLight();
  renderData.mainSkyColor = mainSkyColor();
  tie(renderData.topRectColor, renderData.bottomRectColor) = skyRectColors();
  renderData.flashColor = skyFlashColor();

  return renderData;
}

void Sky::writeNetStates() {
  if (take(m_skyParametersUpdated))
    m_skyParametersNetState.set(DataStreamBuffer::serialize<Json>(m_skyParameters.toJson()));

  m_skyTypeNetState.set((int)m_skyType);
  m_timeNetState.set(m_time);
  m_enterHyperspaceNetState.set(m_enterHyperspace);
  m_startInWarpNetState.set(m_startInWarp);

  m_flyingTypeNetState.set((unsigned)m_flyingType);
  m_warpPhaseNetState.set((int)m_warpPhase);

  m_flyingTimerNetState.set(m_flyingTimer);
  m_worldMoveNetState.set(m_worldMoveOffset);
  m_starMoveNetState.set(m_starMoveOffset);
}

void Sky::readNetStates() {
  if (m_skyParametersNetState.pullUpdated())
    m_skyParameters = SkyParameters(DataStreamBuffer::deserialize<Json>(m_skyParametersNetState.get()));

  m_skyType = (SkyType)m_skyTypeNetState.get();
  m_time = m_timeNetState.get();
  m_enterHyperspace = m_enterHyperspaceNetState.get();
  m_startInWarp = m_startInWarpNetState.get();

  m_flyingType = (FlyingType)m_flyingTypeNetState.get();
  m_warpPhase = (WarpPhase)m_warpPhaseNetState.get();
  stateUpdate();

  if (!m_netInit) {
    m_netInit = true;

    m_flyingTimer = m_flyingTimerNetState.get();
    m_worldMoveOffset = m_worldMoveNetState.get();
    m_starMoveOffset = m_starMoveNetState.get();
  }
}

void Sky::enterHyperspace() {
  m_flashTimer = m_settings.queryFloat("flashTimer");
  setType(SkyType::Warp);

  m_sentSFX = false;
}

void Sky::exitHyperspace() {
  m_flashTimer = m_settings.queryFloat("flashTimer");
  setType(SkyType::Orbital);

  m_sentSFX = false;
  Json originMap;
  Json destMap;
  if (m_skyParameters.skyType == SkyType::Space) {
    originMap = m_settings.getObject("spaceArrivalOrigin");
    destMap = m_settings.getArray("spaceArrivalPath")[0];
  } else {
    originMap = m_settings.getObject("arrivalOrigin");
    destMap = m_settings.getArray("arrivalPath")[0];
  }

  m_pathOffset = jsonToVec2F(originMap.get("offset"));
  m_pathRotation = originMap.get("rotation").toFloat() * Constants::deg2rad;

  float maxVelocity = m_settings.queryFloat("flyMaxVelocity");
  float exitDistance = maxVelocity / 2.0 * slowdownTime();
  m_worldMoveOffset = Vec2F::withAngle(0, exitDistance);
  m_worldOffset = m_worldMoveOffset;

  exitDistance = (maxVelocity * m_settings.queryFloat("starVelocityFactor")) / 2.0 * slowdownTime();
  m_starMoveOffset = Vec2F::withAngle(0, exitDistance);
  m_starOffset = m_starMoveOffset;

  m_worldRotation = m_starRotation = 0;
  m_flyingTimer = 0;
}

bool Sky::controlledMovement(JsonArray const& path, Json const& origin, float timeOffset) {
  float previousTime = 0;
  Vec2F previousOffset = jsonToVec2F(origin.get("offset"));
  float previousRotation = origin.getFloat("rotation") * Constants::deg2rad;

  float stepTime = 0;
  Vec2F stepOffset;
  float stepRotation;
  for (auto const& entry : path) {
    stepOffset = jsonToVec2F(entry.get("offset"));
    stepRotation = entry.getFloat("rotation") * Constants::deg2rad;
    stepTime += entry.getFloat("time");

    if (timeOffset <= stepTime) {
      float percentage = (timeOffset - previousTime) / (stepTime - previousTime);
      m_pathOffset = lerp(percentage, previousOffset, stepOffset);
      m_pathRotation = lerp(percentage, previousRotation, stepRotation);
      return false;
    }

    previousTime = stepTime;
    previousOffset = stepOffset;
    previousRotation = stepRotation;
  }
  // if loop wasn't broken
  // then we're done with this phase of controlled movement
  // signal that we're ready to head out of this system

  return true;
}

Vec2F Sky::getStarOffset() const {
  return (m_starOffset + m_pathOffset);
}

float Sky::getStarRotation() const {
  return m_starRotation + m_pathRotation;
}

Vec2F Sky::getWorldOffset() const {
  return m_worldOffset + m_pathOffset;
}

float Sky::getWorldRotation() const {
  return m_worldRotation + m_pathRotation;
}

float Sky::speedupTime() const {
  if (m_enterHyperspace)
    return m_settings.queryFloat("hyperspaceSpeedupTime");
  else
    return m_settings.queryFloat("speedupTime");
}

float Sky::slowdownTime() const {
  if (m_enterHyperspace)
    return m_settings.queryFloat("hyperspaceSlowdownTime");
  else
    return m_settings.queryFloat("slowdownTime");
}

}
