#include "StarMemoryAssetSource.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarImage.hpp"

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
    AssetReader(char* assetData, size_t assetSize, String name) {
      this->assetData = assetData;
      this->assetSize = assetSize;
      this->name = std::move(name);
      setMode(IOMode::Read);
    }

    size_t read(char* data, size_t len) override {
      len = min<StreamOffset>(len, StreamOffset(assetSize) - assetPos);
      memcpy(data, assetData + assetPos, len);
      assetPos += len;
      return len;
    }

    size_t write(char const*, size_t) override {
      throw IOException("Assets IODevices are read-only");
    }

    StreamOffset size() override { return assetSize; }
    StreamOffset pos() override { return assetPos; }

    String deviceName() const override { return name; }

    bool atEnd() override {
      return assetPos >= assetSize;
    }

    void seek(StreamOffset p, IOSeek mode) override {
      if (mode == IOSeek::Absolute)
        assetPos = p;
      else if (mode == IOSeek::Relative)
        assetPos = clamp<StreamOffset>(assetPos + p, 0, assetSize);
      else
        assetPos = clamp<StreamOffset>(assetPos - p, 0, assetSize);
    }

    char* assetData;
    size_t assetSize;
    StreamOffset assetPos = 0;
    String name;
  };

  auto p = m_files.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in memory", path);
  else if (auto byteArray = p->ptr<ByteArray>())
    return make_shared<AssetReader>(byteArray->ptr(), byteArray->size(), path);
  else {
    auto image = p->get<ImagePtr>().get();
    return make_shared<AssetReader>((char*)image->data(), image->width() * image->height() * image->bytesPerPixel(), path);
  }
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
  m_files[path] = std::move(data);
}

void MemoryAssetSource::set(String const& path, Image const& image) {
  m_files[path] = make_shared<Image>(image);
}

void MemoryAssetSource::set(String const& path, Image&& image) {
  m_files[path] = make_shared<Image>(std::move(image));
}

ByteArray MemoryAssetSource::read(String const& path) {
  auto p = m_files.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in memory", path);
  else if (auto bytes = p->ptr<ByteArray>())
    return *bytes;
  else {
    Image const* image = p->get<ImagePtr>().get();
    return ByteArray((char const*)image->data(), image->width() * image->height() * image->bytesPerPixel());
  }
}

ImageConstPtr MemoryAssetSource::image(String const& path) {
  auto p = m_files.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in memory", path);
  else if (auto imagePtr = p->ptr<ImagePtr>())
    return *imagePtr;
  else
    return nullptr;
}

}
