#include "StarDirectoryAssetSource.hpp"
#include "StarFile.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

DirectoryAssetSource::DirectoryAssetSource(String const& baseDirectory, StringList const& ignorePatterns) {
  m_baseDirectory = baseDirectory;
  m_ignorePatterns = ignorePatterns;

  // Load metadata from either /_metadata or /.metadata, in that order.
  for (auto fileName : {"/_metadata", "/.metadata"}) {
    String metadataFile = toFilesystem(fileName);
    if (File::isFile(metadataFile)) {
      try {
        m_metadataFile = String(fileName);
        m_metadata = Json::parseJson(File::readFileString(metadataFile)).toObject();
        break;
      } catch (JsonException const& e) {
        throw AssetSourceException(strf("Could not load metadata file '%s' from assets", metadataFile), e);
      }
    }
  }

  // Don't scan metadata files
  m_ignorePatterns.append("^/_metadata$");
  m_ignorePatterns.append("^/\\.metadata$");

  scanAll("/", m_assetPaths);

  m_assetPaths.sort();
}

JsonObject DirectoryAssetSource::metadata() const {
  return m_metadata;
}

StringList DirectoryAssetSource::assetPaths() const {
  return m_assetPaths;
}

IODevicePtr DirectoryAssetSource::open(String const& path) {
  auto file = make_shared<File>(toFilesystem(path));
  file->open(IOMode::Read);
  return file;
}

ByteArray DirectoryAssetSource::read(String const& path) {
  auto device = open(path);
  return device->readBytes(device->size());
}

String DirectoryAssetSource::toFilesystem(String const& path) const {
  if (!path.beginsWith("/"))
    throw AssetSourceException::format("Asset path '%s' must be absolute in DirectoryAssetSource::toFilesystem", path);
  else
    return File::relativeTo(m_baseDirectory, File::convertDirSeparators(path.substr(1)));
}

void DirectoryAssetSource::setMetadata(JsonObject metadata) {
  if (metadata != m_metadata) {
    if (!m_metadataFile)
      m_metadataFile = String("/_metadata");

    m_metadata = move(metadata);

    if (m_metadata.empty())
      File::remove(toFilesystem(*m_metadataFile));
    else
      File::writeFile(Json(m_metadata).printJson(2, true), toFilesystem(*m_metadataFile));
  }
}

void DirectoryAssetSource::scanAll(String const& assetDirectory, StringList& output) const {
  auto shouldIgnore = [this](String const& assetPath) {
    for (auto const& pattern : m_ignorePatterns) {
      if (assetPath.regexMatch(pattern, false, false))
        return true;
    }
    return false;
  };

  // path must be passed in including the trailing '/'
  String fsDirectory = toFilesystem(assetDirectory);
  for (auto entry : File::dirList(fsDirectory)) {
    String assetPath = assetDirectory + entry.first;
    if (entry.second) {
      scanAll(assetPath + "/", output);
    } else {
      if (!shouldIgnore(assetPath))
        output.append(move(assetPath));
    }
  }
}

}
