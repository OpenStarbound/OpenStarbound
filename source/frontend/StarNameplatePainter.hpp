#ifndef STAR_NAMEPLATE_PAINTER_HPP
#define STAR_NAMEPLATE_PAINTER_HPP

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

  void update(WorldClientPtr const& world, WorldCamera const& camera, bool inspectionMode);
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

  float m_opacityRate;
  Vec2F m_offset;
  float m_fontSize;
  float m_statusFontSize;
  Vec2F m_statusOffset;
  Color m_statusColor;
  float m_opacityBoost;

  WorldCamera m_camera;

  Set<EntityId> m_entitiesWithNametags;
  BubbleSeparator<Nametag> m_nametags;
};

}

#endif
