#ifndef STAR_STATUSPANE_HPP
#define STAR_STATUSPANE_HPP

#include "StarPane.hpp"
#include "StarMainInterfaceTypes.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(UniverseClient);
STAR_CLASS(StatusPane);

class StatusPane : public Pane {
public:
  StatusPane(MainInterfacePaneManager* paneManager, UniverseClientPtr client);

  virtual PanePtr createTooltip(Vec2I const& screenPosition) override;

protected:
  virtual void renderImpl() override;
  virtual void update(float dt) override;

private:
  struct StatusEffectIndicator {
    String icon;
    Maybe<float> durationPercentage;
    String label;
    RectF screenRect;
  };

  MainInterfacePaneManager* m_paneManager;
  UniverseClientPtr m_client;
  PlayerPtr m_player;

  GuiContext* m_guiContext;
  List<StatusEffectIndicator> m_statusIndicators;
};

}

#endif
