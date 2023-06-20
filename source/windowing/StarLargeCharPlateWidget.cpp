#include "StarLargeCharPlateWidget.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"

namespace Star {

LargeCharPlateWidget::LargeCharPlateWidget(WidgetCallbackFunc mainCallback, PlayerPtr player) : m_player(player) {
  m_portraitScale = 0;

  setSize(ButtonWidget::size());

  auto assets = Root::singleton().assets();
  auto charPlateImage = assets->json("/interface.config:largeCharPlate.backingImage").toString();

  setCallback(mainCallback);
  setImages(charPlateImage);

  m_playerPlate = charPlateImage;
  m_playerPlateHover = assets->json("/interface.config:largeCharPlate.playerHover").toString();
  m_noPlayerPlate = assets->json("/interface.config:largeCharPlate.noPlayer").toString();
  m_noPlayerPlateHover = assets->json("/interface.config:largeCharPlate.noPlayerHover").toString();
  m_portraitOffset = jsonToVec2I(assets->json("/interface.config:largeCharPlate.portraitOffset"));
  m_portraitScale = assets->json("/interface.config:largeCharPlate.portraitScale").toFloat();

  String switchText = assets->json("/interface.config:largeCharPlate.switchText").toString();
  String createText = assets->json("/interface.config:largeCharPlate.createText").toString();

  m_portrait = make_shared<PortraitWidget>();
  m_portrait->setScale(m_portraitScale);
  m_portrait->setPosition(m_portraitOffset);
  addChild("portrait", m_portrait);

  String modeLabelText = assets->json("/interface.config:largeCharPlate.modeText").toString();
  m_regularTextColor = Color::rgb(jsonToVec3B(assets->json("/interface.config:largeCharPlate.textColor")));
  m_disabledTextColor = Color::rgb(jsonToVec3B(assets->json("/interface.config:largeCharPlate.textColorDisabled")));

  m_modeNameOffset = jsonToVec2I(assets->json("/interface.config:largeCharPlate.modeNameOffset"));
  m_modeOffset = jsonToVec2I(assets->json("/interface.config:largeCharPlate.modeOffset"));

  m_modeName = make_shared<LabelWidget>(modeLabelText, Color::White, HorizontalAnchor::HMidAnchor);
  addChild("modeName", m_modeName);
  m_modeName->setPosition(m_modeNameOffset);
  m_modeName->setAnchor(HorizontalAnchor::HMidAnchor, VerticalAnchor::BottomAnchor);

  m_mode = make_shared<LabelWidget>();
  addChild("mode", m_mode);
  m_mode->setPosition(m_modeOffset);
  m_mode->setAnchor(HorizontalAnchor::LeftAnchor, VerticalAnchor::BottomAnchor);

  m_createCharText = assets->json("/interface.config:largeCharPlate.noPlayerText").toString();
  m_createCharTextColor = Color::rgb(jsonToVec3B(assets->json("/interface.config:largeCharPlate.noPlayerTextColor")));
  m_playerNameOffset = jsonToVec2I(assets->json("/interface.config:largeCharPlate.playerNameOffset"));

  m_playerName = make_shared<LabelWidget>();
  m_playerName->setColor(m_createCharTextColor);
  m_playerName->setPosition(m_playerNameOffset);
  m_playerName->setAnchor(HorizontalAnchor::HMidAnchor, VerticalAnchor::BottomAnchor);
  addChild("player", m_playerName);
}

void LargeCharPlateWidget::renderImpl() {
  Vec2I pressedOffset = isPressed() ? ButtonWidget::pressedOffset() : Vec2I(0, 0);

  m_portrait->setPosition(m_portraitOffset + pressedOffset);
  m_mode->setPosition(m_modeOffset + pressedOffset);
  m_modeName->setPosition(m_modeNameOffset + pressedOffset);
  m_playerName->setPosition(m_playerNameOffset + pressedOffset);
  if (m_delete) {
    m_delete->setPosition(m_deleteOffset + pressedOffset);
  }

  if (m_player) {
    ButtonWidget::setImages(m_playerPlate, m_playerPlateHover);
    ButtonWidget::renderImpl();
    m_modeName->setColor(m_regularTextColor);
    m_playerName->setColor(m_regularTextColor);
    m_playerName->setText(m_player->name());
  } else {
    ButtonWidget::setImages(m_noPlayerPlate, m_noPlayerPlateHover);
    ButtonWidget::enable();
    ButtonWidget::renderImpl();
    m_modeName->setColor(m_disabledTextColor);
    m_playerName->setColor(m_createCharTextColor);
    m_playerName->setText(m_createCharText);
  }
}

void LargeCharPlateWidget::mouseOut() {
  if (m_delete)
    m_delete->mouseOut();

  ButtonWidget::mouseOut();
}

void LargeCharPlateWidget::setPlayer(PlayerPtr player) {
  m_player = player;
  m_portrait->setEntity(m_player);

  if (m_player) {
    m_playerName->setText(m_player->name());
  } else {
    m_playerName->setText(m_createCharText);
  }

  auto modeTypeTextAndColor = Root::singleton().assets()->json("/interface.config:modeTypeTextAndColor").toArray();
  int modeType;
  if (m_player) {
    modeType = 1 + (int)m_player->modeType();
  } else {
    modeType = 0;
  }
  auto thisModeType = modeTypeTextAndColor[modeType].toArray();
  String modeTypeText = thisModeType[0].toString();
  Color modeTypeColor = Color::rgb(jsonToVec3B(thisModeType[1]));
  m_mode->setText(modeTypeText);
  m_mode->setColor(modeTypeColor);
}

void LargeCharPlateWidget::enableDelete(WidgetCallbackFunc const& callback) {
  disableDelete();

  auto assets = Root::singleton().assets();

  auto baseImage = assets->json("/interface.config:largeCharPlate.trashButton.baseImage").toString();
  auto hoverImage = assets->json("/interface.config:largeCharPlate.trashButton.hoverImage").toString();
  auto pressedImage = assets->json("/interface.config:largeCharPlate.trashButton.pressedImage").toString();
  auto disabledImage = assets->json("/interface.config:largeCharPlate.trashButton.disabledImage").toString();
  auto offset = jsonToVec2I(assets->json("/interface.config:largeCharPlate.trashButton.offset"));

  m_delete = make_shared<ButtonWidget>(callback, baseImage, hoverImage, pressedImage, disabledImage);
  addChild("trashButton", m_delete);
  m_delete->setPosition(offset);
  m_deleteOffset = offset;
}

void LargeCharPlateWidget::disableDelete() {
  if (m_delete) {
    removeChild(m_delete.get());
  }

  m_delete = {};
}

bool LargeCharPlateWidget::sendEvent(InputEvent const& event) {
  if (event.is<MouseMoveEvent>() && m_delete) {
    if (m_delete->inMember(*m_context->mousePosition(event)))
      m_delete->mouseOver();
    else
      m_delete->mouseOut();
  }

  if (Widget::sendEvent(event))
    return true;

  return ButtonWidget::sendEvent(event);
}

}
