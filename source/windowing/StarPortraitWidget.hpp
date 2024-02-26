#pragma once

#include "StarWidget.hpp"
#include "StarPlayer.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(PortraitWidget);

class PortraitWidget : public Widget {
public:
  PortraitWidget(PortraitEntityPtr entity, PortraitMode mode = PortraitMode::Full);
  PortraitWidget();
  virtual ~PortraitWidget() {}

  void setEntity(PortraitEntityPtr entity);
  void setMode(PortraitMode mode);
  void setScale(float scale);
  void setIconMode();
  void setRenderHumanoid(bool);
  bool sendEvent(InputEvent const& event);

protected:
  virtual RectI getScissorRect() const;
  virtual void renderImpl();

private:
  void init();
  void updateSize();

  PortraitEntityPtr m_entity;
  PortraitMode m_portraitMode;
  AssetPath m_noEntityImageFull;
  AssetPath m_noEntityImagePart;
  float m_scale;

  bool m_renderHumanoid;
  bool m_iconMode;
  AssetPath m_iconImage;
  Vec2I m_iconOffset;
};

}
