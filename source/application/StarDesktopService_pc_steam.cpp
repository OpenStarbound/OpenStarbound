#include "StarDesktopService_pc_steam.hpp"

namespace Star {

SteamDesktopService::SteamDesktopService(PcPlatformServicesStatePtr) {}

void SteamDesktopService::openUrl(String const& url) {
  SteamFriends()->ActivateGameOverlayToWebPage(url.utf8Ptr());
}

}
