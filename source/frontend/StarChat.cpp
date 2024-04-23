#include "StarChat.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarButtonWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarCanvasWidget.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarPlayerStorage.hpp"
#include "StarTeamClient.hpp"

namespace Star {

Chat::Chat(UniverseClientPtr client) : m_client(client) {
  m_chatPrevIndex = 0;
  m_historyOffset = 0;

  auto assets = Root::singleton().assets();
  auto config = assets->json("/interface/chat/chat.config:config");
  m_timeChatLastActive = Time::monotonicMilliseconds();
  m_chatTextStyle = config.get("textStyle");
  m_chatTextStyle.lineSpacing = config.get("lineHeight").toFloat();
  m_chatVisTime = config.get("visTime").toFloat();
  m_fadeRate = config.get("fadeRate").toDouble();
  m_chatHistoryLimit = config.get("chatHistoryLimit").toInt();
  m_chatFormatString = config.getString("chatFormatString");

  m_portraitTextOffset = jsonToVec2I(config.get("portraitTextOffset"));
  m_portraitImageOffset = jsonToVec2I(config.get("portraitImageOffset"));
  m_portraitScale = config.get("portraitScale").toFloat();
  m_portraitVerticalMargin = config.get("portraitVerticalMargin").toFloat();
  m_portraitBackground = config.get("portraitBackground").toString();

  m_bodyHeight = config.get("bodyHeight").toInt();
  m_expandedBodyHeight = config.get("expandedBodyHeight").toInt();

  m_colorCodes[MessageContext::Local] = config.query("colors.local").toString();
  m_colorCodes[MessageContext::Party] = config.query("colors.party").toString();
  m_colorCodes[MessageContext::Broadcast] = config.query("colors.broadcast").toString();
  m_colorCodes[MessageContext::Whisper] = config.query("colors.whisper").toString();
  m_colorCodes[MessageContext::CommandResult] = config.query("colors.commandResult").toString();
  m_colorCodes[MessageContext::RadioMessage] = config.query("colors.radioMessage").toString();
  m_colorCodes[MessageContext::World] = config.query("colors.world").toString();

  GuiReader reader;

  reader.registerCallback("textBox", [=](Widget*) { startChat(); });
  reader.registerCallback("upButton", [=](Widget*) { scrollUp(); });
  reader.registerCallback("downButton", [=](Widget*) { scrollDown(); });
  reader.registerCallback("bottomButton", [=](Widget*) { scrollBottom(); });

  reader.registerCallback("filterGroup", [=](Widget* widget) {
      Json data = as<ButtonWidget>(widget)->data();
      auto filter = data.getArray("filter", {});
      m_modeFilter.clear();
      for (auto mode : filter)
        m_modeFilter.insert(MessageContextModeNames.getLeft(mode.toString()));
      m_sendMode = ChatSendModeNames.getLeft(data.getString("sendMode", "Broadcast"));
      m_historyOffset = 0;
    });

  m_sendMode = ChatSendMode::Broadcast;

  reader.construct(assets->json("/interface/chat/chat.config:gui"), this);

  m_textBox = fetchChild<TextBoxWidget>("textBox");
  m_say = fetchChild<LabelWidget>("say");

  m_chatLog = fetchChild<CanvasWidget>("chatLog");
  if (auto logPadding = config.optQuery("padding")) {
    m_chatLogPadding = jsonToVec2I(logPadding.get());
    m_chatLog->setSize(m_chatLog->size() + m_chatLogPadding * 2);
    m_chatLog->setPosition(m_chatLog->position() - m_chatLogPadding);
  }
  else
    m_chatLogPadding = Vec2I();

  m_bottomButton = fetchChild<ButtonWidget>("bottomButton");
  m_upButton = fetchChild<ButtonWidget>("upButton");

  m_chatHistory.appendAll(m_client->playerStorage()->getMetadata("chatHistory").opt().apply(jsonToStringList).value());

  show();

  updateBottomButton();

  m_background = fetchChild<ImageStretchWidget>("background");
  m_defaultHeight = m_background->size()[1];
  m_expanded = false;
  updateSize();
}

void Chat::update(float dt) {
  Pane::update(dt);

  auto team = m_client->teamClient()->currentTeam();
  for (auto button : fetchChild<ButtonGroup>("filterGroup")->buttons()) {
    auto mode = ChatSendModeNames.getLeft(button->data().getString("sendMode", "Broadcast"));
    if (!team.isValid() && m_sendMode == ChatSendMode::Party && mode == ChatSendMode::Broadcast)
      button->check();
    if (mode == ChatSendMode::Party)
      button->setEnabled(team.isValid());
  }
}

void Chat::startChat() {
  show();
  m_textBox->focus();
}

void Chat::startCommand() {
  show();
  m_textBox->setText("/");
  m_textBox->focus();
}

bool Chat::hasFocus() const {
  return m_textBox->hasFocus();
}

void Chat::stopChat() {
  m_textBox->setText("");
  m_textBox->blur();
  m_timeChatLastActive = Time::monotonicMilliseconds();
}

String Chat::currentChat() const {
  return m_textBox->getText();
}

bool Chat::setCurrentChat(String const& chat, bool moveCursor) {
  return m_textBox->setText(chat, true, moveCursor);
}

void Chat::clearCurrentChat() {
  m_textBox->setText("");
  m_chatPrevIndex = 0;
}

ChatSendMode Chat::sendMode() const {
  return m_sendMode;
}

void Chat::incrementIndex() {
  if (!m_chatHistory.empty()) {
    m_chatPrevIndex = std::min(m_chatPrevIndex + 1, (unsigned)m_chatHistory.size());
    m_textBox->setText(m_chatHistory.at(m_chatPrevIndex - 1));
  }
}

void Chat::decrementIndex() {
  if (m_chatPrevIndex > 1 && !m_chatHistory.empty()) {
    --m_chatPrevIndex;
    m_textBox->setText(m_chatHistory.at(m_chatPrevIndex - 1));
  } else {
    m_chatPrevIndex = 0;
    m_textBox->setText("");
  }
}

void Chat::addLine(String const& text, bool showPane) {
  ChatReceivedMessage message = {{MessageContext::CommandResult}, ServerConnectionId, "", text};
  addMessages({message}, showPane);
}

void Chat::addMessages(List<ChatReceivedMessage> const& messages, bool showPane) {
  if (messages.empty())
    return;

  GuiContext& guiContext = GuiContext::singleton();

  for (auto const& message : messages) {
    Maybe<unsigned> wrapWidth;
    if (message.portrait.empty())
      wrapWidth = m_chatLog->size()[0] - m_chatLogPadding[0] * 2;

    guiContext.setTextStyle(m_chatTextStyle);
    StringList lines;
    if (message.fromNick != "" && message.portrait == "")
      lines = guiContext.wrapInterfaceText(strf(m_chatFormatString.utf8Ptr(), message.fromNick, message.text), wrapWidth);
    else
      lines = guiContext.wrapInterfaceText(message.text, wrapWidth);

    for (size_t i = 0; i < lines.size(); ++i) {
      m_receivedMessages.prepend({
          message.context.mode,
          message.portrait,
          std::move(lines[i])
        });
    }

    if (message.fromNick != "")
      Logger::info("Chat: <{}> {}", message.fromNick, message.text);
    else
      Logger::info("Chat: {}", message.text);
  }

  if (showPane) {
    m_timeChatLastActive = Time::monotonicMilliseconds();
    show();
  }

  if ((unsigned)m_receivedMessages.size() > m_chatHistoryLimit)
    m_receivedMessages.resize(m_chatHistoryLimit);
}

void Chat::addHistory(String const& chat) {
  if (m_chatHistory.size() > 0 && m_chatHistory.get(0).equals(chat))
    return;

  m_chatHistory.prepend(chat);
  m_chatHistory.resize(std::min((unsigned)m_chatHistory.size(), m_chatHistoryLimit));
  m_timeChatLastActive = Time::monotonicMilliseconds();
  m_client->playerStorage()->setMetadata("chatHistory", JsonArray::from(m_chatHistory));
}

void Chat::clear(size_t count) {
  if (count > m_receivedMessages.size())
    m_receivedMessages.clear();
  else
    m_receivedMessages.erase(m_receivedMessages.begin(), m_receivedMessages.begin() + count);
}

void Chat::renderImpl() {
  Pane::renderImpl();
  if (m_textBox->hasFocus())
    m_timeChatLastActive = Time::monotonicMilliseconds();
  Vec4B fade = {255, 255, 255, 255};
  fade[3] = (uint8_t)(visible() * 255);
  if (!visible()) {
    hide();
    return;
  }

  Color fadeGreen = Color::Green;
  fadeGreen.setAlpha(fade[3]);
  m_say->setColor(fadeGreen);

  m_chatLog->clear();
  Vec2I chatMin = m_chatLogPadding;
  int messageIndex = -m_historyOffset;

  GuiContext& guiContext = GuiContext::singleton();
  float lineHeight = m_chatTextStyle.lineSpacing;
  float fontSize = m_chatTextStyle.fontSize;
  guiContext.setTextStyle(m_chatTextStyle);
  for (auto& message : m_receivedMessages) {
    if (!m_modeFilter.empty() && !m_modeFilter.contains(message.mode))
      continue;

    messageIndex++;
    if (messageIndex <= 0)
      continue;
    if (chatMin[1] > m_chatLog->size()[1])
      break;

    String channelColorCode = "^reset";
    if (m_colorCodes.contains(message.mode))
      channelColorCode = m_colorCodes[message.mode];
    channelColorCode += "^set;";

    String messageString = channelColorCode + message.text;

    float messageHeight = 0;
    float lineHeightMargin = ((lineHeight * fontSize) - fontSize);
    unsigned wrapWidth = m_chatLog->size()[0] - m_chatLogPadding[0] * 2;

    if (message.portrait != "") {
      TextPositioning tp = {Vec2F(chatMin +  m_portraitTextOffset), HorizontalAnchor::LeftAnchor, VerticalAnchor::VMidAnchor, (wrapWidth - m_portraitTextOffset[0])};
      Vec2F textSize = guiContext.determineInterfaceTextSize(messageString, tp).size().floor();
      Vec2F portraitSize = Vec2F(guiContext.textureSize(m_portraitBackground));
      messageHeight = max(portraitSize[1] + m_portraitVerticalMargin, textSize[1] + lineHeightMargin);

      // Draw both image and text anchored left and centered vertically
      auto imagePosition = chatMin + Vec2I(0, floor(messageHeight / 2)) - Vec2I(0, floor(portraitSize[1] / 2));
      m_chatLog->drawImage(m_portraitBackground, Vec2F(imagePosition), 1.0f, fade);
      m_chatLog->drawImage(message.portrait, Vec2F(imagePosition + m_portraitImageOffset), m_portraitScale, fade);
      tp.pos += Vec2F(0, floor(messageHeight / 2));
      m_chatTextStyle.color = fade;
      m_chatLog->drawText(messageString, tp, m_chatTextStyle);

    } else {
      TextPositioning tp = {Vec2F(chatMin), HorizontalAnchor::LeftAnchor, VerticalAnchor::BottomAnchor, wrapWidth};
      messageHeight = guiContext.determineInterfaceTextSize(messageString, tp).size()[1] + lineHeightMargin;
      m_chatTextStyle.color = fade;
      m_chatLog->drawText(messageString, tp, m_chatTextStyle);
    }

    chatMin[1] += ceil(messageHeight);
  }

  guiContext.setDefaultLineSpacing();
  guiContext.setDefaultFont();
}

void Chat::hide() {
  stopChat();
  Pane::hide();
}

float Chat::visible() const {
  double difference = (Time::monotonicMilliseconds() - m_timeChatLastActive) / 1000.0;
  if (difference < m_chatVisTime)
    return 1;
  return clamp<float>(1 - (difference - m_chatVisTime) / m_fadeRate, 0, 1);
}

bool Chat::sendEvent(InputEvent const& event) {
  if (active()) {
    if (hasFocus()) {
      if (event.is<KeyDownEvent>()) {
        auto actions = context()->actions(event);
        if (actions.contains(InterfaceAction::ChatStop)) {
          stopChat();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPreviousLine)) {
          incrementIndex();
          return true;
        } else if (actions.contains(InterfaceAction::ChatNextLine)) {
          decrementIndex();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPageDown)) {
          scrollDown();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPageUp)) {
          scrollUp();
          return true;
        }
      }
    }

    if (auto mouseWheel = event.ptr<MouseWheelEvent>()) {
      if (inMember(*context()->mousePosition(event))) {
        if (mouseWheel->mouseWheel == MouseWheel::Down)
          scrollDown();
        else
          scrollUp();
        return true;
      }
    }

    if (event.is<MouseMoveEvent>() && inMember(*context()->mousePosition(event)))
      m_timeChatLastActive = Time::monotonicMilliseconds();

    if (event.is<MouseButtonDownEvent>()) {
      if (m_chatLog->inMember(*context()->mousePosition(event))) {
        m_expanded = !m_expanded;
        updateSize();
        return true;
      }
    }
  }

  return Pane::sendEvent(event);
}

void Chat::scrollUp() {
  auto shownMessages = m_receivedMessages.filtered([=](LogMessage msg) {
      return (m_modeFilter.empty() || m_modeFilter.contains(msg.mode));
    });

  m_historyOffset = std::max(0, std::min((int)shownMessages.size() - 1, m_historyOffset + 1));
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::scrollDown() {
  m_historyOffset = std::max(0, m_historyOffset - 1);
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::scrollBottom() {
  m_historyOffset = 0;
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::updateSize() {
  auto height = m_expanded ? m_expandedBodyHeight : m_bodyHeight;
  m_background->setSize(Vec2I(m_background->size()[0], m_defaultHeight + height));
  m_chatLog->setSize(Vec2I(m_chatLog->size()[0], height));
  m_upButton->setPosition(Vec2I(m_upButton->position()[0], m_chatLog->position()[1] + m_chatLog->size()[1] - m_upButton->size()[1]));
  determineSizeFromChildren();
}

void Chat::updateBottomButton() {
  auto assets = Root::singleton().assets();
  auto bottomConfig = assets->json("/interface/chat/chat.config:bottom");
  if (m_historyOffset == 0)
    m_bottomButton->setImages(bottomConfig.get("atbottom").getString("base"), bottomConfig.get("atbottom").getString("hover"));
  else
    m_bottomButton->setImages(bottomConfig.get("scrolling").getString("base"), bottomConfig.get("scrolling").getString("hover"));
}

}
