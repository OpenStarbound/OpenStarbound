#ifndef STAR_CHAT_PROCESSOR_HPP
#define STAR_CHAT_PROCESSOR_HPP

#include "StarChatTypes.hpp"
#include "StarSet.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(ChatProcessor);

// Handles all chat routing and command parsing for client / server chat.
// Thread safe.
class ChatProcessor {
public:
  static char const* ServerNick;

  // CommandHandler is passed the origin connection, the command portion
  // excluding the '/' character, and the remaining command line in full.
  typedef function<String(ConnectionId, String, String)> CommandHandler;

  String connectClient(ConnectionId clientId, String nick = "");
  // Returns any pending messages.
  List<ChatReceivedMessage> disconnectClient(ConnectionId clientId);

  List<ConnectionId> clients() const;
  bool hasClient(ConnectionId clientId) const;

  // Clears all clients and channels
  void reset();

  // Will return nothing if nick is not found.
  Maybe<ConnectionId> findNick(String const& nick) const;
  String connectionNick(ConnectionId connectionId) const;
  String renick(ConnectionId clientId, String const& nick);

  // join / leave return true in the even that the client channel state was
  // actually changed.
  bool joinChannel(ConnectionId clientId, String const& channelName);
  bool leaveChannel(ConnectionId clientId, String const& channelName);

  StringList clientChannels(ConnectionId clientId) const;
  StringList activeChannels() const;

  void broadcast(ConnectionId sourceConnectionId, String const& text);
  void message(ConnectionId sourceConnectionId, MessageContext::Mode context, String const& channelName, String const& text);
  void whisper(ConnectionId sourceConnectionId, ConnectionId targetClientId, String const& text);

  // Shorthand for passing ServerConnectionId as sourceConnectionId to
  // broadcast / message / whisper
  void adminBroadcast(String const& text);
  void adminMessage(MessageContext::Mode context, String const& channelName, String const& text);
  void adminWhisper(ConnectionId targetClientId, String const& text);

  List<ChatReceivedMessage> pullPendingMessages(ConnectionId clientId);

  void setCommandHandler(CommandHandler commandHandler);
  void clearCommandHandler();

private:
  struct ClientInfo {
    ClientInfo(ConnectionId clientId, String const& nick);

    ConnectionId clientId;
    String nick;
    List<ChatReceivedMessage> pendingMessages;
  };

  String makeNickUnique(String nick);

  // Returns true if message was handled completely and needs no further
  // processing.
  bool handleCommand(ChatReceivedMessage& message);

  mutable RecursiveMutex m_mutex;

  HashMap<ConnectionId, ClientInfo> m_clients;
  StringMap<ConnectionId> m_nicks;
  StringMap<Set<ConnectionId>> m_channels;

  CommandHandler m_commandHandler;
};

}

#endif
