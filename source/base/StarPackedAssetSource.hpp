#pragma once

#include "StarOrderedMap.hpp"
#include "StarFile.hpp"
#include "StarDirectoryAssetSource.hpp"

namespace Star {

STAR_CLASS(PackedAssetSource);

class PackedAssetSource : public AssetSource {
public:
  typedef function<void(size_t, size_t, String, String)> BuildProgressCallback;

  // Build a packed asset file from the given DirectoryAssetSource.
  //
  // 'extensionSorting' sorts the packed file with file extensions that case
  // insensitive match the given extensions in the order they are given.  If a
  // file has an extension that doesn't match any in this list, it goes after
  // all other files.  All files are sorted secondarily by case insensitive
  // alphabetical order.
  //
  // If given, 'progressCallback' will be called with the total number of
  // files, the current file number, the file name, and the asset path.
  static void build(DirectoryAssetSource& directorySource, String const& targetPackedFile,
      StringList const& extensionSorting = {}, BuildProgressCallback progressCallback = {});

  PackedAssetSource(String const& packedFileName);

  JsonObject metadata() const override;
  StringList assetPaths() const override;

  IODevicePtr open(String const& path) override;
  ByteArray read(String const& path) override;

private:
  FilePtr m_packedFile;
  JsonObject m_metadata;
  OrderedHashMap<String, pair<uint64_t, uint64_t>> m_index;
};

}
