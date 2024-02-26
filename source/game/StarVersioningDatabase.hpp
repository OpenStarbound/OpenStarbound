#pragma once

#include "StarJson.hpp"
#include "StarDataStream.hpp"
#include "StarThread.hpp"
#include "StarVersion.hpp"
#include "StarLuaRoot.hpp"

namespace Star {

STAR_STRUCT(VersionedJson);
STAR_CLASS(VersioningDatabase);

STAR_EXCEPTION(VersionedJsonException, StarException);
STAR_EXCEPTION(VersioningDatabaseException, StarException);

struct VersionedJson {
  static char const* const Magic;
  static size_t const MagicStringSize;

  // Writes and reads a binary file containing a versioned json with a magic
  // header marking it as a starbound versioned json file.  Writes using a
  // safe write/flush/swap.
  static VersionedJson readFile(String const& filename);
  static void writeFile(VersionedJson const& versionedJson, String const& filename);

  // Writes and reads a json containing a versioned json
  // This allows embedding versioned metadata within a file
  static VersionedJson fromJson(Json const& source);
  Json toJson() const;

  bool empty() const;

  // If the identifier does not match the given identifier, throws a
  // VersionedJsonException.
  void expectIdentifier(String const& expectedIdentifier) const;

  String identifier;
  VersionNumber version;
  Json content;
};

DataStream& operator>>(DataStream& ds, VersionedJson& versionedJson);
DataStream& operator<<(DataStream& ds, VersionedJson const& versionedJson);

class VersioningDatabase {
public:
  VersioningDatabase();

  // Converts the given content Json to a VersionedJson by marking it with the
  // given identifier and the current version configured in the versioning
  // config file.
  VersionedJson makeCurrentVersionedJson(String const& identifier, Json const& content) const;

  // Returns true if the version in this VersionedJson matches the configured
  // current version and does not need updating.
  bool versionedJsonCurrent(VersionedJson const& versionedJson) const;

  // Brings the given versioned json up to the current configured latest
  // version using update scripts.  If successful, returns the up to date
  // VersionedJson, otherwise throws VersioningDatabaseException.
  VersionedJson updateVersionedJson(VersionedJson const& versionedJson) const;

  // Convenience method, checkts the versionedJson expected identifier and then
  // brings the given versionedJson up to date and returns the content.
  Json loadVersionedJson(VersionedJson const& versionedJson, String const& expectedIdentifier) const;

private:
  struct VersionUpdateScript {
    String script;
    VersionNumber fromVersion;
    VersionNumber toVersion;
  };

  LuaCallbacks makeVersioningCallbacks() const;

  mutable RecursiveMutex m_mutex;
  mutable LuaRoot m_luaRoot;

  StringMap<VersionNumber> m_currentVersions;
  StringMap<List<VersionUpdateScript>> m_versionUpdateScripts;
};

}
