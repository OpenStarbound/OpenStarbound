#pragma once

#include "StarPane.hpp"
#include "StarMainInterfaceTypes.hpp"
#include "StarUniverseClient.hpp"

namespace Star {

STAR_CLASS(GraphicsMenu);
STAR_CLASS(ShadersMenu);

class GraphicsMenu : public Pane {
public:
  GraphicsMenu(PaneManager* manager,UniverseClientPtr client);

  void show() override;
  void dismissed() override;

  void toggleFullscreen();

private:
  static StringList const ConfigKeys;

  void initConfig();
  void syncGui();

  void apply();
  void applyWindowSettings();
  
  void displayShaders();

  List<Vec2U> m_resList;
  List<int> m_interfaceScaleList;
  List<float> m_zoomList;
  List<float> m_cameraSpeedList;

  JsonObject m_localChanges;
  
  ShadersMenuPtr m_shadersMenu;
  PaneManager* m_paneManager;
};

}
