#ifndef STAR_OPTIONS_MENU_HPP
#define STAR_OPTIONS_MENU_HPP

#include "StarPane.hpp"
#include "StarConfiguration.hpp"
#include "StarMainInterfaceTypes.hpp"

namespace Star {

STAR_CLASS(SliderBarWidget);
STAR_CLASS(ButtonWidget);
STAR_CLASS(LabelWidget);
STAR_CLASS(VoiceSettingsMenu);
STAR_CLASS(KeybindingsMenu);
STAR_CLASS(GraphicsMenu);
STAR_CLASS(BindingsMenu);
STAR_CLASS(OptionsMenu);

class OptionsMenu : public Pane {
public:
  OptionsMenu(PaneManager* manager);

  virtual void show() override;

  void toggleFullscreen();

private:
  static StringList const ConfigKeys;

  void initConfig();

  void updateInstrumentVol();
  void updateSFXVol();
  void updateMusicVol();
  void updateTutorialMessages();
  void updateClientIPJoinable();
  void updateClientP2PJoinable();
  void updateAllowAssetsMismatch();

  void syncGuiToConf();

  void displayControls();
  void displayVoiceSettings();
  void displayModBindings();
  void displayGraphics();

  SliderBarWidgetPtr m_instrumentSlider;
  SliderBarWidgetPtr m_sfxSlider;
  SliderBarWidgetPtr m_musicSlider;
  ButtonWidgetPtr m_tutorialMessagesButton;
  ButtonWidgetPtr m_interactiveHighlightButton;
  ButtonWidgetPtr m_clientIPJoinableButton;
  ButtonWidgetPtr m_clientP2PJoinableButton;
  ButtonWidgetPtr m_allowAssetsMismatchButton;

  LabelWidgetPtr m_instrumentLabel;
  LabelWidgetPtr m_sfxLabel;
  LabelWidgetPtr m_musicLabel;
  LabelWidgetPtr m_p2pJoinableLabel;

  //TODO: add instrument range (or just use one range for all 3, it's kinda silly.)
  Vec2I m_sfxRange;
  Vec2I m_musicRange;

  JsonObject m_origConfig;
  JsonObject m_localChanges;

  VoiceSettingsMenuPtr m_voiceSettingsMenu;
  BindingsMenuPtr m_modBindingsMenu;
  KeybindingsMenuPtr m_keybindingsMenu;
  GraphicsMenuPtr m_graphicsMenu;
  PaneManager* m_paneManager;
};

}

#endif
