#include "StarModsMenu.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarListWidget.hpp"

namespace Star {

ModsMenu::ModsMenu() {
  auto assets = Root::singleton().assets();

  GuiReader reader;
  reader.registerCallback("linkbutton", bind(&ModsMenu::openLink, this));
  reader.registerCallback("workshopbutton", bind(&ModsMenu::openWorkshop, this));
  reader.construct(assets->json("/interface/modsmenu/modsmenu.config:paneLayout"), this);

  m_assetsSources = assets->assetSources();
  m_modList = fetchChild<ListWidget>("mods.list");
  for (auto const& assetsSource : m_assetsSources) {
    auto modName = m_modList->addItem()->fetchChild<LabelWidget>("name");
    modName->setText(bestModName(assets->assetSourceMetadata(assetsSource), assetsSource));
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
}

void ModsMenu::update() {
  Pane::update();

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
    m_modVersion->setText(assetsSourceMetadata.value("version", "No Version Set").toString());
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
