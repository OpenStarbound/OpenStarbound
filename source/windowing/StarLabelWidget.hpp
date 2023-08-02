#ifndef STAR_LABEL_WIDGET_HPP
#define STAR_LABEL_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(LabelWidget);
class LabelWidget : public Widget {
public:
  LabelWidget(String text = String(),
      Color const& color = Color::White,
      HorizontalAnchor const& hAnchor = HorizontalAnchor::LeftAnchor,
      VerticalAnchor const& vAnchor = VerticalAnchor::BottomAnchor,
      Maybe<unsigned> wrapWidth = {},
      Maybe<float> lineSpacing = {});

  String const& text() const;
  Maybe<unsigned> getTextCharLimit() const;
  void setText(String newText);
  void setFontSize(int fontSize);
  void setFontMode(FontMode fontMode);
  void setColor(Color newColor);
  void setAnchor(HorizontalAnchor hAnchor, VerticalAnchor vAnchor);
  void setWrapWidth(Maybe<unsigned> wrapWidth);
  void setLineSpacing(Maybe<float> lineSpacing);
  void setDirectives(String const& directives);
  void setTextCharLimit(Maybe<unsigned> charLimit);

  RectI relativeBoundRect() const override;

protected:
  virtual RectI getScissorRect() const override;
  virtual void renderImpl() override;

private:
  void updateTextRegion();

  String m_text;
  int m_fontSize;
  FontMode m_fontMode;
  Color m_color;
  HorizontalAnchor m_hAnchor;
  VerticalAnchor m_vAnchor;
  String m_processingDirectives;
  String m_font;
  Maybe<unsigned> m_wrapWidth;
  Maybe<float> m_lineSpacing;
  Maybe<unsigned> m_textCharLimit;
  RectI m_textRegion;
};

}

#endif
