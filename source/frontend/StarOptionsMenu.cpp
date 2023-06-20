#include "StarOptionsMenu.hpp"
#include "StarRoot.hpp"
#include "StarGuiReader.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarSliderBar.hpp"
#include "StarLabelWidget.hpp"
#include "StarAssets.hpp"
#include "StarKeybindingsMenu.hpp"
#include "StarGraphicsMenu.hpp"

namespace Star {

OptionsMenu::OptionsMenu(PaneManager* manager)
  : m_sfxRange(0, 100), m_musicRange(0, 100), m_paneManager(manager) {
  auto root = Root::singletonPtr();
  auto assets = root->assets();

  GuiReader reader;

  reader.registerCallback("sfxSlider", [=](Widget*) {
      updateSFXVol();
    });
  reader.registerCallback("musicSlider", [=](Widget*) {
      updateMusicVol();
    });
  reader.registerCallback("acceptButton", [=](Widget*) {
      for (auto k : ConfigKeys)
        root->configuration()->set(k, m_localChanges.get(k));

      dismiss();
    });
  reader.registerCallback("tutorialMessagesCheckbox", [=](Widget*) {
      updateTutorialMessages();
    });
  reader.registerCallback("clientIPJoinableCheckbox", [=](Widget*) {
      updateClientIPJoinable();
    });
  reader.registerCallback("clientP2PJoinableCheckbox", [=](Widget*) {
      updateClientP2PJoinable();
    });
  reader.registerCallback("allowAssetsMismatchCheckbox", [=](Widget*) {
      updateAllowAssetsMismatch();
    });
  reader.registerCallback("backButton", [=](Widget*) {
      dismiss();
    });
  reader.registerCallback("showKeybindings", [=](Widget*) {
      displayControls();
    });
  reader.registerCallback("showGraphics", [=](Widget*) {
      displayGraphics();
    });

  reader.construct(assets->json("/interface/optionsmenu/optionsmenu.config:paneLayout"), this);

  m_sfxSlider = fetchChild<SliderBarWidget>("sfxSlider");
  m_musicSlider = fetchChild<SliderBarWidget>("musicSlider");
  m_tutorialMessagesButton = fetchChild<ButtonWidget>("tutorialMessagesCheckbox");
  m_clientIPJoinableButton = fetchChild<ButtonWidget>("clientIPJoinableCheckbox");
  m_clientP2PJoinableButton = fetchChild<ButtonWidget>("clientP2PJoinableCheckbox");
  m_allowAssetsMismatchButton = fetchChild<ButtonWidget>("allowAssetsMismatchCheckbox");

  m_sfxLabel = fetchChild<LabelWidget>("sfxValueLabel");
  m_musicLabel = fetchChild<LabelWidget>("musicValueLabel");
  m_p2pJoinableLabel = fetchChild<LabelWidget>("clientP2PJoinableLabel");

  m_sfxSlider->setRange(m_sfxRange, assets->json("/interface/optionsmenu/optionsmenu.config:sfxDelta").toInt());
  m_musicSlider->setRange(m_musicRange, assets->json("/interface/optionsmenu/optionsmenu.config:musicDelta").toInt());

  m_keybindingsMenu = make_shared<KeybindingsMenu>();
  m_graphicsMenu = make_shared<GraphicsMenu>();

  initConfig();
}

void OptionsMenu::show() {
  initConfig();
  syncGuiToConf();

  Pane::show();
}

void OptionsMenu::toggleFullscreen() {
  m_graphicsMenu->toggleFullscreen();

  syncGuiToConf();
}

StringList const OptionsMenu::ConfigKeys = {
  "sfxVol",
  "musicVol",
  "tutorialMessages",
  "clientIPJoinable",
  "clientP2PJoinable",
  "allowAssetsMismatch"
};

void OptionsMenu::initConfig() {
  auto configuration = Root::singleton().configuration();

  for (auto k : ConfigKeys) {
    m_origConfig[k] = configuration->get(k);
    m_localChanges[k] = configuration->get(k);
  }
}

void OptionsMenu::updateSFXVol() {
  m_localChanges.set("sfxVol", m_sfxSlider->val());
  Root::singleton().configuration()->set("sfxVol", m_sfxSlider->val());
  m_sfxLabel->setText(toString(m_sfxSlider->val()));
}

void OptionsMenu::updateMusicVol() {
  m_localChanges.set("musicVol", {m_musicSlider->val()});
  Root::singleton().configuration()->set("musicVol", m_musicSlider->val());
  m_musicLabel->setText(toString(m_musicSlider->val()));
}


void OptionsMenu::updateTutorialMessages() {
  m_localChanges.set("tutorialMessages", m_tutorialMessagesButton->isChecked());
  Root::singleton().configuration()->set("tutorialMessages", m_tutorialMessagesButton->isChecked());
}

void OptionsMenu::updateClientIPJoinable() {
  m_localChanges.set("clientIPJoinable", m_clientIPJoinableButton->isChecked());
  Root::singleton().configuration()->set("clientIPJoinable", m_clientIPJoinableButton->isChecked());
}

void OptionsMenu::updateClientP2PJoinable() {
  m_localChanges.set("clientP2PJoinable", m_clientP2PJoinableButton->isChecked());
  Root::singleton().configuration()->set("clientP2PJoinable", m_clientP2PJoinableButton->isChecked());
}

void OptionsMenu::updateAllowAssetsMismatch() {
  m_localChanges.set("allowAssetsMismatch", m_allowAssetsMismatchButton->isChecked());
  Root::singleton().configuration()->set("allowAssetsMismatch", m_allowAssetsMismatchButton->isChecked());
}

void OptionsMenu::syncGuiToConf() {
  m_sfxSlider->setVal(m_localChanges.get("sfxVol").toInt(), false);
  m_sfxLabel->setText(toString(m_sfxSlider->val()));

  m_musicSlider->setVal(m_localChanges.get("musicVol").toInt(), false);
  m_musicLabel->setText(toString(m_musicSlider->val()));

  m_tutorialMessagesButton->setChecked(m_localChanges.get("tutorialMessages").toBool());
  m_clientIPJoinableButton->setChecked(m_localChanges.get("clientIPJoinable").toBool());
  m_clientP2PJoinableButton->setChecked(m_localChanges.get("clientP2PJoinable").toBool());
  m_allowAssetsMismatchButton->setChecked(m_localChanges.get("allowAssetsMismatch").toBool());

  auto appController = GuiContext::singleton().applicationController();
  if (!appController->p2pNetworkingService()) {
    m_p2pJoinableLabel->setColor(Color::DarkGray);
    m_clientP2PJoinableButton->setEnabled(false);
    m_clientP2PJoinableButton->setChecked(false);
  }
}

void OptionsMenu::displayControls() {
  m_paneManager->displayPane(PaneLayer::ModalWindow, m_keybindingsMenu);
}

void OptionsMenu::displayGraphics() {
  m_paneManager->displayPane(PaneLayer::ModalWindow, m_graphicsMenu);
}

}
