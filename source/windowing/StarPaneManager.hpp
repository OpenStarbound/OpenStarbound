#pragma once

#include "StarPane.hpp"
#include "StarOrderedMap.hpp"
#include "StarBiMap.hpp"
#include "StarGameTimers.hpp"

namespace Star {

STAR_CLASS(PaneManager);

enum class PaneLayer {
  // A special class of window only meant to be used by PaneManager to display
  // tooltips given by Pane::createTooltip
  Tooltip,
  // A special class of window that is displayed above all other windows and
  // turns off input to other windows and the hud until it is dismissed.
  ModalWindow,
  // Window layer for regular windows that are regularly displayed and
  // dismissed and dragged around.
  Window,
  // The bottom GUI layer, for persistent hud elements that are always or almost
  // always shown.  Not key dismissable.
  Hud,
  // Layer for interface elements which are logically part of the world but
  // handled by GUI panes (such as wires)
  World
};
extern EnumMap<PaneLayer> const PaneLayerNames;


// This class handles a set of panes to be drawn as a collective windowing
// interface.  It is a set of panes on separate distinct layers, where each
// layer contains a z-ordered list of panes to display.
class PaneManager {
public:
  typedef function<void(PanePtr const&)> DismissCallback;

  PaneManager();

  // Display a pane on any given layer.  The pane lifetime in this class is
  // only during display, once dismissed, the pane is forgotten completely.
  void displayPane(PaneLayer paneLayer, PanePtr const& pane, DismissCallback onDismiss = {});

  bool isDisplayed(PanePtr const& pane) const;

  // Dismiss a given displayed pane.  Pane must already be displayed.
  void dismissPane(PanePtr const& pane);

  // Dismisses all panes in the given layers.
  void dismissAllPanes(Set<PaneLayer> const& paneLayers);
  void dismissAllPanes();

  PanePtr topPane(Set<PaneLayer> const& paneLayers) const;
  PanePtr topPane() const;

  // Brign an already displayed pane to the top of its layer.
  void bringToTop(PanePtr const& pane);

  // Position a pane adjacent to an anchor pane in a direction where
  // it will fit on the screen
  void bringPaneAdjacent(PanePtr const& anchor, PanePtr const& adjacent, int gap);

  PanePtr getPaneAt(Set<PaneLayer> const& paneLayers, Vec2I const& position) const;
  PanePtr getPaneAt(Vec2I const& position) const;
  List<PanePtr> getAllPanes();

  void setBackgroundWidget(WidgetPtr bg);

  void dismissWhere(function<bool(PanePtr const&)> func);

  // Returns the pane that has captured the keyboard, if any.
  PanePtr keyboardCapturedPane() const;
  // Returns true if the current pane that has captured the keyboard is
  // accepting text input.
  bool keyboardCapturedForTextInput() const;

  bool sendInputEvent(InputEvent const& event);

  void render();
  void update(float dt);

private:
  Vec2I windowSize() const;
  Vec2I calculatePaneOffset(PanePtr const& pane) const;
  Vec2I calculateNewInterfacePosition(PanePtr const& pane, float interfaceScaleRatio) const;
  bool dismiss(PanePtr const& pane);

  GuiContext* m_context;
  int m_prevInterfaceScale;

  // Map of each pane layer, where the 0th pane is the topmost pane in each layer.
  Map<PaneLayer, OrderedMap<PanePtr, DismissCallback>> m_displayedPanes;

  WidgetPtr m_backgroundWidget;

  float m_tooltipMouseoverRadius;
  Vec2I m_tooltipMouseOffset;
  GameTimer m_tooltipShowTimer;
  Vec2I m_tooltipLastMousePos;
  Vec2I m_tooltipInitialPosition;
  PanePtr m_activeTooltip;
  PanePtr m_tooltipParentPane;
};

}
