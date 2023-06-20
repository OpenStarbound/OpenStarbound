#ifndef STAR_DESKTOP_SERVICE_PC_STEAM_HPP
#define STAR_DESKTOP_SERVICE_PC_STEAM_HPP

#include "StarPlatformServices_pc.hpp"

namespace Star {

STAR_CLASS(SteamDesktopService);

class SteamDesktopService : public DesktopService {
public:
  SteamDesktopService(PcPlatformServicesStatePtr state);

  void openUrl(String const& url) override;
};

}

#endif
