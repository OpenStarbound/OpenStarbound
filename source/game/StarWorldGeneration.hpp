#ifndef STAR_WORLD_GENERATION_HPP
#define STAR_WORLD_GENERATION_HPP

#include "StarDungeonGenerator.hpp"
#include "StarFallingBlocksAgent.hpp"
#include "StarSpawner.hpp"
#include "StarWorldStorage.hpp"
#include "StarMicroDungeon.hpp"
#include "StarCellularLiquid.hpp"
#include "StarBiomePlacement.hpp"

namespace Star {

STAR_CLASS(WorldServer);
STAR_CLASS(FallingBlocksWorld);
STAR_CLASS(DungeonGeneratorWorld);
STAR_CLASS(SpawnerWorld);
STAR_CLASS(WorldGenerator);
STAR_CLASS(Plant);
STAR_CLASS(LiquidsDatabase);

class LiquidWorld : public CellularLiquidWorld<LiquidId> {
public:
  LiquidWorld(WorldServer* world);

  Vec2I uniqueLocation(Vec2I const& location) const override;
  CellularLiquidCell<LiquidId> cell(Vec2I const& location) const override;
  float drainLevel(Vec2I const& location) const override;
  void setFlow(Vec2I const& location, CellularLiquidFlowCell<LiquidId> const& flow) override;
  void liquidInteraction(Vec2I const& a, LiquidId aLiquid, Vec2I const& b, LiquidId bLiquid) override;
  void liquidCollision(Vec2I const& pos, LiquidId liquid, Vec2I const& blockPos) override;

private:
  WorldServer* m_worldServer;
  LiquidsDatabaseConstPtr m_liquidsDatabase;
  MaterialDatabaseConstPtr m_materialDatabase;
};

class FallingBlocksWorld : public FallingBlocksFacade {
public:
  FallingBlocksWorld(WorldServer* world);

  FallingBlockType blockType(Vec2I const& pos) override;
  void moveBlock(Vec2I const& from, Vec2I const& to) override;

private:
  WorldServer* m_worldServer;
  MaterialDatabaseConstPtr m_materialDatabase;
};

class DungeonGeneratorWorld : public DungeonGeneratorWorldFacade {
public:
  DungeonGeneratorWorld(WorldServer* worldServer, bool markForActivation);

  void markRegion(RectI const& region) override;
  void markTerrain(PolyF const& region) override;
  void markSpace(PolyF const& region) override;

  void setForegroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) override;
  void setBackgroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) override;
  void setForegroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) override;
  void setBackgroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) override;
  void placeObject(
      Vec2I const& pos, String const& objectName, Star::Direction direction, Json const& parameters) override;
  void placeVehicle(Vec2F const& pos, String const& vehicleName, Json const& parameters) override;
  void placeSurfaceBiomeItems(Vec2I const& pos) override;
  void placeBiomeTree(Vec2I const& pos) override;
  void addDrop(Vec2F const& position, ItemDescriptor const& item) override;
  void spawnNpc(Vec2F const& position, Json const& parameters) override;
  void spawnStagehand(Vec2F const& position, Json const& definition) override;
  void setLiquid(Vec2I const& pos, LiquidStore const& liquid) override;
  void setPlayerStart(Vec2F const& startPosition) override;
  void connectWireGroup(List<Vec2I> const& wireGroup) override;
  void setTileProtection(DungeonId dungeonId, bool isProtected) override;
  bool checkSolid(Vec2I const& position, TileLayer layer) override;
  bool checkOpen(Vec2I const& position, TileLayer open) override;
  bool checkOceanLiquid(Vec2I const& position) override;
  DungeonId getDungeonIdAt(Vec2I const& position) override;
  void setDungeonIdAt(Vec2I const& position, DungeonId dungeonId) override;
  void clearTileEntities(RectI const& bounds, Set<Vec2I> const& positions, bool clearAnchoredObjects) override;

  WorldGeometry getWorldGeometry() const override;

private:
  void placePlant(PlantPtr const& plant, Vec2I const& position);
  void placeBiomeItems(Vec2I const& pos, List<BiomeItemPlacement>& potentialItems);

  WorldServer* m_worldServer;
  bool m_markForActivation;
};

class SpawnerWorld : public SpawnerFacade {
public:
  SpawnerWorld(WorldServer* worldServer);

  WorldGeometry geometry() const override;
  List<RectF> clientWindows() const override;
  bool signalRegion(RectF const& region) const override;
  CollisionKind collision(Vec2I const& position) const override;
  bool isFreeSpace(RectF const& area) const override;
  bool isBackgroundEmpty(Vec2I const& position) const override;
  LiquidLevel liquidLevel(Vec2I const& position) const override;
  bool spawningProhibited(RectF const& area) const override;
  uint64_t spawnSeed() const override;
  SpawnProfile spawnProfile(Vec2F const& position) const override;
  float dayLevel() const override;
  float threatLevel() const override;
  EntityId spawnEntity(EntityPtr entity) const override;
  void despawnEntity(EntityId entityId) override;
  EntityPtr getEntity(EntityId entityId) const override;

private:
  WorldServer* m_worldServer;
};

class WorldGenerator : public WorldGeneratorFacade {
public:
  WorldGenerator(WorldServer* server);

  void generateSectorLevel(WorldStorage* worldStorage, Sector const& sector, SectorGenerationLevel generationLevel) override;
  void sectorLoadLevelChanged(WorldStorage* worldStorage, Sector const& sector, SectorLoadLevel loadLevel) override;
  void terraformSector(WorldStorage* worldStorage, Sector const& sector) override;
  void initEntity(WorldStorage* worldStorage, EntityId entityId, EntityPtr const& entity) override;
  void destructEntity(WorldStorage* worldStorage, EntityPtr const& entity) override;
  bool entityKeepAlive(WorldStorage* worldStorage, EntityPtr const& entity) const override;
  bool entityPersistent(WorldStorage* worldStorage, EntityPtr const& entity) const override;
  RpcPromise<Vec2I> enqueuePlacement(List<BiomeItemDistribution> distributions, Maybe<DungeonId> id) override;

  void replaceBiomeBlocks(ServerTile* tile);

private:
  struct QueuedPlacement {
    List<BiomeItemDistribution> distributions;
    Maybe<DungeonId> dungeonId;
    RpcPromiseKeeper<Vec2I> promise;
    bool fulfilled;
  };

  void prepareTiles(WorldStorage* worldStorage, Sector const& sector);
  void generateMicroDungeons(WorldStorage* worldStorage, Sector const& sector);
  void generateCaveLiquid(WorldStorage* worldStorage, Sector const& sector);
  void prepareSector(WorldStorage* worldStorage, Sector const& sector);
  void prepareSectorBiomeBlocks(WorldStorage* worldStorage, Sector const& sector);

  void placeBiomeGrass(WorldStorage* worldStorage, ServerTile* tile, Vec2I const& position);

  void reapplyBiome(WorldStorage* worldStorage, Sector const& sector);

  Set<Vec2I> caveLiquidSeeds(WorldStorage* worldStorage, Sector const& sector);
  Map<Vec2I, float> determineLiquidLevel(Set<Vec2I> const& spots, Set<Vec2I> const& filled);
  void levelCluster(Set<Vec2I>& cluster, Set<Vec2I> const& filled, Map<Vec2I, float>& results);

  // Special plant placement routine that does slight terrain adjustments to
  // fit plants.
  bool placePlant(WorldStorage* worldStorage, PlantPtr const& plant, Vec2I const& position);

  WorldServer* m_worldServer;
  MicroDungeonFactoryPtr m_microDungeonFactory;
  List<QueuedPlacement> m_queuedPlacements;
};

}

#endif
