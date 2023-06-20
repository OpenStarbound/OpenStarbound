#ifndef STAR_SCRIPT_PANE_HPP
#define STAR_SCRIPT_PANE_HPP

#include "StarPane.hpp"
#include "StarLuaComponents.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);
STAR_CLASS(ScriptPane);
STAR_CLASS(UniverseClient);

class ScriptPane : public Pane {
public:
  ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId = NullEntityId);

  void displayed() override;
  void dismissed() override;

  void tick() override;

  bool sendEvent(InputEvent const& event) override;

  PanePtr createTooltip(Vec2I const& screenPosition) override;
  Maybe<String> cursorOverride(Vec2I const& screenPosition) override;

  bool openWithInventory() const;

private:
  LuaCallbacks makePaneCallbacks();

  UniverseClientPtr m_client;
  EntityId m_sourceEntityId;
  Json m_config;

  GuiReader m_reader;

  Map<CanvasWidgetPtr, String> m_canvasClickCallbacks;
  Map<CanvasWidgetPtr, String> m_canvasKeyCallbacks;

  LuaWorldComponent<LuaUpdatableComponent<LuaBaseComponent>> m_script;

  List<pair<String, AudioInstancePtr>> m_playingSounds;
};

}

#endif
