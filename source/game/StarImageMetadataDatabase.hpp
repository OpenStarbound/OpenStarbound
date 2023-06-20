#ifndef STAR_IMAGE_METADATA_DATABASE_HPP
#define STAR_IMAGE_METADATA_DATABASE_HPP

#include "StarRect.hpp"
#include "StarMap.hpp"
#include "StarString.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(ImageMetadataDatabase);

// Caches image size, image spaces, and nonEmptyRegion completely until a
// reload, does not expire cached values in a TTL based way like Assets,
// because they are expensive to compute and cheap to keep around.
class ImageMetadataDatabase {
public:
  Vec2U imageSize(String const& path) const;
  List<Vec2I> imageSpaces(String const& path, Vec2F position, float fillLimit, bool flip) const;
  RectU nonEmptyRegion(String const& path) const;

private:
  // Removes image processing directives that don't affect image spaces /
  // non-empty regions.
  static String filterProcessing(String const& path);

  Vec2U calculateImageSize(String const& path) const;

  // Path, position, fillLimit, and flip
  typedef tuple<String, Vec2I, float, bool> SpacesEntry;

  mutable Mutex m_mutex;
  mutable StringMap<Vec2U> m_sizeCache;
  mutable HashMap<SpacesEntry, List<Vec2I>> m_spacesCache;
  mutable StringMap<RectU> m_regionCache;
};

}

#endif
