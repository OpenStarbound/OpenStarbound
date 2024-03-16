#pragma once

#include "StarAssetSource.hpp"
#include "StarIODevice.hpp"

namespace Star {

STAR_CLASS(MemoryAssetSource);

class MemoryAssetSource : public AssetSource {
public:
  MemoryAssetSource(String const& name, JsonObject metadata = JsonObject());

  String name() const;
  JsonObject metadata() const override;
  StringList assetPaths() const override;

  IODevicePtr open(String const& path) override;

  bool empty() const;
  bool contains(String const& path) const;
  bool erase(String const& path);
  void set(String const& path, ByteArray data);
  ByteArray read(String const& path) override;
private:
  String m_name;
  JsonObject m_metadata;
  StringMap<ByteArrayPtr> m_files;
};

}
