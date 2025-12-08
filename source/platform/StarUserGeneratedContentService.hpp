#pragma once

namespace Star {

STAR_CLASS(UserGeneratedContentService);

class UserGeneratedContentService {
public:
  ~UserGeneratedContentService() = default;

  // Returns a list of the content the user is currently subscribed to.
  virtual StringList subscribedContentIds() const = 0;

  // If the content has been downloaded successfully, returns the path to the
  // downloaded content directory on the filesystem, otherwise nothing.
  virtual Maybe<String> contentDownloadDirectory(String const& contentId) const = 0;

  // Start downloading subscribed content in the background, returns true when
  // all content is synchronized.
  virtual bool triggerContentDownload() = 0;

  // Check if it's necessary to download steam workshop content.
  virtual bool contentNeedsDownload() const = 0;
};

}
