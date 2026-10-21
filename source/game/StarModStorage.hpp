#pragma once

#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarString.hpp"

namespace Star {

STAR_CLASS(ModStorage);

// Persistent key / value storage that is private to each mod.  Every mod gets
// its own json object stored in <storage>/modstorage/<mod>.json, and scripts
// can only reach the object of the mod their code comes from, so no file path
// is ever exposed to Lua and this is safe to offer with safeScripts on.
class ModStorage {
public:
  // Upper bound on the serialized size of one mod's data.
  static size_t constexpr MaxModDataSize = 4 * 1024 * 1024;

  ModStorage(String storageDirectory);

  Json get(String const& mod, String const& key);
  // Throws if the mod's data would grow beyond MaxModDataSize.
  void set(String const& mod, String const& key, Json const& value);
  JsonArray keys(String const& mod);
  void clear(String const& mod);

private:
  JsonObject& load(String const& mod);
  void save(String const& mod);
  String filePath(String const& mod) const;

  Mutex m_mutex;
  String m_directory;
  StringMap<JsonObject> m_data;
};

}
