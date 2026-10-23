#include "StarModStorage.hpp"
#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

ModStorage::ModStorage(String storageDirectory) : Thread("ModStorage"), m_directory(std::move(storageDirectory)) {
  start();
}

ModStorage::~ModStorage() {
  {
    MutexLocker locker(m_mutex);
    m_stop = true;
    m_wake.signal();
  }
  join();
  writeDirty();
}

Json ModStorage::get(String const& mod, String const& key) {
  MutexLocker locker(m_mutex);
  return load(mod).data.value(key, Json());
}

void ModStorage::set(String const& mod, String const& key, Json const& value) {
  MutexLocker locker(m_mutex);
  auto& mdata = load(mod);

  auto previousSize = mdata.sizes.maybe(key);
  if (value.isNull()) {
    if (!previousSize)
      return;
    mdata.totalSize -= *previousSize;
    mdata.sizes.erase(key);
    mdata.data.erase(key);
  } else {
    if (auto previous = mdata.data.maybe(key); previous && *previous == value)
      return;

    // The key and its separators are counted too.
    size_t size = value.repr().utf8Size() + key.utf8Size() + 4;
    if (mdata.totalSize - previousSize.value(0) + size > MaxModDataSize)
      throw JsonException::format("Mod storage of '{}' would exceed {} bytes", mod, MaxModDataSize);

    mdata.totalSize = mdata.totalSize - previousSize.value(0) + size;
    mdata.sizes[key] = size;
    mdata.data.set(key, value);
  }

  markDirty(mdata);
}

JsonArray ModStorage::keys(String const& mod) {
  MutexLocker locker(m_mutex);
  JsonArray result;
  for (auto const& p : load(mod).data)
    result.append(p.first);
  return result;
}

void ModStorage::clear(String const& mod) {
  MutexLocker locker(m_mutex);
  auto& mdata = load(mod);
  if (mdata.data.empty())
    return;
  mdata.data.clear();
  mdata.sizes.clear();
  mdata.totalSize = 0;
  markDirty(mdata);
}

void ModStorage::flush() {
  writeDirty();
}

void ModStorage::markDirty(ModData& mdata) {
  mdata.dirty = true;
  if (!m_pending) {
    m_pending = true;
    m_wake.signal();
  }
}

ModStorage::ModData& ModStorage::load(String const& mod) {
  auto i = m_mods.find(mod);
  if (i != m_mods.end())
    return i->second;

  ModData mdata;
  String path = filePath(mod);
  if (File::isFile(path)) {
    try {
      mdata.data = Json::parseJson(File::readFileString(path)).toObject();
    } catch (StarException const& e) {
      Logger::error("Could not read mod storage '{}', starting empty: {}", path, outputException(e, false));
    }
  }
  for (auto const& p : mdata.data) {
    size_t size = p.second.repr().utf8Size() + p.first.utf8Size() + 4;
    mdata.sizes[p.first] = size;
    mdata.totalSize += size;
  }
  return m_mods.emplace(mod, std::move(mdata)).first->second;
}

void ModStorage::run() {
  MutexLocker locker(m_mutex);
  while (!m_stop) {
    if (!m_pending) {
      m_wake.wait(m_mutex);
      continue;
    }

    // Changes are signalled only when nothing is pending, so this wait ends at the
    // delay (or on stop), and changes in the meantime go into the same write.
    m_wake.wait(m_mutex, WriteDelayMillis);
    if (m_stop)
      break;

    m_pending = false;
    locker.unlock();
    writeDirty();
    locker.lock();
  }
}

void ModStorage::writeDirty() {
  // Serialize under the lock, but write to disk outside of it.
  List<pair<String, String>> pending;
  {
    MutexLocker locker(m_mutex);
    for (auto& p : m_mods) {
      if (p.second.dirty) {
        p.second.dirty = false;
        pending.append({p.first, Json(p.second.data).repr()});
      }
    }
  }

  for (auto const& p : pending) {
    try {
      if (!File::isDirectory(m_directory))
        File::makeDirectory(m_directory);
      File::overwriteFileWithRename(p.second, filePath(p.first));
    } catch (StarException const& e) {
      Logger::error("Could not write mod storage of '{}': {}", p.first, outputException(e, false));
    }
  }
}

String ModStorage::filePath(String const& mod) const {
  // The mod name comes from pak metadata, so never let it act as a path.
  String safe;
  for (auto c : mod)
    safe.append((isalnum(c) && c < 128) || c == '-' || c == '_' ? c : '_');
  if (safe.empty())
    safe = "_";
  return File::relativeTo(m_directory, safe + ".json");
}

}
