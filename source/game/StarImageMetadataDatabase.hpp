#pragma once

#include "StarRect.hpp"
#include "StarMap.hpp"
#include "StarString.hpp"
#include "StarThread.hpp"
#include "StarAssetPath.hpp"
#include "StarTtlCache.hpp"

namespace Star {

STAR_CLASS(ImageMetadataDatabase);

// Caches image size, image spaces, and nonEmptyRegion completely until a
// reload, does not expire cached values in a TTL based way like Assets,
// because they are expensive to compute and cheap to keep around.
class ImageMetadataDatabase {
public:
  ImageMetadataDatabase();
  Vec2U imageSize(AssetPath const& path) const;
  List<Vec2I> imageSpaces(AssetPath const& path, Vec2F position, float fillLimit, bool flip) const;
  RectU nonEmptyRegion(AssetPath const& path) const;
  void cleanup() const;

private:
  // Removes image processing directives that don't affect image spaces /
  // non-empty regions.
  static AssetPath filterProcessing(AssetPath const& path);

  Vec2U calculateImageSize(AssetPath const& path) const;

  // Path, position, fillLimit, and flip
  typedef tuple<AssetPath, Vec2I, float, bool> SpacesEntry;

  mutable Mutex m_mutex;
  mutable HashTtlCache<AssetPath, Vec2U> m_sizeCache;
  mutable HashTtlCache<SpacesEntry, List<Vec2I>> m_spacesCache;
  mutable HashTtlCache<AssetPath, RectU> m_regionCache;
};

}
