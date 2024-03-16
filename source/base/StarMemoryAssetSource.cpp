#include "StarMemoryAssetSource.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarSha256.hpp"

namespace Star {

MemoryAssetSource::MemoryAssetSource(String const& name, JsonObject metadata) : m_name(name), m_metadata(metadata) {}

String MemoryAssetSource::name() const {
  return m_name;
}

JsonObject MemoryAssetSource::metadata() const {
  return m_metadata;
}

StringList MemoryAssetSource::assetPaths() const {
  return m_files.keys();
}

IODevicePtr MemoryAssetSource::open(String const& path) {
  struct AssetReader : public IODevice {
    AssetReader(ByteArrayPtr assetData, String name) : assetData(assetData), name(name) { setMode(IOMode::Read); }

    size_t read(char* data, size_t len) override {
      len = min<StreamOffset>(len, StreamOffset(assetData->size()) - assetPos);
      assetData->copyTo(data, len);
      return len;
    }

    size_t write(char const*, size_t) override {
      throw IOException("Assets IODevices are read-only");
    }

    StreamOffset size() override { return assetData->size(); }
    StreamOffset pos() override { return assetPos; }

    String deviceName() const override { return name; }

    bool atEnd() override {
      return assetPos >= assetData->size();
    }

    void seek(StreamOffset p, IOSeek mode) override {
      if (mode == IOSeek::Absolute)
        assetPos = p;
      else if (mode == IOSeek::Relative)
        assetPos = clamp<StreamOffset>(assetPos + p, 0, assetData->size());
      else
        assetPos = clamp<StreamOffset>(assetPos - p, 0, assetData->size());
    }

    ByteArrayPtr assetData;
    StreamOffset assetPos = 0;
    String name;
  };

  auto p = m_files.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in memory", path);

  return make_shared<AssetReader>(*p, path);
}

bool MemoryAssetSource::empty() const {
  return m_files.empty();
}

bool MemoryAssetSource::contains(String const& path) const {
  return m_files.contains(path);
}

bool MemoryAssetSource::erase(String const& path) {
  return m_files.erase(path) != 0;
}

void MemoryAssetSource::set(String const& path, ByteArray data) {
  m_files[path] = make_shared<ByteArray>(std::move(data));
}

ByteArray MemoryAssetSource::read(String const& path) {
  auto p = m_files.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in memory", path);
  else
    return *p->get(); // this is a double indirection, and that freaking sucks!!
}

}
