#include "StarButtonWidget.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"
#include "StarInput.hpp"

namespace Star {

ButtonWidget::ButtonWidget() {
  m_hovered = false;
  m_pressed = false;
  m_checkable = false;
  m_checked = false;
  m_disabled = false;
  m_highlighted = false;
  m_hasCheckedImages = false;
  m_sustain = false;
  m_invisible = false;
  m_hTextAnchor = HorizontalAnchor::HMidAnchor;
  m_fontColor = Color::White;
  m_fontColorDisabled = Color::Gray;

  auto assets = Root::singleton().assets();

  auto interfaceConfig = assets->json("/interface.config");
  m_pressedOffset = jsonToVec2I(interfaceConfig.get("buttonPressedOffset"));
  m_textStyle = interfaceConfig.get("buttonTextStyle");
  m_clickSounds = jsonToStringList(interfaceConfig.get("buttonClickSound"));
  m_releaseSounds = jsonToStringList(interfaceConfig.get("buttonReleaseSound"));
  m_hoverSounds = jsonToStringList(interfaceConfig.get("buttonHoverSound"));
  m_hoverOffSounds = jsonToStringList(interfaceConfig.get("buttonHoverOffSound"));
}

ButtonWidget::ButtonWidget(WidgetCallbackFunc callback,
    String const& baseImage,
    String const& hoverImage,
    String const& pressedImage,
    String const& disabledImage)
  : ButtonWidget() {
  setCallback(callback);
  setImages(baseImage, hoverImage, pressedImage, disabledImage);
}

ButtonWidget::~ButtonWidget() {
  if (m_buttonGroup)
    m_buttonGroup->removeButton(this);
}

void ButtonWidget::renderImpl() {
  if (isPressed() && sustainCallbackOnDownHold()) {
    if (m_callback) {
      auto unlocker = Input::singleton().unlockClipboard();
      m_callback(this);
    }
  }

  Vec2F position = Vec2F(screenPosition());
  Vec2F textPosition = position + Vec2F(m_textOffset);
  if (m_hTextAnchor == HorizontalAnchor::HMidAnchor)
    textPosition += Vec2F(size()) / 2;
  else if (m_hTextAnchor == HorizontalAnchor::RightAnchor)
    textPosition += Vec2F(size()[0], size()[1] / 2);
  else if (m_hTextAnchor == HorizontalAnchor::LeftAnchor)
    textPosition += Vec2F(0, size()[1] / 2);
  // We need to show the down button offset if we're pressing the button or
  // don't have checked images and thus need some way to show that the button
  // is checked (there's probably some better default behavior in that case)
  if (m_pressed || (m_checked && !m_hasCheckedImages)) {
    position += Vec2F(m_pressedOffset);
    textPosition += Vec2F(m_pressedOffset);
  }

  if (m_hasCheckedImages && m_checked) {
    if (m_disabled)
      drawButtonPart(m_disabledImageChecked, position);
    else if ((m_pressed || m_highlighted) && !m_pressedImageChecked.empty())
      drawButtonPart(m_pressedImageChecked, position);
    else if ((m_pressed || m_hovered || m_highlighted) && !m_hoverImageChecked.empty())
      drawButtonPart(m_hoverImageChecked, position);
    else
      drawButtonPart(m_baseImageChecked, position);
  } else {
    if (m_disabled)
      drawButtonPart(m_disabledImage, position);
    else if ((m_pressed || m_highlighted) && !m_pressedImage.empty())
      drawButtonPart(m_pressedImage, position);
    else if ((m_pressed || m_hovered || m_highlighted) && !m_hoverImage.empty())
      drawButtonPart(m_hoverImage, position);
    else if (!m_invisible)
      drawButtonPart(m_baseImage, position);
  }

  if (!m_overlayImage.empty())
    drawButtonPart(m_overlayImage, position);

  if (!m_text.empty()) {
    auto& guiContext = GuiContext::singleton();
    guiContext.setTextStyle(m_textStyle);
    if (m_disabled)
      guiContext.setFontColor(m_fontColorDisabled.toRgba());
    else if (m_fontColorChecked && m_checked)
      guiContext.setFontColor(m_fontColorChecked.value().toRgba());
    else
      guiContext.setFontColor(m_fontColor.toRgba());
    guiContext.renderInterfaceText(m_text, {textPosition, m_hTextAnchor, VerticalAnchor::VMidAnchor});
    guiContext.clearTextStyle();
  }
}

bool ButtonWidget::sendEvent(InputEvent const& event) {
  if (m_visible && !m_disabled) {
    if (event.is<MouseButtonDownEvent>() && event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Left) {
      if (inMember(*context()->mousePosition(event))) {
        if (!isPressed()) {
          auto assets = Root::singleton().assets();
          auto sound = Random::randValueFrom(m_clickSounds, "");
          if (!sound.empty())
            context()->playAudio(sound);
        }
        setPressed(true);
        if (m_callback) {
          focus();
          return true;
        }
      } else {
        blur();
        return false;
      }
    } else if (event.is<MouseButtonUpEvent>()) {
      if (isPressed()) {
        auto assets = Root::singleton().assets();
        auto sound = Random::randValueFrom(m_releaseSounds, "");
        if (!sound.empty())
          context()->playAudio(sound);
      }
      setPressed(false);
      return false;
    }
  }

  return false;
}

void ButtonWidget::mouseOver() {
  Widget::mouseOver();
  if (!m_disabled) {
    if (!m_hovered) {
      auto assets = Root::singleton().assets();
      auto sound = Random::randValueFrom(m_hoverSounds);
      if (!sound.empty())
        context()->playAudio(sound);
    }
    m_hovered = true;
  }
}

void ButtonWidget::mouseOut() {
  Widget::mouseOut();
  if (!m_disabled && m_hovered) {
    auto assets = Root::singleton().assets();
    auto sound = Random::randValueFrom(m_hoverOffSounds);
    if (!sound.empty())
      context()->playAudio(sound);
  }
  m_hovered = false;
  m_pressed = false;
}

void ButtonWidget::mouseReturnStillDown() {
  Widget::mouseReturnStillDown();
  if (!isPressed()) {
    auto assets = Root::singleton().assets();
    auto sound = Random::randValueFrom(m_clickSounds, "");
    if (!sound.empty())
      context()->playAudio(sound);
  }
  m_hovered = true;
  m_pressed = true;
}

void ButtonWidget::hide() {
  Widget::hide();
  m_pressed = false;
  m_hovered = false;
}

void ButtonWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

ButtonGroupPtr ButtonWidget::buttonGroup() const {
  return m_buttonGroup;
}

void ButtonWidget::setButtonGroup(ButtonGroupPtr newGroup, int id) {
  if (m_buttonGroup != newGroup) {
    if (m_buttonGroup)
      m_buttonGroup->removeButton(this);

    m_buttonGroup = std::move(newGroup);

    if (m_buttonGroup) {
      setCheckable(true);
      m_buttonGroup->addButton(this, id);
    }
  }
}

int ButtonWidget::buttonGroupId() {
  if (m_buttonGroup)
    return m_buttonGroup->id(this);
  else
    return ButtonGroup::NoButton;
}

bool ButtonWidget::isHovered() const {
  return m_hovered;
}

bool ButtonWidget::isPressed() const {
  return m_pressed;
}

void ButtonWidget::setPressed(bool pressed) {
  if (m_pressed != pressed) {
    // Button action is triggered when the button is released after being pressed
    if (m_pressed) {
      check();
      if (m_callback) {
        auto unlocker = Input::singleton().unlockClipboard();
        m_callback(this);
      }
    }
    m_pressed = pressed;
  }
}

bool ButtonWidget::isCheckable() const {
  return m_checkable;
}

void ButtonWidget::setCheckable(bool checkable) {
  m_checkable = checkable;
}

bool ButtonWidget::isHighlighted() const {
  return m_highlighted;
}

void ButtonWidget::setHighlighted(bool highlighted) {
  m_highlighted = highlighted;
}

bool ButtonWidget::isChecked() const {
  return m_checked;
}

void ButtonWidget::setChecked(bool checked) {
  // might cause button groups to have multiple selected against its rules, be careful with direct poking, use check()
  // instead.
  m_checked = checked;
}

void ButtonWidget::check() {
  if (m_checkable) {
    // If we are part of an exclusive button group, then don't uncheck if
    // we are already checked and pressed again.
    if (m_buttonGroup) {
      if (m_buttonGroup->toggle() || !isChecked()) {
        setChecked(!m_buttonGroup->toggle() || !isChecked());
        m_buttonGroup->wasChecked(this);
      }
    } else {
      setChecked(!isChecked());
    }
  }
}

bool ButtonWidget::sustainCallbackOnDownHold() {
  return m_sustain;
}

void ButtonWidget::setSustainCallbackOnDownHold(bool sustain) {
  m_sustain = sustain;
}

void ButtonWidget::setImages(String const& baseImage, String const& hoverImage, String const& pressedImage, String const& disabledImage) {
  m_baseImage = baseImage;
  m_hoverImage = hoverImage;
  m_pressedImage = pressedImage;
  m_disabledImage = disabledImage;
  if (m_disabledImage.empty() && !m_baseImage.empty())
    m_disabledImage = m_baseImage + Root::singleton().assets()->json("/interface.config:disabledButton").toString();
  updateSize();
}

void ButtonWidget::setCheckedImages(String const& baseImage, String const& hoverImage, String const& pressedImage, String const& disabledImage) {
  m_hasCheckedImages = !baseImage.empty();
  m_baseImageChecked = baseImage;
  m_hoverImageChecked = hoverImage;
  m_pressedImageChecked = pressedImage;
  m_disabledImageChecked = disabledImage;
  if (m_hasCheckedImages && m_disabledImageChecked.empty())
    m_disabledImageChecked = m_baseImageChecked + Root::singleton().assets()->json("/interface.config:disabledButton").toString();
  updateSize();
}

void ButtonWidget::setOverlayImage(String const& overlayImage) {
  m_overlayImage = overlayImage;
}

Vec2I const& ButtonWidget::pressedOffset() const {
  return m_pressedOffset;
}

void ButtonWidget::setPressedOffset(Vec2I const& offset) {
  m_pressedOffset = offset;
}

String const& ButtonWidget::getText() const {
  return m_text;
}

void ButtonWidget::setText(String const& text) {
  m_text = text;
}

void ButtonWidget::setFontSize(int size) {
  m_textStyle.fontSize = size;
}

void ButtonWidget::setFontDirectives(String directives) {
  m_textStyle.directives = directives;
}

void ButtonWidget::setTextOffset(Vec2I textOffset) {
  m_textOffset = textOffset;
}

void ButtonWidget::setTextAlign(HorizontalAnchor hAnchor) {
  m_hTextAnchor = hAnchor;
}

void ButtonWidget::setFontColor(Color color) {
  m_textStyle.color = (m_fontColor = color).toRgba();
}

void ButtonWidget::setFontColorDisabled(Color color) {
  m_fontColorDisabled = color;
}

void ButtonWidget::setFontColorChecked(Color color) {
  m_fontColorChecked = color;
}

// Although ButtonWidget wraps other widgets from time to time.  These should never be "accessible"
WidgetPtr ButtonWidget::getChildAt(Vec2I const&) {
  return {};
}

void ButtonWidget::disable() {
  m_disabled = true;
  m_pressed = false;
}

void ButtonWidget::enable() {
  m_disabled = false;
}

void ButtonWidget::setEnabled(bool enabled) {
  m_disabled = !enabled;
  m_pressed &= enabled; // and off the pressed flag if the button is no longer enabled.
}

void ButtonWidget::setInvisible(bool invisible) {
  m_invisible = invisible;
}

RectI ButtonWidget::getScissorRect() const {
  if (m_pressed || (m_checked && !m_hasCheckedImages)) {
    return RectI::withSize(screenPosition() + m_pressedOffset, size());
  }
  return RectI::withSize(screenPosition(), size());
}

void ButtonWidget::drawButtonPart(String const& image, Vec2F const& position) {
  auto& guiContext = GuiContext::singleton();
  auto imageSize = guiContext.textureSize(image);
  guiContext.drawInterfaceQuad(image, position + Vec2F(m_buttonBoundSize - imageSize) / 2);
}

void ButtonWidget::updateSize() {
  if (m_invisible || m_baseImage.empty())
    return;
  auto& guiContext = GuiContext::singleton();
  m_buttonBoundSize = guiContext.textureSize(m_baseImage);
  if (!m_hoverImage.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_hoverImage));
  if (!m_pressedImage.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_pressedImage));
  if (!m_baseImageChecked.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_baseImageChecked));
  if (!m_hoverImageChecked.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_hoverImageChecked));
  if (!m_pressedImageChecked.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_pressedImageChecked));
  if (!m_disabledImageChecked.empty())
    m_buttonBoundSize = m_buttonBoundSize.piecewiseMax(guiContext.textureSize(m_disabledImageChecked));

  setSize(Vec2I(m_buttonBoundSize));
}

}
