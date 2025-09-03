#include "StarInterfaceLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMainInterface.hpp"
#include "StarGuiContext.hpp"
#include "StarChat.hpp"
#include "StarUniverseClient.hpp"
#include "StarClientCommandProcessor.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeInterfaceCallbacks(MainInterface* mainInterface) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<bool>(
    "hudVisible", bind(mem_fn(&MainInterface::hudVisible), mainInterface));
  callbacks.registerCallbackWithSignature<void, bool>(
    "setHudVisible", bind(mem_fn(&MainInterface::setHudVisible), mainInterface, _1));

  callbacks.registerCallback("bindCanvas", [mainInterface](String const& canvasName, Maybe<bool> ignoreInterfaceScale) -> Maybe<CanvasWidgetPtr> {
    if (auto canvas = mainInterface->fetchCanvas(canvasName, ignoreInterfaceScale.value(false)))
      return canvas;
    return {};
  });

  
  callbacks.registerCallback("bindRegisteredPane", [mainInterface](String const& registeredPaneName) -> Maybe<LuaCallbacks> {
    if (auto pane = mainInterface->paneManager()->maybeRegisteredPane(MainInterfacePanesNames.getLeft(registeredPaneName)))
      return pane->makePaneCallbacks();
    return {};
  });
  
  callbacks.registerCallback("displayRegisteredPane", [mainInterface](String const& registeredPaneName) {
    auto pane = MainInterfacePanesNames.getLeft(registeredPaneName);
    auto paneManager = mainInterface->paneManager();
    if (paneManager->maybeRegisteredPane(pane))
      paneManager->displayRegisteredPane(pane);
  });

  callbacks.registerCallback("scale", []() {
    return GuiContext::singleton().interfaceScale();
  });

  callbacks.registerCallback("queueMessage", [mainInterface](String const& message, Maybe<float> cooldown, Maybe<float> springState) {
    mainInterface->queueMessage(message, cooldown, springState.value(0));
  });


  return callbacks;
}

LuaCallbacks LuaBindings::makeChatCallbacks(MainInterface* mainInterface, UniverseClient* client) {
  LuaCallbacks callbacks;

  auto chat = as<Chat>(mainInterface->paneManager()->registeredPane(MainInterfacePanes::Chat).get());

  callbacks.registerCallback("send", [client](String const& message, Maybe<String> modeName, Maybe<bool> speak, Maybe<JsonObject> data) {
    auto sendMode = modeName ? ChatSendModeNames.getLeft(*modeName) : ChatSendMode::Broadcast;
    client->sendChat(message, sendMode, speak, data);
  });

  // just for SE compat - this shoulda been a utility callback :moyai:
  callbacks.registerCallback("parseArguments", [](String const& args) -> LuaVariadic<Json> {
    return Json::parseSequence(args).toArray();
  });

  callbacks.registerCallback("command", [mainInterface](String const& command) -> StringList {
    return mainInterface->commandProcessor()->handleCommand(command);
  });

  callbacks.registerCallback("addMessage", [client, chat](String const& text, Maybe<Json> config) {
    ChatReceivedMessage message({MessageContext::Mode::CommandResult, ""}, client->clientContext()->connectionId(), "", text);
    if (config) {
      if (auto mode = config->optString("mode"))
        message.context.mode = MessageContextModeNames.getLeft(*mode);
      if (auto channelName = config->optString("channelName"))
        message.context.channelName = std::move(*channelName);
      if (auto portrait = config->optString("portrait"))
        message.portrait = std::move(*portrait);
      if (auto fromNick = config->optString("fromNick"))
        message.fromNick = std::move(*fromNick);
    }
    chat->addMessages({std::move(message)}, config ? config->getBool("showPane", true) : true);
  });

  callbacks.registerCallback("input", [chat]() -> String {
    return chat->currentChat();
  });

  callbacks.registerCallback("setInput", [chat](String const& text, Maybe<bool> moveCursor) -> bool {
    return chat->setCurrentChat(text, moveCursor.value(false));
  });

  callbacks.registerCallback("clear", [chat](Maybe<size_t> count) {
    chat->clear(count.value(std::numeric_limits<size_t>::max()));
  });

  return callbacks;
}

}
