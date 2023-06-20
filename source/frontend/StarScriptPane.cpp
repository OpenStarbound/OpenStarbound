#include "StarScriptPane.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarJsonExtra.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarWorldClient.hpp"
#include "StarPlayer.hpp"
#include "StarUniverseClient.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarCanvasWidget.hpp"
#include "StarItemTooltip.hpp"
#include "StarItemGridWidget.hpp"
#include "StarSimpleTooltip.hpp"
#include "StarImageWidget.hpp"

namespace Star {

ScriptPane::ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  m_client = move(client);

  if (config.type() == Json::Type::Object && config.contains("baseConfig")) {
    auto baseConfig = assets->fetchJson(config.getString("baseConfig"));
    m_config = jsonMerge(baseConfig, config);
  } else {
    m_config = assets->fetchJson(config);
  }
  m_sourceEntityId = sourceEntityId;

  m_reader.registerCallback("close", [this](Widget*) { dismiss(); });

  for (auto const& callbackName : jsonToStringList(m_config.get("scriptWidgetCallbacks", JsonArray{}))) {
    m_reader.registerCallback(callbackName, [this, callbackName](Widget* widget) {
      m_script.invoke(callbackName, widget->name(), widget->data());
    });
  }

  m_reader.construct(assets->fetchJson(m_config.get("gui")), this);

  for (auto pair : m_config.getObject("canvasClickCallbacks", {}))
    m_canvasClickCallbacks.set(findChild<CanvasWidget>(pair.first), pair.second.toString());
  for (auto pair : m_config.getObject("canvasKeyCallbacks", {}))
    m_canvasKeyCallbacks.set(findChild<CanvasWidget>(pair.first), pair.second.toString());

  m_script.setScripts(jsonToStringList(m_config.get("scripts", JsonArray())));
  m_script.addCallbacks("pane", makePaneCallbacks());
  m_script.addCallbacks("widget", LuaBindings::makeWidgetCallbacks(this, &m_reader));
  m_script.addCallbacks("config", LuaBindings::makeConfigCallbacks( [this](String const& name, Json const& def) {
      return m_config.query(name, def);
    }));
  m_script.addCallbacks("player", LuaBindings::makePlayerCallbacks(m_client->mainPlayer().get()));
  m_script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_client->mainPlayer()->statusController()));
  m_script.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client.get()));
  m_script.setUpdateDelta(m_config.getUInt("scriptDelta", 1));
}

void ScriptPane::displayed() {
  Pane::displayed();
  auto world = m_client->worldClient();
  if (world && world->inWorld())
    m_script.init(world.get());

  m_script.invoke("displayed");
}

void ScriptPane::dismissed() {
  Pane::dismissed();
  m_script.invoke("dismissed");
  m_script.uninit();
}

void ScriptPane::tick() {
  Pane::tick();

  if (m_sourceEntityId != NullEntityId && !m_client->worldClient()->playerCanReachEntity(m_sourceEntityId))
    dismiss();

  for (auto p : m_canvasClickCallbacks) {
    for (auto const& clickEvent : p.first->pullClickEvents())
      m_script.invoke(p.second, jsonFromVec2I(clickEvent.position), (uint8_t)clickEvent.button, clickEvent.buttonDown);
  }
  for (auto p : m_canvasKeyCallbacks) {
    for (auto const& keyEvent : p.first->pullKeyEvents())
      m_script.invoke(p.second, (int)keyEvent.key, keyEvent.keyDown);
  }

  m_playingSounds.filter([](pair<String, AudioInstancePtr> const& p) {
      return p.second->finished() == false;
    });

  m_script.update(m_script.updateDt());
}

bool ScriptPane::sendEvent(InputEvent const& event) {
  // Intercept GuiClose before the canvas child so GuiClose always closes
  // ScriptPanes without having to support it in the script.
  if (context()->actions(event).contains(InterfaceAction::GuiClose)) {
    dismiss();
    return true;
  }

  return Pane::sendEvent(event);
}

PanePtr ScriptPane::createTooltip(Vec2I const& screenPosition) {
  auto result = m_script.invoke<Json>("createTooltip", screenPosition);
  if (result && !result.value().isNull()) {
    if (result->type() == Json::Type::String) {
      return SimpleTooltipBuilder::buildTooltip(result->toString());
    } else {
      PanePtr tooltip = make_shared<Pane>();
      m_reader.construct(*result, tooltip.get());
      return tooltip;
    }
  } else {
    ItemPtr item;
    if (auto child = getChildAt(screenPosition)) {
      if (auto itemSlot = as<ItemSlotWidget>(child))
        item = itemSlot->item();
      if (auto itemGrid = as<ItemGridWidget>(child))
        item = itemGrid->itemAt(screenPosition);
    }
    if (item)
      return ItemTooltipBuilder::buildItemTooltip(item, m_client->mainPlayer());
    return {};
  }
}

Maybe<String> ScriptPane::cursorOverride(Vec2I const& screenPosition) {
  auto result = m_script.invoke<Maybe<String>>("cursorOverride", screenPosition);
  if (result)
    return *result;
  else
    return {};
}

LuaCallbacks ScriptPane::makePaneCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("sourceEntity", [this]() { return m_sourceEntityId; });
  callbacks.registerCallback("dismiss", [this]() { dismiss(); });

  callbacks.registerCallback("playSound",
    [this](String const& audio, Maybe<int> loops, Maybe<float> volume) {
      auto assets = Root::singleton().assets();
      auto config = Root::singleton().configuration();
      auto audioInstance = make_shared<AudioInstance>(*assets->audio(audio));
      audioInstance->setVolume(volume.value(1.0));
      audioInstance->setLoops(loops.value(0));
      auto& guiContext = GuiContext::singleton();
      guiContext.playAudio(audioInstance);
      m_playingSounds.append({audio, move(audioInstance)});
    });

  callbacks.registerCallback("stopAllSounds", [this](String const& audio) {
      m_playingSounds.filter([audio](pair<String, AudioInstancePtr> const& p) {
        if (p.first == audio) {
          p.second->stop();
          return false;
        }
        return true;
      });
    });

  callbacks.registerCallback("setTitle", [this](String const& title, String const& subTitle) {
      setTitleString(title, subTitle);
    });

  callbacks.registerCallback("setTitleIcon", [this](String const& image) {
      if (auto icon = as<ImageWidget>(titleIcon()))
        icon->setImage(image);
    });

  callbacks.registerCallback("addWidget", [this](Json const& newWidgetConfig, Maybe<String> const& newWidgetName) {
      String name = newWidgetName.value(strf("%d", Random::randu64()));
      WidgetPtr newWidget = m_reader.makeSingle(name, newWidgetConfig);
      this->addChild(name, newWidget);
    });

  callbacks.registerCallback("removeWidget", [this](String const& widgetName) {
      this->removeChild(widgetName);
    });

  return callbacks;
}

bool ScriptPane::openWithInventory() const {
  return m_config.getBool("openWithInventory", false);
}

}
