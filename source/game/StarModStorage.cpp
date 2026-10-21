#include "StarModStorage.hpp"
#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

ModStorage::ModStorage(String storageDirectory) : m_directory(std::move(storageDirectory)) {}

Json ModStorage::get(String const& mod, String const& key) {
  MutexLocker locker(m_mutex);
  return load(mod).value(key, Json());
}

void ModStorage::set(String const& mod, String const& key, Json const& value) {
  MutexLocker locker(m_mutex);
  auto& data = load(mod);

  if (value.isNull()) {
    if (data.erase(key) == 0)
      return;
  } else {
    auto previous = data.maybe(key);
    if (previous && *previous == value)
      return;
    data.set(key, value);

    if (Json(data).repr().utf8Size() > MaxModDataSize) {
      if (previous)
        data.set(key, *previous);
      else
        data.erase(key);
      throw JsonException::format("Mod storage of '{}' would exceed {} bytes", mod, MaxModDataSize);
    }
  }

  save(mod);
}

JsonArray ModStorage::keys(String const& mod) {
  MutexLocker locker(m_mutex);
  JsonArray result;
  for (auto const& p : load(mod))
    result.append(p.first);
  return result;
}

void ModStorage::clear(String const& mod) {
  MutexLocker locker(m_mutex);
  auto& data = load(mod);
  if (data.empty())
    return;
  data.clear();
  save(mod);
}

JsonObject& ModStorage::load(String const& mod) {
  auto i = m_data.find(mod);
  if (i != m_data.end())
    return i->second;

  JsonObject data;
  String path = filePath(mod);
  if (File::isFile(path)) {
    try {
      data = Json::parseJson(File::readFileString(path)).toObject();
    } catch (StarException const& e) {
      Logger::error("Could not read mod storage '{}', starting empty: {}", path, outputException(e, false));
    }
  }
  return m_data.emplace(mod, std::move(data)).first->second;
}

void ModStorage::save(String const& mod) {
  try {
    if (!File::isDirectory(m_directory))
      File::makeDirectory(m_directory);
    File::overwriteFileWithRename(Json(m_data.at(mod)).repr(), filePath(mod));
  } catch (StarException const& e) {
    Logger::error("Could not write mod storage of '{}': {}", mod, outputException(e, false));
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
