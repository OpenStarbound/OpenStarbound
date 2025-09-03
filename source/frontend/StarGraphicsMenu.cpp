#include "StarGraphicsMenu.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarConfiguration.hpp"
#include "StarGuiReader.hpp"
#include "StarListWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarSliderBar.hpp"
#include "StarButtonWidget.hpp"
#include "StarOrderedSet.hpp"
#include "StarJsonExtra.hpp"
#include "StarShadersMenu.hpp"

namespace Star {

GraphicsMenu::GraphicsMenu(PaneManager* manager,UniverseClientPtr client)
  : m_paneManager(manager) {
  GuiReader reader;
  reader.registerCallback("cancel",
      [&](Widget*) {
        dismiss();
      });
  reader.registerCallback("accept",
      [&](Widget*) {
        apply();
        applyWindowSettings();
      });
  reader.registerCallback("resSlider", [=](Widget*) {
      Vec2U res = m_resList[fetchChild<SliderBarWidget>("resSlider")->val()];
      m_localChanges.set("fullscreenResolution", jsonFromVec2U(res));
      syncGui();
    });
  reader.registerCallback("interfaceScaleSlider", [=](Widget*) {
      auto interfaceScaleSlider = fetchChild<SliderBarWidget>("interfaceScaleSlider");
      m_localChanges.set("interfaceScale", m_interfaceScaleList[interfaceScaleSlider->val()]);
      syncGui();
    });
  reader.registerCallback("zoomSlider", [=](Widget*) {
      auto zoomSlider = fetchChild<SliderBarWidget>("zoomSlider");
      m_localChanges.set("zoomLevel", m_zoomList[zoomSlider->val()]);
      Root::singleton().configuration()->set("zoomLevel", m_zoomList[zoomSlider->val()]);
      syncGui();
    });
  reader.registerCallback("cameraSpeedSlider", [=](Widget*) {
      auto cameraSpeedSlider = fetchChild<SliderBarWidget>("cameraSpeedSlider");
      m_localChanges.set("cameraSpeedFactor", m_cameraSpeedList[cameraSpeedSlider->val()]);
      Root::singleton().configuration()->set("cameraSpeedFactor", m_cameraSpeedList[cameraSpeedSlider->val()]);
      syncGui();
    });
  reader.registerCallback("speechBubbleCheckbox", [=](Widget*) {
      auto button = fetchChild<ButtonWidget>("speechBubbleCheckbox");
      m_localChanges.set("speechBubbles", button->isChecked());
      Root::singleton().configuration()->set("speechBubbles", button->isChecked());
      syncGui();
    });
  reader.registerCallback("interactiveHighlightCheckbox", [=](Widget*) {
      auto button = fetchChild<ButtonWidget>("interactiveHighlightCheckbox");
      m_localChanges.set("interactiveHighlight", button->isChecked());
      Root::singleton().configuration()->set("interactiveHighlight", button->isChecked());
      syncGui();
    });
  reader.registerCallback("fullscreenCheckbox", [=](Widget*) {
      bool checked = fetchChild<ButtonWidget>("fullscreenCheckbox")->isChecked();
      m_localChanges.set("fullscreen", checked);
      if (checked)
        m_localChanges.set("borderless", !checked);
      syncGui();
    });
  reader.registerCallback("borderlessCheckbox", [=](Widget*) {
      bool checked = fetchChild<ButtonWidget>("borderlessCheckbox")->isChecked();
      m_localChanges.set("borderless", checked);
      if (checked)
        m_localChanges.set("fullscreen", !checked);
      syncGui();
    });
  reader.registerCallback("textureLimitCheckbox", [=](Widget*) {
      m_localChanges.set("limitTextureAtlasSize", fetchChild<ButtonWidget>("textureLimitCheckbox")->isChecked());
      syncGui();
    });
  reader.registerCallback("multiTextureCheckbox", [=](Widget*) {
      m_localChanges.set("useMultiTexturing", fetchChild<ButtonWidget>("multiTextureCheckbox")->isChecked());
      syncGui();
    });
  reader.registerCallback("antiAliasingCheckbox", [=](Widget*) {
    bool checked = fetchChild<ButtonWidget>("antiAliasingCheckbox")->isChecked();
    m_localChanges.set("antiAliasing", checked);
    Root::singleton().configuration()->set("antiAliasing", checked);
    syncGui();
  });
  reader.registerCallback("hardwareCursorCheckbox", [=](Widget*) {
    bool checked = fetchChild<ButtonWidget>("hardwareCursorCheckbox")->isChecked();
    m_localChanges.set("hardwareCursor", checked);
    Root::singleton().configuration()->set("hardwareCursor", checked);
    GuiContext::singleton().applicationController()->setCursorHardware(checked);
  });
  reader.registerCallback("monochromeCheckbox", [=](Widget*) {
      bool checked = fetchChild<ButtonWidget>("monochromeCheckbox")->isChecked();
      m_localChanges.set("monochromeLighting", checked);
      Root::singleton().configuration()->set("monochromeLighting", checked);
      syncGui();
    });
  reader.registerCallback("newLightingCheckbox", [=](Widget*) {
    bool checked = fetchChild<ButtonWidget>("newLightingCheckbox")->isChecked();
    m_localChanges.set("newLighting", checked);
    Root::singleton().configuration()->set("newLighting", checked);
    syncGui();
  });
  reader.registerCallback("showShadersMenu", [=](Widget*) {
      displayShaders();
    });

  auto assets = Root::singleton().assets();

  auto config = assets->json("/interface/windowconfig/graphicsmenu.config");
  Json paneLayout = config.get("paneLayout");

  m_interfaceScaleList = jsonToFloatList(assets->json("/interface/windowconfig/graphicsmenu.config:interfaceScaleList"));
  m_resList = jsonToVec2UList(assets->json("/interface/windowconfig/graphicsmenu.config:resolutionList"));
  m_zoomList = jsonToFloatList(assets->json("/interface/windowconfig/graphicsmenu.config:zoomList"));
  m_cameraSpeedList = jsonToFloatList(assets->json("/interface/windowconfig/graphicsmenu.config:cameraSpeedList"));

  reader.construct(paneLayout, this);

  fetchChild<SliderBarWidget>("interfaceScaleSlider")->setRange(0, m_interfaceScaleList.size() - 1, 1);
  fetchChild<SliderBarWidget>("resSlider")->setRange(0, m_resList.size() - 1, 1);
  fetchChild<SliderBarWidget>("zoomSlider")->setRange(0, m_zoomList.size() - 1, 1);
  fetchChild<SliderBarWidget>("cameraSpeedSlider")->setRange(0, m_cameraSpeedList.size() - 1, 1);

  initConfig();
  syncGui();
  
  m_shadersMenu = make_shared<ShadersMenu>(assets->json(config.getString("shadersPanePath", "/interface/opensb/shaders/shaders.config")), client);
}

void GraphicsMenu::show() {
  Pane::show();
  initConfig();
  syncGui();
}

void GraphicsMenu::dismissed() {
  Pane::dismissed();
}

void GraphicsMenu::toggleFullscreen() {  
  bool fullscreen = m_localChanges.get("fullscreen").toBool();
  bool borderless = m_localChanges.get("borderless").toBool();

  m_localChanges.set("fullscreen", !(fullscreen || borderless));
  Root::singleton().configuration()->set("fullscreen", !(fullscreen || borderless));

  m_localChanges.set("borderless", false);
  Root::singleton().configuration()->set("borderless", false);

  applyWindowSettings();
  syncGui();
}

StringList const GraphicsMenu::ConfigKeys = {
  "fullscreenResolution",
  "interfaceScale",
  "zoomLevel",
  "cameraSpeedFactor",
  "speechBubbles",
  "interactiveHighlight",
  "fullscreen",
  "borderless",
  "limitTextureAtlasSize",
  "useMultiTexturing",
  "antiAliasing",
  "hardwareCursor",
  "monochromeLighting",
  "newLighting"
};

void GraphicsMenu::initConfig() {
  auto configuration = Root::singleton().configuration();

  for (auto key : ConfigKeys) {
    m_localChanges.set(key, configuration->get(key));
  }
}

void GraphicsMenu::syncGui() {
  Vec2U res = jsonToVec2U(m_localChanges.get("fullscreenResolution"));
  auto resSlider = fetchChild<SliderBarWidget>("resSlider");
  auto resIt = std::lower_bound(m_resList.begin(), m_resList.end(), res, [&](Vec2U const& a, Vec2U const& b) {
      return a[0] * a[1] < b[0] * b[1]; // sort by number of pixels
    });
  if (resIt != m_resList.end()) {
    size_t resIndex = resIt - m_resList.begin();
    resIndex = std::min(resIndex, m_resList.size() - 1);
    resSlider->setVal(resIndex, false);
  } else {
    resSlider->setVal(m_resList.size() - 1);
  }
  fetchChild<LabelWidget>("resValueLabel")->setText(strf("{}x{}", res[0], res[1]));

  auto interfaceScaleSlider = fetchChild<SliderBarWidget>("interfaceScaleSlider");
  auto interfaceScale = m_localChanges.get("interfaceScale").optFloat().value();
  auto interfaceScaleIt = std::lower_bound(m_interfaceScaleList.begin(), m_interfaceScaleList.end(), interfaceScale);
  if (interfaceScaleIt != m_interfaceScaleList.end()) {
    size_t scaleIndex = interfaceScaleIt - m_interfaceScaleList.begin();
    interfaceScaleSlider->setVal(std::min(scaleIndex, m_interfaceScaleList.size() - 1), false);
  } else {
    interfaceScaleSlider->setVal(m_interfaceScaleList.size() - 1);
  }
  fetchChild<LabelWidget>("interfaceScaleValueLabel")->setText(interfaceScale != 0 ? toString(interfaceScale) : "AUTO");

  auto zoomSlider = fetchChild<SliderBarWidget>("zoomSlider");
  auto zoomLevel = m_localChanges.get("zoomLevel").toFloat();
  auto zoomIt = std::lower_bound(m_zoomList.begin(), m_zoomList.end(), zoomLevel);
  if (zoomIt != m_zoomList.end()) {
    size_t zoomIndex = zoomIt - m_zoomList.begin();
    zoomSlider->setVal(std::min(zoomIndex, m_zoomList.size() - 1), false);
  } else {
    zoomSlider->setVal(m_zoomList.size() - 1);
  }
  fetchChild<LabelWidget>("zoomValueLabel")->setText(strf("{}x", zoomLevel));

  auto cameraSpeedSlider = fetchChild<SliderBarWidget>("cameraSpeedSlider");
  auto cameraSpeedFactor = m_localChanges.get("cameraSpeedFactor").toFloat();
  auto speedIt = std::lower_bound(m_cameraSpeedList.begin(), m_cameraSpeedList.end(), cameraSpeedFactor);
  if (speedIt != m_cameraSpeedList.end()) {
    size_t speedIndex = speedIt - m_cameraSpeedList.begin();
    cameraSpeedSlider->setVal(std::min(speedIndex, m_cameraSpeedList.size() - 1), false);
  } else {
    cameraSpeedSlider->setVal(m_cameraSpeedList.size() - 1);
  }
  fetchChild<LabelWidget>("cameraSpeedValueLabel")->setText(strf("{}x", cameraSpeedFactor));

  fetchChild<ButtonWidget>("speechBubbleCheckbox")->setChecked(m_localChanges.get("speechBubbles").toBool());
  fetchChild<ButtonWidget>("interactiveHighlightCheckbox")->setChecked(m_localChanges.get("interactiveHighlight").toBool());
  fetchChild<ButtonWidget>("fullscreenCheckbox")->setChecked(m_localChanges.get("fullscreen").toBool());
  fetchChild<ButtonWidget>("borderlessCheckbox")->setChecked(m_localChanges.get("borderless").toBool());
  fetchChild<ButtonWidget>("textureLimitCheckbox")->setChecked(m_localChanges.get("limitTextureAtlasSize").toBool());
  fetchChild<ButtonWidget>("multiTextureCheckbox")->setChecked(m_localChanges.get("useMultiTexturing").optBool().value(true));
  fetchChild<ButtonWidget>("antiAliasingCheckbox")->setChecked(m_localChanges.get("antiAliasing").toBool());
  fetchChild<ButtonWidget>("monochromeCheckbox")->setChecked(m_localChanges.get("monochromeLighting").toBool());
  fetchChild<ButtonWidget>("newLightingCheckbox")->setChecked(m_localChanges.get("newLighting").optBool().value(true));
  fetchChild<ButtonWidget>("hardwareCursorCheckbox")->setChecked(m_localChanges.get("hardwareCursor").toBool());
}

void GraphicsMenu::apply() {
  auto configuration = Root::singleton().configuration();
  for (auto p : m_localChanges) {
    configuration->set(p.first, p.second);
  }
}

void GraphicsMenu::displayShaders() {
  m_paneManager->displayPane(PaneLayer::ModalWindow, m_shadersMenu);
}

void GraphicsMenu::applyWindowSettings() {
  auto configuration = Root::singleton().configuration();
  auto appController = GuiContext::singleton().applicationController();
  if (configuration->get("fullscreen").toBool())
    appController->setFullscreenWindow(jsonToVec2U(configuration->get("fullscreenResolution")));
  else if (configuration->get("borderless").toBool())
    appController->setBorderlessWindow();
  else if (configuration->get("maximized").toBool())
    appController->setMaximizedWindow();
  else
    appController->setNormalWindow(jsonToVec2U(configuration->get("windowedResolution")));
}

}
