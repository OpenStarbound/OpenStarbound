#pragma once

#include "StarPlatformServices_pc.hpp"

namespace Star {

STAR_CLASS(SteamUserGeneratedContentService);

class SteamUserGeneratedContentService : public UserGeneratedContentService {
public:
  SteamUserGeneratedContentService(PcPlatformServicesStatePtr state);

  StringList subscribedContentIds() const override;
  Maybe<String> contentDownloadDirectory(String const& contentId) const override;
  bool triggerContentDownload() override;

private:
  STEAM_CALLBACK(SteamUserGeneratedContentService, onDownloadResult, DownloadItemResult_t, m_callbackDownloadResult);

  HashMap<PublishedFileId_t, bool> m_currentDownloadState;
};

}
