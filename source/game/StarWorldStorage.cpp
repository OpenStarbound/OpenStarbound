#include "StarWorldStorage.hpp"
#include "StarFile.hpp"
#include "StarCompression.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarIterator.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarEntityMap.hpp"
#include "StarEntityFactory.hpp"
#include "StarAssets.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"

namespace Star {

WorldChunks WorldStorage::getWorldChunksUpdate(WorldChunks const& oldChunks, WorldChunks const& newChunks) {
  WorldChunks update;
  for (auto const& p : oldChunks) {
    if (!newChunks.contains(p.first))
      update[p.first] = {};
  }

  for (auto const& p : newChunks) {
    if (oldChunks.value(p.first) != p.second)
      update[p.first] = p.second;
  }
  return update;
}

void WorldStorage::applyWorldChunksUpdateToFile(String const& file, WorldChunks const& update) {
  BTreeDatabase db;
  openDatabase(db, File::open(file, IOMode::ReadWrite));

  for (auto const& p : update) {
    if (p.second)
      db.insert(p.first, *p.second);
    else
      db.remove(p.first);
  }
}

WorldChunks WorldStorage::getWorldChunksFromFile(String const& file) {
  BTreeDatabase db;
  openDatabase(db, File::open(file, IOMode::Read));

  WorldChunks chunks;
  db.forAll([&chunks](ByteArray key, ByteArray value) { chunks.add(std::move(key), std::move(value)); });
  return chunks;
}

WorldStorage::WorldStorage(Vec2U const& worldSize, IODevicePtr const& device, WorldGeneratorFacadePtr const& generatorFacade)
  : WorldStorage() {
  m_tileArray = make_shared<ServerTileSectorArray>(worldSize);
  m_entityMap = make_shared<EntityMap>(worldSize, MinServerEntityId, MaxServerEntityId);
  m_generatorFacade = generatorFacade;
  m_floatingDungeonWorld = false;

  // Creating a new world, clear any existing data.
  device->resize(0);

  openDatabase(m_db, device);

  m_db.insert(metadataKey(), writeWorldMetadata(WorldMetadataStore{worldSize, VersionedJson()}));
  m_db.commit();
}

WorldStorage::WorldStorage(IODevicePtr const& device, WorldGeneratorFacadePtr const& generatorFacade) : WorldStorage() {
  m_generatorFacade = generatorFacade;
  m_floatingDungeonWorld = false;

  openDatabase(m_db, device);

  Vec2U worldSize = readWorldMetadata(*m_db.find(metadataKey())).worldSize;
  m_tileArray = make_shared<ServerTileSectorArray>(worldSize);
  m_entityMap = make_shared<EntityMap>(worldSize, MinServerEntityId, MaxServerEntityId);
}

WorldStorage::WorldStorage(WorldChunks const& chunks, WorldGeneratorFacadePtr const& generatorFacade) : WorldStorage() {
  m_generatorFacade = generatorFacade;
  m_floatingDungeonWorld = false;

  openDatabase(m_db, File::ephemeralFile());

  for (auto const& p : chunks) {
    if (p.second)
      m_db.insert(p.first, *p.second);
  }

  Vec2U worldSize = readWorldMetadata(*m_db.find(metadataKey())).worldSize;
  m_tileArray = make_shared<ServerTileSectorArray>(worldSize);
  m_entityMap = make_shared<EntityMap>(worldSize, MinServerEntityId, MaxServerEntityId);
}

WorldStorage::~WorldStorage() {
  if (m_db.isOpen()) {
    unloadAll(true);
    m_db.close();
  }
}

VersionedJson WorldStorage::worldMetadata() {
  return readWorldMetadata(*m_db.find(metadataKey())).userMetadata;
}

void WorldStorage::setWorldMetadata(VersionedJson const& metadata) {
  m_db.insert(metadataKey(), writeWorldMetadata({Vec2U(m_tileArray->size()), metadata}));
}

ServerTileSectorArrayPtr const& WorldStorage::tileArray() const {
  return m_tileArray;
}

EntityMapPtr const& WorldStorage::entityMap() const {
  return m_entityMap;
}

Maybe<WorldStorage::Sector> WorldStorage::sectorForPosition(Vec2I const& position) const {
  auto s = m_tileArray->sectorFor(position);
  if (m_tileArray->sectorValid(s))
    return s;
  return {};
}

List<WorldStorage::Sector> WorldStorage::sectorsForRegion(RectI const& region) const {
  return m_tileArray->validSectorsFor(region);
}

Maybe<RectI> WorldStorage::regionForSector(Sector sector) const {
  if (m_tileArray->sectorValid(sector))
    return m_tileArray->sectorRegion(sector);
  return {};
}

SectorLoadLevel WorldStorage::sectorLoadLevel(Sector sector) const {
  return m_sectorMetadata.value(sector).loadLevel;
}

Maybe<SectorGenerationLevel> WorldStorage::sectorGenerationLevel(Sector sector) const {
  if (auto p = m_sectorMetadata.ptr(sector))
    return p->generationLevel;
  return {};
}

bool WorldStorage::sectorActive(Sector sector) const {
  if (auto p = m_sectorMetadata.ptr(sector)) {
    if (p->loadLevel == SectorLoadLevel::Loaded && p->generationLevel == SectorGenerationLevel::Complete)
      return true;
  }
  return false;
}

void WorldStorage::loadSector(Sector sector) {
  try {
    loadSectorToLevel(sector, SectorLoadLevel::Loaded);
    setSectorTimeToLive(sector, randomizedSectorTTL());
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException(strf("Failed to load sector {}", sector), e);
  }
}

void WorldStorage::activateSector(Sector sector) {
  try {
    generateSectorToLevel(sector, SectorGenerationLevel::Complete);
    setSectorTimeToLive(sector, randomizedSectorTTL());
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException(strf("Failed to load sector {}", sector), e);
  }
}

void WorldStorage::queueSectorActivation(Sector sector) {
  if (auto p = m_sectorMetadata.ptr(sector)) {
    p->timeToLive = randomizedSectorTTL();
    // Don't bother queueing the sector if it is already fully loaded
    if (p->loadLevel == SectorLoadLevel::Loaded && p->generationLevel == SectorGenerationLevel::Complete)
      return;
  }

  auto p = m_generationQueue.insert(sector, m_generationQueueTimeToLive);
  m_generationQueue.toFront(p.first);
}

void WorldStorage::triggerTerraformSector(Sector sector) {
  try {
    loadSectorToLevel(sector, SectorLoadLevel::Loaded);
    if (auto p = m_sectorMetadata.ptr(sector)) {
      if (p->generationLevel < SectorGenerationLevel::Complete)
        generateSectorToLevel(sector, SectorGenerationLevel::Complete);

      p->generationLevel = SectorGenerationLevel::Terraform;
    } else {
      throw WorldStorageException(strf("Couldn't flag sector {} for terraforming; metadata unavailable", sector));
    }
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException(strf("Failed to terraform sector {}", sector), e);
  }
}

RpcPromise<Vec2I> WorldStorage::enqueuePlacement(List<BiomeItemDistribution> distributions, Maybe<DungeonId> id) {
  return m_generatorFacade->enqueuePlacement(std::move(distributions), id);
}

Maybe<float> WorldStorage::sectorTimeToLive(Sector sector) const {
  if (auto p = m_sectorMetadata.ptr(sector))
    return p->timeToLive;
  return {};
}

bool WorldStorage::setSectorTimeToLive(Sector sector, float newTimeToLive) {
  if (auto p = m_sectorMetadata.ptr(sector)) {
    p->timeToLive = newTimeToLive;
    return true;
  }
  return false;
}

Maybe<Vec2F> WorldStorage::findUniqueEntity(String const& uniqueId) {
  if (auto entity = m_entityMap->entity(m_entityMap->uniqueEntityId(uniqueId)))
    return entity->position();

  // Only return the unique index entry for the entity IF that stored sector is
  // not loaded, if the stored sector is loaded then the entity ought to have
  // been in the live entity map.
  if (auto sectorAndPosition = getUniqueIndexEntry(uniqueId)) {
    if (m_sectorMetadata.value(sectorAndPosition->first).loadLevel < SectorLoadLevel::Entities)
      return sectorAndPosition->second;
  }

  return {};
}

EntityId WorldStorage::loadUniqueEntity(String const& uniqueId) {
  EntityId entityId = m_entityMap->uniqueEntityId(uniqueId);
  if (entityId != NullEntityId)
    return entityId;

  if (auto sectorAndPosition = getUniqueIndexEntry(uniqueId)) {
    loadSector(sectorAndPosition->first);
    return m_entityMap->uniqueEntityId(uniqueId);
  }

  return {};
}

void WorldStorage::generateQueue(Maybe<size_t> sectorGenerationLevelLimit, function<bool(Sector, Sector)> sectorOrdering) {
  try {
    if (sectorOrdering) {
      m_generationQueue.sort([&sectorOrdering](auto const& a, auto const& b) {
          return sectorOrdering(a.first, b.first);
        });
    }

    while (!m_generationQueue.empty()) {
      if (sectorGenerationLevelLimit && *sectorGenerationLevelLimit == 0)
        break;

      auto p = generateSectorToLevel(m_generationQueue.firstKey(), SectorGenerationLevel::Complete, sectorGenerationLevelLimit.value(NPos));
      if (p.first)
        m_generationQueue.removeFirst();
      if (sectorGenerationLevelLimit)
        *sectorGenerationLevelLimit -= p.second;
    }
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException("WorldStorage generation failed while generating from queue", e);
  }
}

void WorldStorage::tick(float dt, String const* worldId) {
  try {
    // Tick down generation queue entries, and erase any that are expired.
    eraseWhere(m_generationQueue, [dt](auto& p) {
        p.second -= dt;
        return p.second <= 0.0f;
      });

    // Tick down sector TTL values
    for (auto& p : m_sectorMetadata)
      p.second.timeToLive -= dt;

    // Loop over every loaded sector, figure out whether the sector needs to be
    // unloaded, kept alive by a keep-alive entity, or has any entities that need
    // to be stored because they moved into an entity-unloaded sector (zombies).
    auto entityFactory = Root::singleton().entityFactory();
    unsigned unloaded = 0, skipped = 0;
    for (auto const& p : m_sectorMetadata.pairs()) {
      auto const& sector = p.first;
      auto const& metadata = p.second;

      bool needsUnload = metadata.timeToLive <= 0.0f;

      // If it is not time to unload the sector, then we don't need to scan for
      // keep-alive entities.  If the sector is fully loaded, it can not have any
      // zombie entities.  If both of these are true, there is no work to do.
      if (!needsUnload && metadata.loadLevel == SectorLoadLevel::Entities)
        continue;

      bool keepAlive = false;
      List<EntityPtr> zombieEntities;
      m_entityMap->forEachEntity(RectF(m_tileArray->sectorRegion(sector)), [&](EntityPtr const& entity) {
          if (belongsInSector(sector, entity->position())) {
            if (!keepAlive && m_generatorFacade->entityKeepAlive(this, entity))
              keepAlive = true;
            else if (metadata.loadLevel < SectorLoadLevel::Entities)
              zombieEntities.append(entity);
          }
        });

      if (keepAlive) {
        setSectorTimeToLive(sector, randomizedSectorTTL());
      } else if (needsUnload) {
        (unloadSectorToLevel(sector, SectorLoadLevel::None) ? unloaded : skipped)++;
      } else if (!zombieEntities.empty()) {
        List<EntityPtr> zombiesToStore;
        List<EntityPtr> zombiesToRemove;
        for (auto const& entity : zombieEntities) {
          if (m_generatorFacade->entityPersistent(this, entity))
            zombiesToStore.append(entity);
          else
            zombiesToRemove.append(entity);
        }

        for (auto const& entity : zombiesToRemove) {
          m_entityMap->removeEntity(entity->entityId());
          m_generatorFacade->destructEntity(this, entity);
        }

        if (!zombiesToStore.empty()) {
          EntitySectorStore sectorStore;
          if (auto res = m_db.find(entitySectorKey(sector)))
            sectorStore = readEntitySector(*res);

          UniqueIndexStore storedUniques;
          for (auto const& entity : zombiesToStore) {
            m_entityMap->removeEntity(entity->entityId());
            m_generatorFacade->destructEntity(this, entity);
            if (auto uniqueId = entity->uniqueId())
              storedUniques.add(*uniqueId, {sector, entity->position()});
            sectorStore.append(entityFactory->storeVersionedEntity(entity));
          }
          m_db.insert(entitySectorKey(sector), writeEntitySector(sectorStore));
          mergeSectorUniques(sector, storedUniques);
        }
      }
    }
    if (worldId) {
      LogMap::set(strf("server_{}_storage", *worldId),
        strf("{} active, {}/{} unloaded ({} held)", m_sectorMetadata.size(), unloaded, skipped + unloaded, skipped));
    }
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException("WorldStorage exception during tick", e);
  }
}

void WorldStorage::unloadAll(bool force) {
  try {
    auto storageConfig = Root::singleton().assets()->json("/worldstorage.config");
    auto sectors = m_sectorMetadata.keys();

    // Entities can do some strange things during unload, such as repeatedly
    // creating new entities during uninit, or setting their bounding box null
    // or being entirely outside of the world geometry.  This limits the number
    // of tries to completely uninit and store all entities before giving up
    // and just letting some entities not be stored.
    if (m_entityMap) {
      unsigned forceUnloadTries = storageConfig.getUInt("forceUnloadTries");
      for (unsigned i = 0; i < forceUnloadTries; ++i) {
        for (auto& sector : sectors)
          unloadSectorToLevel(sector, SectorLoadLevel::Tiles, force);

        if (!force || m_entityMap->size() == 0)
          break;
      }
    }
    for (auto& sector : sectors)
      unloadSectorToLevel(sector, SectorLoadLevel::None, force);

  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException("WorldStorage exception during unload", e);
  }
}

void WorldStorage::sync() {
  try {
    for (auto const& pair : m_sectorMetadata)
      syncSector(pair.first);
    m_db.commit();
  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException("WorldStorage exception during sync", e);
  }
}

WorldChunks WorldStorage::readChunks() {
  try {
    for (auto const& pair : m_sectorMetadata)
      syncSector(pair.first);

    WorldChunks chunks;
    m_db.forAll([&chunks](ByteArray k, ByteArray v) {
        chunks.add(std::move(k), std::move(v));
      });

    return WorldChunks(chunks);

  } catch (std::exception const& e) {
    m_db.rollback();
    m_db.close();
    throw WorldStorageException("WorldStorage exception during readChunks", e);
  }
}

bool WorldStorage::floatingDungeonWorld() const {
  return m_floatingDungeonWorld;
}

void WorldStorage::setFloatingDungeonWorld(bool floatingDungeonWorld) {
  m_floatingDungeonWorld = floatingDungeonWorld;
}

WorldStorage::TileSectorStore::TileSectorStore()
  : tileSerializationVersion(ServerTile::CurrentSerializationVersion) {}

WorldStorage::SectorMetadata::SectorMetadata()
  : loadLevel(SectorLoadLevel::None), generationLevel(SectorGenerationLevel::None), timeToLive(0.0f) {}

ByteArray WorldStorage::metadataKey() {
  DataStreamBuffer metadata(5);
  metadata.write(StoreType::Metadata);
  return metadata.takeData();
}

WorldStorage::WorldMetadataStore WorldStorage::readWorldMetadata(ByteArray const& data) {
  DataStreamBuffer ds(uncompressData(data));

  WorldMetadataStore metadata;
  ds.read(metadata.worldSize);
  ds.read(metadata.userMetadata);

  return metadata;
}

ByteArray WorldStorage::writeWorldMetadata(WorldMetadataStore const& metadata) {
  DataStreamBuffer ds;

  ds.write(metadata.worldSize);
  ds.write(metadata.userMetadata);

  return compressData(ds.data());
}

ByteArray WorldStorage::entitySectorKey(Sector const& sector) {
  DataStreamBuffer ds(5);
  ds.write(StoreType::EntitySector);
  ds.cwrite<uint16_t>(sector[0]);
  ds.cwrite<uint16_t>(sector[1]);
  return ds.takeData();
}

WorldStorage::EntitySectorStore WorldStorage::readEntitySector(ByteArray const& data) {
  return DataStreamBuffer::deserialize<EntitySectorStore>(uncompressData(data));
}

ByteArray WorldStorage::writeEntitySector(EntitySectorStore const& store) {
  return compressData(DataStreamBuffer::serialize(store));
}

ByteArray WorldStorage::tileSectorKey(Sector const& sector) {
  DataStreamBuffer ds(5);
  ds.write(StoreType::TileSector);
  ds.cwrite<uint16_t>(sector[0]);
  ds.cwrite<uint16_t>(sector[1]);
  return ds.takeData();
}

WorldStorage::TileSectorStore WorldStorage::readTileSector(ByteArray const& data) {
  auto& root = Root::singleton();
  auto matDatabase = root.materialDatabase();
  auto liqDatabase = root.liquidsDatabase();
  auto storageConfig = root.assets()->json("/worldstorage.config");

  DataStreamBuffer ds(uncompressData(data));
  TileSectorStore store;
  ds.vuread(store.generationLevel);
  ds.vuread(store.tileSerializationVersion);

  store.tiles.reset(new TileArray());
  for (size_t y = 0; y < WorldSectorSize; ++y) {
    for (size_t x = 0; x < WorldSectorSize; ++x) {
      ServerTile tile;
      tile.read(ds, store.tileSerializationVersion);

      if (!matDatabase->isValidMaterialId(tile.foreground))
        tile.foreground = storageConfig.getUInt("replacementMaterialId");
      if (!matDatabase->isValidMaterialId(tile.background))
        tile.background = storageConfig.getUInt("replacementMaterialId");
      if (!matDatabase->isValidModId(tile.foregroundMod))
        tile.foregroundMod = storageConfig.getUInt("replacementModId");
      if (!matDatabase->isValidModId(tile.backgroundMod))
        tile.backgroundMod = storageConfig.getUInt("replacementModId");
      if (!liqDatabase->isValidLiquidId(tile.liquid.liquid)) {
        LiquidId replacementLiquid = storageConfig.getUInt("replacementLiquidId");
        if (replacementLiquid == EmptyLiquidId)
          tile.liquid = LiquidStore();
        else
          tile.liquid.liquid = replacementLiquid;
      }

      (*store.tiles)(x, y) = tile;
    }
  }

  return store;
}

ByteArray WorldStorage::writeTileSector(TileSectorStore const& store) {
  DataStreamBuffer ds;
  ds.vuwrite(store.generationLevel);
  ds.vuwrite(store.tileSerializationVersion);
  starAssert(store.tiles);
  for (size_t y = 0; y < WorldSectorSize; ++y) {
    for (size_t x = 0; x < WorldSectorSize; ++x)
      (*store.tiles)(x, y).write(ds);
  }
  return compressData(ds.takeData());
}

ByteArray WorldStorage::uniqueIndexKey(String const& uniqueId) {
  DataStreamBuffer ds(5);
  ds.write(StoreType::UniqueIndex);
  ds.write(xxHash32(uniqueId));
  return ds.takeData();
}

WorldStorage::UniqueIndexStore WorldStorage::readUniqueIndexStore(ByteArray const& data) {
  return DataStreamBuffer::deserializeMapContainer<UniqueIndexStore>(uncompressData(data),
      [](DataStream& ds, String& key, SectorAndPosition& value) {
        ds.read(key);
        ds.cread<uint16_t>(value.first[0]);
        ds.cread<uint16_t>(value.first[1]);
        ds.read(value.second);
      });
}

ByteArray WorldStorage::writeUniqueIndexStore(UniqueIndexStore const& store) {
  return compressData(DataStreamBuffer::serializeMapContainer(store,
      [](DataStream& ds, String const& key, SectorAndPosition const& value) {
        ds.write(key);
        ds.cwrite<uint16_t>(value.first[0]);
        ds.cwrite<uint16_t>(value.first[1]);
        ds.write(value.second);
      }));
}

ByteArray WorldStorage::sectorUniqueKey(Sector const& sector) {
  DataStreamBuffer ds(5);
  ds.write(StoreType::SectorUniques);
  ds.cwrite<uint16_t>(sector[0]);
  ds.cwrite<uint16_t>(sector[1]);
  return ds.takeData();
}

WorldStorage::SectorUniqueStore WorldStorage::readSectorUniqueStore(ByteArray const& data) {
  return DataStreamBuffer::deserialize<SectorUniqueStore>(uncompressData(data));
}

ByteArray WorldStorage::writeSectorUniqueStore(SectorUniqueStore const& store) {
  return compressData(DataStreamBuffer::serialize(store));
}

void WorldStorage::openDatabase(BTreeDatabase& db, IODevicePtr device) {
  db.setContentIdentifier("World4");
  db.setKeySize(5);
  db.setIODevice(std::move(device));
  db.setBlockSize(2048);
  db.setAutoCommit(false);
  db.open();

  if (db.contentIdentifier() != "World4" || db.keySize() != 5)
    throw WorldStorageException::format("World database format is too old or unrecognized!");
}

WorldStorage::WorldStorage() {
  auto storageConfig = Root::singleton().assets()->json("/worldstorage.config");
  m_sectorTimeToLive = jsonToVec2F(storageConfig.get("sectorTimeToLive"));
  m_generationQueueTimeToLive = storageConfig.getFloat("generationQueueTimeToLive");
}

bool WorldStorage::belongsInSector(Sector const& sector, Vec2F const& position) const {
  WorldGeometry geometry(m_tileArray->size());
  return RectF(m_tileArray->sectorRegion(sector)).belongs(geometry.limit(position));
}

float WorldStorage::randomizedSectorTTL() const {
  return Random::randf(m_sectorTimeToLive[0], m_sectorTimeToLive[1]);
}

pair<bool, size_t> WorldStorage::generateSectorToLevel(Sector const& sector, SectorGenerationLevel targetGenerationLevel, size_t sectorGenerationLevelLimit) {
  if (!m_tileArray->sectorValid(sector))
    return {false, 0};

  loadSectorToLevel(sector, SectorLoadLevel::Loaded);

  auto& metadata = m_sectorMetadata[sector];

  if (targetGenerationLevel == SectorGenerationLevel::Complete && metadata.generationLevel == SectorGenerationLevel::Terraform) {
    m_generatorFacade->terraformSector(this, sector);
    metadata.generationLevel = SectorGenerationLevel::Complete;
    metadata.timeToLive = randomizedSectorTTL();
    return {true, 1};
  }

  if (metadata.generationLevel >= targetGenerationLevel)
    return {true, 0};

  metadata.timeToLive = randomizedSectorTTL();

  size_t totalGeneratedLevels = 0;
  for (uint8_t i = (uint8_t)metadata.generationLevel + 1; i <= (uint8_t)targetGenerationLevel; ++i) {
    SectorGenerationLevel currentGeneration = (SectorGenerationLevel)i;
    SectorGenerationLevel stepDownGeneration = (SectorGenerationLevel)(i - 1);

    if (stepDownGeneration != SectorGenerationLevel::None) {
      for (auto adjacentSector : adjacentSectors(sector)) {
        auto p = generateSectorToLevel(adjacentSector, stepDownGeneration, sectorGenerationLevelLimit - totalGeneratedLevels);
        totalGeneratedLevels += p.second;
        if (!p.first || totalGeneratedLevels >= sectorGenerationLevelLimit)
          return {false, totalGeneratedLevels};
      }
    }

    m_generatorFacade->generateSectorLevel(this, sector, currentGeneration);
    metadata.generationLevel = currentGeneration;

    ++totalGeneratedLevels;
    if (totalGeneratedLevels >= sectorGenerationLevelLimit)
      return {metadata.generationLevel == targetGenerationLevel, totalGeneratedLevels};
  }

  return {true, totalGeneratedLevels};
}

void WorldStorage::loadSectorToLevel(Sector const& sector, SectorLoadLevel targetLoadLevel) {
  if (!m_tileArray->sectorValid(sector))
    return;

  auto entityFactory = Root::singleton().entityFactory();

  auto& metadata = m_sectorMetadata[sector];
  if (metadata.loadLevel >= targetLoadLevel)
    return;

  metadata.timeToLive = randomizedSectorTTL();

  for (uint8_t i = (uint8_t)metadata.loadLevel + 1; i <= (uint8_t)targetLoadLevel; ++i) {
    SectorLoadLevel currentLoad = (SectorLoadLevel)i;
    SectorLoadLevel stepDownLoad = (SectorLoadLevel)(i - 1);

    if (stepDownLoad != SectorLoadLevel::None) {
      for (auto adjacentSector : adjacentSectors(sector))
        loadSectorToLevel(adjacentSector, stepDownLoad);
    }

    if (currentLoad == SectorLoadLevel::Tiles) {
      if (auto res = m_db.find(tileSectorKey(sector))) {
        TileSectorStore sectorStore = readTileSector(*res);

        m_tileArray->loadSector(sector, std::move(sectorStore.tiles));

        metadata.generationLevel = sectorStore.generationLevel;
      } else {
        if (!m_tileArray->sectorLoaded(sector))
          m_tileArray->loadDefaultSector(sector);
      }

      metadata.loadLevel = currentLoad;
      m_generatorFacade->sectorLoadLevelChanged(this, sector, currentLoad);

    } else if (currentLoad == SectorLoadLevel::Entities) {
      List<EntityPtr> addedEntities;
      if (auto res = m_db.find(entitySectorKey(sector))) {
        EntitySectorStore sectorStore = readEntitySector(*res);
        for (auto const& entityStore : sectorStore) {
          try {
            addedEntities.append(entityFactory->loadVersionedEntity(entityStore));
          } catch (std::exception const& e) {
            Logger::warn("Failed to deserialize entity: {}", outputException(e, true));
          }
        }
      }

      UniqueIndexStore readUniques;
      for (auto const& entity : addedEntities) {
        m_generatorFacade->initEntity(this, m_entityMap->reserveEntityId(), entity);
        m_entityMap->addEntity(entity);
        if (auto uniqueId = entity->uniqueId())
          readUniques.add(*uniqueId, {sector, entity->position()});
      }

      // Update the stored unique ids on load, in case a desync has happened
      // and there are stale entries in the index.
      updateSectorUniques(sector, readUniques);

      metadata.loadLevel = currentLoad;
      m_generatorFacade->sectorLoadLevelChanged(this, sector, currentLoad);
    }
  }
}

bool WorldStorage::unloadSectorToLevel(Sector const& sector, SectorLoadLevel targetLoadLevel, bool force) {
  if (!m_tileArray->sectorValid(sector) || targetLoadLevel == SectorLoadLevel::Loaded)
    return true;

  auto& metadata = m_sectorMetadata[sector];
  bool entitiesOverlap = false;
  if (m_entityMap) {
    auto entityFactory = Root::singleton().entityFactory();
    List<EntityPtr> entitiesToStore;
    List<EntityPtr> entitiesToRemove;

    for (auto& entity : m_entityMap->entityQuery(RectF(m_tileArray->sectorRegion(sector)))) {
      // Only store / remove entities who belong to this sector.  If an entity
      // overlaps with this sector but does not belong to it, we may not want to
      // completely unload it.
      auto position = entity->position();
      if (!belongsInSector(sector, position)) {
        if (auto entitySector = sectorForPosition(Vec2I(position))) {
          if (auto p = m_sectorMetadata.ptr(*entitySector))
            entitiesOverlap |= p->timeToLive > 0.0f;
        }
        continue;
      }

      bool keepAlive = m_generatorFacade->entityKeepAlive(this, entity);
      if (keepAlive && !force)
        return false;

      if (m_generatorFacade->entityPersistent(this, entity))
        entitiesToStore.append(std::move(entity));
      else
        entitiesToRemove.append(std::move(entity));
    }

    for (auto const& entity : entitiesToRemove) {
      m_entityMap->removeEntity(entity->entityId());
      m_generatorFacade->destructEntity(this, entity);
    }

    if (metadata.loadLevel == SectorLoadLevel::Entities || !entitiesToStore.empty()) {
      EntitySectorStore sectorStore;

      // If our current load level indicates that we might have entities that are
      // not loaded, we need to load and merge with them, otherwise we should be
      // overwriting them.
      if (metadata.loadLevel < SectorLoadLevel::Entities) {
        if (auto res = m_db.find(entitySectorKey(sector)))
          sectorStore = readEntitySector(*res);
      }

      UniqueIndexStore storedUniques;
      for (auto const& entity : entitiesToStore) {
        m_entityMap->removeEntity(entity->entityId());
        m_generatorFacade->destructEntity(this, entity);
        auto position = entity->position();
        if (auto uniqueId = entity->uniqueId())
          storedUniques.add(*uniqueId, {sector, position});
        sectorStore.append(entityFactory->storeVersionedEntity(entity));
      }
      m_db.insert(entitySectorKey(sector), writeEntitySector(sectorStore));
      if (metadata.loadLevel < SectorLoadLevel::Entities)
        mergeSectorUniques(sector, storedUniques);
      else
        updateSectorUniques(sector, storedUniques);

      if (metadata.loadLevel == SectorLoadLevel::Entities) {
        metadata.loadLevel = SectorLoadLevel::Tiles;
        m_generatorFacade->sectorLoadLevelChanged(this, sector, SectorLoadLevel::Tiles);
      }
    }
  }

  if (targetLoadLevel == SectorLoadLevel::None) {
    if (metadata.loadLevel > SectorLoadLevel::None && !entitiesOverlap) {
      TileSectorStore sectorStore;
      sectorStore.tiles = m_tileArray->unloadSector(sector);
      sectorStore.generationLevel = metadata.generationLevel;
      m_db.insert(tileSectorKey(sector), writeTileSector(sectorStore));
      m_sectorMetadata.remove(sector);
      m_generatorFacade->sectorLoadLevelChanged(this, sector, SectorLoadLevel::None);
      return true;
    }
    return false;
  }
  return true;
}

void WorldStorage::syncSector(Sector const& sector) {
  if (!m_tileArray->sectorValid(sector))
    return;

  auto entityFactory = Root::singleton().entityFactory();
  auto& metadata = m_sectorMetadata[sector];

  // Only sync the levels that we know are loaded.  It is possible that this
  // sector is at load level < Entities but has zombie entities in it,  but
  // storing those without unloading them will lead to duplication.  Zombie
  // entities will be unloaded in update eventually anyway.

  if (metadata.loadLevel >= SectorLoadLevel::Entities) {
    EntitySectorStore sectorStore;
    UniqueIndexStore storedUniques;
    for (auto const& entity : m_entityMap->entityQuery(RectF(m_tileArray->sectorRegion(sector)))) {
      if (!belongsInSector(sector, entity->position()))
        continue;

      if (m_generatorFacade->entityPersistent(this, entity)) {
        if (auto uniqueId = entity->uniqueId())
          storedUniques.add(*uniqueId, {sector, entity->position()});
        sectorStore.append(entityFactory->storeVersionedEntity(entity));
      }
    }
    m_db.insert(entitySectorKey(sector), writeEntitySector(sectorStore));
    updateSectorUniques(sector, storedUniques);
  }

  if (metadata.loadLevel >= SectorLoadLevel::Tiles) {
    TileSectorStore sectorStore;
    sectorStore.tiles = m_tileArray->copySector(sector);
    sectorStore.generationLevel = metadata.generationLevel;
    m_db.insert(tileSectorKey(sector), writeTileSector(sectorStore));
  }
}

List<WorldStorage::Sector> WorldStorage::adjacentSectors(Sector const& sector) const {
  auto tiles = m_tileArray->sectorRegion(sector);
  return m_tileArray->validSectorsFor(tiles.padded(WorldSectorSize));
}

void WorldStorage::updateSectorUniques(Sector const& sector, UniqueIndexStore const& sectorUniques) {
  // If there was an old unique sector store here, then we need to remove all
  // the unique index entries for uniques that used to be in this sector but
  // now aren't, in case they are now gone.
  if (auto oldSectorUniques = m_db.find(sectorUniqueKey(sector)).apply(readSectorUniqueStore)) {
    for (auto const& uniqueId : *oldSectorUniques) {
      if (!sectorUniques.contains(uniqueId))
        removeUniqueIndexEntry(uniqueId, sector);
    }
  }

  for (auto const& p : sectorUniques)
    setUniqueIndexEntry(p.first, p.second);

  if (sectorUniques.empty())
    m_db.remove(sectorUniqueKey(sector));
  else
    m_db.insert(sectorUniqueKey(sector), writeSectorUniqueStore(HashSet<String>::from(sectorUniques.keys())));
}

void WorldStorage::mergeSectorUniques(Sector const& sector, UniqueIndexStore const& sectorUniques) {
  auto sectorUniqueStore = m_db.find(sectorUniqueKey(sector)).apply(readSectorUniqueStore).value();
  for (auto const& p : sectorUniques) {
    setUniqueIndexEntry(p.first, p.second);
    sectorUniqueStore.add(p.first);
  }

  if (sectorUniqueStore.empty())
    m_db.remove(sectorUniqueKey(sector));
  else
    m_db.insert(sectorUniqueKey(sector), writeSectorUniqueStore(sectorUniqueStore));
}

auto WorldStorage::getUniqueIndexEntry(String const& uniqueId) -> Maybe<SectorAndPosition> {
  if (auto uniqueIndex = m_db.find(uniqueIndexKey(uniqueId)).apply(readUniqueIndexStore))
    return uniqueIndex->maybe(uniqueId);
  return {};
}

void WorldStorage::setUniqueIndexEntry(String const& uniqueId, SectorAndPosition const& sectorAndPosition) {
  UniqueIndexStore uniqueIndex = m_db.find(uniqueIndexKey(uniqueId)).apply(readUniqueIndexStore).value();
  auto p = uniqueIndex.insert(uniqueId, sectorAndPosition);
  if (!p.second) {
    // Don't need to update the index if the entry was already there and the
    // sector and position haven't changed
    if (p.first->second == sectorAndPosition)
      return;
    p.first->second = sectorAndPosition;
  }
  m_db.insert(uniqueIndexKey(uniqueId), writeUniqueIndexStore(uniqueIndex));
}

void WorldStorage::removeUniqueIndexEntry(String const& uniqueId, Sector const& sector) {
  if (auto uniqueIndex = m_db.find(uniqueIndexKey(uniqueId)).apply(readUniqueIndexStore)) {
    if (auto sectorAndPosition = uniqueIndex->maybe(uniqueId)) {
      if (sectorAndPosition->first == sector) {
        uniqueIndex->remove(uniqueId);
        if (uniqueIndex->empty())
          m_db.remove(uniqueIndexKey(uniqueId));
        else
          m_db.insert(uniqueIndexKey(uniqueId), writeUniqueIndexStore(*uniqueIndex));
      }
    }
  }
}

}
