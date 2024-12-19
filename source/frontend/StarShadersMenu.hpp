#pragma once

#include "StarBaseScriptPane.hpp"
#include "StarUniverseClient.hpp"

namespace Star {

STAR_CLASS(ShadersMenu);

class ShadersMenu : public BaseScriptPane {
public:
  ShadersMenu(Json const& config, UniverseClientPtr client);

  virtual void show() override;
  void displayed() override;
  void dismissed() override;

private:
  UniverseClientPtr m_client;
};

}
