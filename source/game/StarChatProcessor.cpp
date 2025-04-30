#include "StarChatProcessor.hpp"

namespace Star {

char const* ChatProcessor::ServerNick = "server";

String ChatProcessor::connectClient(ConnectionId clientId, String nick) {
  RecursiveMutexLocker locker(m_mutex);

  if (nick.empty())
    nick = strf("Player_{}", clientId);

  nick = makeNickUnique(nick);

  for (auto& pair : m_clients) {
    pair.second.pendingMessages.append({
        {MessageContext::Broadcast},
        ServerConnectionId,
        ServerNick,
        strf("Player '{}' connected", nick)
      });
  }

  m_clients.add(clientId, ClientInfo(clientId, nick));
  m_nicks[nick] = clientId;
  return nick;
}

List<ChatReceivedMessage> ChatProcessor::disconnectClient(ConnectionId clientId) {
  RecursiveMutexLocker locker(m_mutex);

  for (auto channel : clientChannels(clientId))
    leaveChannel(clientId, channel);

  auto clientInfo = m_clients.take(clientId);

  m_nicks.remove(clientInfo.nick);

  for (auto& pair : m_clients) {
    pair.second.pendingMessages.append({
        {MessageContext::Broadcast},
        ServerConnectionId,
        ServerNick,
        strf("Player '{}' disconnected", clientInfo.nick)
      });
  }

  return clientInfo.pendingMessages;
}

List<ConnectionId> ChatProcessor::clients() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_clients.keys();
}

bool ChatProcessor::hasClient(ConnectionId clientId) const {
  RecursiveMutexLocker locker(m_mutex);
  return m_clients.contains(clientId);
}

Maybe<ConnectionId> ChatProcessor::findNick(String const& nick) const {
  RecursiveMutexLocker locker(m_mutex);
  if (auto m = m_nicks.maybe(nick))
    return m;
  if (nick == ServerNick)
    return ServerConnectionId;
  return {};
}

String ChatProcessor::connectionNick(ConnectionId clientId) const {
  RecursiveMutexLocker locker(m_mutex);

  if (clientId == ServerConnectionId)
    return ServerNick;
  else
    return m_clients.get(clientId).nick;
}

String ChatProcessor::renick(ConnectionId clientId, String const& nick) {
  RecursiveMutexLocker locker(m_mutex);

  auto& clientInfo = m_clients.get(clientId);
  m_nicks.remove(clientInfo.nick);

  clientInfo.nick = makeNickUnique(nick);
  m_clients.get(clientId).nick = nick;
  m_nicks[nick] = clientId;
  return nick;
}

bool ChatProcessor::joinChannel(ConnectionId clientId, String const& channelName) {
  RecursiveMutexLocker locker(m_mutex);

  // Right now channels are simply created on join if they don't exist.
  return m_channels[channelName].add(clientId);
}

bool ChatProcessor::leaveChannel(ConnectionId clientId, String const& channelName) {
  RecursiveMutexLocker locker(m_mutex);
  return m_channels[channelName].remove(clientId);
}

StringList ChatProcessor::clientChannels(ConnectionId clientId) const {
  RecursiveMutexLocker locker(m_mutex);

  StringList channels;
  for (auto const& pair : m_channels) {
    if (pair.second.contains(clientId))
      channels.append(pair.first);
  }
  return channels;
}

StringList ChatProcessor::activeChannels() const {
  RecursiveMutexLocker locker(m_mutex);

  StringList channels;
  for (auto const& pair : m_channels) {
    if (!pair.second.empty())
      channels.append(pair.first);
  }
  return channels;
}

void ChatProcessor::broadcast(ConnectionId sourceConnectionId, String const& text, JsonObject data) {
  RecursiveMutexLocker locker(m_mutex);

  ChatReceivedMessage message = {
    {MessageContext::Broadcast},
    sourceConnectionId,
    connectionNick(sourceConnectionId),
    text
  };

  message.data = std::move(data);

  if (handleCommand(message))
    return;

  for (auto& pair : m_clients)
    pair.second.pendingMessages.append(message);
}

void ChatProcessor::message(ConnectionId sourceConnectionId, MessageContext::Mode mode, String const& channelName, String const& text, JsonObject data) {
  RecursiveMutexLocker locker(m_mutex);

  ChatReceivedMessage message = {
    {mode, channelName},
    sourceConnectionId,
    connectionNick(sourceConnectionId),
    text
  };

  message.data = std::move(data);

  if (handleCommand(message))
    return;

  for (auto clientId : m_channels[channelName]) {
    auto& clientInfo = m_clients.get(clientId);
    clientInfo.pendingMessages.append(message);
  }
}

void ChatProcessor::whisper(ConnectionId sourceConnectionId, ConnectionId targetClientId, String const& text, JsonObject data) {
  RecursiveMutexLocker locker(m_mutex);

  ChatReceivedMessage message = {
    {MessageContext::Whisper},
    sourceConnectionId,
    connectionNick(sourceConnectionId),
    text
  };

  message.data = std::move(data);

  if (handleCommand(message))
    return;

  if (sourceConnectionId != ServerConnectionId)
    m_clients.get(sourceConnectionId).pendingMessages.append(message);

  m_clients.get(targetClientId).pendingMessages.append(message);
}

void ChatProcessor::adminBroadcast(String const& text) {
  RecursiveMutexLocker locker(m_mutex);
  broadcast(ServerConnectionId, text);
}

void ChatProcessor::adminMessage(MessageContext::Mode context, String const& channelName, String const& text) {
  RecursiveMutexLocker locker(m_mutex);
  ChatProcessor::message(ServerConnectionId, context, channelName, text);
}

void ChatProcessor::adminWhisper(ConnectionId targetClientId, String const& text) {
  RecursiveMutexLocker locker(m_mutex);
  whisper(ServerConnectionId, targetClientId, text);
}

List<ChatReceivedMessage> ChatProcessor::pullPendingMessages(ConnectionId clientId) {
  RecursiveMutexLocker locker(m_mutex);
  if (m_clients.contains(clientId))
    return take(m_clients.get(clientId).pendingMessages);
  return {};
}

void ChatProcessor::setCommandHandler(CommandHandler commandHandler) {
  RecursiveMutexLocker locker(m_mutex);
  m_commandHandler = commandHandler;
}

void ChatProcessor::clearCommandHandler() {
  RecursiveMutexLocker locker(m_mutex);
  m_commandHandler = {};
}

ChatProcessor::ClientInfo::ClientInfo(ConnectionId clientId, String const& nick) : clientId(clientId), nick(nick) {}

String ChatProcessor::makeNickUnique(String nick) {
  while (m_nicks.contains(nick) || nick == ServerNick)
    nick.append("_");

  return nick;
}

bool ChatProcessor::handleCommand(ChatReceivedMessage& message) {
  if (!message.text.beginsWith("/")) {
    return false;
  } else if (message.text.beginsWith("//")) {
    message.text = message.text.substr(1);
    return false;
  }

  String commandLine = message.text.substr(1);
  String command = commandLine.extract();

  String response;

  if (command == "nick") {
    auto newNick = renick(message.fromConnection, commandLine.trim());
    response = strf("Nick changed to {}", newNick);
  } else if (command == "w") {
    String target = commandLine.extract();
    if (m_nicks.contains(target))
      whisper(message.fromConnection, m_nicks.get(target), commandLine.trim());
    else
      response = strf("No such nick {}", target);
  } else if (m_commandHandler) {
    response = m_commandHandler(message.fromConnection, command, commandLine);
  } else {
    response = strf("No such command {}", command);
  }

  if (!response.empty()) {
    m_clients.get(message.fromConnection).pendingMessages.append({
        MessageContext(MessageContext::CommandResult),
        ServerConnectionId,
        connectionNick(ServerConnectionId),
        response
      });
  }

  return true;
}

}
