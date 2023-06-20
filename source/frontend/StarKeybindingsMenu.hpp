#ifndef STAR_KEYBINDINGS_MENU_HPP
#define STAR_KEYBINDINGS_MENU_HPP

#include "StarPane.hpp"

namespace Star {

STAR_CLASS(TabSetWidget);
STAR_CLASS(ListWidget);
STAR_CLASS(KeybindingsMenu);

class KeybindingsMenu : public Pane {
public:
  KeybindingsMenu();

  // We need to handle our own Esc dismissal
  KeyboardCaptureMode keyboardCaptured() const override;
  bool sendEvent(InputEvent const& event) override;

  void show() override;
  void dismissed() override;

private:
  void buildListsFromConfig();
  bool activateBinding(Widget* widget);
  void setKeybinding(KeyChord desc);
  void clearActive();
  void exitActiveMode();
  void apply();
  void revert();
  void resetDefaults();

  Widget* m_activeKeybinding;

  Map<Widget*, InterfaceAction> m_childToAction;
  TabSetWidgetPtr m_tabSet;
  ListWidgetPtr m_playerList;
  ListWidgetPtr m_toolBarList;
  ListWidgetPtr m_gameList;

  Json m_origConfiguration;

  size_t m_maxBindings;
  KeyMod m_currentMods;
};

}

#endif
