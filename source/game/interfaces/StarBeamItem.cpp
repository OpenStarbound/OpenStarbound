#include "StarBeamItem.hpp"
#include "StarJsonExtra.hpp"
#include "StarImageProcessing.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarRandom.hpp"
#include "StarItem.hpp"
#include "StarToolUserEntity.hpp"
#include "StarWorld.hpp"

namespace Star {

BeamItem::BeamItem(Json config) {
  config = Root::singleton().assets()->json("/player.config:beamGunConfig").setAll(config.toObject());

  m_image = config.get("image").toString();
  m_endImages = jsonToStringList(config.get("endImages"));
  m_endType = EndType::Invalid;
  m_segmentsPerUnit = config.get("segmentsPerUnit").toFloat();
  m_nearControlPointElasticity = config.get("nearControlPointElasticity").toFloat();
  m_farControlPointElasticity = config.get("farControlPointElasticity").toFloat();
  m_nearControlPointDistance = config.get("nearControlPointDistance").toFloat();
  m_handPosition = jsonToVec2F(config.get("handPosition"));
  m_firePosition = jsonToVec2F(config.get("firePosition"));
  m_range = 1.0f;
  m_targetSegmentRun = config.get("targetSegmentRun").toFloat();
  m_minBeamWidth = config.get("minBeamWidth").toFloat();
  m_maxBeamWidth = config.get("maxBeamWidth").toFloat();
  m_beamWidthDev = config.getFloat("beamWidthDev", (m_maxBeamWidth - m_minBeamWidth) / 3);
  m_minBeamJitter = config.get("minBeamJitter").toFloat();
  m_maxBeamJitter = config.get("maxBeamJitter").toFloat();
  m_beamJitterDev = config.getFloat("beamJitterDev", (m_maxBeamJitter * 2) / 3);
  m_minBeamTrans = config.get("minBeamTrans").toFloat();
  m_maxBeamTrans = config.get("maxBeamTrans").toFloat();
  m_beamTransDev = config.getFloat("beamTransDev", (m_maxBeamTrans - m_minBeamTrans) / 3);
  m_minBeamLines = config.get("minBeamLines").toInt();
  m_maxBeamLines = config.get("maxBeamLines").toInt();
  m_innerBrightnessScale = config.get("innerBrightnessScale").toFloat();
  m_firstStripeThickness = config.get("firstStripeThickness").toFloat();
  m_secondStripeThickness = config.get("secondStripeThickness").toFloat();
  m_color = {255, 255, 255, 255};
  m_particleGenerateCooldown = .25;
  m_inRangeLastUpdate = false;
}

void BeamItem::init(ToolUserEntity* owner, ToolHand hand) {
  ToolUserItem::init(owner, hand);

  m_beamCurve = CSplineF();

  if (initialized()) {
    m_color = owner->favoriteColor();
    m_range = owner->beamGunRadius();
    return;
  }

  throw ItemException("BeamItem::init: Beam Gun not init'd properly, or user not recognized as Tool User.");
}

void BeamItem::update(FireMode, bool, HashSet<MoveControlType> const&) {
  if (m_particleGenerateCooldown >= 0)
    m_particleGenerateCooldown -= WorldTimestep;

  if (!initialized())
    throw ItemException("BeamItem::update: Beam Gun not init'd properly, or user not recognized as Tool User.");

  m_beamCurve.origin() = owner()->handPosition(hand(), (m_firePosition - m_handPosition) / TilePixels);

  if (m_endType == EndType::TileGroup)
    m_beamCurve.dest() = world()->geometry().diff(owner()->aimPosition().round(), owner()->position());
  else if (m_endType == EndType::Wire)
    m_beamCurve.dest() = world()->geometry().diff(owner()->aimPosition(), owner()->position());
  else
    m_beamCurve.dest() = world()->geometry().diff(centerOfTile(owner()->aimPosition()), owner()->position());

  if (m_beamCurve.dest().magnitudeSquared() < m_beamCurve.origin().magnitudeSquared())
    m_beamCurve[2] = m_beamCurve.dest();
  else
    m_beamCurve[2] = m_beamCurve[2] + (m_beamCurve.dest() - m_beamCurve[2]) * m_farControlPointElasticity;

  Vec2F desiredNearControlPoint = (m_beamCurve.dest() - m_beamCurve.origin()) * m_nearControlPointDistance;

  if (m_beamCurve.dest().magnitudeSquared() < m_beamCurve.origin().magnitudeSquared())
    m_beamCurve[1] = m_beamCurve.origin();
  else if (owner()->facingDirection() != getAngleSide(m_beamCurve[1].angle()).second)
    m_beamCurve[1] = desiredNearControlPoint;
  else
    m_beamCurve[1] = m_beamCurve[1] + (desiredNearControlPoint - m_beamCurve[1]) * m_nearControlPointElasticity;
}

List<Drawable> BeamItem::nonRotatedDrawables() const {
  return beamDrawables();
}

float BeamItem::getAngle(float angle) {
  if (m_beamCurve.dest().magnitudeSquared() < m_beamCurve.origin().magnitudeSquared()
      || m_beamCurve.origin() == m_beamCurve[1])
    return angle;
  return getAngleSide(m_beamCurve[1].angle()).first;
}

List<Drawable> BeamItem::drawables() const {
  return {Drawable::makeImage(m_image, 1.0f / TilePixels, true, -handPosition() / TilePixels)};
}

Vec2F BeamItem::handPosition() const {
  return m_handPosition;
}

Vec2F BeamItem::firePosition() const {
  return m_firePosition;
}

void BeamItem::setRange(float range) {
  m_range = range;
}

float BeamItem::getAppropriateOpacity() const {
  float curveLen = m_beamCurve.length();
  const float rangeEffect = (m_range - curveLen) / m_range;

  auto projectOntoRange = [&](float min, float max) { return rangeEffect * (max - min) + min; };
  auto rangeRand = [&](float dev, float min, float max) {
    return clamp<float>(Random::nrandf(dev, projectOntoRange(min, max)), min, max);
  };

  int numLines = projectOntoRange(m_minBeamLines, m_maxBeamLines);
  float res = (1 - rangeRand(m_beamTransDev, m_minBeamTrans, m_maxBeamTrans));
  if (numLines > 0) {
    for (auto line = 0; line < numLines - 1; line++)
      res *= (1 - rangeRand(m_beamTransDev, m_minBeamTrans, m_maxBeamTrans));
  }
  return 1 - res;
}

void BeamItem::setEnd(EndType type) {
  m_endType = type;
}

List<Drawable> BeamItem::beamDrawables(bool canPlace) const {
  List<Drawable> res;

  float curveLen = m_beamCurve.length();
  const float rangeEffect = (m_range - curveLen) / m_range;

  auto projectOntoRange = [&](float min, float max) { return rangeEffect * (max - min) + min; };
  auto rangeRand = [&](float dev, float min, float max) {
    return clamp<float>(Random::nrandf(dev, projectOntoRange(min, max)), min, max);
  };

  if (initialized()) {
    Vec2F endPoint;
    if (m_endType == EndType::TileGroup)
      endPoint = owner()->aimPosition().round();
    else if (m_endType == EndType::Wire)
      endPoint = owner()->aimPosition();
    else
      endPoint = centerOfTile(owner()->aimPosition());

    if ((endPoint - owner()->position()).magnitude() <= m_range && curveLen <= m_range) {
      m_inRangeLastUpdate = true;
      int numLines = projectOntoRange(m_minBeamLines, m_maxBeamLines);
      Vec4B mainColor = m_color;
      if (!canPlace) {
        Color temp = Color::rgba(m_color);
        temp.setHue(temp.hue() + 120);
        mainColor = temp.toRgba();
      }
      m_lastUpdateColor = mainColor;

      String endImage = "";
      if (m_endType != EndType::Invalid) {
        endImage = m_endImages[(unsigned)m_endType];
      }

      if (!endImage.empty()) {
        if (!canPlace) {
          ImageOperation op = HueShiftImageOperation::hueShiftDegrees(120);
          endImage = strf("{}?{}", endImage, imageOperationToString(op));
        }

        Drawable ball = Drawable::makeImage(endImage, 1.0f / TilePixels, true, m_beamCurve.dest());
        Color ballColor = Color::White;
        ballColor.setAlphaF(getAppropriateOpacity());
        ball.color = ballColor;
        res.push_back(ball);
      }

      for (auto line = 0; line < numLines; line++) {
        float lineThickness = rangeRand(m_beamWidthDev, m_minBeamWidth, m_maxBeamWidth);
        float beamTransparency = rangeRand(m_beamTransDev, m_minBeamTrans, m_maxBeamTrans);
        mainColor[3] = mainColor[3] * beamTransparency;
        Vec2F previousLoc = m_beamCurve.origin(); // lines meet at origin and dest.
        Color innerStripe = Color::rgba(mainColor);
        innerStripe.setValue(1 - (1 - innerStripe.value()) / m_innerBrightnessScale);
        innerStripe.setSaturation(innerStripe.saturation() / m_innerBrightnessScale);
        Vec4B firstStripe = innerStripe.toRgba();
        innerStripe.setValue(1 - (1 - innerStripe.value()) / m_innerBrightnessScale);
        innerStripe.setSaturation(innerStripe.saturation() / m_innerBrightnessScale);
        Vec4B secondStripe = innerStripe.toRgba();

        for (auto i = 1; i < (int)(curveLen * m_targetSegmentRun - .5); i++) { // one less than full length
          float pos = (float)i / (float)(int)(curveLen * m_targetSegmentRun + .5); // project the discrete steps evenly

          Vec2F currentLoc =
              m_beamCurve.pointAt(pos) + Vec2F(rangeRand(m_beamJitterDev, -m_maxBeamJitter, m_maxBeamJitter),
                                             rangeRand(m_beamJitterDev, -m_maxBeamJitter, m_maxBeamJitter));
          res.push_back(
              Drawable::makeLine(Line2F(previousLoc, currentLoc), lineThickness, Color::rgba(mainColor), Vec2F()));
          res.push_back(Drawable::makeLine(Line2F(previousLoc, currentLoc),
              lineThickness * m_firstStripeThickness,
              Color::rgba(firstStripe),
              Vec2F()));
          res.push_back(Drawable::makeLine(Line2F(previousLoc, currentLoc),
              lineThickness * m_secondStripeThickness,
              Color::rgba(secondStripe),
              Vec2F()));
          previousLoc = std::move(currentLoc);
        }
        res.push_back(Drawable::makeLine(
            Line2F(previousLoc, m_beamCurve.dest()), lineThickness, Color::rgba(mainColor), Vec2F()));
        res.push_back(Drawable::makeLine(Line2F(previousLoc, m_beamCurve.dest()),
            lineThickness * m_firstStripeThickness,
            Color::rgba(firstStripe),
            Vec2F()));
        res.push_back(Drawable::makeLine(Line2F(previousLoc, m_beamCurve.dest()),
            lineThickness * m_secondStripeThickness,
            Color::rgba(secondStripe),
            Vec2F()));
      }
    } else {
      if (m_inRangeLastUpdate) {
        m_inRangeLastUpdate = false;
        m_particleGenerateCooldown = .25; // TODO, expose to json
        List<Particle> beamLeftovers;
        for (auto i = 1; i < (int)(curveLen * m_targetSegmentRun * 2 - .5); i++) { // one less than full length
          float pos =
              (float)i / (float)(int)(curveLen * m_targetSegmentRun * 2 + .5); // project the discrete steps evenly
          float curveLoc = m_beamCurve.arcLenPara(pos);

          Particle beamParticle;
          beamParticle.type = Particle::Type::Ember;
          beamParticle.position = m_beamCurve.pointAt(curveLoc);
          beamParticle.size = 1.0f;

          Color randomColor = Color::rgba(m_lastUpdateColor);
          randomColor.setValue(1 - (1 - randomColor.value()) / Random::randf(1, 4));
          randomColor.setSaturation(randomColor.saturation() / Random::randf(1, 4));

          beamParticle.color = randomColor;
          beamParticle.velocity = Vec2F::filled(Random::randf());
          beamParticle.finalVelocity = Vec2F(0.0f, -20.0f);
          beamParticle.approach = Vec2F(0.0f, 5.0f);
          beamParticle.timeToLive = 0.25f;
          beamParticle.destructionAction = Particle::DestructionAction::Shrink;
          beamParticle.destructionTime = 0.2f;
          beamLeftovers.append(beamParticle);
        }

        owner()->addParticles(beamLeftovers);
      }
    }
  }

  return res;
}

}
