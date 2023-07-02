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

ScriptPane::ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId) : BaseScriptPane(config) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  m_client = move(client);
  m_sourceEntityId = sourceEntityId;

  m_script.addCallbacks("player", LuaBindings::makePlayerCallbacks(m_client->mainPlayer().get()));
  m_script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_client->mainPlayer()->statusController()));
  m_script.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client.get()));
}

void ScriptPane::displayed() {
  auto world = m_client->worldClient();
  if (world && world->inWorld()) {
    m_script.setLuaRoot(world->luaRoot());
    m_script.addCallbacks("world", LuaBindings::makeWorldCallbacks(world.get()));
  }
  BaseScriptPane::displayed();
}

void ScriptPane::dismissed() {
  BaseScriptPane::dismissed();
  m_script.removeCallbacks("world");
}

void ScriptPane::tick() {
  BaseScriptPane::tick();

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

LuaCallbacks ScriptPane::makePaneCallbacks() {
  LuaCallbacks callbacks = BaseScriptPane::makePaneCallbacks();
  callbacks.registerCallback("sourceEntity", [this]() { return m_sourceEntityId; });
  return callbacks;
}

bool ScriptPane::openWithInventory() const {
  return m_config.getBool("openWithInventory", false);
}

}
