#pragma once

#include "StarPane.hpp"
#include "StarWarping.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarBookmarkInterface.hpp"

namespace Star {

STAR_CLASS(UniverseClient);
STAR_CLASS(PaneManager);

class TeleportDialog : public Pane {
public:
  TeleportDialog(UniverseClientPtr client,
      PaneManager* paneManager,
      Json config,
      EntityId sourceEntityId,
      TeleportBookmark currentLocation);

  void tick(float dt) override;

  void selectDestination();
  void teleport();
  void editBookmark();

private:
  EntityId m_sourceEntityId;
  UniverseClientPtr m_client;
  PaneManager* m_paneManager;
  List<pair<WarpAction, bool>> m_destinations;
  TeleportBookmark m_currentLocation;
};

}
