#pragma once

#include "StarBTreeDatabase.hpp"
#include "StarVersioningDatabase.hpp"
#include "StarEntity.hpp"
#include "StarOrderedSet.hpp"
#include "StarWorldTiles.hpp"
#include "StarRpcPromise.hpp"
#include "StarBiomePlacement.hpp"

namespace Star {

STAR_EXCEPTION(WorldStorageException, StarException);

STAR_CLASS(EntityMap);
STAR_STRUCT(WorldGeneratorFacade);
STAR_CLASS(WorldStorage);

typedef HashMap<ByteArray, Maybe<ByteArray>> WorldChunks;

enum class SectorLoadLevel : uint8_t {
  None = 0,
  Tiles = 1,
  Entities = 2,

  Loaded = 2
};

enum class SectorGenerationLevel : uint8_t {
  None = 0,
  BaseTiles = 1,
  MicroDungeons = 2,
  CaveLiquid = 3,
  Finalize = 4,

  Complete = 4,

  Terraform = 5
};

struct WorldGeneratorFacade {
  typedef ServerTileSectorArray::Sector Sector;

  WorldGeneratorFacade() {}
  virtual ~WorldGeneratorFacade() {}

  // Should bring a given sector from generationLevel - 1 to generationLevel.
  virtual void generateSectorLevel(WorldStorage* storage, Sector const& sector, SectorGenerationLevel generationLevel) = 0;

  virtual void sectorLoadLevelChanged(WorldStorage* storage, Sector const& sector, SectorLoadLevel loadLevel) = 0;

  // Perform terraforming operations (biome reapplication) on the given sector
  virtual void terraformSector(WorldStorage* storage, Sector const& sector) = 0;

  // Called after an entity is loaded, but before the entity is added to the
  // EntityMap.
  virtual void initEntity(WorldStorage* storage, EntityId newEntityId, EntityPtr const& entity) = 0;

  // Called after the entity is removed from the entity map but before it is
  // stored.
  virtual void destructEntity(WorldStorage* storage, EntityPtr const& entity) = 0;

  // Should return true if this entity should maintain the sector, false
  // otherwise.
  virtual bool entityKeepAlive(WorldStorage* storage, EntityPtr const& entity) const = 0;

  // Should return true if this entity should be stored along with the world,
  // false otherwise.
  virtual bool entityPersistent(WorldStorage* storage, EntityPtr const& entity) const = 0;
  
  // Queues up a microdungeon. Fulfills the rpc promise with the position the
  // microdungeon was placed at
  virtual RpcPromise<Vec2I> enqueuePlacement(List<BiomeItemDistribution> placements, Maybe<DungeonId> id) = 0;
};

// Handles paging entity and tile data in / out of disk backed storage for
// WorldServer and triggers initial generation.  Ties tile sectors to entity
// sectors, and allows for multiple stage generation of those sectors.  Sector
// generation is done in stages, so that lower generation stages are done in a
// one sector border around the higher generation stages.
//
// WorldStorage is designed so that once constructed, any exceptions triggered
// during loading, unloading, or generation that would result in an
// indeterminate world state cause the underlying database to be rolled back
// and then immediately closed.  The underlying database committed only when
// destructed without error, or a manual call to sync().
class WorldStorage {
public:
  typedef ServerTileSectorArray::Sector Sector;
  typedef ServerTileSectorArray::Array TileArray;
  typedef ServerTileSectorArray::ArrayPtr TileArrayPtr;

  static WorldChunks getWorldChunksUpdate(WorldChunks const& oldChunks, WorldChunks const& newChunks);
  static void applyWorldChunksUpdateToFile(String const& file, WorldChunks const& update);
  static WorldChunks getWorldChunksFromFile(String const& file);

  // Create a new world of the given size.
  WorldStorage(Vec2U const& worldSize, IODevicePtr const& device, WorldGeneratorFacadePtr const& generatorFacade);
  // Read an existing world.
  WorldStorage(IODevicePtr const& device, WorldGeneratorFacadePtr const& generatorFacade);
  // Read an in-memory world.
  WorldStorage(WorldChunks const& chunks, WorldGeneratorFacadePtr const& generatorFacade);
  ~WorldStorage();

  VersionedJson worldMetadata();
  void setWorldMetadata(VersionedJson const& metadata);

  ServerTileSectorArrayPtr const& tileArray() const;
  EntityMapPtr const& entityMap() const;

  Maybe<Sector> sectorForPosition(Vec2I const& position) const;
  List<Sector> sectorsForRegion(RectI const& region) const;
  Maybe<RectI> regionForSector(Sector sector) const;

  SectorLoadLevel sectorLoadLevel(Sector sector) const;
  // Returns the sector generation level if it is currently loaded, nothing
  // otherwise.
  Maybe<SectorGenerationLevel> sectorGenerationLevel(Sector sector) const;
  // Returns true if the sector is both loaded and fully generated.
  bool sectorActive(Sector) const;

  // Fully load the given sector and reset its TTL without triggering any
  // generation.
  void loadSector(Sector sector);
  // Fully load, reset the TTL, and if necessary, fully generate the given
  // sector.
  void activateSector(Sector sector);
  // Queue the given sector for activation, if it is not already active.  If
  // the sector is loaded at all, also resets the TTL.
  void queueSectorActivation(Sector sector);

  // Immediately (synchronously) fully generates the sector, then flags it as requiring
  // terraforming (biome reapplication) which will be handled by the normal generation process
  void triggerTerraformSector(Sector sector);
  
  // Queues up a microdungeon. Fulfills the rpc promise with the position the
  // microdungeon was placed at
  RpcPromise<Vec2I> enqueuePlacement(List<BiomeItemDistribution> placements, Maybe<DungeonId> id);

  // Return the remaining time to live for a sector, if loaded.  A sector's
  // time to live is reset when loaded or generated, and when the time to live
  // reaches zero, the sector is automatically unloaded.
  Maybe<float> sectorTimeToLive(Sector sector) const;
  // Set the given sector's time to live, if it is loaded at all.  Returns
  // false if the sector was not loaded so no action was taken.
  bool setSectorTimeToLive(Sector sector, float newTimeToLive);

  // Returns the position for a given unique entity if it exists in this world,
  // loaded or not.
  Maybe<Vec2F> findUniqueEntity(String const& uniqueId);

  // If the given unique entity is not loaded, loads its sector and then if the
  // unique entity is found, returns the entity id, otherwise NullEntityId.
  EntityId loadUniqueEntity(String const& uniqueId);

  // Does any queued generation work, potentially limiting the total number of
  // increases of SectorGenerationLevel by the sectorGenerationLevelLimit, if
  // given.  If sectorOrdering is given, then it will be used to prioritize the
  // queued sectors.
  void generateQueue(Maybe<size_t> sectorGenerationLevelLimit, function<bool(Sector, Sector)> sectorOrdering = {});
  // Ticks down the TTL on sectors and generation queue entries, stores old
  // sectors, expires old generation queue entries, and unloads any zombie
  // entities.
  void tick(float dt, String const* worldId = nullptr);

  // Unload all sectors that can be unloaded (if force is specified, ALWAYS
  // unloads all sectors)
  void unloadAll(bool force = false);

  // Sync all active sectors without unloading them, and commits the underlying
  // database.
  void sync();

  // Syncs all active sectors to disk and stores the full content of the world
  // into memory.
  WorldChunks readChunks();

  // if this is set, all terrain generation is assumed to be handled by dungeon placement
  // and steps such as microdungeons, biome objects and grass mods will be skipped
  bool floatingDungeonWorld() const;
  void setFloatingDungeonWorld(bool floatingDungeonWorld);

private:
  enum class StoreType : uint8_t {
    Metadata = 0,
    TileSector = 1,
    EntitySector = 2,
    UniqueIndex = 3,
    SectorUniques = 4
  };

  typedef pair<Sector, Vec2F> SectorAndPosition;

  struct WorldMetadataStore {
    Vec2U worldSize;
    VersionedJson userMetadata;
  };

  typedef List<VersionedJson> EntitySectorStore;
  // Map of uuid to entity's position and sector they were stored in.
  typedef HashMap<String, SectorAndPosition> UniqueIndexStore;
  // Set of unique ids that are stored in a given sector
  typedef HashSet<String> SectorUniqueStore;

  struct TileSectorStore {
    TileSectorStore();

    // Also store generation level along with tiles, simply because tiles are
    // the first things to be loaded and the last to be stored.
    SectorGenerationLevel generationLevel;

    VersionNumber tileSerializationVersion;
    TileArrayPtr tiles;
  };

  struct SectorMetadata {
    SectorMetadata();

    SectorLoadLevel loadLevel;
    SectorGenerationLevel generationLevel;
    float timeToLive;
  };

  static ByteArray metadataKey();
  static WorldMetadataStore readWorldMetadata(ByteArray const& data);
  static ByteArray writeWorldMetadata(WorldMetadataStore const& metadata);

  static ByteArray entitySectorKey(Sector const& sector);
  static EntitySectorStore readEntitySector(ByteArray const& data);
  static ByteArray writeEntitySector(EntitySectorStore const& store);

  static ByteArray tileSectorKey(Sector const& sector);
  static TileSectorStore readTileSector(ByteArray const& data);
  static ByteArray writeTileSector(TileSectorStore const& store);

  static ByteArray uniqueIndexKey(String const& uniqueId);
  static UniqueIndexStore readUniqueIndexStore(ByteArray const& data);
  static ByteArray writeUniqueIndexStore(UniqueIndexStore const& store);

  static ByteArray sectorUniqueKey(Sector const& sector);
  static SectorUniqueStore readSectorUniqueStore(ByteArray const& data);
  static ByteArray writeSectorUniqueStore(SectorUniqueStore const& store);

  static void openDatabase(BTreeDatabase& db, IODevicePtr device);

  WorldStorage();

  bool belongsInSector(Sector const& sector, Vec2F const& position) const;

  // Generate a random TTL value in the configured range
  float randomizedSectorTTL() const;

  // Generate the given sector to the given generation level.  If
  // sectorGenerationLevelLimit is given, stops work as soon as the given
  // number of generation level changes has occurred.  Returns whether the
  // given sector was fully generated, and the total number of generation
  // levels increased.  If any sector's generation level is brought up at all,
  // it will also reset the TTL for that sector.
  pair<bool, size_t> generateSectorToLevel(Sector const& sector, SectorGenerationLevel targetGenerationLevel, size_t sectorGenerationLevelLimit = NPos);

  // Bring the sector up to the given load level, and all surrounding sectors
  // as appropriate.  If the load level is brought up, also resets the TTL.
  void loadSectorToLevel(Sector const& sector, SectorLoadLevel targetLoadLevel);

  // Store and unload the given sector to the given level, given the state of
  // the surrounding sectors.  If force is true, will always unload to the
  // given level.
  bool unloadSectorToLevel(Sector const& sector, SectorLoadLevel targetLoadLevel, bool force = false);

  // Sync this sector to disk without unloading it.
  void syncSector(Sector const& sector);

  // Returns the sectors within WorldSectorSize of the given sector.  This is
  // *not exactly the same* as the surrounding 9 sectors in a square pattern,
  // because first this does not return invalid sectors, and second, If a world
  // is not evenly divided by the sector size, this may return extra sectors on
  // one side because they are within range.
  List<Sector> adjacentSectors(Sector const& sector) const;

  // Replace the sector uniques for this sector with the given set
  void updateSectorUniques(Sector const& sector, UniqueIndexStore const& sectorUniques);
  // Merge the stored sector uniques for this sector with the given set
  void mergeSectorUniques(Sector const& sector, UniqueIndexStore const& sectorUniques);

  Maybe<SectorAndPosition> getUniqueIndexEntry(String const& uniqueId);
  void setUniqueIndexEntry(String const& uniqueId, SectorAndPosition const& sectorAndPosition);
  // Remove the index entry for this unique id, if the index entry found points
  // to the given sector
  void removeUniqueIndexEntry(String const& uniqueId, Sector const& sector);

  Vec2F m_sectorTimeToLive;
  float m_generationQueueTimeToLive;

  ServerTileSectorArrayPtr m_tileArray;
  EntityMapPtr m_entityMap;
  WorldGeneratorFacadePtr m_generatorFacade;

  bool m_floatingDungeonWorld;

  StableHashMap<Sector, SectorMetadata> m_sectorMetadata;
  OrderedHashMap<Sector, float> m_generationQueue;
  BTreeDatabase m_db;
};

}
