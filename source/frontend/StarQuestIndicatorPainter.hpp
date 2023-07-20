#ifndef STAR_QUEST_INDICATOR_PAINTER_HPP
#define STAR_QUEST_INDICATOR_PAINTER_HPP

#include "StarWorldCamera.hpp"
#include "StarWorldClient.hpp"

namespace Star {

STAR_CLASS(UniverseClient);
STAR_CLASS(QuestIndicatorPainter);

class QuestIndicatorPainter {
public:
  QuestIndicatorPainter(UniverseClientPtr const& client);

  void update(float dt, WorldClientPtr const& world, WorldCamera const& camera);
  void render();

private:
  struct Indicator {
    Drawable render(float pixelRatio) const;

    EntityId entityId;
    Vec2F screenPos;
    String indicatorName;
    AnimationPtr animation;
  };

  UniverseClientPtr m_client;
  WorldCamera m_camera;
  Map<EntityId, Indicator> m_indicators;
};

}

#endif
