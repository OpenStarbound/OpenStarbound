#include "StarTextBoxWidget.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"

namespace Star {

TextBoxWidget::TextBoxWidget(String const& startingText, String const& hint, WidgetCallbackFunc callback)
  : m_text(startingText), m_hint(hint), m_callback(callback) {
  auto assets = Root::singleton().assets();
  m_textHidden = false;
  m_regex = ".*";
  m_repeatKeyThreshold = 0;
  m_repeatCode = SpecialRepeatKeyCodes::None;
  m_isPressed = false;
  m_isHover = false;
  m_cursorOffset = startingText.size();
  m_hAnchor = HorizontalAnchor::LeftAnchor;
  m_vAnchor = VerticalAnchor::BottomAnchor;
  m_drawBorder = false;
  m_cursorHoriz = Vec2I();
  m_cursorVert = Vec2I();
  m_overfillMode = true;

  m_maxWidth = assets->json("/interface.config:textBoxDefaultWidth").toInt();
  auto fontConfig = assets->json("/interface.config:font");
  m_fontSize = fontConfig.getInt("baseSize");
  m_processingDirectives = fontConfig.getString("defaultDirectives");
  m_font = fontConfig.queryString("defaultFont", "");
  m_color = Color::rgb(jsonToVec3B(fontConfig.getArray("defaultColor")));

  // Meh, padding is hard-coded here
  setSize({m_maxWidth + 6, m_fontSize + 2});
  m_cursorHoriz = jsonToVec2I(assets->json("/interface.config:textboxCursorHorizontal"));
  m_cursorVert = jsonToVec2I(assets->json("/interface.config:textboxCursorVertical"));
}

void TextBoxWidget::renderImpl() {
  float blueRate = 0;
  if (m_isHover && !m_isPressed)
    blueRate = 0.2f;
  else
    blueRate = 0.0f;

  Vec2F pos(screenPosition());
  if (m_hAnchor == HorizontalAnchor::HMidAnchor)
    pos += Vec2F(size()[0] / 2.0f, 0);
  else if (m_hAnchor == HorizontalAnchor::RightAnchor)
    pos += Vec2F(size()[0], 0);

  context()->setFont(m_font);
  if ((m_maxWidth != -1) && m_overfillMode) {
    context()->setFontSize(m_fontSize);
    int shift = std::max(0, getCursorOffset() - m_maxWidth);
    pos += Vec2F(-shift, 0);
  }

  context()->setFontSize(m_fontSize);
  context()->setFontProcessingDirectives(m_processingDirectives);
  if (m_text.empty()) {
    context()->setFontColor(m_color.mix(Color::rgbf(0.3f, 0.3f, 0.3f), 0.8f).mix(Color::rgbf(0.0f, 0.0f, 1.0f), blueRate).toRgba());
    context()->renderInterfaceText(m_hint, {pos, m_hAnchor, m_vAnchor});
  } else {
    context()->setFontColor(m_color.mix(Color::rgbf(0, 0, 1), blueRate).toRgba());
    if (m_textHidden) {
      String hiddenText('*', m_text.length());
      context()->renderInterfaceText(hiddenText, { pos, m_hAnchor, m_vAnchor });
    }
    else
      context()->renderInterfaceText(m_text, { pos, m_hAnchor, m_vAnchor });
  }
  context()->setDefaultFont();
  context()->setFontProcessingDirectives("");
  context()->setFontColor(Vec4B::filled(255));

  if (hasFocus()) {
    // render cursor
    float cc = 0.6f + 0.4f * std::sin((double)Time::monotonicMilliseconds() / 300.0);
    Color cursorColor = Color::rgbf(cc, cc, cc);

    context()->drawInterfaceLine(
        pos + Vec2F(getCursorOffset(), m_fontSize * m_cursorVert[0]),
        pos + Vec2F(getCursorOffset(), m_fontSize * m_cursorVert[1]),
        cursorColor.toRgba());
    context()->drawInterfaceLine(
        pos + Vec2F(getCursorOffset() + m_fontSize * m_cursorHoriz[0], m_fontSize * m_cursorVert[0]),
        pos + Vec2F(getCursorOffset() + m_fontSize * m_cursorHoriz[1], m_fontSize * m_cursorVert[0]),
        cursorColor.toRgba());
  }

  if (m_drawBorder)
    context()->drawInterfacePolyLines(PolyF(screenBoundRect()), Color(Color::White).toRgba());
}

int TextBoxWidget::getCursorOffset() { // horizontal only
  float scale;
  context()->setFont(m_font);
  context()->setFontSize(m_fontSize);
  if (m_hAnchor == HorizontalAnchor::LeftAnchor) {
    scale = 1.0;
  } else if (m_hAnchor == HorizontalAnchor::HMidAnchor) {
    scale = 0.5;
  } else if (m_hAnchor == HorizontalAnchor::RightAnchor) {
    scale = -1.0;
    if (m_textHidden) {
      int width = context()->stringInterfaceWidth("*");
      size_t chars = m_text.size();
      return (width * chars) * scale + (width * (chars - m_cursorOffset));
    } else {
      return context()->stringInterfaceWidth(m_text) * scale
           + context()->stringInterfaceWidth(m_text.substr(m_cursorOffset, m_text.size()));
    }
  } else {
    throw GuiException("Somehow, the value of m_hAnchor became bad");
  }

  if (m_textHidden) {
    int width = context()->stringInterfaceWidth("*");
    size_t chars = m_text.size();
    return (int)std::ceil((width * chars) * scale - (width * (chars - m_cursorOffset)));
  } else {
  return (int)std::ceil(context()->stringInterfaceWidth(m_text) * scale
                      - context()->stringInterfaceWidth(m_text.substr(m_cursorOffset, m_text.size())));
  }
}

void TextBoxWidget::update(float dt) {
  Widget::update(dt);
  if (m_repeatCode != SpecialRepeatKeyCodes::None) {
    if (Time::monotonicMilliseconds() >= m_repeatKeyThreshold) {
      m_repeatKeyThreshold += 50;
      if (m_repeatCode == SpecialRepeatKeyCodes::Delete) {
        if (m_cursorOffset < (int)m_text.size())
          modText(m_text.substr(0, m_cursorOffset) + m_text.substr(m_cursorOffset + 1));
      } else if (m_repeatCode == SpecialRepeatKeyCodes::Backspace) {
        if (m_cursorOffset > 0) {
          if (modText(m_text.substr(0, m_cursorOffset - 1) + m_text.substr(m_cursorOffset)))
            m_cursorOffset -= 1;
        }
      } else if (m_repeatCode == SpecialRepeatKeyCodes::Left) {
        if (m_cursorOffset > 0) {
          m_cursorOffset--;
          if (m_cursorOffset < 0)
            m_cursorOffset = 0;
        }
      } else if (m_repeatCode == SpecialRepeatKeyCodes::Right) {
        if (m_cursorOffset < (int)m_text.size()) {
          m_cursorOffset++;
          if (m_cursorOffset > (int)m_text.size())
            m_cursorOffset = m_text.size();
        }
      }
    }
  }
}

String const& TextBoxWidget::getText() const {
  return m_text;
}

bool TextBoxWidget::setText(String const& text, bool callback) {
  if (m_text == text)
    return true;

  if (!newTextValid(text))
    return false;

  m_text = text;
  m_cursorOffset = m_text.size();
  m_repeatCode = SpecialRepeatKeyCodes::None;
  if (callback)
    m_callback(this);
  return true;
}

bool TextBoxWidget::getHidden() const {
  return m_textHidden;
}

void TextBoxWidget::setHidden(bool hidden) {
  m_textHidden = hidden;
}

String TextBoxWidget::getRegex() {
  return m_regex;
}

void TextBoxWidget::setRegex(String const& regex) {
  m_regex = regex;
}

void TextBoxWidget::setColor(Color const& color) {
  m_color = color;
}

void TextBoxWidget::setDirectives(String const& directives) {
  m_processingDirectives = directives;
}

void TextBoxWidget::setFontSize(int fontSize) {
  m_fontSize = fontSize;
}

void TextBoxWidget::setMaxWidth(int maxWidth) {
  m_maxWidth = maxWidth;
  setSize({m_maxWidth + 6, m_fontSize + 2});
}

void TextBoxWidget::setOverfillMode(bool overtype) {
  m_overfillMode = overtype;
}

void TextBoxWidget::setOnBlurCallback(WidgetCallbackFunc onBlur) {
  m_onBlur = onBlur;
}

void TextBoxWidget::setOnEnterKeyCallback(WidgetCallbackFunc onEnterKey) {
  m_onEnterKey = onEnterKey;
}

void TextBoxWidget::setOnEscapeKeyCallback(WidgetCallbackFunc onEscapeKey) {
  m_onEscapeKey = onEscapeKey;
}

void TextBoxWidget::setNextFocus(Maybe<String> nextFocus) {
  m_nextFocus = nextFocus;
}

void TextBoxWidget::setPrevFocus(Maybe<String> prevFocus) {
  m_prevFocus = prevFocus;
}

void TextBoxWidget::mouseOver() {
  Widget::mouseOver();
  m_isHover = true;
}

void TextBoxWidget::mouseOut() {
  Widget::mouseOut();
  m_isHover = false;
  m_isPressed = false;
}

void TextBoxWidget::mouseReturnStillDown() {
  Widget::mouseReturnStillDown();
  m_isHover = true;
  m_isPressed = true;
}

void TextBoxWidget::blur() {
  Widget::blur();
  if (m_onBlur)
    m_onBlur(this);
  m_repeatCode = SpecialRepeatKeyCodes::None;
}

KeyboardCaptureMode TextBoxWidget::keyboardCaptured() const {
  if (active() && hasFocus())
    return KeyboardCaptureMode::TextInput;
  return KeyboardCaptureMode::None;
}

bool TextBoxWidget::sendEvent(InputEvent const& event) {
  if (!hasFocus())
    return false;

  if (event.is<KeyUpEvent>()) {
    m_repeatCode = SpecialRepeatKeyCodes::None;
    m_callback(this);
    return false;
  }

  if (innerSendEvent(event)) {
    m_callback(this);
    return true;
  }

  return false;
}

bool TextBoxWidget::innerSendEvent(InputEvent const& event) {
  if (auto keyDown = event.ptr<KeyDownEvent>()) {
    m_repeatKeyThreshold = Time::monotonicMilliseconds() + 300LL;

    if (keyDown->key == Key::Escape) {
      if (m_onEscapeKey) {
        m_onEscapeKey(this);
        return true;
      }
      return false;
    }
    if (keyDown->key == Key::Return || keyDown->key == Key::Kp_enter) {
      if (m_onEnterKey) {
        m_onEnterKey(this);
        return true;
      }
      return false;
    }
    if (keyDown->key == Key::Tab) {
      if ((KeyMod::LShift & keyDown->mods) == KeyMod::LShift || (KeyMod::RShift & keyDown->mods) == KeyMod::RShift) {
        if (m_prevFocus) {
          if (auto prevWidget = parent()->fetchChild(*m_prevFocus)) {
            prevWidget->focus();
            return true;
          }
        }
      } else {
        if (m_nextFocus) {
          if (auto nextWidget = parent()->fetchChild(*m_nextFocus)) {
            nextWidget->focus();
            return true;
          }
        }
      }
      return false;
    }
    if ((keyDown->mods & (KeyMod::LCtrl | KeyMod::RCtrl)) != KeyMod::NoMod) {
      if (keyDown->key == Key::C) {
        context()->setClipboard(m_text);
        return true;
      }
      if (keyDown->key == Key::X) {
        context()->setClipboard(m_text);
        if (modText(""))
          m_cursorOffset = 0;
        return true;
      }
      if (keyDown->key == Key::V) {
        String clipboard = context()->getClipboard();
        if (modText(m_text.substr(0, m_cursorOffset) + clipboard + m_text.substr(m_cursorOffset)))
          m_cursorOffset += clipboard.size();
        return true;
      }
    }

    auto calculateSteps = [&](bool dir) {
      int steps = 1;
      if ((keyDown->mods & (KeyMod::LCtrl | KeyMod::RCtrl)) != KeyMod::NoMod || (keyDown->mods & (KeyMod::LAlt | KeyMod::RAlt)) != KeyMod::NoMod) {
        if (dir) // right
          steps = m_text.findNextBoundary(m_cursorOffset) - m_cursorOffset;
        else // left
          steps = m_cursorOffset - m_text.findNextBoundary(m_cursorOffset, true);

        return steps < 1 ? 1 : steps;
      }
      return steps;
    };

    if (keyDown->key == Key::Backspace) {
      int steps = calculateSteps(false);
      m_repeatCode = SpecialRepeatKeyCodes::Backspace;
      for (int i = 0; i < steps; i++) {
        if (m_cursorOffset > 0) {
          if (modText(m_text.substr(0, m_cursorOffset - 1) + m_text.substr(m_cursorOffset)))
            m_cursorOffset -= 1;
        }
      }
      return true;
    }
    if (keyDown->key == Key::Delete) {
      int steps = calculateSteps(true);
      m_repeatCode = SpecialRepeatKeyCodes::Delete;
      for (int i = 0; i < steps; i++) {
        if (m_cursorOffset < (int)m_text.size())
          modText(m_text.substr(0, m_cursorOffset) + m_text.substr(m_cursorOffset + 1));
      }
      return true;
    }
    if (keyDown->key == Key::Left) {
      int steps = calculateSteps(false);
      m_repeatCode = SpecialRepeatKeyCodes::Left;
      for (int i = 0; i < steps; i++) {
        m_cursorOffset--;
        if (m_cursorOffset < 0)
          m_cursorOffset = 0;
      }
      return true;
    }
    if (keyDown->key == Key::Right) {
      int steps = calculateSteps(true);
      m_repeatCode = SpecialRepeatKeyCodes::Right;
      for (int i = 0; i < steps; i++) {
        m_cursorOffset++;
        if (m_cursorOffset > (int)m_text.size())
          m_cursorOffset = m_text.size();
      }
      return true;
    }
    if (keyDown->key == Key::Home) {
      m_cursorOffset = 0;
      return true;
    }
    if (keyDown->key == Key::End) {
      m_cursorOffset = m_text.size();
      return true;
    }
  }

  if (auto textInput = event.ptr<TextInputEvent>()) {
    if (modText(m_text.substr(0, m_cursorOffset) + textInput->text + m_text.substr(m_cursorOffset)))
      m_cursorOffset += textInput->text.size();
    return true;
  }

  return false;
}

void TextBoxWidget::setDrawBorder(bool drawBorder) {
  m_drawBorder = drawBorder;
}

void TextBoxWidget::setTextAlign(HorizontalAnchor hAnchor) {
  m_hAnchor = hAnchor;
}

bool TextBoxWidget::modText(String const& text) {
  if (m_text != text && newTextValid(text)) {
    m_text = text;
    return true;
  } else {
    return false;
  }
}

bool TextBoxWidget::newTextValid(String const& text) const {
  if (!text.regexMatch(m_regex))
    return false;
  if ((m_maxWidth != -1) && !m_overfillMode) {
    context()->setFont(m_font);
    context()->setFontSize(m_fontSize);
    return context()->stringInterfaceWidth(text) <= m_maxWidth;
  }
  return true;
}

}
