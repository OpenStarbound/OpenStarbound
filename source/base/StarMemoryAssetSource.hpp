#pragma once

#include "StarAssetSource.hpp"
#include "StarIODevice.hpp"

namespace Star {

STAR_CLASS(MemoryAssetSource);
STAR_CLASS(Image);

class MemoryAssetSource : public AssetSource {
public:
  MemoryAssetSource(String const& name, JsonObject metadata = JsonObject());

  String name() const;
  JsonObject metadata() const override;
  StringList assetPaths() const override;

  // do not use the returned IODevice after the file is gone or bad things will happen
  IODevicePtr open(String const& path) override;

  bool empty() const;
  bool contains(String const& path) const;
  bool erase(String const& path);
  void set(String const& path, ByteArray data);
  void set(String const& path, Image const& image);
  void set(String const& path, Image&& image);
  ByteArray read(String const& path) override;
  ImageConstPtr image(String const& path);
private:
  typedef Variant<ByteArray, ImagePtr> FileEntry;

  String m_name;
  JsonObject m_metadata;
  StringMap<FileEntry> m_files;
};

}
