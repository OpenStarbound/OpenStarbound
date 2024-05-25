#pragma once

#include "StarPane.hpp"

namespace Star {

STAR_CLASS(GraphicsMenu);

class GraphicsMenu : public Pane {
public:
  GraphicsMenu();

  void show() override;
  void dismissed() override;

  void toggleFullscreen();

private:
  static StringList const ConfigKeys;

  void initConfig();
  void syncGui();

  void apply();
  void applyWindowSettings();

  List<Vec2U> m_resList;
  List<int> m_interfaceScaleList;
  List<float> m_zoomList;
  List<float> m_cameraSpeedList;

  JsonObject m_localChanges;
};

}
