#pragma once

#include "StarButtonGroup.hpp"

namespace Star {

STAR_CLASS(FlowLayout);
STAR_CLASS(StackWidget);

STAR_CLASS(TabBarWidget);
STAR_CLASS(TabSetWidget);

struct TabSetConfig {
  String tabButtonBaseImage;
  String tabButtonHoverImage;
  String tabButtonPressedImage;
  String tabButtonBaseImageSelected;
  String tabButtonHoverImageSelected;
  String tabButtonPressedImageSelected;
  Vec2I tabButtonPressedOffset;
  Vec2I tabButtonTextOffset;
  Vec2I tabButtonSpacing;
};

class TabSetWidget : public Widget {
public:
  TabSetWidget(TabSetConfig const& tabSetconfig);

  virtual void setSize(Vec2I const& size) override;

  void addTab(String const& widgetName, WidgetPtr widget, String const& title);

  size_t tabCount() const;
  void tabSelect(size_t page);
  size_t selectedTab() const;

  // Callback is called when the tab changes
  void setCallback(WidgetCallbackFunc callback);

private:
  TabSetConfig m_tabSetConfig;
  FlowLayoutPtr m_tabBar;
  StackWidgetPtr m_stack;
  WidgetCallbackFunc m_callback;
  Maybe<size_t> m_lastSelected;
};

}
