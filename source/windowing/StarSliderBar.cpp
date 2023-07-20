#include "StarSliderBar.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarImageMetadataDatabase.hpp"

namespace Star {

SliderBarWidget::SliderBarWidget(String const& grid, bool showSpinner)
  : m_grid(make_shared<ImageWidget>(grid)),
    m_low(0),
    m_high(1),
    m_delta(1),
    m_val(0),
    m_updateJog(true),
    m_jogDragActive(false),
    m_enabled(true) {

  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  m_jog = make_shared<ButtonWidget>();
  m_jog->setImages(assets->json("/interface.config:slider.jog").toString());
  m_jog->setPressedOffset({0, 0});

  if (showSpinner) {
    GuiReader guiReader;
    guiReader.registerCallback("spinner.down", bind(&SliderBarWidget::leftCallback, this));
    guiReader.registerCallback("spinner.up", bind(&SliderBarWidget::rightCallback, this));

    float rightButtonOffset = imgMetadata->imageSize(assets->json("/interface.config:slider.leftBase").toString())[0];
    rightButtonOffset += assets->json("/interface.config:slider.defaultPadding").toInt();

    float gridOffset = rightButtonOffset;

    rightButtonOffset += imgMetadata->imageSize(grid)[0];
    rightButtonOffset += assets->json("/interface.config:slider.defaultPadding").toInt();

    // TODO: Make spinners a thing already
    Json config = Json::parse(strf(
        R"SPINNER(
          {{
            "spinner" : {{
              "type" : "spinner",
              "leftBase" : "{}",
              "leftHover" : "{}",
              "rightBase" : "{}",
              "rightHover" : "{}",
              "position" : [0, 0],
              "upOffset" : {}
            }}
          }}
        )SPINNER",
        assets->json("/interface.config:slider.leftBase").toString(),
        assets->json("/interface.config:slider.leftHover").toString(),
        assets->json("/interface.config:slider.rightBase").toString(),
        assets->json("/interface.config:slider.rightHover").toString(),
        rightButtonOffset));

    guiReader.construct(config, this);

    m_grid->setPosition(Vec2I(gridOffset, 0));

    m_leftButton = fetchChild<ButtonWidget>("spinner.down");
    m_rightButton = fetchChild<ButtonWidget>("spinner.up");
  }

  addChild("grid", m_grid);
  addChild("jog", m_jog);

  markAsContainer();
  disableScissoring();
}

void SliderBarWidget::setJogImages(String const& baseImage, String const& hoverImage, String const& pressedImage, String const& disabledImage) {
  m_jog->setImages(baseImage, hoverImage, pressedImage, disabledImage);
}

void SliderBarWidget::setRange(int low, int high, int delta) {
  m_low = low;
  m_high = high;
  m_high = std::max(m_low + 1, m_high);
  if (delta <= 0)
    m_delta = 1;
  else
    m_delta = delta;
  setVal(m_val);
}

void SliderBarWidget::setRange(Vec2I const& range, int delta) {
  setRange(range[0], range[1], delta);
}

void SliderBarWidget::setVal(int val, bool callbackIfChanged) {
  bool doCallback = false;
  if (m_callback && callbackIfChanged && m_val != val)
    doCallback = true;

  if (m_val != val)
    m_updateJog = true;

  m_val = val;
  m_val = clamp(m_val, m_low, m_high);

  if (doCallback)
    m_callback(this);
}

int SliderBarWidget::val() const {
  return m_val;
}

void SliderBarWidget::setEnabled(bool enabled) {
  if (m_enabled != enabled) {
    m_enabled = enabled;
    m_jogDragActive = false;
    m_jog->setEnabled(enabled);
    if (m_rightButton)
      m_rightButton->setEnabled(enabled);
    if (m_leftButton)
      m_leftButton->setEnabled(enabled);
  }
}

void SliderBarWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

void SliderBarWidget::update(float dt) {
  float gridLow = m_grid->position()[0];
  float gridHigh = gridLow + m_grid->size()[0];

  if (m_jogDragActive) {
    auto clampedPos = clamp<float>(m_jogDragPos[0], gridLow, gridHigh);
    float clampedPercent = (clampedPos - gridLow) / (gridHigh - gridLow);
    setVal(clampedPercent * (m_high - m_low));
  }

  if (m_updateJog) {
    float percent = (float)m_val / (m_high - m_low);

    float jogPos = (gridHigh - gridLow) * percent + gridLow - m_jog->size()[0] * 0.5;

    m_jog->setPosition(Vec2I(jogPos, 0));
    m_updateJog = false;
  }

  Widget::update(dt);
}

bool SliderBarWidget::sendEvent(InputEvent const& event) {
  if (event.is<MouseButtonUpEvent>()) {
    blur();
    m_jogDragActive = false;
  }

  if (m_enabled) {
    if (event.is<MouseButtonDownEvent>()) {
      if (m_jog->inMember(*context()->mousePosition(event))) {
        focus();
        m_jogDragPos = *context()->mousePosition(event) - screenPosition();
        m_jogDragActive = true;
      }
    }

    if (event.is<MouseMoveEvent>()) {
      if (m_jogDragActive)
        m_jogDragPos = *context()->mousePosition(event) - screenPosition();
    }
  }

  return Widget::sendEvent(event);
}

void SliderBarWidget::leftCallback() {
  setVal(std::max(m_low, m_val - m_delta));
}

void SliderBarWidget::rightCallback() {
  setVal(std::min(m_high, m_val + m_delta));
}

}
