#include "StarVersioningDatabase.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarFormat.hpp"
#include "StarLexicalCast.hpp"
#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarAssets.hpp"
#include "StarStoredFunctions.hpp"
#include "StarNpcDatabase.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

char const* const VersionedJson::Magic = "SBVJ01";
size_t const VersionedJson::MagicStringSize = 6;

VersionNumber const VersionedJson::SubVersioning = 1;

VersionedJson VersionedJson::readFile(String const& filename) {
  DataStreamIODevice ds(File::open(filename, IOMode::Read));

  if (ds.readBytes(MagicStringSize) != ByteArray(Magic, MagicStringSize))
    throw IOException(strf("Wrong magic bytes at start of versioned json file, expected '{}'", Magic));
  auto versionedJson = ds.read<VersionedJson>();
  readSubVersioning(ds, versionedJson);

  return versionedJson;
}

void VersionedJson::writeFile(VersionedJson const& versionedJson, String const& filename) {
  DataStreamBuffer ds;
  ds.writeData(Magic, MagicStringSize);
  ds.write(versionedJson);
  writeSubVersioning(ds, versionedJson);
  File::overwriteFileWithRename(ds.takeData(), filename);
}

void VersionedJson::writeSubVersioning(DataStream& ds, VersionedJson const& versionedJson) {
  ds.write(VersionedJson::SubVersioning);
  ds.write(versionedJson.subVersions);
}
void VersionedJson::readSubVersioning(DataStream& ds, VersionedJson& versionedJson) {
  VersionNumber extraVersioning = 0;
  try {
    ds.read(extraVersioning);
  } catch (...) {
  }
  if (extraVersioning == 1) {
    ds.read(versionedJson.subVersions);
  }
}

Json VersionedJson::toJson() const {
  JsonObject subVersionsOut;
  for (auto const& p : subVersions)
    subVersionsOut.set(p.first, p.second);
  return JsonObject{
    {"id", identifier},
    {"version", version},
    {"content", content},
    {"subVersions", subVersionsOut}
  };
}

VersionedJson VersionedJson::fromJson(Json const& source) {
  // Old versions of VersionedJson used '__' to distinguish between actual
  // content and versioned content, but this is no longer necessary or
  // relevant.
  auto id = source.optString("id").orMaybe(source.optString("__id"));
  auto version = source.optUInt("version").orMaybe(source.optUInt("__version"));
  auto content = source.opt("content").orMaybe(source.opt("__content"));
  StringMap<VersionNumber> subVersions;
  for (auto const& p : source.getObject("subVersions", JsonObject()))
    subVersions[p.first] = p.second.toUInt();

  return {*id, (VersionNumber)*version, *content, subVersions};
}

bool VersionedJson::empty() const {
  return content.isNull();
}

void VersionedJson::expectIdentifier(String const& expectedIdentifier) const {
  if (identifier != expectedIdentifier)
    throw VersionedJsonException::format("VersionedJson identifier mismatch, expected '{}' but got '{}'", expectedIdentifier, identifier);
}


DataStream& operator>>(DataStream& ds, VersionedJson& versionedJson) {
  ds.read(versionedJson.identifier);
  // This is a holdover from when the verison number was optional in
  // VersionedJson.  We should convert versioned json binary files and the
  // celestial chunk database and world storage to a new format eventually
  versionedJson.version = ds.read<Maybe<VersionNumber>>().value();
  ds.read(versionedJson.content);

  // this is a holdover from when sub versions were smuggled into content without realizing this caused issues, can potentially be removed later
  if (versionedJson.content.isType(Json::Type::Object) && versionedJson.content.contains("subVersions")) {
    for (auto const& p : versionedJson.content.getObject("subVersions", JsonObject()))
      versionedJson.subVersions[p.first] = p.second.toUInt();
    versionedJson.content = versionedJson.content.eraseKey("subVersions");
  }
  return ds;
}

DataStream& operator<<(DataStream& ds, VersionedJson const& versionedJson) {
  ds.write(versionedJson.identifier);
  ds.write(Maybe<VersionNumber>(versionedJson.version));
  ds.write(versionedJson.content);
  return ds;
}

VersioningDatabase::VersioningDatabase() {
  auto assets = Root::singleton().assets();

  for (auto const& pair : assets->json("/versioning.config").iterateObject())
    m_currentVersions[pair.first] = pair.second.toUInt();

  for (auto const& pair : assets->json("/versioning/subVersioning.config").iterateObject())
    for (auto const& p : pair.second.iterateObject())
      m_currentSubVersions[pair.first][p.first] = p.second.toUInt();

  for (auto const& scriptFile : assets->scan("/versioning/", ".lua")) {
    try {
      auto scriptParts = File::baseName(scriptFile).splitAny("_.");
      if (scriptParts.size() == 4) {
        String identifier = scriptParts.at(0);
        VersionNumber fromVersion = lexicalCast<VersionNumber>(scriptParts.at(1));
        VersionNumber toVersion = lexicalCast<VersionNumber>(scriptParts.at(2));

        m_versionUpdateScripts[identifier.toLower()].append({scriptFile, fromVersion, toVersion});
      } else if (scriptParts.size() == 6) {
        String identifier = scriptParts.at(0);
        VersionNumber atVersion = lexicalCast<VersionNumber>(scriptParts.at(1));
        String subIdentifier = scriptParts.at(2);
        VersionNumber fromVersion = lexicalCast<VersionNumber>(scriptParts.at(3));
        VersionNumber toVersion = lexicalCast<VersionNumber>(scriptParts.at(4));

        m_subVersionUpdateScripts[identifier.toLower()][atVersion][subIdentifier.toLower()].append({scriptFile, fromVersion, toVersion});
      } else {
        throw VersioningDatabaseException::format("Script file '{}' filename not of the form <identifier>_<fromversion>_<toversion>.lua or <identifier>_<atVersion>_<subIdentifier>_<fromSubVersion>_<toSubVersion>.lua", scriptFile);
      }

    } catch (StarException const&) {
      throw VersioningDatabaseException::format("Error parsing version information from versioning script '{}'", scriptFile);
    }
  }

  // Sort each set of update scripts first by fromVersion, and then in
  // *reverse* order of toVersion.  This way, the first matching script for a
  // given fromVersion should take the json to the *furthest* toVersion.
  for (auto& pair : m_versionUpdateScripts) {
    pair.second.sort([](VersionUpdateScript const& lhs, VersionUpdateScript const& rhs) {
      if (lhs.fromVersion != rhs.fromVersion)
        return lhs.fromVersion < rhs.fromVersion;
      else
        return lhs.toVersion < rhs.toVersion;
    });
  }
  for (auto& p : m_subVersionUpdateScripts)
    for (auto& version : p.second)
      for (auto& pair : version.second) {
        pair.second.sort([](VersionUpdateScript const& lhs, VersionUpdateScript const& rhs) {
          if (lhs.fromVersion != rhs.fromVersion)
            return lhs.fromVersion < rhs.fromVersion;
          else
            return lhs.toVersion < rhs.toVersion;
        });
      }



}

VersionedJson VersioningDatabase::makeCurrentVersionedJson(String const& identifier, Json const& content) const {
  RecursiveMutexLocker locker(m_mutex);
  return VersionedJson{identifier, m_currentVersions.get(identifier), content, m_currentSubVersions.value(identifier)};
}

bool VersioningDatabase::versionedJsonCurrent(VersionedJson const& versionedJson) const {
  RecursiveMutexLocker locker(m_mutex);
  return (versionedJson.version == m_currentVersions.get(versionedJson.identifier))
  && (versionedJson.subVersions == m_currentSubVersions.value(versionedJson.identifier));
}

VersionedJson VersioningDatabase::updateVersionedJson(VersionedJson const& versionedJson) const {
  RecursiveMutexLocker locker(m_mutex);

  auto& root = Root::singleton();
  CelestialMasterDatabase celestialDatabase;

  VersionedJson result = versionedJson;
  Maybe<VersionNumber> targetVersion = m_currentVersions.maybe(versionedJson.identifier);
  if (!targetVersion)
    throw VersioningDatabaseException::format("Versioned JSON has an unregistered identifier '{}'", versionedJson.identifier);

  LuaCallbacks celestialCallbacks;
  celestialCallbacks.registerCallback("parameters", [&celestialDatabase](Json const& coord) {
      return celestialDatabase.parameters(CelestialCoordinate(coord))->diskStore();
    });

  try {
    for (auto const& updateScript : m_versionUpdateScripts.value(versionedJson.identifier.toLower())) {
      for (auto const& subVersionScripts : m_subVersionUpdateScripts.value(versionedJson.identifier.toLower()).value(result.version)) {
        auto targetSubVersion = m_currentSubVersions.value(result.identifier).value(subVersionScripts.first);
        for (auto const& subVersionUpdateScript : subVersionScripts.second) {
          if (result.subVersions.value(subVersionScripts.first) >= targetSubVersion)
            break;

          if (subVersionUpdateScript.fromVersion == result.subVersions.value(subVersionScripts.first)) {
            auto scriptContext = m_luaRoot.createContext();
            scriptContext.load(*root.assets()->bytes(subVersionUpdateScript.script), subVersionUpdateScript.script);
            scriptContext.setCallbacks("root", LuaBindings::makeRootCallbacks());
            scriptContext.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
            scriptContext.setCallbacks("celestial", celestialCallbacks);
            scriptContext.setCallbacks("versioning", makeVersioningCallbacks());

            result.content = scriptContext.invokePath<Json>("update", result.content);
            if (!result.content) {
              throw VersioningDatabaseException::format(
                  "Could not bring versionedJson with identifier '{}' and version {} forward to current version of {}, conversion script of sub identifier '{}' from {} to {} returned null (un-upgradeable)",
                  versionedJson.identifier, result.version, targetVersion, subVersionScripts.first, subVersionUpdateScript.fromVersion, subVersionUpdateScript.toVersion);
            }
            Logger::debug("Brought versionedJson '{}' sub identifier '{}' from version {} to {}",
                versionedJson.identifier, subVersionScripts.first, result.subVersions.value(subVersionScripts.first), subVersionUpdateScript.toVersion);
            result.subVersions[subVersionScripts.first] = subVersionUpdateScript.toVersion;
          }
        }
      }

      if (result.version >= *targetVersion)
        break;

      if (updateScript.fromVersion == result.version) {
        auto scriptContext = m_luaRoot.createContext();
        scriptContext.load(*root.assets()->bytes(updateScript.script), updateScript.script);
        scriptContext.setCallbacks("root", LuaBindings::makeRootCallbacks());
        scriptContext.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
        scriptContext.setCallbacks("celestial", celestialCallbacks);
        scriptContext.setCallbacks("versioning", makeVersioningCallbacks());

        result.content = scriptContext.invokePath<Json>("update", result.content);
        if (!result.content) {
          throw VersioningDatabaseException::format(
              "Could not bring versionedJson with identifier '{}' and version {} forward to current version of {}, conversion script from {} to {} returned null (un-upgradeable)",
              versionedJson.identifier, result.version, targetVersion, updateScript.fromVersion, updateScript.toVersion);
        }
        Logger::debug("Brought versionedJson '{}' from version {} to {}",
            versionedJson.identifier, result.version, updateScript.toVersion);
        result.version = updateScript.toVersion;
      }
    }
  } catch (std::exception const& e) {
    throw VersioningDatabaseException(strf("Could not bring versionedJson with identifier '{}' and version {} forward to current version of {}",
            versionedJson.identifier, result.version, targetVersion), e);
  }

  if (result.version > *targetVersion) {
    throw VersioningDatabaseException::format(
        "VersionedJson with identifier '{}' and version {} is newer than current version of {}, cannot load",
        versionedJson.identifier, result.version, targetVersion);
  }

  if (result.version != *targetVersion) {
    throw VersioningDatabaseException::format(
        "Could not bring VersionedJson with identifier '{}' and version {} forward to current version of {}, best version was {}",
        versionedJson.identifier, result.version, targetVersion, result.version);
  }

  return result;
}

Json VersioningDatabase::loadVersionedJson(VersionedJson const& versionedJson, String const& expectedIdentifier) const {
  versionedJson.expectIdentifier(expectedIdentifier);
  if (versionedJsonCurrent(versionedJson))
    return versionedJson.content;
  return updateVersionedJson(versionedJson).content;
}

LuaCallbacks VersioningDatabase::makeVersioningCallbacks() const {
  LuaCallbacks versioningCallbacks;

  versioningCallbacks.registerCallback("loadVersionedJson", [this](String const& storagePath) {
      try {
        auto& root = Root::singleton();
        String filePath = File::fullPath(root.toStoragePath(storagePath));
        String basePath = File::fullPath(root.toStoragePath("."));
        if (!filePath.beginsWith(basePath))
          throw VersioningDatabaseException::format(
              "Cannot load external VersionedJson outside of the Root storage path");
        auto loadedJson = VersionedJson::readFile(filePath);
        return updateVersionedJson(loadedJson).content;
      } catch (IOException const& e) {
        Logger::debug(
            "Unable to load versioned JSON file {} in versioning script: {}", storagePath, outputException(e, false));
        return Json();
      }
    });

  return versioningCallbacks;
}

}
