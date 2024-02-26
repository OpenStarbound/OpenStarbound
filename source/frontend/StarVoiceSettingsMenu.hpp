#pragma once

#include "StarBaseScriptPane.hpp"

namespace Star {

STAR_CLASS(VoiceSettingsMenu);

class VoiceSettingsMenu : public BaseScriptPane {
public:
  VoiceSettingsMenu(Json const& config);

  virtual void show() override;
  void displayed() override;
  void dismissed() override;

private:

};

}
