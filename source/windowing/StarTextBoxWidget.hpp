#pragma once

#include "StarWidget.hpp"

namespace Star {

enum class SpecialRepeatKeyCodes : String::Char { None, Delete, Backspace, Left, Right };

STAR_CLASS(TextBoxWidget);
class TextBoxWidget : public Widget {
public:
  TextBoxWidget(String const& startingText, String const& hint, WidgetCallbackFunc callback);

  virtual void update(float dt) override;

  String const& getText() const;
  bool setText(String const& text, bool callback = true);

  bool getHidden() const;
  void setHidden(bool hidden);

  // Set the regex that the text-box must match.  Defaults to .*
  String getRegex();
  void setRegex(String const& regex);

  void setColor(Color const& color);
  void setDirectives(String const& directives);
  void setFontSize(int fontSize);
  void setMaxWidth(int maxWidth);
  void setOverfillMode(bool overfillMode);

  void setOnBlurCallback(WidgetCallbackFunc onBlur);
  void setOnEnterKeyCallback(WidgetCallbackFunc onEnterKey);
  void setOnEscapeKeyCallback(WidgetCallbackFunc onEscapeKey);

  void setNextFocus(Maybe<String> nextFocus);
  void setPrevFocus(Maybe<String> prevFocus);

  bool sendEvent(InputEvent const& event) override;

  void setDrawBorder(bool drawBorder);
  void setTextAlign(HorizontalAnchor hAnchor);
  int getCursorOffset();

  virtual void mouseOver() override;
  virtual void mouseOut() override;
  virtual void mouseReturnStillDown() override;

  virtual void blur() override;

  virtual KeyboardCaptureMode keyboardCaptured() const override;

protected:
  virtual void renderImpl() override;

private:
  bool innerSendEvent(InputEvent const& event);
  bool modText(String const& text);
  bool newTextValid(String const& text) const;

  bool m_textHidden;
  String m_text;
  String m_hint;
  String m_regex;
  HorizontalAnchor m_hAnchor;
  VerticalAnchor m_vAnchor;
  Color m_color;
  String m_processingDirectives;
  String m_font;
  int m_fontSize;
  int m_maxWidth;
  int m_cursorOffset;
  bool m_isHover;
  bool m_isPressed;
  SpecialRepeatKeyCodes m_repeatCode;
  int64_t m_repeatKeyThreshold;
  WidgetCallbackFunc m_callback;
  WidgetCallbackFunc m_onBlur;
  WidgetCallbackFunc m_onEnterKey;
  WidgetCallbackFunc m_onEscapeKey;
  Maybe<String> m_nextFocus;
  Maybe<String> m_prevFocus;
  bool m_drawBorder;
  Vec2I m_cursorHoriz;
  Vec2I m_cursorVert;
  bool m_overfillMode;
};

}
