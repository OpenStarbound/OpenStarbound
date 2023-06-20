#include "StarSwingableItem.hpp"

namespace Star {

SwingableItem::SwingableItem() {
  m_swingAimFactor = 0;
  m_swingStart = 0;
  m_swingFinish = 0;
}

SwingableItem::SwingableItem(Json const& params) : FireableItem(params) {
  setParams(params);
}

void SwingableItem::setParams(Json const& params) {
  m_swingStart = params.getFloat("swingStart", 60) * Constants::pi / 180;
  m_swingFinish = params.getFloat("swingFinish", -40) * Constants::pi / 180;
  m_swingAimFactor = params.getFloat("swingAimFactor", 1);
  m_coolingDownAngle = params.optFloat("coolingDownAngle").apply([](float angle) { return angle * Constants::pi / 180; });
  FireableItem::setParams(params);
}

float SwingableItem::getAngleDir(float angle, Direction) {
  return getAngle(angle);
}

float SwingableItem::getAngle(float aimAngle) {
  if (!ready()) {
    if (coolingDown()) {
      if (m_coolingDownAngle)
        return *m_coolingDownAngle + aimAngle * m_swingAimFactor;
      else
        return -Constants::pi / 2;
    }

    if (m_timeFiring < windupTime())
      return m_swingStart + (m_swingFinish - m_swingStart) * m_timeFiring / windupTime() + aimAngle * m_swingAimFactor;

    return m_swingFinish + (m_swingStart - m_swingFinish) * fireTimer() / (cooldownTime() + windupTime()) + aimAngle * m_swingAimFactor;
  }

  return -Constants::pi / 2;
}

float SwingableItem::getItemAngle(float aimAngle) {
  return getAngle(aimAngle);
}

String SwingableItem::getArmFrame() {
  return "rotation";
}

}
