#include "StarLabelWidget.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

LabelWidget::LabelWidget(String text,
    Color const& color,
    HorizontalAnchor const& hAnchor,
    VerticalAnchor const& vAnchor,
    Maybe<unsigned> wrapWidth,
    Maybe<float> lineSpacing)
  : m_color(color),
    m_hAnchor(hAnchor),
    m_vAnchor(vAnchor),
    m_wrapWidth(move(wrapWidth)),
    m_lineSpacing(move(lineSpacing)),
    m_fontMode(FontMode::Normal) {
  auto assets = Root::singleton().assets();
  auto fontConfig = assets->json("/interface.config:font");
  m_fontSize = fontConfig.getInt("baseSize");
  m_processingDirectives = fontConfig.getString("defaultDirectives");
  m_font = fontConfig.queryString("defaultFont", "");
  setText(move(text));
}

String const& LabelWidget::text() const {
  return m_text;
}

Maybe<unsigned> LabelWidget::getTextCharLimit() const {
  return m_textCharLimit;
}

void LabelWidget::setText(String newText) {
  m_text = move(newText);
  updateTextRegion();
}

void LabelWidget::setFontSize(int fontSize) {
  m_fontSize = fontSize;
  updateTextRegion();
}

void LabelWidget::setFontMode(FontMode fontMode) {
  m_fontMode = fontMode;
}

void LabelWidget::setColor(Color newColor) {
  m_color = move(newColor);
}

void LabelWidget::setAnchor(HorizontalAnchor hAnchor, VerticalAnchor vAnchor) {
  m_hAnchor = hAnchor;
  m_vAnchor = vAnchor;
  updateTextRegion();
}

void LabelWidget::setWrapWidth(Maybe<unsigned> wrapWidth) {
  m_wrapWidth = move(wrapWidth);
  updateTextRegion();
}

void LabelWidget::setLineSpacing(Maybe<float> lineSpacing) {
  m_lineSpacing = move(lineSpacing);
  updateTextRegion();
}

void LabelWidget::setDirectives(String const& directives) {
  m_processingDirectives = directives;
  updateTextRegion();
}

void LabelWidget::setTextCharLimit(Maybe<unsigned> charLimit) {
  m_textCharLimit = charLimit;
  updateTextRegion();
}

RectI LabelWidget::relativeBoundRect() const {
  return RectI(m_textRegion).translated(relativePosition());
}

RectI LabelWidget::getScissorRect() const {
  return noScissor();
}

void LabelWidget::renderImpl() {
  context()->setFont(m_font);
  context()->setFontSize(m_fontSize);
  context()->setFontMode(m_fontMode);
  context()->setFontColor(m_color.toRgba());
  context()->setFontProcessingDirectives(m_processingDirectives);

  if (m_lineSpacing)
    context()->setLineSpacing(*m_lineSpacing);
  else
    context()->setDefaultLineSpacing();

  context()->renderInterfaceText(m_text, {Vec2F(screenPosition()), m_hAnchor, m_vAnchor, m_wrapWidth, m_textCharLimit});

  context()->setDefaultFont();
  context()->setFontMode(FontMode::Normal);
  context()->setFontProcessingDirectives("");
  context()->setDefaultLineSpacing();
}

void LabelWidget::updateTextRegion() {
  context()->setFontSize(m_fontSize);
  context()->setFontColor(m_color.toRgba());
  context()->setFontProcessingDirectives(m_processingDirectives);
  context()->setFont(m_font);
  if (m_lineSpacing)
    context()->setLineSpacing(*m_lineSpacing);
  else
    context()->setDefaultLineSpacing();

  m_textRegion = RectI(context()->determineInterfaceTextSize(m_text, {Vec2F(), m_hAnchor, m_vAnchor, m_wrapWidth, m_textCharLimit}));
  setSize(m_textRegion.size());

  context()->setDefaultFont();
  context()->setFontProcessingDirectives("");
  context()->setDefaultLineSpacing();
}

}
