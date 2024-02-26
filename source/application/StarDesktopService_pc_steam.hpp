#pragma once

#include "StarPlatformServices_pc.hpp"

namespace Star {

STAR_CLASS(SteamDesktopService);

class SteamDesktopService : public DesktopService {
public:
  SteamDesktopService(PcPlatformServicesStatePtr state);

  void openUrl(String const& url) override;
};

}
