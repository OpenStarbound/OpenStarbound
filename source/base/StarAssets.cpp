#include "StarAssets.hpp"
#include "StarAssetPath.hpp"
#include "StarFile.hpp"
#include "StarTime.hpp"
#include "StarDirectoryAssetSource.hpp"
#include "StarPackedAssetSource.hpp"
#include "StarJsonBuilder.hpp"
#include "StarJsonExtra.hpp"
#include "StarJsonPatch.hpp"
#include "StarIterator.hpp"
#include "StarImageProcessing.hpp"
#include "StarLogging.hpp"
#include "StarRandom.hpp"
#include "StarFont.hpp"
#include "StarAudio.hpp"
#include "StarCasting.hpp"
#include "StarLexicalCast.hpp"
#include "StarSha256.hpp"
#include "StarDataStreamDevices.hpp"

namespace Star {

static void validatePath(AssetPath const& components, bool canContainSubPath, bool canContainDirectives) {
  if (components.basePath.empty() || components.basePath[0] != '/')
    throw AssetException(strf("Path '{}' must be absolute", components.basePath));

  bool first = true;
  bool slashed = true;
  bool dotted = false;
  for (auto c : components.basePath) {
    if (c == '/') {
      if (!first) {
        if (slashed)
          throw AssetException(strf("Path '{}' contains consecutive //, not allowed", components.basePath));
        else if (dotted)
          throw AssetException(strf("Path '{}' '.' and '..' not allowed", components.basePath));
      }
      slashed = true;
      dotted = false;
    } else if (c == ':') {
      if (slashed)
        throw AssetException(strf("Path '{}' has ':' after directory", components.basePath));
      break;
    } else if (c == '?') {
      if (slashed)
        throw AssetException(strf("Path '{}' has '?' after directory", components.basePath));
      break;
    } else {
      slashed = false;
      dotted = c == '.';
    }
    first = false;
  }
  if (slashed)
    throw AssetException(strf("Path '{}' cannot be a file", components.basePath));

  if (!canContainSubPath && components.subPath)
    throw AssetException::format("Path '{}' cannot contain sub-path", components);
  if (!canContainDirectives && !components.directives.empty())
    throw AssetException::format("Path '{}' cannot contain directives", components);
}

Maybe<RectU> FramesSpecification::getRect(String const& frame) const {
  if (auto alias = aliases.ptr(frame)) {
    return frames.get(*alias);
  } else {
    return frames.maybe(frame);
  }
}

Assets::Assets(Settings settings, StringList assetSources) {
  const char* const AssetsPatchSuffix = ".patch";

  m_settings = move(settings);
  m_stopThreads = false;
  m_assetSources = move(assetSources);

  for (auto& sourcePath : m_assetSources) {
    Logger::info("Loading assets from: '{}'", sourcePath);
    AssetSourcePtr source;
    if (File::isDirectory(sourcePath))
      source = make_shared<DirectoryAssetSource>(sourcePath, m_settings.pathIgnore);
    else
      source = make_shared<PackedAssetSource>(sourcePath);

    m_assetSourcePaths.add(sourcePath, source);

    for (auto const& filename : source->assetPaths()) {
      if (filename.endsWith(AssetsPatchSuffix, String::CaseInsensitive)) {
        auto targetPatchFile = filename.substr(0, filename.size() - strlen(AssetsPatchSuffix));
        if (auto p = m_files.ptr(targetPatchFile))
          p->patchSources.append({filename, source});
      }
      auto& descriptor = m_files[filename];
      descriptor.sourceName = filename;
      descriptor.source = source;
    }
  }

  Sha256Hasher digest;

  for (auto const& assetPath : m_files.keys().transformed([](String const& s) {
        return s.toLower();
      }).sorted()) {
    bool digestFile = true;
    for (auto const& pattern : m_settings.digestIgnore) {
      if (assetPath.regexMatch(pattern, false, false)) {
        digestFile = false;
        break;
      }
    }

    auto const& descriptor = m_files.get(assetPath);

    if (digestFile) {
      digest.push(assetPath);
      digest.push(DataStreamBuffer::serialize(descriptor.source->open(descriptor.sourceName)->size()));
      for (auto const& pair : descriptor.patchSources)
        digest.push(DataStreamBuffer::serialize(pair.second->open(pair.first)->size()));
    }
  }

  m_digest = digest.compute();

  for (auto const& filename : m_files.keys())
    m_filesByExtension[AssetPath::extension(filename).toLower()].append(filename);

  int workerPoolSize = m_settings.workerPoolSize;
  for (int i = 0; i < workerPoolSize; i++)
    m_workerThreads.append(Thread::invoke("Assets::workerMain", mem_fn(&Assets::workerMain), this));
}

Assets::~Assets() {
  m_stopThreads = true;

  {
    // Should lock associated mutex to prevent loss of wakeups,
    MutexLocker locker(m_assetsMutex);
    // Notify all worker threads to allow them to stop
    m_assetsQueued.broadcast();
  }

  // Join them all
  m_workerThreads.clear();
}

StringList Assets::assetSources() const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSources;
}

JsonObject Assets::assetSourceMetadata(String const& sourceName) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSourcePaths.getRight(sourceName)->metadata();
}

ByteArray Assets::digest() const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_digest;
}

bool Assets::assetExists(String const& path) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_files.contains(path);
}

String Assets::assetSource(String const& path) const {
  MutexLocker assetsLocker(m_assetsMutex);
  if (auto p = m_files.ptr(path))
    return m_assetSourcePaths.getLeft(p->source);
  throw AssetException(strf("No such asset '{}'", path));
}

StringList Assets::scan(String const& suffix) const {
  if (suffix.beginsWith(".") && !suffix.substr(1).hasChar('.')) {
    return scanExtension(suffix);
  } else {
    StringList result;
    for (auto const& fileEntry : m_files) {
      String const& file = fileEntry.first;
      if (file.endsWith(suffix, String::CaseInsensitive))
        result.append(file);
    }

    return result;
  }
}

StringList Assets::scan(String const& prefix, String const& suffix) const {
  StringList result;
  if (suffix.beginsWith(".") && !suffix.substr(1).hasChar('.')) {
    StringList filesWithExtension = scanExtension(suffix);
    for (auto const& file : filesWithExtension) {
      if (file.beginsWith(prefix, String::CaseInsensitive))
        result.append(file);
    }
  } else {
    for (auto const& fileEntry : m_files) {
      String const& file = fileEntry.first;
      if (file.beginsWith(prefix, String::CaseInsensitive) && file.endsWith(suffix, String::CaseInsensitive))
        result.append(file);
    }
  }
  return result;
}

StringList Assets::scanExtension(String const& extension) const {
  if (extension.beginsWith("."))
    return m_filesByExtension.value(extension.substr(1));
  else
    return m_filesByExtension.value(extension);
}

Json Assets::json(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, true, false);

  return as<JsonData>(getAsset(AssetId{AssetType::Json, move(components)}))->json;
}

Json Assets::fetchJson(Json const& v, String const& dir) const {
  if (v.isType(Json::Type::String))
    return Assets::json(AssetPath::relativeTo(dir, v.toString()));
  else
    return v;
}

void Assets::queueJsons(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    auto components = AssetPath::split(path);
    validatePath(components, true, false);

    return AssetId{AssetType::Json, {components.basePath, {}, {}}};
  }));
}

ImageConstPtr Assets::image(AssetPath const& path) const {
  validatePath(path, true, true);

  return as<ImageData>(getAsset(AssetId{AssetType::Image, path}))->image;
}

void Assets::queueImages(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    auto components = AssetPath::split(path);
    validatePath(components, true, true);

    return AssetId{AssetType::Image, move(components)};
  }));
}

ImageConstPtr Assets::tryImage(AssetPath const& path) const {
  validatePath(path, true, true);

  if (auto imageData = as<ImageData>(tryAsset(AssetId{AssetType::Image, path})))
    return imageData->image;
  else
    return {};
}

FramesSpecificationConstPtr Assets::imageFrames(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  MutexLocker assetsLocker(m_assetsMutex);
  return bestFramesSpecification(path);
}

AudioConstPtr Assets::audio(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<AudioData>(getAsset(AssetId{AssetType::Audio, move(components)}))->audio;
}

void Assets::queueAudios(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    const auto components = AssetPath::split(path);
    validatePath(components, false, false);

    return AssetId{AssetType::Audio, move(components)};
  }));
}

AudioConstPtr Assets::tryAudio(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  if (auto audioData = as<AudioData>(tryAsset(AssetId{AssetType::Audio, move(components)})))
    return audioData->audio;
  else
    return {};
}

FontConstPtr Assets::font(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<FontData>(getAsset(AssetId{AssetType::Font, move(components)}))->font;
}

ByteArrayConstPtr Assets::bytes(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<BytesData>(getAsset(AssetId{AssetType::Bytes, move(components)}))->bytes;
}

IODevicePtr Assets::openFile(String const& path) const {
  return open(path);
}

void Assets::clearCache() {
  MutexLocker assetsLocker(m_assetsMutex);

  // Clear all assets that are not queued or broken.
  auto it = makeSMutableMapIterator(m_assetsCache);
  while (it.hasNext()) {
    auto const& pair = it.next();
    // Don't clean up queued, persistent, or broken assets.
    if (pair.second && !pair.second->shouldPersist() && !m_queue.contains(pair.first))
      it.remove();
  }
}

void Assets::cleanup() {
  MutexLocker assetsLocker(m_assetsMutex);

  double time = Time::monotonicTime();

  auto it = makeSMutableMapIterator(m_assetsCache);
  while (it.hasNext()) {
    auto pair = it.next();
    // Don't clean up broken assets or queued assets.
    if (pair.second && !m_queue.contains(pair.first)) {
      double liveTime = time - pair.second->time;
      if (liveTime > m_settings.assetTimeToLive) {
        // If the asset should persist, just refresh the access time.
        if (pair.second->shouldPersist())
          pair.second->time = time;
        else
          it.remove();
      }
    }
  }
}

bool Assets::AssetId::operator==(AssetId const& assetId) const {
  return tie(type, path) == tie(assetId.type, assetId.path);
}

size_t Assets::AssetIdHash::operator()(AssetId const& id) const {
  return hashOf(id.type, id.path.basePath, id.path.subPath, id.path.directives);
}

bool Assets::JsonData::shouldPersist() const {
  return !json.unique();
}

bool Assets::ImageData::shouldPersist() const {
  return !alias && !image.unique();
}

bool Assets::AudioData::shouldPersist() const {
  return !audio.unique();
}

bool Assets::FontData::shouldPersist() const {
  return !font.unique();
}

bool Assets::BytesData::shouldPersist() const {
  return !bytes.unique();
}

FramesSpecification Assets::parseFramesSpecification(Json const& frameConfig, String path) {
  FramesSpecification framesSpecification;

  framesSpecification.framesFile = move(path);

  if (frameConfig.contains("frameList")) {
    for (auto const& pair : frameConfig.get("frameList").toObject()) {
      String frameName = pair.first;
      RectU rect = RectU(jsonToRectI(pair.second));
      if (rect.isEmpty())
        throw AssetException(
            strf("Empty rect in frame specification in image {} frame {}", framesSpecification.framesFile, frameName));
      else
        framesSpecification.frames[frameName] = rect;
    }
  }

  if (frameConfig.contains("frameGrid")) {
    auto grid = frameConfig.get("frameGrid").toObject();

    Vec2U begin(jsonToVec2I(grid.value("begin", jsonFromVec2I(Vec2I()))));
    Vec2U size(jsonToVec2I(grid.get("size")));
    Vec2U dimensions(jsonToVec2I(grid.get("dimensions")));

    if (dimensions[0] == 0 || dimensions[1] == 0)
      throw AssetException(strf("Image {} \"dimensions\" in frameGrid cannot be zero", framesSpecification.framesFile));

    if (grid.contains("names")) {
      auto nameList = grid.get("names");
      for (size_t y = 0; y < nameList.size(); ++y) {
        if (y >= dimensions[1])
          throw AssetException(strf("Image {} row {} is out of bounds for y-dimension {}",
              framesSpecification.framesFile,
              y + 1,
              dimensions[1]));
        auto rowList = nameList.get(y);
        if (rowList.isNull())
          continue;
        for (unsigned x = 0; x < rowList.size(); ++x) {
          if (x >= dimensions[0])
            throw AssetException(strf("Image {} column {} is out of bounds for x-dimension {}",
                framesSpecification.framesFile,
                x + 1,
                dimensions[0]));

          auto frame = rowList.get(x);
          if (frame.isNull())
            continue;
          auto frameName = frame.toString();
          if (!frameName.empty())
            framesSpecification.frames[frameName] =
                RectU::withSize(Vec2U(begin[0] + x * size[0], begin[1] + y * size[1]), size);
        }
      }
    } else {
      // If "names" not specified, use auto naming algorithm
      for (size_t y = 0; y < dimensions[1]; ++y)
        for (size_t x = 0; x < dimensions[0]; ++x)
          framesSpecification.frames[toString(y * dimensions[0] + x)] =
              RectU::withSize(Vec2U(begin[0] + x * size[0], begin[1] + y * size[1]), size);
    }
  }

  if (auto aliasesConfig = frameConfig.opt("aliases")) {
    auto aliases = aliasesConfig->objectPtr();
    for (auto const& pair : *aliases) {
      String const& key = pair.first;
      String value = pair.second.toString();

      // Resolve aliases to aliases by checking to see if the alias value in
      // the alias map itself.  Don't do this more than aliases.size() times to
      // avoid infinite cycles.
      for (size_t i = 0; i <= aliases->size(); ++i) {
        auto it = aliases->find(value);
        if (it != aliases->end()) {
          if (i == aliases->size())
            throw AssetException(strf("Infinite alias loop detected for alias '{}'", key));

          value = it->second.toString();
        } else {
          break;
        }
      }

      if (!framesSpecification.frames.contains(value))
        throw AssetException(strf("No such frame '{}' found for alias '{}'", value, key));
      framesSpecification.aliases[key] = move(value);
    }
  }

  return framesSpecification;
}

void Assets::queueAssets(List<AssetId> const& assetIds) const {
  MutexLocker assetsLocker(m_assetsMutex);

  for (auto const& id : assetIds) {
    auto i = m_assetsCache.find(id);
    if (i != m_assetsCache.end()) {
      if (i->second)
        freshen(i->second);
    } else {
      auto j = m_queue.find(id);
      if (j == m_queue.end()) {
        m_queue[id] = QueuePriority::Load;
        m_assetsQueued.signal();
      }
    }
  }
}

shared_ptr<Assets::AssetData> Assets::tryAsset(AssetId const& id) const {
  MutexLocker assetsLocker(m_assetsMutex);

  auto i = m_assetsCache.find(id);
  if (i != m_assetsCache.end()) {
    if (i->second) {
      freshen(i->second);
      return i->second;
    } else {
      throw AssetException::format("Error loading asset {}", id.path);
    }
  } else {
    auto j = m_queue.find(id);
    if (j == m_queue.end()) {
      m_queue[id] = QueuePriority::Load;
      m_assetsQueued.signal();
    }
    return {};
  }
}

shared_ptr<Assets::AssetData> Assets::getAsset(AssetId const& id) const {
  MutexLocker assetsLocker(m_assetsMutex);

  while (true) {
    auto j = m_assetsCache.find(id);
    if (j != m_assetsCache.end()) {
      if (j->second) {
        auto asset = j->second;
        freshen(asset);
        return asset;
      } else {
        throw AssetException::format("Error loading asset {}", id.path);
      }
    } else {
      // Try to load the asset in-thread, if we cannot, then the asset has been
      // queued so wait for a worker thread to finish it.
      if (!doLoad(id))
        m_assetsDone.wait(m_assetsMutex);
    }
  }
}

void Assets::workerMain() {
  while (true) {
    if (m_stopThreads)
      break;

    MutexLocker assetsLocker(m_assetsMutex);

    AssetId assetId;
    QueuePriority queuePriority = QueuePriority::None;

    // Find the highest priority queue entry
    for (auto const& pair : m_queue) {
      if (pair.second == QueuePriority::Load || pair.second == QueuePriority::PostProcess) {
        assetId = pair.first;
        queuePriority = pair.second;
        if (pair.second == QueuePriority::Load)
          break;
      }
    }

    if (queuePriority != QueuePriority::Load && queuePriority != QueuePriority::PostProcess) {
      // Nothing in the queue that needs work
      m_assetsQueued.wait(m_assetsMutex);
      continue;
    }

    bool workIsBlocking;
    if (queuePriority == QueuePriority::PostProcess)
      workIsBlocking = !doPost(assetId);
    else
      workIsBlocking = !doLoad(assetId);

    if (workIsBlocking) {
      // We are blocking on some sort of busy asset, so need to wait on
      // something to complete here, rather than spinning and burning cpu.
      m_assetsDone.wait(m_assetsMutex);
      continue;
    }

    // After processing an asset, unlock the main asset mutex and yield so we
    // don't starve other threads.
    assetsLocker.unlock();
    Thread::yield();
  }
}

template <typename Function>
decltype(auto) Assets::unlockDuring(Function f) const {
  m_assetsMutex.unlock();
  try {
    auto r = f();
    m_assetsMutex.lock();
    return r;
  } catch (...) {
    m_assetsMutex.lock();
    throw;
  }
}

FramesSpecificationConstPtr Assets::bestFramesSpecification(String const& image) const {
  if (auto framesSpecification = m_framesSpecifications.maybe(image)) {
    return *framesSpecification;
  }

  String framesFile;

  if (auto bestFramesFile = m_bestFramesFiles.maybe(image)) {
    framesFile = *bestFramesFile;

  } else {
    String searchPath = AssetPath::directory(image);
    String filePrefix = AssetPath::filename(image);
    filePrefix = filePrefix.substr(0, filePrefix.findLast('.'));

    auto subdir = [](String const& dir) -> String {
      auto dirsplit = dir.substr(0, dir.size() - 1).rsplit("/", 1);
      if (dirsplit.size() < 2)
        return "";
      else
        return dirsplit[0] + "/";
    };

    Maybe<String> foundFramesFile;

    // look for <full-path-minus-extension>.frames or default.frames up to root
    while (!searchPath.empty()) {
      String framesPath = searchPath + filePrefix + ".frames";
      if (m_files.contains(framesPath)) {
        foundFramesFile = framesPath;
        break;
      }

      framesPath = searchPath + "default.frames";
      if (m_files.contains(framesPath)) {
        foundFramesFile = framesPath;
        break;
      }

      searchPath = subdir(searchPath);
    }

    if (foundFramesFile) {
      framesFile = foundFramesFile.take();
      m_bestFramesFiles[image] = framesFile;

    } else {
      return {};
    }
  }

  auto framesSpecification = unlockDuring([&]() {
      return make_shared<FramesSpecification>(parseFramesSpecification(readJson(framesFile), framesFile));
    });
  m_framesSpecifications[image] = framesSpecification;

  return framesSpecification;
}

IODevicePtr Assets::open(String const& path) const {
  if (auto p = m_files.ptr(path))
    return p->source->open(p->sourceName);
  throw AssetException(strf("No such asset '{}'", path));
}

ByteArray Assets::read(String const& path) const {
  if (auto p = m_files.ptr(path))
    return p->source->read(p->sourceName);
  throw AssetException(strf("No such asset '{}'", path));
}

Json Assets::readJson(String const& path) const {
  ByteArray streamData = read(path);
  try {
    Json result = inputUtf8Json(streamData.begin(), streamData.end(), false);
    for (auto const& pair : m_files.get(path).patchSources) {
      auto patchStream = pair.second->read(pair.first);
      auto patchJson = inputUtf8Json(patchStream.begin(), patchStream.end(), false);
      if (patchJson.isType(Json::Type::Array)) {
        auto patchData = patchJson.toArray();
        try {
          if (patchData.size()) {
            if (patchData.at(0).type() == Json::Type::Array) {
              for (auto const& patch : patchData) {
                try {
                  result = jsonPatch(result, patch.toArray());
                } catch (JsonPatchTestFail const& e) {
                  Logger::debug("Patch test failure from file {} in source: {}. Caused by: {}", pair.first, m_assetSourcePaths.getLeft(pair.second), e.what());
                }
              }
            } else if (patchData.at(0).type() == Json::Type::Object) {
              try {
                result = jsonPatch(result, patchData);
              } catch (JsonPatchTestFail const& e) {
                Logger::debug("Patch test failure from file {} in source: {}. Caused by: {}", pair.first, m_assetSourcePaths.getLeft(pair.second), e.what());
              }
            } else {
              throw JsonPatchException(strf("Patch data is wrong type: {}", Json::typeName(patchData.at(0).type())));
            }
          }
        } catch (JsonPatchException const& e) {
          Logger::error("Could not apply patch from file {} in source: {}.  Caused by: {}", pair.first, m_assetSourcePaths.getLeft(pair.second), e.what());
        }
      }
      else if (patchJson.isType(Json::Type::Object)) { //Kae: Do a good ol' json merge instead if the .patch file is a Json object
        auto patchData = patchJson.toObject();
        result = jsonMerge(result, patchData);
      }
    }
    return result;
  } catch (std::exception const& e) {
    throw JsonParsingException(strf("Cannot parse json file: {}", path), e);
  }
}

bool Assets::doLoad(AssetId const& id) const {
  try {
    // loadAsset automatically manages the queue and freshens the asset
    // data.
    return (bool)loadAsset(id);
  } catch (std::exception const& e) {
    Logger::error("Exception caught loading asset: {}, {}", id.path, outputException(e, true));
  } catch (...) {
    Logger::error("Unknown exception caught loading asset: {}", id.path);
  }

  // There was an exception, remove the asset from the queue and fill the cache
  // with null so that getAsset will throw.
  m_assetsCache[id] = {};
  m_assetsDone.broadcast();
  m_queue.remove(id);
  return true;
}

bool Assets::doPost(AssetId const& id) const {
  shared_ptr<AssetData> assetData;
  try {
    assetData = m_assetsCache.get(id);
    if (id.type == AssetType::Audio)
      assetData = postProcessAudio(assetData);
  } catch (std::exception const& e) {
    Logger::error("Exception caught post-processing asset: {}, {}", id.path, outputException(e, true));
  } catch (...) {
    Logger::error("Unknown exception caught post-processing asset: {}", id.path);
  }

  m_queue.remove(id);
  if (assetData) {
    assetData->needsPostProcessing = false;
    m_assetsCache[id] = assetData;
    freshen(assetData);
    m_assetsDone.broadcast();
  }

  return true;
}

shared_ptr<Assets::AssetData> Assets::loadAsset(AssetId const& id) const {
  if (auto asset = m_assetsCache.value(id))
    return asset;

  if (m_queue.value(id, QueuePriority::None) == QueuePriority::Working)
    return {};

  try {
    m_queue[id] = QueuePriority::Working;
    shared_ptr<AssetData> assetData;

    try {
      if (id.type == AssetType::Json) {
        assetData = loadJson(id.path);
      } else if (id.type == AssetType::Image) {
        assetData = loadImage(id.path);
      } else if (id.type == AssetType::Audio) {
        assetData = loadAudio(id.path);
      } else if (id.type == AssetType::Font) {
        assetData = loadFont(id.path);
      } else if (id.type == AssetType::Bytes) {
        assetData = loadBytes(id.path);
      }

    } catch (StarException const& e) {
      if (id.type == AssetType::Image && m_settings.missingImage) {
        Logger::error("Could not load image asset '{}', using placeholder default.\n{}", id.path, outputException(e, false));
        assetData = loadImage({*m_settings.missingImage, {}, {}});
      } else if (id.type == AssetType::Audio && m_settings.missingAudio) {
        Logger::error("Could not load audio asset '{}', using placeholder default.\n{}", id.path, outputException(e, false));
        assetData = loadAudio({*m_settings.missingAudio, {}, {}});
      } else {
        throw;
      }
    }

    if (assetData) {
      if (assetData->needsPostProcessing)
        m_queue[id] = QueuePriority::PostProcess;
      else
        m_queue.remove(id);
      m_assetsCache[id] = assetData;
      m_assetsDone.broadcast();
      freshen(assetData);

    } else {
      // We have failed to load an asset because it depends on an asset
      // currently being worked on.  Mark it as needing loading and move it to
      // the end of the queue.
      m_queue[id] = QueuePriority::Load;
      m_assetsQueued.signal();
      m_queue.toBack(id);
    }

    return assetData;

  } catch (...) {
    m_queue.remove(id);
    m_assetsCache[id] = {};
    m_assetsDone.broadcast();
    throw;
  }
}

shared_ptr<Assets::AssetData> Assets::loadJson(AssetPath const& path) const {
  Json json;

  if (path.subPath) {
    auto topJson =
      as<JsonData>(loadAsset(AssetId{AssetType::Json, {path.basePath, {}, {}}}));
    if (!topJson)
      return {};

    try {
      auto newData = make_shared<JsonData>();
      newData->json = topJson->json.query(*path.subPath);
      return newData;
    } catch (StarException const& e) {
      throw AssetException(strf("Could not read JSON value {}", path), e);
    }
  } else {
    return unlockDuring([&]() {
      try {
        auto newData = make_shared<JsonData>();
        newData->json = readJson(path.basePath);
        return newData;
      } catch (StarException const& e) {
        throw AssetException(strf("Could not read JSON asset {}", path), e);
      }
    });
  }
}

shared_ptr<Assets::AssetData> Assets::loadImage(AssetPath const& path) const {
  if (!path.directives.empty()) {
    shared_ptr<ImageData> source =
        as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, path.subPath, {}}}));
    if (!source)
      return {};
    StringMap<ImageConstPtr> references;
    StringList referencePaths;

    for (auto& directives : path.directives.list())
      directives.loadOperations();

    path.directives.forEach([&](auto const& entry, Directives const& directives) {
      addImageOperationReferences(entry.operation, referencePaths);
    }); // TODO: This can definitely be better, was changed quickly to support the new Directives.


    for (auto const& ref : referencePaths) {
      auto components = AssetPath::split(ref);
      validatePath(components, true, false);
      auto refImage = as<ImageData>(loadAsset(AssetId{AssetType::Image, move(components)}));
      if (!refImage)
        return {};
      references[ref] = refImage->image;
    }

    return unlockDuring([&]() {
      auto newData = make_shared<ImageData>();
      Image newImage = *source->image;
      path.directives.forEach([&](auto const& entry, Directives const& directives) {
        if (auto error = entry.operation.template ptr<ErrorImageOperation>())
          std::rethrow_exception(error->exception);
        else
          processImageOperation(entry.operation, newImage, [&](String const& ref) { return references.get(ref).get(); });
      });
      newData->image = make_shared<Image>(move(newImage));
      return newData;
    });

  } else if (path.subPath) {
    auto imageData = as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, {}, {}}}));
    if (!imageData)
      return {};

    // Base image must have frames data associated with it.
    if (!imageData->frames)
      throw AssetException::format("No associated frames file found for image '{}' while resolving image frame '{}'", path.basePath, path);

    if (auto alias = imageData->frames->aliases.ptr(*path.subPath)) {
      imageData = as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, *alias, path.directives}}));
      if (!imageData)
        return {};

      auto newData = make_shared<ImageData>();
      newData->image = imageData->image;
      newData->alias = true;
      return newData;

    } else {
      auto frameRect = imageData->frames->frames.ptr(*path.subPath);
      if (!frameRect)
        throw AssetException(strf("No such frame {} in frames spec {}", *path.subPath, imageData->frames->framesFile));

      return unlockDuring([&]() {
        // Need to flip frame coordinates because frame configs assume top
        // down image coordinates
        auto newData = make_shared<ImageData>();
        newData->image = make_shared<Image>(imageData->image->subImage(
            Vec2U(frameRect->xMin(), imageData->image->height() - frameRect->yMax()), frameRect->size()));
        return newData;
      });
    }

  } else {
    auto imageData = make_shared<ImageData>();
    imageData->image = unlockDuring([&]() {
      return make_shared<Image>(Image::readPng(open(path.basePath)));
    });
    imageData->frames = bestFramesSpecification(path.basePath);

    return imageData;
  }
}

shared_ptr<Assets::AssetData> Assets::loadAudio(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<AudioData>();
    newData->audio = make_shared<Audio>(open(path.basePath));
    newData->needsPostProcessing = newData->audio->compressed();
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::loadFont(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<FontData>();
    newData->font = Font::loadTrueTypeFont(make_shared<ByteArray>(read(path.basePath)));
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::loadBytes(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<BytesData>();
    newData->bytes = make_shared<ByteArray>(read(path.basePath));
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::postProcessAudio(shared_ptr<AssetData> const& original) const {
  return unlockDuring([&]() -> shared_ptr<AssetData> {
    if (auto audioData = as<AudioData>(original)) {
      if (audioData->audio->totalTime() < m_settings.audioDecompressLimit) {
        auto audio = make_shared<Audio>(*audioData->audio);
        audio->uncompress();

        auto newData = make_shared<AudioData>();
        newData->audio = audio;
        return newData;
      } else {
        return audioData;
      }
    } else {
      return {};
    }
  });
}

void Assets::freshen(shared_ptr<AssetData> const& asset) const {
  asset->time = Time::monotonicTime();
}

}
