#include "StarChatTypes.hpp"

namespace Star {

EnumMap<ChatSendMode> const ChatSendModeNames{
    {ChatSendMode::Broadcast, "Broadcast"},
    {ChatSendMode::Local, "Local"},
    {ChatSendMode::Party, "Party"}
  };

MessageContext::MessageContext() : mode() {}

MessageContext::MessageContext(Mode mode) : mode(mode) {}

MessageContext::MessageContext(Mode mode, String const& channelName) : mode(mode), channelName(channelName) {}

EnumMap<MessageContext::Mode> const MessageContextModeNames{
    {MessageContext::Mode::Local, "Local"},
    {MessageContext::Mode::Party, "Party"},
    {MessageContext::Mode::Broadcast, "Broadcast"},
    {MessageContext::Mode::Whisper, "Whisper"},
    {MessageContext::Mode::CommandResult, "CommandResult"},
    {MessageContext::Mode::RadioMessage, "RadioMessage"},
    {MessageContext::Mode::World, "World"}
  };

DataStream& operator>>(DataStream& ds, MessageContext& messageContext) {
  ds.read(messageContext.mode);
  ds.read(messageContext.channelName);

  return ds;
}

DataStream& operator<<(DataStream& ds, MessageContext const& messageContext) {
  ds.write(messageContext.mode);
  ds.write(messageContext.channelName);

  return ds;
}

ChatReceivedMessage::ChatReceivedMessage() : fromConnection() {}

ChatReceivedMessage::ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text)
  : context(context), fromConnection(fromConnection), fromNick(fromNick), text(text) {}

ChatReceivedMessage::ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text, String const& portrait)
  : context(context), fromConnection(fromConnection), fromNick(fromNick), portrait(portrait), text(text) {}

DataStream& operator>>(DataStream& ds, ChatReceivedMessage& receivedMessage) {
  ds.read(receivedMessage.context);
  ds.read(receivedMessage.fromConnection);
  ds.read(receivedMessage.fromNick);
  ds.read(receivedMessage.portrait);
  ds.read(receivedMessage.text);

  return ds;
}

DataStream& operator<<(DataStream& ds, ChatReceivedMessage const& receivedMessage) {
  ds.write(receivedMessage.context);
  ds.write(receivedMessage.fromConnection);
  ds.write(receivedMessage.fromNick);
  ds.write(receivedMessage.portrait);
  ds.write(receivedMessage.text);

  return ds;
}

}
