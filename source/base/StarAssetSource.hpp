#pragma once

#include "StarIODevice.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_CLASS(AssetSource);

STAR_EXCEPTION(AssetSourceException, StarException);

// An asset source could be a directory on a filesystem, where assets are
// pulled directly from files, or a single pak-like file containing all assets,
// where assets are pulled from the correct region of the pak-like file.
class AssetSource {
public:
  virtual ~AssetSource() = default;

  // An asset source can have arbitrary metadata attached.
  virtual JsonObject metadata() const = 0;

  // Should return all the available assets in this source
  virtual StringList assetPaths() const = 0;

  // Open the given path in this source and return an IODevicePtr to it.
  virtual IODevicePtr open(String const& path) = 0;

  // Read the entirety of the given path into a buffer.
  virtual ByteArray read(String const& path) = 0;
};

}
