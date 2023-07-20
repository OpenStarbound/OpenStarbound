#ifndef STAR_MODS_MENU_HPP
#define STAR_MODS_MENU_HPP

#include "StarPane.hpp"

namespace Star {

STAR_CLASS(LabelWidget);
STAR_CLASS(ButtonWidget);
STAR_CLASS(ListWidget);

class ModsMenu : public Pane {
public:
  ModsMenu();

  void update(float dt) override;

private:
  static String bestModName(JsonObject const& metadata, String const& sourcePath);

  void openLink();
  void openWorkshop();

  StringList m_assetsSources;

  ListWidgetPtr m_modList;
  LabelWidgetPtr m_modName;
  LabelWidgetPtr m_modAuthor;
  LabelWidgetPtr m_modVersion;
  LabelWidgetPtr m_modPath;
  LabelWidgetPtr m_modDescription;

  ButtonWidgetPtr m_linkButton;
  ButtonWidgetPtr m_copyLinkButton;
};

}

#endif
