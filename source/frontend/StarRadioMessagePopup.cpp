#include "StarRadioMessagePopup.hpp"
#include "StarGuiReader.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarLogging.hpp"
#include "StarJsonExtra.hpp"
#include "StarInterpolation.hpp"
#include "StarLexicalCast.hpp"
#include "StarMixer.hpp"
#include "StarTextPainter.hpp"

namespace Star {

RadioMessagePopup::RadioMessagePopup() {
  auto assets = Root::singleton().assets();
  auto config = assets->json("/interface/radiomessage/radiomessage.config");

  GuiReader reader;
  reader.construct(config.get("paneLayout"), this);

  m_messageLabel = fetchChild<LabelWidget>("lblMessage");
  m_portraitImage = fetchChild<ImageWidget>("imgPortrait");

  m_backgroundImage = config.getString("backgroundImage");

  m_animateInTime = config.getFloat("animateInTime");
  m_animateInImage = config.getString("animateInImage");
  m_animateInFrames = config.getInt("animateInFrames");

  m_animateOutTime = config.getFloat("animateOutTime");
  m_animateOutImage = config.getString("animateOutImage");
  m_animateOutFrames = config.getInt("animateOutFrames");

  m_chatOffset = jsonToVec2I(config.get("chatOffset"));
  m_chatStartPosition = m_chatOffset;
  m_chatEndPosition = m_chatOffset;

  m_slideTime = config.getFloat("slideTime");
  m_slideTimer = m_slideTime;
  updateAnchorOffset();

  enterStage(PopupStage::Hidden);
}

void RadioMessagePopup::update() {
  if (messageActive()) {
    if (m_stageTimer.tick())
      nextPopupStage();

    if (m_popupStage == PopupStage::AnimateIn) {
      int frame = floor((1.0f - m_stageTimer.percent()) * m_animateInFrames);
      setBG("", strf("{}:{}", m_animateInImage, frame), "");
    } else if (m_popupStage == PopupStage::ScrollText) {
      int frame =
          int((m_stageTimer.timer / m_message.portraitSpeed) * m_message.portraitFrames) % m_message.portraitFrames;
      m_portraitImage->setImage(m_message.portraitImage.replace("<frame>", toString(frame)));
      int textLength = floor(Text::stripEscapeCodes(m_message.text).length() * (1.0f - m_stageTimer.percent()));
      m_messageLabel->setTextCharLimit(textLength);
    } else if (m_popupStage == PopupStage::Persist) {
      // you're cool, just stay cool, cool person
    } else if (m_popupStage == PopupStage::AnimateOut) {
      int frame = floor((1.0f - m_stageTimer.percent()) * m_animateOutFrames);
      setBG("", strf("{}:{}", m_animateOutImage, frame), "");
    }

    m_slideTimer = min(m_slideTimer + WorldTimestep, m_slideTime);
    updateAnchorOffset();
  }

  Pane::update();
}

void RadioMessagePopup::dismissed() {
  if (m_chatterSound)
    m_chatterSound->stop();
  Pane::dismissed();
}

bool RadioMessagePopup::messageActive() {
  return m_popupStage != PopupStage::Hidden;
}

void RadioMessagePopup::setMessage(RadioMessage message) {
  m_message = message;

  if (!message.chatterSound.empty() && message.textSpeed > 0) {
    if (m_chatterSound)
      m_chatterSound->stop();
    auto assets = Root::singleton().assets();
    m_chatterSound = make_shared<AudioInstance>(*assets->audio(message.chatterSound));
    m_chatterSound->setLoops(-1);
  }

  enterStage(PopupStage::AnimateIn);

  updateAnchorOffset();
}

void RadioMessagePopup::setChatHeight(int chatHeight) {
  auto endPosition = m_chatOffset + Vec2I(0, chatHeight);
  if (endPosition != m_chatEndPosition) {
    m_chatStartPosition = anchorOffset();
    m_chatEndPosition = endPosition;
    m_slideTimer = 0.0f;
  }
}

void RadioMessagePopup::interrupt() {
  if (m_popupStage != PopupStage::Hidden && m_popupStage != PopupStage::AnimateOut)
    enterStage(PopupStage::AnimateOut);
}

void RadioMessagePopup::updateAnchorOffset() {
  float slideRatio = m_slideTimer / m_slideTime;
  setAnchorOffset(Vec2I(int(m_chatStartPosition[0] * (1 - slideRatio) + m_chatEndPosition[0] * slideRatio),
      int(m_chatStartPosition[1] * (1 - slideRatio) + m_chatEndPosition[1] * slideRatio)));
}

void RadioMessagePopup::nextPopupStage() {
  if (m_popupStage != PopupStage::Hidden)
    enterStage((PopupStage)((int)m_popupStage + 1));
}

void RadioMessagePopup::enterStage(PopupStage newStage) {
  m_popupStage = newStage;
  if (m_popupStage == PopupStage::Hidden) {
    m_portraitImage->hide();
    m_messageLabel->hide();
    setBG("", strf("{}:0", m_animateInImage), "");
  } else if (m_popupStage == PopupStage::AnimateIn) {
    m_stageTimer = GameTimer(m_animateInTime);
    m_portraitImage->hide();
    m_messageLabel->hide();
  } else if (m_popupStage == PopupStage::ScrollText) {
    if (m_message.textSpeed == 0) {
      // skip this stage if text is instant
      enterStage(PopupStage::Persist);
    } else {
      m_stageTimer = GameTimer(Text::stripEscapeCodes(m_message.text).length() / m_message.textSpeed);
      m_portraitImage->show();
      m_messageLabel->show();
      m_messageLabel->setText(m_message.text);
      m_messageLabel->setTextCharLimit(0);
      setBG(m_backgroundImage);
      if (m_chatterSound)
        GuiContext::singleton().playAudio(m_chatterSound);
    }
  } else if (m_popupStage == PopupStage::Persist) {
    m_stageTimer = GameTimer(m_message.persistTime);
    m_portraitImage->show();
    m_portraitImage->setImage(m_message.portraitImage.replace("<frame>", "0"));
    m_messageLabel->show();
    m_messageLabel->setText(m_message.text);
    m_messageLabel->setTextCharLimit({});
    setBG(m_backgroundImage);
    if (m_chatterSound)
      m_chatterSound->stop();
  } else if (m_popupStage == PopupStage::AnimateOut) {
    m_stageTimer = GameTimer(m_animateOutTime);
    m_portraitImage->hide();
    m_messageLabel->hide();
  }
}

}
