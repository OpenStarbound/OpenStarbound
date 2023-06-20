#include "StarProgressWidget.hpp"
#include "StarLexicalCast.hpp"
#include "StarInterpolation.hpp"

namespace Star {

ProgressWidget::ProgressWidget(String const& background,
    String const& overlay,
    ImageStretchSet const& progressSet,
    GuiDirection direction)
  : m_background(background),
    m_overlay(overlay),
    m_bar(progressSet),
    m_direction(direction) {

  m_progressLevel = 0;
  m_maxLevel = 1;

  if (!m_background.empty())
    setSize(Vec2I(context()->textureSize(m_background)));
  else if (!m_overlay.empty())
    setSize(Vec2I(context()->textureSize(m_overlay)));

  m_color = Color::White;
}

void ProgressWidget::renderImpl() {
  float progress = 1;
  if (m_maxLevel > 0)
    progress = m_progressLevel / m_maxLevel;

  auto shift = [&](float begin, float end, RectF templ) {
    RectF result = templ;

    if (m_direction == GuiDirection::Horizontal) {
      result.min()[0] = lerp(begin, templ.min()[0], templ.max()[0]);
      result.max()[0] = lerp(end, templ.min()[0], templ.max()[0]);
    } else {
      result.min()[1] = lerp(begin, templ.min()[1], templ.max()[1]);
      result.max()[1] = lerp(end, templ.min()[1], templ.max()[1]);
    }

    return result;
  };

  if (!m_background.empty())
    context()->drawInterfaceQuad(m_background, shift(0, 1, RectF(Vec2F(), Vec2F(size()))), shift(0, 1, RectF(screenBoundRect())));

  context()->drawImageStretchSet(m_bar, shift(0, progress, RectF(screenBoundRect())), m_direction, m_color.toRgba());

  if (!m_overlay.empty())
    context()->drawInterfaceQuad(m_overlay, shift(0, 1, RectF({}, Vec2F(size()))), shift(0, 1, RectF(screenBoundRect())));
}

void ProgressWidget::setCurrentProgressLevel(float amount) {
  m_progressLevel = amount;
}

void ProgressWidget::setMaxProgressLevel(float amount) {
  m_maxLevel = amount;
}

void ProgressWidget::setColor(Color const& color) {
  m_color = color;
}

void ProgressWidget::setOverlay(String const& overlay) {
  m_overlay = overlay;
}

}
