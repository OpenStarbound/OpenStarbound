#ifndef STAR_PLAYER_STORAGE_HPP
#define STAR_PLAYER_STORAGE_HPP

#include "StarOrderedMap.hpp"
#include "StarUuid.hpp"
#include "StarPlayerFactory.hpp"
#include "StarThread.hpp"
#include "StarWorldStorage.hpp"
#include "StarStatistics.hpp"

namespace Star {

class PlayerStorage {
public:
  PlayerStorage(String const& storageDir);
  ~PlayerStorage();

  size_t playerCount() const;
  // Returns nothing if index is out of bounds.
  Maybe<Uuid> playerUuidAt(size_t index);

  void savePlayer(PlayerPtr const& player);
  PlayerPtr loadPlayer(Uuid const& uuid);
  void deletePlayer(Uuid const& uuid);

  WorldChunks loadShipData(Uuid const& uuid);
  void applyShipUpdates(Uuid const& uuid, WorldChunks const& updates);

  // Move the given player to the top of the player ordering.
  void moveToFront(Uuid const& uuid);

  // Copy all the player relevant files for this uuid into .bak1 .bak2 etc
  // files for however many backups are configured
  void backupCycle(Uuid const& uuid);

  // Get / Set PlayerStorage global metadata
  void setMetadata(String key, Json value);
  Json getMetadata(String const& key);

private:
  void writeMetadata();

  mutable RecursiveMutex m_mutex;
  String m_storageDirectory;
  OrderedHashMap<Uuid, Json> m_savedPlayersCache;
  JsonObject m_metadata;
};

}

#endif
