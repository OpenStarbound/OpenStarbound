#pragma once

#include "StarWidget.hpp"
#include "StarBiMap.hpp"

namespace Star {

STAR_CLASS(Pane);
STAR_CLASS(LuaCallbacks);
STAR_CLASS(AudioInstance);
STAR_CLASS(GuiReader);

enum class PaneAnchor {
  None,
  BottomLeft,
  BottomRight,
  TopLeft,
  TopRight,
  CenterBottom,
  CenterTop,
  CenterLeft,
  CenterRight,
  Center
};
extern EnumMap<PaneAnchor> const PaneAnchorNames;

class Pane : public Widget {
public:
  Pane();

  struct BGResult {
    String header;
    String body;
    String footer;
  };

  virtual void displayed();
  virtual void dismissed();

  void dismiss();
  bool isDismissed() const;
  bool isDisplayed() const;

  Vec2I centerOffset() const;

  // members are drawn strictly in the order they are added,
  // so add them in the correct order.

  virtual bool sendEvent(InputEvent const& event);
  virtual void setFocus(Widget const* focus);
  virtual void removeFocus(Widget const* focus);
  virtual void removeFocus();

  virtual void update(float dt);
  virtual void tick(float dt);

  bool dragActive() const;
  Vec2I dragMouseOrigin() const;
  void setDragActive(bool dragActive, Vec2I dragMouseOrigin);
  void drag(Vec2I mousePosition);

  bool inWindow(Vec2I const& position) const;
  bool inDragArea(Vec2I const& position) const;
  Vec2I cursorRelativeToPane(Vec2I const& position) const;

  void setBG(BGResult const& res);
  void setBG(String const& header, String const& body = "", String const& footer = "");
  BGResult getBG() const;

  void lockPosition();
  void unlockPosition();

  void setTitle(WidgetPtr icon, String const& title, String const& subTitle);
  void setTitleString(String const& title, String const& subTitle);
  void setTitleIcon(WidgetPtr icon);
  String title() const;
  String subTitle() const;
  WidgetPtr titleIcon() const;

  virtual Pane* window();
  virtual Pane const* window() const;

  PaneAnchor anchor();
  void setAnchor(PaneAnchor anchor);
  Vec2I anchorOffset() const;
  void setAnchorOffset(Vec2I anchorOffset);
  bool hasDisplayed() const;

  // If a tooltip popup should be created at the given mouse position, return a
  // new pane to be used as the tooltip.
  virtual PanePtr createTooltip(Vec2I const& screenPosition);
  virtual Maybe<String> cursorOverride(Vec2I const& screenPosition);

  virtual LuaCallbacks makePaneCallbacks();
protected:
  virtual GuiReaderPtr reader();
  virtual void renderImpl();

  String m_bgHeader;
  String m_bgBody;
  String m_bgFooter;

  Vec2I m_footerSize;
  Vec2I m_bodySize;
  Vec2I m_headerSize;

  bool m_dismissed;
  bool m_dragActive;
  Vec2I m_dragMouseOrigin;
  bool m_lockPosition;
  Vec2I m_centerOffset;

  WidgetPtr m_mouseOver;
  WidgetPtr m_clickDown;
  WidgetPtr m_focusWidget;

  WidgetPtr m_icon;
  String m_title;
  String m_subTitle;
  String m_font;
  unsigned m_fontSize;
  Vec2I m_iconOffset;
  Vec2I m_titleOffset;
  Vec2I m_subTitleOffset;
  Color m_titleColor;
  Color m_subTitleColor;

  PaneAnchor m_anchor;
  Vec2I m_anchorOffset;
  bool m_hasDisplayed;

  List<pair<String, AudioInstancePtr>> m_playingSounds;
};

}
