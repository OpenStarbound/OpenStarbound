#ifndef STAR_PROGRESS_WIDGET_HPP
#define STAR_PROGRESS_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ProgressWidget);
class ProgressWidget : public Widget {
public:
  ProgressWidget(String const& background,
      String const& overlay,
      ImageStretchSet const& progressSet,
      GuiDirection direction);
  virtual ~ProgressWidget() {}

  void setCurrentProgressLevel(float amount);
  void setMaxProgressLevel(float amount);

  void setColor(Color const& color);
  void setOverlay(String const& overlay);

protected:
  virtual void renderImpl();
  RectI shift(float begin, float end, RectI templ);

  float m_progressLevel;
  float m_maxLevel;

  Color m_color;

  String m_background;
  String m_overlay;
  ImageStretchSet m_bar;
  GuiDirection m_direction;

private:
};

}

#endif
