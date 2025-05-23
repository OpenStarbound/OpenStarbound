#pragma once

#include "StarPane.hpp"
#include "StarRegisteredPaneManager.hpp"

namespace Star {

STAR_CLASS(LabelWidget);
STAR_CLASS(ButtonWidget);
STAR_CLASS(ListWidget);

class ModsMenu : public Pane {
public:
  ModsMenu(RegisteredPaneManager<String>* manager);

  void update(float dt) override;

private:
  static String bestModName(JsonObject const& metadata, String const& sourcePath);

  void openLink();
  void openWorkshop();
  void enableMod();
  void populatePresetList(Json const& disabledAssetsPresets);

  StringList m_assetsSources;

  ListWidgetPtr m_modList;
  LabelWidgetPtr m_modName;
  LabelWidgetPtr m_modAuthor;
  LabelWidgetPtr m_modVersion;
  LabelWidgetPtr m_modPath;
  LabelWidgetPtr m_modDescription;

  ButtonWidgetPtr m_linkButton;
  ButtonWidgetPtr m_copyLinkButton;

  String m_modPresetName;
  Json m_presets;
  StringList m_disabledPaths;

  PanePtr m_presetSelectPane;

  RegisteredPaneManager<String>* m_paneManager;
};

}
