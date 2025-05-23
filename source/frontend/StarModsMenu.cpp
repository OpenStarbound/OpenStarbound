#include "StarModsMenu.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarListWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarJsonExtra.hpp"
#include "StarUuid.hpp"

namespace Star {

ModsMenu::ModsMenu(RegisteredPaneManager<String>* manager) {
  auto assets = Root::singleton().assets();

  GuiReader reader;
  GuiReader presetReader;
  GuiReader saveReader;
  m_paneManager = manager;

  m_presetSelectPane = make_shared<Pane>();

  m_modPresetName = Root::singleton().configuration()->get("selectedDisabledAssetsPreset", "").toString();
  m_presets = Root::singleton().configuration()->get("disabledAssetsPresets", JsonObject());

  auto saveDialog = make_shared<Pane>();
  saveReader.registerCallback("save", [=](Widget*) { 
    auto textBox = saveDialog->fetchChild<TextBoxWidget>("name");
    if (textBox->getText().empty())
      return;

    m_modPresetName = textBox->getText();
    m_presets = m_presets.set(m_modPresetName, jsonFromStringList(m_disabledPaths));
    Root::singleton().configuration()->set("disabledAssetsPresets", m_presets);

    populatePresetList(m_presets);
    saveDialog->fetchChild<TextBoxWidget>("name")->setText("");
    saveDialog->dismiss(); 
  });
  saveReader.registerCallback("cancel", [=](Widget*) { 
    saveDialog->fetchChild<TextBoxWidget>("name")->setText("");
    saveDialog->dismiss(); 
  });
  saveReader.construct(Root::singleton().assets()->json("/interface/modsmenu/savedialog.config"), saveDialog.get());
  m_paneManager->registerPane("savePresetDialog", PaneLayer::ModalWindow, saveDialog);

  
  presetReader.registerCallback("savePreset", [=](Widget*) {
    auto savePresetDialog = m_paneManager->registeredPane("savePresetDialog");

    m_paneManager->displayRegisteredPane("savePresetDialog");
  });
  presetReader.construct(assets->json("/interface/modsmenu/presetlist.config"), m_presetSelectPane.get());

  auto presetList = m_presetSelectPane->fetchChild<ListWidget>("presetSelectArea.presetList");
  presetList->registerMemberCallback("delete", [=](Widget* widget) {
    if (auto const pos = presetList->selectedItem(); pos != NPos) {
      m_presets = m_presets.eraseKey(presetList->selectedWidget()->data().toString());
    }
    Root::singleton().configuration()->set("disabledAssetsPresets", m_presets);

    populatePresetList(m_presets);
  });
  presetList->setCallback([=](Widget* widget) {
    if (auto selectedItem = presetList->selectedWidget()) {
      if (selectedItem->findChild<ButtonWidget>("delete")->isHovered())
        return;

      m_modPresetName = selectedItem->data().toString();
      Root::singleton().configuration()->set("selectedDisabledAssetsPreset", m_modPresetName);
      m_disabledPaths = jsonToStringList(m_presets.get(m_modPresetName, JsonArray()));
      for (size_t i = 0; i != m_modList->listSize(); ++i) {
        auto item = m_modList->itemAt(i);
        auto name = item->data().toString();
        item->findChild<ButtonWidget>("enabled")->setChecked(!m_disabledPaths.contains(name));
      }
    }
  });
  m_paneManager->registerPane("presetSelect", PaneLayer::Hud, m_presetSelectPane);


  reader.registerCallback("linkbutton", bind(&ModsMenu::openLink, this));
  reader.registerCallback("workshopbutton", bind(&ModsMenu::openWorkshop, this));
  reader.registerCallback("load", [=] (Widget* widget) {
    //TODO: Kae please help >3<
    auto& guiContext = GuiContext::singleton();
    guiContext.applicationController()->quit();
  });
  reader.construct(assets->json("/interface/modsmenu/modsmenu.config:paneLayout"), this);

  m_assetsSources = assets->assetSources();
  m_modList = fetchChild<ListWidget>("mods.list");
  m_modList->registerMemberCallback("enabled", bind(&ModsMenu::enableMod, this));
  StringList baseMods = {"base", "opensb", "user"};
  for (auto const& assetsSource : m_assetsSources) {
    auto item = m_modList->addItem();
    auto modName = item->fetchChild<LabelWidget>("name");
    modName->setText(bestModName(assets->assetSourceMetadata(assetsSource), assetsSource));
    item->setData(assets->assetName(assetsSource));
    item->findChild<ButtonWidget>("enabled")->setChecked(assets->assetSourceEnabled(assetsSource));
    if (baseMods.contains(assets->assetName(assetsSource)))
      item->findChild<ButtonWidget>("enabled")->disable();
  }

  m_modName = findChild<LabelWidget>("modname");
  m_modAuthor = findChild<LabelWidget>("modauthor");
  m_modVersion = findChild<LabelWidget>("modversion");
  m_modPath = findChild<LabelWidget>("modpath");
  m_modDescription = findChild<LabelWidget>("moddescription");

  m_linkButton = fetchChild<ButtonWidget>("linkbutton");
  m_copyLinkButton = fetchChild<ButtonWidget>("copylinkbutton");

  auto linkLabel = fetchChild<LabelWidget>("linklabel");
  auto copyLinkLabel = fetchChild<LabelWidget>("copylinklabel");
  auto workshopLinkButton = fetchChild<ButtonWidget>("workshopbutton");

  auto& guiContext = GuiContext::singleton();
  bool hasDesktopService = (bool)guiContext.applicationController()->desktopService();

  workshopLinkButton->setEnabled(hasDesktopService);

  m_linkButton->setVisibility(hasDesktopService);
  m_copyLinkButton->setVisibility(!hasDesktopService);

  m_linkButton->setEnabled(false);
  m_copyLinkButton->setEnabled(false);

  linkLabel->setVisibility(hasDesktopService);
  copyLinkLabel->setVisibility(!hasDesktopService);

  populatePresetList(m_presets);
}

void ModsMenu::populatePresetList(Json const& disabledAssetsPresets) {
  auto presetList = m_presetSelectPane->fetchChild<ListWidget>("presetSelectArea.presetList");
  presetList->clear();
  for (auto const& preset : disabledAssetsPresets.iterateObject()) {
    auto item = presetList->addItem();
    item->fetchChild<LabelWidget>("name")->setText(preset.first);
    item->setData(preset.first);

    if (preset.first == m_modPresetName)
      presetList->setSelectedWidget(item);
  }
}

void ModsMenu::update(float dt) {
  Pane::update(dt);

  size_t selectedItem = m_modList->selectedItem();
  if (selectedItem == NPos) {
    m_modName->setText("");
    m_modAuthor->setText("");
    m_modVersion->setText("");
    m_modPath->setText("");
    m_modDescription->setText("");

  } else {
    String assetsSource = m_assetsSources.at(selectedItem);
    JsonObject assetsSourceMetadata = Root::singleton().assets()->assetSourceMetadata(assetsSource);

    m_modName->setText(bestModName(assetsSourceMetadata, assetsSource));
    m_modAuthor->setText(assetsSourceMetadata.value("author", "No Author Set").toString());
    m_modVersion->setText(assetsSourceMetadata.value("version", "No Version Set").printString());
    m_modPath->setText(assetsSource);
    m_modDescription->setText(assetsSourceMetadata.value("description", "").toString());

    String link = assetsSourceMetadata.value("link", "").toString();

    m_linkButton->setEnabled(!link.empty());
    m_copyLinkButton->setEnabled(!link.empty());
  }
}

String ModsMenu::bestModName(JsonObject const& metadata, String const& sourcePath) {
  if (auto ptr = metadata.ptr("friendlyName"))
    return ptr->toString();
  if (auto ptr = metadata.ptr("name"))
    return ptr->toString();
  String baseName = File::baseName(sourcePath);
  if (baseName.contains("."))
    baseName.rextract(".");
  return baseName;
}

void ModsMenu::enableMod() {
  size_t selectedItem = m_modList->selectedItem();
  if (selectedItem == NPos)
    return;
  String assetsSource = m_assetsSources.at(selectedItem);

  String name = Root::singleton().assets()->assetName(assetsSource);
  bool enabled = m_modList->selectedWidget()->fetchChild<ButtonWidget>("enabled")->isChecked();

  if (enabled) {
    m_disabledPaths.remove(name);
  } else if (!m_disabledPaths.contains(name)) {
    m_disabledPaths.append(name);
  }
}

void ModsMenu::openLink() {
  size_t selectedItem = m_modList->selectedItem();
  if (selectedItem == NPos)
    return;

  String assetsSource = m_assetsSources.at(selectedItem);
  JsonObject assetsSourceMetadata = Root::singleton().assets()->assetSourceMetadata(assetsSource);
  String link = assetsSourceMetadata.value("link", "").toString();

  if (link.empty())
    return;

  auto& guiContext = GuiContext::singleton();
  if (auto desktopService = guiContext.applicationController()->desktopService())
    desktopService->openUrl(link);
  else
    guiContext.setClipboard(link);
}

void ModsMenu::openWorkshop() {
  auto assets = Root::singleton().assets();
  auto& guiContext = GuiContext::singleton();
  if (auto desktopService = guiContext.applicationController()->desktopService())
    desktopService->openUrl(assets->json("/interface/modsmenu/modsmenu.config:workshopLink").toString());
}

}
