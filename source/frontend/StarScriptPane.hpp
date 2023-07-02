#ifndef STAR_SCRIPT_PANE_HPP
#define STAR_SCRIPT_PANE_HPP

#include "StarBaseScriptPane.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);
STAR_CLASS(ScriptPane);
STAR_CLASS(UniverseClient);

class ScriptPane : public BaseScriptPane {
public:
  ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId = NullEntityId);

  void displayed() override;
  void dismissed() override;

  void tick() override;

  PanePtr createTooltip(Vec2I const& screenPosition) override;

  bool openWithInventory() const;

private:
  LuaCallbacks makePaneCallbacks() override;

  UniverseClientPtr m_client;
  EntityId m_sourceEntityId;
};

}

#endif
