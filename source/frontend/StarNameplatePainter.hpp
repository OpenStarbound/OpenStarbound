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
  String m_font;
  String m_statusFont;
  String m_fontDirectives;
  String m_statusFontDirectives;
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
