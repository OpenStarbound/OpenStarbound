#include "StarPlayerStorage.hpp"
#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarIterator.hpp"
#include "StarTime.hpp"
#include "StarConfiguration.hpp"
#include "StarPlayer.hpp"
#include "StarAssets.hpp"
#include "StarEntityFactory.hpp"
#include "StarRoot.hpp"
#include "StarText.hpp"

namespace Star {

PlayerStorage::PlayerStorage(String const& storageDir) {
  m_storageDirectory = storageDir;

  if (!File::isDirectory(m_storageDirectory)) {
    Logger::info("Creating player storage directory");
    File::makeDirectory(m_storageDirectory);
    return;
  }

  auto configuration = Root::singleton().configuration();
  if (configuration->get("clearPlayerFiles").toBool()) {
    Logger::info("Clearing all player files");
    for (auto file : File::dirList(m_storageDirectory)) {
      if (!file.second)
        File::remove(File::relativeTo(m_storageDirectory, file.first));
    }
  } else {
    auto versioningDatabase = Root::singleton().versioningDatabase();
    auto entityFactory = Root::singleton().entityFactory();

    for (auto file : File::dirList(m_storageDirectory)) {
      if (file.second)
        continue;

      String filename = File::relativeTo(m_storageDirectory, file.first);
      if (filename.endsWith(".player")) {
        try {
          Uuid uuid(file.first.rsplit('.').at(0));
          auto& playerCacheData = m_savedPlayersCache[uuid];
          playerCacheData = entityFactory->loadVersionedJson(VersionedJson::readFile(filename), EntityType::Player);
        } catch (std::exception const& e) {
          Logger::error("Error loading player file, ignoring! {} : {}", filename, outputException(e, false));
        }
      }
    }

    // Remove all the player entries that are missing player data or fail to
    // load.
    auto it = makeSMutableMapIterator(m_savedPlayersCache);
    while (it.hasNext()) {
      auto& entry = it.next();
      if (entry.second.isNull()) {
        it.remove();
      } else {
        try {
          auto entityFactory = Root::singleton().entityFactory();
          auto player = as<Player>(entityFactory->diskLoadEntity(EntityType::Player, entry.second));
          if (player->uuid() != entry.first)
            throw PlayerException(strf("Uuid mismatch in loaded player with filename uuid '{}'", entry.first.hex()));
        } catch (StarException const& e) {
          Logger::error("Failed to valid player with uuid {} : {}", entry.first.hex(), outputException(e, true));
          it.remove();
        }
      }
    }
  }

  try {
    String filename = File::relativeTo(m_storageDirectory, "metadata");
    m_metadata = Json::parseJson(File::readFileString(filename)).toObject();

    if (auto order = m_metadata.value("order")) {
      for (auto const& uuid : order.iterateArray())
        m_savedPlayersCache.toBack(Uuid(uuid.toString()));
    }
  } catch (std::exception const& e) {
    Logger::warn("Error loading player storage metadata file, resetting: {}", outputException(e, false));
  }
}

PlayerStorage::~PlayerStorage() {
  writeMetadata();
}

size_t PlayerStorage::playerCount() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_savedPlayersCache.size();
}

Maybe<Uuid> PlayerStorage::playerUuidAt(size_t index) {
  RecursiveMutexLocker locker(m_mutex);
  if (index < m_savedPlayersCache.size())
    return m_savedPlayersCache.keyAt(index);
  else
    return {};
}

Maybe<Uuid> PlayerStorage::playerUuidByName(String const& name, Maybe<Uuid> except) {
  String cleanMatch = Text::stripEscapeCodes(name).toLower();
  Maybe<Uuid> uuid;

  RecursiveMutexLocker locker(m_mutex);

  size_t longest = SIZE_MAX;
  for (auto& cache : m_savedPlayersCache) {
    if (except && *except == cache.first)
      continue;
    else if (auto name = cache.second.optQueryString("identity.name")) {
      auto cleanName = Text::stripEscapeCodes(*name).toLower();
      auto len = cleanName.size();
      if (len < longest && cleanName.utf8().rfind(cleanMatch.utf8()) == 0) {
        longest = len;
        uuid = cache.first;
      }
    }
  }

  return uuid;
}

Json PlayerStorage::savePlayer(PlayerPtr const& player) {
  auto entityFactory = Root::singleton().entityFactory();
  auto versioningDatabase = Root::singleton().versioningDatabase();

  RecursiveMutexLocker locker(m_mutex);

  auto uuid = player->uuid();

  auto& playerCacheData = m_savedPlayersCache[uuid];
  auto newPlayerData = player->diskStore();
  if (playerCacheData != newPlayerData) {
    playerCacheData = newPlayerData;
    VersionedJson versionedJson = entityFactory->storeVersionedJson(EntityType::Player, playerCacheData);
    VersionedJson::writeFile(versionedJson, File::relativeTo(m_storageDirectory, strf("{}.player", uuid.hex())));
  }
  return newPlayerData;
}

Maybe<Json> PlayerStorage::maybeGetPlayerData(Uuid const& uuid) {
  RecursiveMutexLocker locker(m_mutex);
  if (auto cache = m_savedPlayersCache.ptr(uuid))
    return *cache;
  else
    return {};
}

Json PlayerStorage::getPlayerData(Uuid const& uuid) {
  auto data = maybeGetPlayerData(uuid);
  if (!data)
    throw PlayerException(strf("No such stored player with uuid '{}'", uuid.hex()));
  else
    return *data;
}

PlayerPtr PlayerStorage::loadPlayer(Uuid const& uuid) {
  auto playerCacheData = getPlayerData(uuid);
  auto entityFactory = Root::singleton().entityFactory();
  try {
    auto player = convert<Player>(entityFactory->diskLoadEntity(EntityType::Player, playerCacheData));
    if (player->uuid() != uuid)
      throw PlayerException(strf("Uuid mismatch in loaded player with filename uuid '{}'", uuid.hex()));
    return player;
  } catch (std::exception const& e) {
    Logger::error("Error loading player file, ignoring! {}", outputException(e, false));
    RecursiveMutexLocker locker(m_mutex);
    m_savedPlayersCache.remove(uuid);
    return {};
  }
}

void PlayerStorage::deletePlayer(Uuid const& uuid) {
  RecursiveMutexLocker locker(m_mutex);
  if (!m_savedPlayersCache.contains(uuid))
    throw PlayerException(strf("No such stored player with uuid '{}'", uuid.hex()));

  m_savedPlayersCache.remove(uuid);

  auto filePrefix = File::relativeTo(m_storageDirectory, uuid.hex());

  auto removeIfExists = [&filePrefix](String suffix) {
    if (File::exists(filePrefix + suffix)) {
      File::remove(filePrefix + suffix);
    }
  };

  removeIfExists(".player");
  removeIfExists(".shipworld");

  auto configuration = Root::singleton().configuration();
  unsigned playerBackupFileCount = configuration->get("playerBackupFileCount").toUInt();

  for (unsigned i = 1; i <= playerBackupFileCount; ++i) {
    removeIfExists(strf(".player.bak{}", i));
    removeIfExists(strf(".shipworld.bak{}", i));
  }
}

WorldChunks PlayerStorage::loadShipData(Uuid const& uuid) {
  RecursiveMutexLocker locker(m_mutex);
  if (!m_savedPlayersCache.contains(uuid))
    throw PlayerException(strf("No such stored player with uuid '{}'", uuid.hex()));

  String filename = File::relativeTo(m_storageDirectory, strf("{}.shipworld", uuid.hex()));
  try {
    if (File::exists(filename))
      return WorldStorage::getWorldChunksFromFile(filename);
  } catch (StarException const& e) {
    Logger::error("Failed to load shipworld file, removing {} : {}", filename, outputException(e, false));
    File::remove(filename);
  }

  return {};
}

void PlayerStorage::applyShipUpdates(Uuid const& uuid, WorldChunks const& updates) {
  RecursiveMutexLocker locker(m_mutex);
  if (!m_savedPlayersCache.contains(uuid))
    throw PlayerException(strf("No such stored player with uuid '{}'", uuid.hex()));

  if (updates.empty())
    return;

  String filename = File::relativeTo(m_storageDirectory, strf("{}.shipworld", uuid.hex()));
  WorldStorage::applyWorldChunksUpdateToFile(filename, updates);
}

void PlayerStorage::moveToFront(Uuid const& uuid) {
  m_savedPlayersCache.toFront(uuid);
  writeMetadata();
}

void PlayerStorage::backupCycle(Uuid const& uuid) {
  RecursiveMutexLocker locker(m_mutex);

  auto configuration = Root::singleton().configuration();
  unsigned playerBackupFileCount = configuration->get("playerBackupFileCount").toUInt();

  File::backupFileInSequence(File::relativeTo(m_storageDirectory, strf("{}.player", uuid.hex())), playerBackupFileCount, ".bak");
  File::backupFileInSequence(File::relativeTo(m_storageDirectory, strf("{}.shipworld", uuid.hex())), playerBackupFileCount, ".bak");
  File::backupFileInSequence(File::relativeTo(m_storageDirectory, strf("{}.metadata", uuid.hex())), playerBackupFileCount, ".bak");
}

void PlayerStorage::setMetadata(String key, Json value) {
  auto& val = m_metadata[move(key)];
  if (val != value) {
    val = move(value);
    writeMetadata();
  }
}

Json PlayerStorage::getMetadata(String const& key) {
  return m_metadata.value(key);
}

void PlayerStorage::writeMetadata() {
  JsonArray order;
  for (auto const& p : m_savedPlayersCache)
    order.append(p.first.hex());

  m_metadata["order"] = move(order);

  String filename = File::relativeTo(m_storageDirectory, "metadata");
  File::overwriteFileWithRename(Json(m_metadata).printJson(0), filename);
}

}
