#ifndef STAR_FUEL_WIDGET_HPP
#define STAR_FUEL_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(FuelWidget);

class FuelWidget : public Widget {
public:
  FuelWidget();
  virtual ~FuelWidget() {}

  virtual void update(float dt);

  void setCurrentFuelLevel(float amount);
  void setMaxFuelLevel(float amount);
  void setPotentialFuelAmount(float amount);
  void setRequestedFuelAmount(float amount);

  void ping();

protected:
  virtual void renderImpl();

  float m_fuelLevel;
  float m_maxLevel;
  float m_potential;
  float m_requested;

  float m_pingTimeout;

  unsigned m_fontSize;
  String m_font;

private:
};

}

#endif
