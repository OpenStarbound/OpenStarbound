#ifndef STAR_BASE_SCRIPT_PANE_HPP
#define STAR_BASE_SCRIPT_PANE_HPP

#include "StarPane.hpp"
#include "StarLuaComponents.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);
STAR_CLASS(BaseScriptPane);

// A more 'raw' script pane that doesn't depend on a world being present.
// Requires a derived class to provide a Lua root.
// Should maybe move into windowing?

class BaseScriptPane : public Pane {
public:
  BaseScriptPane(Json config);

  virtual void show() override;
  void displayed() override;
  void dismissed() override;

  void tick() override;

  bool sendEvent(InputEvent const& event) override;

  PanePtr createTooltip(Vec2I const& screenPosition) override;
  Maybe<String> cursorOverride(Vec2I const& screenPosition) override;
protected:
  virtual GuiReaderPtr reader();
  Json m_config;

  GuiReaderPtr m_reader;

  Map<CanvasWidgetPtr, String> m_canvasClickCallbacks;
  Map<CanvasWidgetPtr, String> m_canvasKeyCallbacks;

  bool m_callbacksAdded;
  LuaUpdatableComponent<LuaBaseComponent> m_script;
};

}

#endif
