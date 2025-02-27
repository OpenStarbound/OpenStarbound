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
  m_config = assets->json("/interface.config:largeCharPlate");
  auto charPlateImage = m_config.getString("backingImage");

  setCallback(mainCallback);
  setImages(charPlateImage);

  m_playerPlate = charPlateImage;
  m_playerPlateHover = m_config.getString("playerHover");
  m_noPlayerPlate = m_config.getString("noPlayer");
  m_noPlayerPlateHover = m_config.getString("noPlayerHover");
  m_portraitOffset = jsonToVec2I(m_config.get("portraitOffset"));
  m_portraitScale = m_config.getFloat("portraitScale");

  String switchText = m_config.getString("switchText");
  String createText = m_config.getString("createText");

  m_portrait = make_shared<PortraitWidget>();
  m_portrait->setScale(m_portraitScale);
  m_portrait->setPosition(m_portraitOffset);
  m_portrait->setRenderHumanoid(true);
  addChild("portrait", m_portrait);

  String modeLabelText = m_config.getString("modeText");
  m_regularTextColor = Color::rgb(jsonToVec3B(m_config.get("textColor")));
  m_disabledTextColor = Color::rgb(jsonToVec3B(m_config.get("textColorDisabled")));

  m_modeNameOffset = jsonToVec2I(m_config.get("modeNameOffset"));
  m_modeOffset = jsonToVec2I(m_config.get("modeOffset"));

  auto modeNameHAnchor = HorizontalAnchorNames.getLeft(m_config.getString("modeNameHAnchor", "mid"));
  auto modeNameVAnchor = VerticalAnchorNames  .getLeft(m_config.getString("modeNameVAnchor", "bottom"));
  m_modeName = make_shared<LabelWidget>(modeLabelText, Color::White, modeNameHAnchor);
  addChild("modeName", m_modeName);
  m_modeName->setPosition(m_modeNameOffset);
  m_modeName->setAnchor(modeNameHAnchor, modeNameVAnchor);

  auto modeHAnchor = HorizontalAnchorNames.getLeft(m_config.getString("modeHAnchor", "left"));
  auto modeVAnchor = VerticalAnchorNames  .getLeft(m_config.getString("modeVAnchor", "bottom"));
  m_mode = make_shared<LabelWidget>();
  addChild("mode", m_mode);
  m_mode->setPosition(m_modeOffset);
  m_mode->setAnchor(modeHAnchor, modeVAnchor);

  m_createCharText = m_config.getString("noPlayerText");
  m_createCharTextColor = Color::rgb(jsonToVec3B(m_config.get("noPlayerTextColor")));
  m_playerNameOffset = jsonToVec2I(m_config.get("playerNameOffset"));

  auto playerNameHAnchor = HorizontalAnchorNames.getLeft(m_config.getString("playerNameHAnchor", "mid"));
  auto playerNameVAnchor = VerticalAnchorNames  .getLeft(m_config.getString("playerNameVAnchor", "bottom"));
  m_playerName = make_shared<LabelWidget>();
  m_playerName->setColor(m_createCharTextColor);
  m_playerName->setPosition(m_playerNameOffset);
  m_playerName->setAnchor(playerNameHAnchor, playerNameVAnchor);
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

  auto trashButton = m_config.get("trashButton");
  auto baseImage = trashButton.getString("baseImage");
  auto hoverImage = trashButton.getString("hoverImage");
  auto pressedImage = trashButton.getString("pressedImage");
  auto disabledImage = trashButton.getString("disabledImage");
  auto offset = jsonToVec2I(trashButton.get("offset"));

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

void LargeCharPlateWidget::update(float dt) {
  ButtonWidget::update(dt);

  if (!m_player || !m_config.getBool("animatePortrait", true))
    return;

  auto humanoid = m_player->humanoid();
  if (m_delete && m_delete->isHovered()) {
    humanoid->setEmoteState(HumanoidEmote::Sad);
    humanoid->setState(Humanoid::Run);
  } else {
    humanoid->setEmoteState(HumanoidEmote::Idle);
    humanoid->setState(isHovered() ? Humanoid::Walk : Humanoid::Idle);
  }
  humanoid->animate(dt);
}

}
