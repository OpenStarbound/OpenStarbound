#include "StarUserGeneratedContentService_pc_steam.hpp"
#include "StarLogging.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

SteamUserGeneratedContentService::SteamUserGeneratedContentService(PcPlatformServicesStatePtr)
  : m_callbackDownloadResult(this, &SteamUserGeneratedContentService::onDownloadResult) {};

StringList SteamUserGeneratedContentService::subscribedContentIds() const {
  List<PublishedFileId_t> contentIds(SteamUGC()->GetNumSubscribedItems(), {});
  SteamUGC()->GetSubscribedItems(contentIds.ptr(), contentIds.size());
  return contentIds.transformed([](PublishedFileId_t id) {
      return String(toString(id));
    });
}

Maybe<String> SteamUserGeneratedContentService::contentDownloadDirectory(String const& contentId) const {
  PublishedFileId_t id = lexicalCast<PublishedFileId_t>(contentId);
  uint32 itemState = SteamUGC()->GetItemState(id);
  if (itemState & k_EItemStateInstalled) {
    char path[4096];
    if (SteamUGC()->GetItemInstallInfo(id, nullptr, path, sizeof(path), nullptr))
      return String(path);
  }
  return {};
}

bool SteamUserGeneratedContentService::triggerContentDownload() {
  List<PublishedFileId_t> contentIds(SteamUGC()->GetNumSubscribedItems(), {});
  SteamUGC()->GetSubscribedItems(contentIds.ptr(), contentIds.size());

  bool contentNeedsDownload = false;
  for (uint64 contentId : contentIds) {
    if (!m_currentDownloadState.contains(contentId)) {
      uint32 itemState = SteamUGC()->GetItemState(contentId);
      if (!(itemState & k_EItemStateInstalled) || itemState & k_EItemStateNeedsUpdate) {
        contentNeedsDownload = true;
        SteamUGC()->DownloadItem(contentId, true);
        itemState = SteamUGC()->GetItemState(contentId);
        m_currentDownloadState[contentId] = !(itemState & k_EItemStateDownloading);
      } else {
        m_currentDownloadState[contentId] = true;
      }
    }
  }
  if (!contentNeedsDownload)
    return false;

  bool allDownloaded = true;
  for (auto const& p : m_currentDownloadState) {
    if (!p.second)
      allDownloaded = false;
  }

  return allDownloaded;
}

void SteamUserGeneratedContentService::onDownloadResult(DownloadItemResult_t* result) {
  m_currentDownloadState[result->m_nPublishedFileId] = true;
}


}
