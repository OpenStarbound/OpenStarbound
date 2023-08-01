#include "StarPackedAssetSource.hpp"
#include "StarDirectoryAssetSource.hpp"
#include "StarOrderedSet.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarSha256.hpp"
#include "StarFile.hpp"

namespace Star {

void PackedAssetSource::build(DirectoryAssetSource& directorySource, String const& targetPackedFile,
    StringList const& extensionSorting, BuildProgressCallback progressCallback) {
  FilePtr file = File::open(targetPackedFile, IOMode::ReadWrite | IOMode::Truncate);

  DataStreamIODevice ds(file);

  ds.writeData("SBAsset6", 8);
  // Skip 8 bytes, this will be a pointer to the index once we are done.
  ds.seek(8, IOSeek::Relative);

  // Insert every found entry into the packed file, and also simultaneously
  // compute the full index.
  StringMap<pair<uint64_t, uint64_t>> index;

  OrderedHashSet<String> extensionOrdering;
  for (auto const& str : extensionSorting)
    extensionOrdering.add(str.toLower());

  StringList assetPaths = directorySource.assetPaths();

  // Returns a value for the asset that can be used to predictably sort assets
  // by name and then by extension, where every extension listed in
  // "extensionSorting" will come first, and then any extension not listed will
  // come after.
  auto getOrderingValue = [&extensionOrdering](String const& asset) -> pair<size_t, String> {
    String extension;
    auto lastDot = asset.findLast(".");
    if (lastDot != NPos)
      extension = asset.substr(lastDot + 1);

    if (auto i = extensionOrdering.indexOf(extension.toLower())) {
      return {*i, asset.toLower()};
    } else {
      return {extensionOrdering.size(), asset.toLower()};
    }
  };

  assetPaths.sort([&getOrderingValue](String const& a, String const& b) {
      return getOrderingValue(a) < getOrderingValue(b);
    });

  for (size_t i = 0; i < assetPaths.size(); ++i) {
    String const& assetPath = assetPaths[i];
    ByteArray contents = directorySource.read(assetPath);

    if (progressCallback)
      progressCallback(i, assetPaths.size(), directorySource.toFilesystem(assetPath), assetPath);
    index.add(assetPath, {ds.pos(), contents.size()});
    ds.writeBytes(contents);
  }

  uint64_t indexStart = ds.pos();
  ds.writeData("INDEX", 5);
  ds.write(directorySource.metadata());
  ds.write(index);

  ds.seek(8);
  ds.write(indexStart);
}

PackedAssetSource::PackedAssetSource(String const& filename) {
  m_packedFile = File::open(filename, IOMode::Read);

  DataStreamIODevice ds(m_packedFile);
  if (ds.readBytes(8) != ByteArray("SBAsset6", 8))
    throw AssetSourceException("Packed assets file format unrecognized!");

  uint64_t indexStart = ds.read<uint64_t>();

  ds.seek(indexStart);
  ByteArray header = ds.readBytes(5);
  if (header != ByteArray("INDEX", 5))
    throw AssetSourceException("No index header found!");
  ds.read(m_metadata);
  ds.read(m_index);
}

JsonObject PackedAssetSource::metadata() const {
  return m_metadata;
}

StringList PackedAssetSource::assetPaths() const {
  return m_index.keys();
}

IODevicePtr PackedAssetSource::open(String const& path) {
  struct AssetReader : public IODevice {
    AssetReader(FilePtr file, String path, StreamOffset offset, StreamOffset size)
      : file(file), path(path), fileOffset(offset), assetSize(size), assetPos(0) {
      setMode(IOMode::Read);
    }

    size_t read(char* data, size_t len) override {
      len = min<StreamOffset>(len, assetSize - assetPos);
      file->readFullAbsolute(fileOffset + assetPos, data, len);
      assetPos += len;
      return len;
    }

    size_t write(char const*, size_t) override {
      throw IOException("Assets IODevices are read-only");
    }

    StreamOffset size() override {
      return assetSize;
    }

    StreamOffset pos() override {
      return assetPos;
    }

    String deviceName() const override {
      return strf("{}:{}", file->deviceName(), path);
    }

    bool atEnd() override {
      return assetPos >= assetSize;
    }

    void seek(StreamOffset p, IOSeek mode) override {
      if (mode == IOSeek::Absolute)
        assetPos = p;
      else if (mode == IOSeek::Relative)
        assetPos = clamp<StreamOffset>(assetPos + p, 0, assetSize);
      else
        assetPos = clamp<StreamOffset>(assetSize - p, 0, assetSize);
    }

    FilePtr file;
    String path;
    StreamOffset fileOffset;
    StreamOffset assetSize;
    StreamOffset assetPos;
  };

  auto p = m_index.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in the packed assets file", path);

  return make_shared<AssetReader>(m_packedFile, path, p->first, p->second);
}

ByteArray PackedAssetSource::read(String const& path) {
  auto p = m_index.ptr(path);
  if (!p)
    throw AssetSourceException::format("Requested file '{}' does not exist in the packed assets file", path);

  ByteArray data(p->second, 0);
  m_packedFile->readFullAbsolute(p->first, data.ptr(), p->second);
  return data;
}

}
