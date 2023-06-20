#include "StarTeleportDialog.hpp"
#include "StarWorldClient.hpp"
#include "StarUniverseClient.hpp"
#include "StarClientContext.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarTeamClient.hpp"
#include "StarPlayer.hpp"
#include "StarQuestManager.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarGuiReader.hpp"
#include "StarPaneManager.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarListWidget.hpp"

namespace Star {

TeleportDialog::TeleportDialog(UniverseClientPtr client,
    PaneManager* paneManager,
    Json config,
    EntityId sourceEntityId,
    TeleportBookmark currentLocation) {
  m_client = client;
  m_paneManager = paneManager;
  m_sourceEntityId = sourceEntityId;
  m_currentLocation = currentLocation;

  auto assets = Root::singleton().assets();

  GuiReader reader;

  reader.registerCallback("dismiss", bind(&Pane::dismiss, this));
  reader.registerCallback("teleport", bind(&TeleportDialog::teleport, this));
  reader.registerCallback("selectDestination", bind(&TeleportDialog::selectDestination, this));

  reader.construct(assets->json("/interface/windowconfig/teleportdialog.config:paneLayout"), this);

  config = assets->fetchJson(config);
  auto destList = fetchChild<ListWidget>("bookmarkList.bookmarkItemList");
  destList->registerMemberCallback("editBookmark", bind(&TeleportDialog::editBookmark, this));

  for (auto dest : config.getArray("destinations", JsonArray())) {
    if (auto prerequisite = dest.optString("prerequisiteQuest")) {
      if (!m_client->mainPlayer()->questManager()->hasCompleted(*prerequisite))
        continue;
    }

    auto warpAction = parseWarpAction(dest.getString("warpAction"));
    bool deploy = dest.getBool("deploy", false);
    if (warpAction == WarpAlias::OrbitedWorld && !m_client->canBeamDown(deploy))
      continue;

    auto entry = destList->addItem();
    entry->fetchChild<LabelWidget>("name")->setText(dest.getString("name"));
    entry->fetchChild<LabelWidget>("planetName")->setText(dest.getString("planetName", ""));
    if (dest.contains("icon"))
      entry->fetchChild<ImageWidget>("icon")->setImage(
          strf("/interface/bookmarks/icons/%s.png", dest.getString("icon")));
    entry->fetchChild<ButtonWidget>("editButton")->hide();

    if (dest.getBool("mission", false)) {
      // if the warpaction is for an instance world, set the uuid to the team uuid
      if (auto warpToWorld = warpAction.ptr<WarpToWorld>()) {
        if (auto worldId = warpToWorld->world.ptr<InstanceWorldId>())
          warpAction = WarpToWorld(InstanceWorldId(worldId->instance, m_client->teamUuid(), worldId->level), warpToWorld->target);
      }
    }

    m_destinations.append({warpAction, deploy});
  }

  String beamPartyMember = assets->json("/interface/windowconfig/teleportdialog.config:beamPartyMemberLabel").toString();
  String deployPartyMember = assets->json("/interface/windowconfig/teleportdialog.config:deployPartyMemberLabel").toString();
  String beamPartyMemberIcon = assets->json("/interface/windowconfig/teleportdialog.config:beamPartyMemberIcon").toString();
  String deployPartyMemberIcon = assets->json("/interface/windowconfig/teleportdialog.config:deployPartyMemberIcon").toString();

  if (config.getBool("includePartyMembers", false)) {
    auto teamClient = m_client->teamClient();
    for (auto member : teamClient->members()) {
      if (member.uuid == m_client->mainPlayer()->uuid() || member.warpMode == WarpMode::None)
        continue;

      auto entry = destList->addItem();
      entry->fetchChild<LabelWidget>("name")->setText(member.name);

      if (member.warpMode == WarpMode::DeployOnly)
        entry->fetchChild<LabelWidget>("planetName")->setText(deployPartyMember);
      else
        entry->fetchChild<LabelWidget>("planetName")->setText(beamPartyMember);

      if (member.warpMode == WarpMode::DeployOnly)
        entry->fetchChild<ImageWidget>("icon")->setImage(deployPartyMemberIcon);
      else
        entry->fetchChild<ImageWidget>("icon")->setImage(beamPartyMemberIcon);

      entry->fetchChild<ButtonWidget>("editButton")->hide();

      m_destinations.append({WarpToPlayer(member.uuid), member.warpMode == WarpMode::DeployOnly});
    }
  }

  if (config.getBool("includePlayerBookmarks", false)) {
    auto teleportBookmarks = m_client->mainPlayer()->universeMap()->teleportBookmarks();

    teleportBookmarks.sort([](auto const& a, auto const& b) { return a.bookmarkName.toLower() < b.bookmarkName.toLower(); });

    for (auto bookmark : teleportBookmarks) {
      auto entry = destList->addItem();
      setupBookmarkEntry(entry, bookmark);
      if (bookmark == m_currentLocation) {
        destList->setEnabled(destList->itemPosition(entry), false);
        entry->fetchChild<ButtonWidget>("editButton")->setEnabled(false);
      }
      m_destinations.append({WarpToWorld(bookmark.target.first, bookmark.target.second), false});
    }
  }

  fetchChild<ButtonWidget>("btnTeleport")->setEnabled(destList->selectedItem() != NPos);
}

void TeleportDialog::tick() {
  if (!m_client->worldClient()->playerCanReachEntity(m_sourceEntityId))
    dismiss();
}

void TeleportDialog::selectDestination() {
  auto destList = fetchChild<ListWidget>("bookmarkList.bookmarkItemList");
  fetchChild<ButtonWidget>("btnTeleport")->setEnabled(destList->selectedItem() != NPos);
}

void TeleportDialog::teleport() {
  auto destList = fetchChild<ListWidget>("bookmarkList.bookmarkItemList");
  if (destList->selectedItem() != NPos) {
    auto& destination = m_destinations[destList->selectedItem()];
    auto warpAction = destination.first;
    bool deploy = destination.second;

    auto warp = [this, deploy](WarpAction const& action, String const& animation = "default") {
      if (deploy)
        m_client->warpPlayer(action, true, "deploy", true);
      else
        m_client->warpPlayer(action, true, animation);
    };

    m_client->worldClient()->sendEntityMessage(m_sourceEntityId, "onTeleport", {printWarpAction(warpAction)});
    if (warpAction.is<WarpAlias>() && warpAction.get<WarpAlias>() == WarpAlias::OrbitedWorld) {
      warp(take(destination).first, "beam");
    } else {
      warp(take(destination).first);
    }
    dismiss();
  }
}

void TeleportDialog::editBookmark() {
  auto destList = fetchChild<ListWidget>("bookmarkList.bookmarkItemList");
  if (destList->selectedItem() != NPos) {
    size_t selectedItem = destList->selectedItem();
    auto bookmarks = m_client->mainPlayer()->universeMap()->teleportBookmarks();
    bookmarks.sort([](auto const& a, auto const& b) { return a.bookmarkName.toLower() < b.bookmarkName.toLower(); });
    selectedItem = selectedItem - (m_destinations.size() - bookmarks.size());
    if (bookmarks.size() > selectedItem) {
      auto editBookmarkDialog = make_shared<EditBookmarkDialog>(m_client->mainPlayer()->universeMap());
      editBookmarkDialog->setBookmark(bookmarks[selectedItem]);
      m_paneManager->displayPane(PaneLayer::ModalWindow, editBookmarkDialog);
    }
    dismiss();
  }
}

}
