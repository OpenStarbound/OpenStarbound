#pragma once

#include "StarDataStream.hpp"
#include "StarGameTypes.hpp"
#include "StarJson.hpp"

namespace Star {

enum class ChatSendMode : uint8_t {
  Broadcast,
  Local,
  Party
};

extern EnumMap<ChatSendMode> const ChatSendModeNames;

struct MessageContext {
  enum Mode : uint8_t {
    Local,
    Party,
    Broadcast,
    Whisper,
    CommandResult,
    RadioMessage,
    World
  };

  MessageContext();
  MessageContext(Mode mode);
  MessageContext(Mode mode, String const& channelName);

  Mode mode;

  // only for Local and Party modes
  String channelName;
};

extern EnumMap<MessageContext::Mode> const MessageContextModeNames;

DataStream& operator>>(DataStream& ds, MessageContext& messageContext);
DataStream& operator<<(DataStream& ds, MessageContext const& messageContext);

struct ChatReceivedMessage {
  ChatReceivedMessage();
  ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text);
  ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text, String const& portrait);
  ChatReceivedMessage(Json const& json);

  Json toJson() const;

  MessageContext context;

  ConnectionId fromConnection;
  String fromNick;
  String portrait;

  String text;
};

DataStream& operator>>(DataStream& ds, ChatReceivedMessage& receivedMessage);
DataStream& operator<<(DataStream& ds, ChatReceivedMessage const& receivedMessage);

};
