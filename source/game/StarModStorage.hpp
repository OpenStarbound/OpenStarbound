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
//
// Changes are kept in memory and written by a background thread a short time
// after the last change, and when this is destroyed, so frequent sets by a
// script cost no disk writes of their own.
class ModStorage : private Thread {
public:
  // Upper bound on the serialized size of one mod's data.
  static size_t constexpr MaxModDataSize = 4 * 1024 * 1024;
  // How long after a change the data is written to disk.
  static unsigned constexpr WriteDelayMillis = 2000;

  ModStorage(String storageDirectory);
  ~ModStorage() override;

  Json get(String const& mod, String const& key);
  // Throws if the mod's data would grow beyond MaxModDataSize.
  void set(String const& mod, String const& key, Json const& value);
  JsonArray keys(String const& mod);
  void clear(String const& mod);

  // Writes everything that changed to disk now.
  void flush();

private:
  struct ModData {
    JsonObject data;
    // Serialized size of each value, so a set only serializes the value that
    // changed and not the whole object to check the limit.
    StringMap<size_t> sizes;
    size_t totalSize = 0;
    bool dirty = false;
  };


  ModData& load(String const& mod);
  void run() override;
  void markDirty(ModData& mdata);
  void writeDirty();
  String filePath(String const& mod) const;

  Mutex m_mutex;
  ConditionVariable m_wake;
  String m_directory;
  StringMap<ModData> m_mods;
  bool m_stop = false;
  bool m_pending = false;
};

}
