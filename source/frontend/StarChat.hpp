#ifndef STAR_CHAT_HPP
#define STAR_CHAT_HPP

#include "StarPane.hpp"
#include "StarChatTypes.hpp"

namespace Star {

STAR_CLASS(UniverseClient);
STAR_CLASS(TextBoxWidget);
STAR_CLASS(LabelWidget);
STAR_CLASS(ButtonWidget);
STAR_CLASS(ImageStretchWidget);
STAR_CLASS(CanvasWidget);
STAR_CLASS(Chat);

class Chat : public Pane {
public:
  Chat(UniverseClientPtr client);

  void startChat();
  void startCommand();
  bool hasFocus() const override;
  virtual bool sendEvent(InputEvent const& event) override;
  void stopChat();
  virtual void renderImpl() override;
  virtual void hide() override;

  virtual void update() override;

  void addLine(String const& text, bool showPane = true);
  void addMessages(List<ChatReceivedMessage> const& messages, bool showPane = true);
  void addHistory(String const& chat);

  String currentChat() const;
  void setCurrentChat(String const& chat);
  void clearCurrentChat();

  ChatSendMode sendMode() const;

  void incrementIndex();
  void decrementIndex();

  float visible() const;

  void scrollUp();
  void scrollDown();
  void scrollBottom();

private:
  struct LogMessage {
    MessageContext::Mode mode;
    String portrait;
    String text;
  };

  void updateBottomButton();

  UniverseClientPtr m_client;

  TextBoxWidgetPtr m_textBox;
  LabelWidgetPtr m_say;
  ButtonWidgetPtr m_bottomButton;
  ButtonWidgetPtr m_upButton;
  Deque<String> m_chatHistory;
  unsigned m_chatPrevIndex;
  int64_t m_timeChatLastActive;
  float m_chatVisTime;
  float m_fadeRate;
  unsigned m_fontSize;
  float m_chatLineHeight;
  unsigned m_chatHistoryLimit;
  int m_historyOffset;

  CanvasWidgetPtr m_chatLog;

  ImageStretchWidgetPtr m_background;
  int m_defaultHeight;
  int m_bodyHeight;
  int m_expandedBodyHeight;
  bool m_expanded;

  void updateSize();

  Vec2I m_portraitTextOffset;
  Vec2I m_portraitImageOffset;
  float m_portraitScale;
  int m_portraitVerticalMargin;
  String m_portraitBackground;

  Map<MessageContext::Mode, String> m_colorCodes;
  Deque<LogMessage> m_receivedMessages;

  Set<MessageContext::Mode> m_modeFilter;
  ChatSendMode m_sendMode;
};

}

#endif
