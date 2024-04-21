#pragma once

#include "StarWorldClient.hpp"
#include "StarWorldCamera.hpp"
#include "StarChatBubbleSeparation.hpp"
#include "StarTextPainter.hpp"

namespace Star {

STAR_CLASS(WorldClient);
STAR_CLASS(NameplatePainter);

class NameplatePainter {
public:
  NameplatePainter();

  void update(float dt, WorldClientPtr const& world, WorldCamera const& camera, bool inspectionMode);
  void render();

private:
  struct Nametag {
    String name;
    Maybe<String> statusText;
    Vec3B color;
    float opacity;
    EntityId entityId;
  };

  TextPositioning namePosition(Vec2F bubblePosition) const;
  TextPositioning statusPosition(Vec2F bubblePosition) const;
  RectF determineBoundBox(Vec2F bubblePosition, Nametag const& nametag) const;

  bool m_showMasterNames;
  float m_opacityRate;
  float m_inspectOpacityRate;
  Vec2F m_offset;
  Vec2F m_statusOffset;
  TextStyle m_textStyle;
  TextStyle m_statusTextStyle;
  float m_opacityBoost;

  WorldCamera m_camera;

  Set<EntityId> m_entitiesWithNametags;
  BubbleSeparator<Nametag> m_nametags;
};

}
