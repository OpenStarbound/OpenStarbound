#include "StarWorldGeneration.hpp"
#include "StarWorldServer.hpp"
#include "StarMaterialItem.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarNpcDatabase.hpp"
#include "StarMonsterDatabase.hpp"
#include "StarNpc.hpp"
#include "StarBiome.hpp"
#include "StarSky.hpp"
#include "StarWorldTemplate.hpp"
#include "StarBiomePlacement.hpp"
#include "StarWireEntity.hpp"
#include "StarItemDrop.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarProjectile.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObject.hpp"
#include "StarContainerObject.hpp"
#include "StarMonster.hpp"
#include "StarEntityMap.hpp"
#include "StarPlant.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarStagehand.hpp"
#include "StarVehicleDatabase.hpp"

namespace Star {

static int const PlantAdjustmentLimit = 2;

LiquidWorld::LiquidWorld(WorldServer* world) {
  m_worldServer = world;
  auto& root = Root::singleton();
  m_liquidsDatabase = root.liquidsDatabase();
  m_materialDatabase = root.materialDatabase();
}

Vec2I LiquidWorld::uniqueLocation(Vec2I const& location) const {
  return m_worldServer->geometry().xwrap(location);
}

float LiquidWorld::drainLevel(Vec2I const& location) const {
  if (location[1] > m_worldServer->worldTemplate()->undergroundLevel()) {
    auto const& tile = m_worldServer->getServerTile(location);
    if (!m_materialDatabase->blocksLiquidFlow(tile.background)) {
      auto const& belowTile = m_worldServer->getServerTile(location + Vec2I(0, -1));
      if (m_materialDatabase->blocksLiquidFlow(belowTile.background) || m_materialDatabase->blocksLiquidFlow(belowTile.foreground) || belowTile.liquid.source)
        return m_liquidsDatabase->backgroundDrain();
    }
  }
  return 0.0f;
}

CellularLiquidCell<LiquidId> LiquidWorld::cell(Vec2I const& location) const {
  auto const& tile = m_worldServer->getServerTile(location);
  if (m_materialDatabase->blocksLiquidFlow(tile.foreground)) {
    return CellularLiquidCollisionCell();
  } else {
    if (tile.liquid.source)
      return CellularLiquidSourceCell<LiquidId>{tile.liquid.liquid, tile.liquid.pressure};
    else if (tile.liquid.liquid != EmptyLiquidId)
      return CellularLiquidFlowCell<LiquidId>{tile.liquid.liquid, tile.liquid.level, tile.liquid.pressure};
    else
      return CellularLiquidFlowCell<LiquidId>{{}, 0.0f, 0.0f};
  }
}

void LiquidWorld::setFlow(Vec2I const& location, CellularLiquidFlowCell<LiquidId> const& flow) {
  if (flow.liquid) {
    m_worldServer->setLiquid(location, *flow.liquid, flow.level, flow.pressure);

    auto const& tile = m_worldServer->getServerTile(location);
    if (auto materialInteraction = m_materialDatabase->liquidMaterialInteraction(*flow.liquid, tile.background)) {
      if (!materialInteraction->topOnly && tile.liquid.level >= materialInteraction->consumeLiquid) {
        if (auto modifyTile = m_worldServer->modifyServerTile(location)) {
          modifyTile->liquid.take(materialInteraction->consumeLiquid);
          modifyTile->background = materialInteraction->transformTo;
          m_worldServer->activateLiquidLocation(location);
        }
      }
    }
    if (auto modInteraction = m_materialDatabase->liquidModInteraction(*flow.liquid, tile.backgroundMod)) {
      if (!modInteraction->topOnly && tile.liquid.level >= modInteraction->consumeLiquid) {
        if (auto modifyTile = m_worldServer->modifyServerTile(location)) {
          modifyTile->liquid.take(modInteraction->consumeLiquid);
          modifyTile->backgroundMod = modInteraction->transformTo;
          m_worldServer->activateLiquidLocation(location);
        }
      }
    }
  } else {
    m_worldServer->setLiquid(location, EmptyLiquidId, 0.0f, 0.0f);
  }
}

void LiquidWorld::liquidInteraction(Vec2I const& a, LiquidId aLiquid, Vec2I const& b, LiquidId bLiquid) {
  auto handleInteraction = [this](Vec2I const& target, Maybe<LiquidInteractionResult> interaction) {
    if (interaction) {
      if (interaction->isLeft()) {
        m_worldServer->modifyTile(target, PlaceMaterial{TileLayer::Foreground, interaction->left(), 0}, false);
      } else {
        auto liquidLevel = m_worldServer->liquidLevel(target);
        m_worldServer->setLiquid(target, interaction->right(), liquidLevel.level, liquidLevel.level);
      }
    }
  };

  handleInteraction(a, m_liquidsDatabase->interact(aLiquid, bLiquid));
  handleInteraction(b, m_liquidsDatabase->interact(bLiquid, aLiquid));
}

void LiquidWorld::liquidCollision(Vec2I const& liquidPos, LiquidId liquidId, Vec2I const& blockPos) {
  auto const& blockTile = m_worldServer->getServerTile(blockPos);

  if (auto materialInteraction = m_materialDatabase->liquidMaterialInteraction(liquidId, blockTile.foreground)) {
    if ((!materialInteraction->topOnly || liquidPos[1] > blockPos[1]) && m_worldServer->liquidLevel(liquidPos).level >= materialInteraction->consumeLiquid) {
      auto modifyLiquidTile = m_worldServer->modifyServerTile(liquidPos);
      auto modifyBlockTile = m_worldServer->modifyServerTile(blockPos);
      if (modifyLiquidTile && modifyBlockTile) {
        modifyLiquidTile->liquid.take(materialInteraction->consumeLiquid);
        modifyBlockTile->foreground = materialInteraction->transformTo;
        if (!m_materialDatabase->isMultiColor(materialInteraction->transformTo))
          modifyBlockTile->foregroundColorVariant = DefaultMaterialColorVariant;
        m_worldServer->activateLiquidLocation(liquidPos);
      }
    }
  }
  if (auto modInteraction = m_materialDatabase->liquidModInteraction(liquidId, blockTile.foregroundMod)) {
    if ((!modInteraction->topOnly || liquidPos[1] > blockPos[1]) && m_worldServer->liquidLevel(liquidPos).level >= modInteraction->consumeLiquid) {
      auto modifyLiquidTile = m_worldServer->modifyServerTile(liquidPos);
      auto modifyBlockTile = m_worldServer->modifyServerTile(blockPos);
      if (modifyLiquidTile && modifyBlockTile) {
        modifyLiquidTile->liquid.take(modInteraction->consumeLiquid);
        modifyBlockTile->foregroundMod = modInteraction->transformTo;
        m_worldServer->activateLiquidLocation(liquidPos);
      }
    }
  }
}

FallingBlocksWorld::FallingBlocksWorld(WorldServer* w)
  : m_worldServer(w), m_materialDatabase(Root::singleton().materialDatabase()) {}

FallingBlockType FallingBlocksWorld::blockType(Vec2I const& pos) {
  auto const& tile =  m_worldServer->getServerTile(pos, true);
  if (tile.rootSource) {
    return FallingBlockType::Immovable;
  } if (tile.foreground == EmptyMaterialId) {
    if (m_worldServer->tileIsOccupied(pos, TileLayer::Foreground))
      return FallingBlockType::Immovable;
    else
      return FallingBlockType::Open;
  } else if (m_materialDatabase->isCascadingFallingMaterial(tile.foreground)) {
    return FallingBlockType::Cascading;
  } else if (m_materialDatabase->isFallingMaterial(tile.foreground)) {
    return FallingBlockType::Falling;
  } else {
    return FallingBlockType::Immovable;
  }
}

void FallingBlocksWorld::moveBlock(Vec2I const& from, Vec2I const& to) {
  auto fromTile = m_worldServer->modifyServerTile(from, true);
  auto toTile = m_worldServer->modifyServerTile(to, true);
  if (!fromTile || !toTile)
    return;

  if (m_worldServer->isTileProtected(to)) {
    for (auto drop : m_worldServer->destroyBlock(TileLayer::Foreground, from, true, true))
      m_worldServer->addEntity(ItemDrop::createRandomizedDrop(drop, Vec2F(to)));
  } else {
    toTile->foreground = fromTile->foreground;
    toTile->foregroundMod = NoModId;
    toTile->foregroundHueShift = fromTile->foregroundHueShift;
    toTile->foregroundColorVariant = fromTile->foregroundColorVariant;
    toTile->updateCollision(m_materialDatabase->materialCollisionKind(toTile->foreground));

    fromTile->foreground = EmptyMaterialId;
    fromTile->foregroundMod = NoModId;
    fromTile->updateCollision(CollisionKind::None);

    m_worldServer->requestGlobalBreakCheck();
  }
}

DungeonGeneratorWorld::DungeonGeneratorWorld(WorldServer* worldServer, bool markForActivation)
  : m_worldServer(worldServer), m_markForActivation(markForActivation) {}

WorldGeometry DungeonGeneratorWorld::getWorldGeometry() const {
  return m_worldServer->geometry();
}

void DungeonGeneratorWorld::markRegion(RectI const& region) {
  if (!m_markForActivation)
    return;

  Logger::debug("Marking {} as dungeon region", region);

  m_worldServer->signalRegion(region);
  m_worldServer->activateLiquidRegion(region);
}

void DungeonGeneratorWorld::markTerrain(PolyF const& region) {
  if (!m_markForActivation)
    return;

  Logger::debug("Marking poly as dungeon terrain region: {}", region);
  m_worldServer->worldTemplate()->addCustomTerrainRegion(region);
}

void DungeonGeneratorWorld::markSpace(PolyF const& region) {
  if (!m_markForActivation)
    return;

  Logger::debug("Marking poly as dungeon space region: {}", region);
  m_worldServer->worldTemplate()->addCustomSpaceRegion(region);
}

void DungeonGeneratorWorld::setForegroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) {
  if (ServerTile* tile = m_worldServer->modifyServerTile(position)) {
    m_worldServer->modifyLiquid(position, EmptyLiquidId, 0);
    tile->foreground = material;
    tile->foregroundHueShift = hueshift;
    tile->foregroundColorVariant = colorVariant;
    tile->foregroundMod = NoModId;
    tile->foregroundModHueShift = MaterialHue();
    tile->collision = Root::singleton().materialDatabase()->materialCollisionKind(tile->foreground);
    tile->collisionCacheDirty = true;
  }
}

void DungeonGeneratorWorld::setBackgroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) {
  if (ServerTile* tile = m_worldServer->modifyServerTile(position)) {
    m_worldServer->modifyLiquid(position, EmptyLiquidId, 0);
    tile->background = material;
    tile->backgroundHueShift = hueshift;
    tile->backgroundColorVariant = colorVariant;
    tile->backgroundMod = NoModId;
    tile->backgroundModHueShift = MaterialHue();
  }
}

void DungeonGeneratorWorld::placeObject(Vec2I const& pos, String const& objectName, Star::Direction direction, Json const& parameters) {
  m_worldServer->signalRegion(RectI::withSize(pos, {1, 1}));

  auto objectDatabase = Root::singleton().objectDatabase();
  if (auto object = objectDatabase->createForPlacement(m_worldServer, objectName, pos, direction, parameters))
    m_worldServer->addEntity(object);
  else
    Logger::warn("Failed to place dungeon object: {} direction: {} position: {}", objectName, (int)direction, pos);
}

void DungeonGeneratorWorld::placeVehicle(Vec2F const& pos, String const& vehicleName, Json const& parameters) {
  m_worldServer->signalRegion(RectI::withSize(Vec2I(pos), {1, 1}));

  auto vehicleDatabase = Root::singleton().vehicleDatabase();
  auto vehicle = vehicleDatabase->create(vehicleName, parameters.opt().value(JsonObject{}).set("persistent", true));
  vehicle->setPosition(pos);
  m_worldServer->addEntity(vehicle);
}

void DungeonGeneratorWorld::placeSurfaceBiomeItems(Vec2I const& pos) {
  List<BiomeItemPlacement> surfaceItems = m_worldServer->worldTemplate()->potentialBiomeItemsAt(pos[0], pos[1]).surfaceBiomeItems;
  placeBiomeItems(pos, surfaceItems);
}

void DungeonGeneratorWorld::placeBiomeTree(Vec2I const& pos) {
  if (auto biome = m_worldServer->worldTemplate()->blockBiome(pos[0], pos[1])) {
    m_worldServer->signalRegion(RectI::withSize(pos, {1, 1}));
    auto seed = m_worldServer->worldTemplate()->seedFor(pos[0], pos[1]);
    if (auto treeVariant = biome->surfacePlaceables.firstTreeType())
      placePlant(Root::singleton().plantDatabase()->createPlant(*treeVariant, seed), pos);
  }
}

// yay, copy paste coding, kyren WILL kill me
void DungeonGeneratorWorld::placePlant(PlantPtr const& plant, Vec2I const& position) {
  if (!plant)
    return;

  auto spaces = plant->spaces();
  auto roots = plant->roots();
  auto const& primaryRoot = plant->primaryRoot();

  auto background = m_worldServer->getServerTile(position).background;
  bool adjustBackground = background == EmptyMaterialId || background == NullMaterialId;

  auto withinAdjustment = [=](Vec2I const& pos) {
    return PlantAdjustmentLimit - std::abs(pos[0]) > 0 && PlantAdjustmentLimit - std::abs(pos[1]) > 0;
  };

  // Bail out if we don't have at least one free space, and root in the primary
  // root position, or if we're in a dungeon region.
  auto primaryTile = m_worldServer->getServerTile(position);
  auto rootTile = m_worldServer->getServerTile(position + primaryRoot);
  if (isConnectableMaterial(primaryTile.foreground) || !isConnectableMaterial(rootTile.foreground))
    return;

  // First bail out if we can't fit anything we're not adjusting
  for (auto space : spaces) {
    Vec2I pspace = space + position;

    if (withinAdjustment(space) && !m_worldServer->atTile<Plant>(pspace).empty())
      return;

    // Bail out if we hit a different plant's root tile, or if we're not in the
    // adjustment space and we hit a non-empty tile.
    auto tile = m_worldServer->getServerTile(pspace);
    if (tile.rootSource || (!withinAdjustment(space) && !(tile.foreground == EmptyMaterialId || tile.foreground == NullMaterialId)))
      return;
  }

  // Check all the roots outside of the adjustment limit
  for (auto root : roots) {
    root += position;
    if (!withinAdjustment(root) && !isConnectableMaterial(m_worldServer->getServerTile(root).foreground))
      return;
  }

  // Clear all the necessary blocks within the adjustment limit
  for (auto space : spaces) {
    if (!withinAdjustment(space))
      continue;

    space += position;
    if (auto tile = m_worldServer->modifyServerTile(space)) {
      if (isConnectableMaterial(tile->foreground))
        *tile = primaryTile;
      if (adjustBackground)
        tile->background = EmptyMaterialId;
      tile->collision = CollisionKind::None;
      tile->collision = Root::singleton().materialDatabase()->materialCollisionKind(tile->foreground);
      tile->collisionCacheDirty = true;
    } else {
      return;
    }
  }

  // Make all the root blocks a real material based on the primary root.
  for (auto root : roots) {
    root += position;
    if (auto tile = m_worldServer->modifyServerTile(root)) {
      if (!isRealMaterial(tile->foreground)) {
        *tile = rootTile;
        tile->collision = Root::singleton().materialDatabase()->materialCollisionKind(tile->foreground);
        tile->collisionCacheDirty = true;
      }
    } else {
      return;
    }
  }

  plant->setTilePosition(position);
  m_worldServer->addEntity(plant);
  return;
}

void DungeonGeneratorWorld::placeBiomeItems(Vec2I const& pos, List<BiomeItemPlacement>& potentialItems) {
  m_worldServer->signalRegion(RectI::withSize(pos, {1, 1}));
  sort(potentialItems);
  for (auto const& placement : potentialItems) {
    auto seed = m_worldServer->worldTemplate()->seedFor(placement.position[0], placement.position[1]);
    if (placement.item.is<GrassVariant>()) {
      auto& grass = placement.item.get<GrassVariant>();
      placePlant(Root::singleton().plantDatabase()->createPlant(grass, seed), placement.position);
    } else if (placement.item.is<BushVariant>()) {
      auto& bush = placement.item.get<BushVariant>();
      placePlant(Root::singleton().plantDatabase()->createPlant(bush, seed), placement.position);
    } else if (placement.item.is<TreePair>()) {
      auto& treePair = placement.item.get<TreePair>();
      TreeVariant treeVariant;
      if (seed % 2 == 0)
        treeVariant = treePair.first;
      else
        treeVariant = treePair.second;

      placePlant(Root::singleton().plantDatabase()->createPlant(treeVariant, seed), placement.position);
    } else if (placement.item.is<ObjectPool>()) {
      auto& objectPool = placement.item.get<ObjectPool>();
      auto direction = seed % 2 ? Direction::Left : Direction::Right;
      auto objectPair = objectPool.select(seed);
      if (auto object = Root::singleton().objectDatabase()->createForPlacement(
              m_worldServer, objectPair.first, placement.position, direction, objectPair.second))
        m_worldServer->addEntity(object);
    } else if (placement.item.is<TreasureBoxSet>()) {
      auto& treasureBoxSet = placement.item.get<TreasureBoxSet>();
      auto direction = seed % 2 ? Direction::Left : Direction::Right;
      if (auto treasureContainer = Root::singleton().treasureDatabase()->createTreasureChest(m_worldServer, treasureBoxSet, placement.position, direction, seed))
        m_worldServer->addEntity(treasureContainer);
    }
  }
}

void DungeonGeneratorWorld::addDrop(Vec2F const& position, ItemDescriptor const& item) {
  m_worldServer->addEntity(ItemDrop::createRandomizedDrop(item, position));
}

void DungeonGeneratorWorld::spawnNpc(Vec2F const& position, Json const& parameters) {
  auto kind = parameters.getString("kind");
  if (kind.equals("npc", String::CaseInsensitive)) {
    auto npcDatabase = Root::singleton().npcDatabase();
    uint64_t seed = parameters.getUInt("seed", Random::randu64());
    String species = parameters.getString("species");
    String typeName = parameters.getString("typeName", "default");
    JsonObject uniqueParameters = parameters.getObject("parameters", {});
    if (!uniqueParameters.contains("persistent"))
      uniqueParameters["persistent"] = true;
    auto npc = npcDatabase->createNpc(npcDatabase->generateNpcVariant(species, typeName, m_worldServer->threatLevel(), seed, uniqueParameters));
    npc->setPosition(position - npc->feetOffset());
    m_worldServer->addEntity(npc);
  } else if (kind.equals("monster", String::CaseInsensitive)) {
    auto monsterDatabase = Root::singleton().monsterDatabase();
    uint64_t seed = parameters.getUInt("seed", Random::randu64());
    String typeName = parameters.getString("typeName");
    JsonObject uniqueParameters = parameters.getObject("parameters", {});
    if (!uniqueParameters.contains("persistent"))
      uniqueParameters["persistent"] = true;
    auto monster = monsterDatabase->createMonster(monsterDatabase->monsterVariant(typeName, seed, uniqueParameters));
    monster->setPosition(position);
    m_worldServer->addEntity(monster);
  } else
    throw StarException(strf("Unknown spawnable kind '{}'", kind));
}

void DungeonGeneratorWorld::spawnStagehand(Vec2F const& position, Json const& definition) {
  auto stagehand = Root::singleton().stagehandDatabase()->createStagehand(definition.getString("type"), definition.get("parameters", Json()));
  stagehand->setPosition(position);
  m_worldServer->addEntity(stagehand);
}

void DungeonGeneratorWorld::setLiquid(Vec2I const& pos, LiquidStore const& liquid) {
  ServerTile* tile = m_worldServer->modifyServerTile(pos);
  starAssert(tile);
  if (tile)
    tile->liquid = liquid;
}

void DungeonGeneratorWorld::setPlayerStart(Vec2F const& startPosition) {
  m_worldServer->setPlayerStart(startPosition);
}

void DungeonGeneratorWorld::connectWireGroup(List<Vec2I> const& wireGroup) {
  List<WireConnection> outbounds;
  List<WireConnection> inbounds;

  for (auto entry : wireGroup) {
    bool found = false;
    Vec2F posf = centerOfTile(entry);
    RectF bounds = {posf - Vec2F(16, 16), posf + Vec2F(16, 16)};
    for (auto entity : m_worldServer->query<WireEntity>(bounds)) {
      for (size_t i = 0; i < entity->nodeCount(WireDirection::Input); ++i) {
        if (entity->tilePosition() + entity->nodePosition({WireDirection::Input, i}) == entry) {
          inbounds.append(WireConnection{entity->tilePosition(), i});
          found = true;
        }
      }
      for (size_t i = 0; i < entity->nodeCount(WireDirection::Output); ++i) {
        if (entity->tilePosition() + entity->nodePosition({WireDirection::Output, i}) == entry) {
          outbounds.append(WireConnection{entity->tilePosition(), i});
          found = true;
        }
      }
    }
    if (!found)
      Logger::warn("Dungeon wire endpoint not found. {}", entry);
  }

  if (!outbounds.size() || !inbounds.size()) {
    Logger::warn("Dungeon wires did not make a circuit.");
    return;
  }

  for (auto outbound : outbounds) {
    auto out = m_worldServer->atTile<WireEntity>(outbound.entityLocation).first();
    for (auto inbound : inbounds) {
      auto in = m_worldServer->atTile<WireEntity>(inbound.entityLocation).first();
      in->addNodeConnection({WireDirection::Input, inbound.nodeIndex}, outbound);
      out->addNodeConnection({WireDirection::Output, outbound.nodeIndex}, inbound);
    }
  }
}

void DungeonGeneratorWorld::setForegroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) {
  ServerTile* tile = m_worldServer->modifyServerTile(position);
  if (tile) {
    tile->foregroundMod = mod;
    tile->foregroundModHueShift = hueshift;
  }
}

void DungeonGeneratorWorld::setBackgroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) {
  ServerTile* tile = m_worldServer->modifyServerTile(position);
  if (tile) {
    tile->backgroundMod = mod;
    tile->foregroundModHueShift = hueshift;
  }
}

void DungeonGeneratorWorld::setTileProtection(DungeonId dungeonId, bool isProtected) {
  m_worldServer->setTileProtection(dungeonId, isProtected);
}

bool DungeonGeneratorWorld::checkSolid(Vec2I const& position, TileLayer layer) {
  auto const& tile = m_worldServer->getServerTile(position);
  return tile.material(layer) != EmptyMaterialId && tile.material(layer) != NullMaterialId;
}

bool DungeonGeneratorWorld::checkOpen(Vec2I const& position, TileLayer layer) {
  auto const& tile = m_worldServer->getServerTile(position);
  return tile.material(layer) == EmptyMaterialId || tile.material(layer) == NullMaterialId;
}

bool DungeonGeneratorWorld::checkOceanLiquid(Vec2I const& position) {
  auto const& block = m_worldServer->worldTemplate()->blockInfo(position[0], position[1]);
  return block.oceanLiquid != EmptyLiquidId && position[1] < block.oceanLiquidLevel;
}

DungeonId DungeonGeneratorWorld::getDungeonIdAt(Vec2I const& position) {
  return m_worldServer->getServerTile(position).dungeonId;
}

void DungeonGeneratorWorld::setDungeonIdAt(Vec2I const& position, DungeonId dungeonId) {
  if (auto tile = m_worldServer->modifyServerTile(position))
    tile->dungeonId = dungeonId;
}

void DungeonGeneratorWorld::clearTileEntities(RectI const& bounds, Set<Vec2I> const& positions, bool clearAnchoredObjects) {
  auto entities = m_worldServer->entityQuery(RectF(bounds).padded(1), entityTypeFilter<TileEntity>());
  auto geometry = m_worldServer->geometry();
  entities.filter([positions, geometry, clearAnchoredObjects](EntityPtr entity) {
      auto tileEntity = as<TileEntity>(entity);
      for (auto pos : tileEntity->spaces()) {
        if (positions.contains(geometry.xwrap(pos + tileEntity->tilePosition())))
          return true;
      }
      if (clearAnchoredObjects) {
        for (auto pos : tileEntity->roots()) {
          if (positions.contains(geometry.xwrap(pos + tileEntity->tilePosition())))
            return true;
        }
        if (auto object = as<Object>(entity)) {
          for (auto pos : object->anchorPositions()) {
            if (positions.contains(geometry.xwrap(pos)))
              return true;
          }
        }
      }

      return false;
    });

  for (auto entity : entities)
    m_worldServer->removeEntity(entity->entityId(), false);
}

SpawnerWorld::SpawnerWorld(WorldServer* server)
  : m_worldServer(server) {}

WorldGeometry SpawnerWorld::geometry() const {
  return m_worldServer->geometry();
}

List<RectF> SpawnerWorld::clientWindows() const {
  List<RectF> windows;
  for (auto clientId : m_worldServer->clientIds())
    windows.append(m_worldServer->clientWindow(clientId));
  return windows;
}

bool SpawnerWorld::signalRegion(RectF const& region) const {
  return m_worldServer->signalRegion(RectI::integral(region));
}

CollisionKind SpawnerWorld::collision(Vec2I const& position) const {
  return m_worldServer->getServerTile(position + Vec2I(0, 1)).collision;
}

bool SpawnerWorld::isFreeSpace(RectF const& area) const {
  return !m_worldServer->polyCollision(PolyF(area));
}

bool SpawnerWorld::isBackgroundEmpty(Vec2I const& pos) const {
  return m_worldServer->getServerTile(pos).background == EmptyMaterialId;
}

LiquidLevel SpawnerWorld::liquidLevel(Vec2I const& position) const {
  return m_worldServer->liquidLevel(position);
}

bool SpawnerWorld::spawningProhibited(RectF const& area) const {
  RectI region = RectI::integral(area);

  // Don't spawn the entity if its region overlaps with a dungeon
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y) {
      auto const& tile = m_worldServer->getServerTile({x, y});
      if (tile.collision == CollisionKind::Null || tile.dungeonId != NoDungeonId)
        return true;
    }
  }

  return false;
}

uint64_t SpawnerWorld::spawnSeed() const {
  return m_worldServer->worldTemplate()->worldSeed();
}

SpawnProfile SpawnerWorld::spawnProfile(Vec2F const& position) const {
  Vec2I ipos = Vec2I::floor(position);
  // Block biome, *not* environment biome, includes things like detached
  // biomes.
  if (auto biome = m_worldServer->worldTemplate()->blockBiome(ipos[0], ipos[1])) {
    // Dungeons, including ConstructionDungeonId (player constructed areas)
    // should be immune from spawning.
    auto tile = m_worldServer->getServerTile(ipos);
    if (tile.dungeonId == NoDungeonId)
      return biome->spawnProfile;
  }
  return {};
}

float SpawnerWorld::dayLevel() const {
  return m_worldServer->sky()->dayLevel();
}

float SpawnerWorld::threatLevel() const {
  return m_worldServer->threatLevel();
}

EntityId SpawnerWorld::spawnEntity(EntityPtr entity) const {
  m_worldServer->addEntity(entity);
  return entity->entityId();
}

void SpawnerWorld::despawnEntity(EntityId entityId) {
  m_worldServer->removeEntity(entityId, false);
}

EntityPtr SpawnerWorld::getEntity(EntityId entityId) const {
  return m_worldServer->entity(entityId);
}

WorldGenerator::WorldGenerator(WorldServer* server) : m_worldServer(server) {
  m_microDungeonFactory = make_shared<MicroDungeonFactory>();
}

void WorldGenerator::generateSectorLevel(WorldStorage* worldStorage, Sector const& sector, SectorGenerationLevel generationLevel) {
  if (generationLevel == SectorGenerationLevel::BaseTiles) {
    prepareTiles(worldStorage, sector);
  } else if (generationLevel == SectorGenerationLevel::MicroDungeons) {
    if (!worldStorage->floatingDungeonWorld())
      generateMicroDungeons(worldStorage, sector);
  } else if (generationLevel == SectorGenerationLevel::CaveLiquid) {
    if (!worldStorage->floatingDungeonWorld())
      generateCaveLiquid(worldStorage, sector);
  } else if (generationLevel == SectorGenerationLevel::Finalize) {
    if (!worldStorage->floatingDungeonWorld())
      prepareSector(worldStorage, sector);
    else
      prepareSectorBiomeBlocks(worldStorage, sector);
    m_worldServer->activateLiquidRegion(worldStorage->tileArray()->sectorRegion(sector));
  }
}

void WorldGenerator::sectorLoadLevelChanged(WorldStorage* worldStorage, Sector const& sector, SectorLoadLevel loadLevel) {
  if (loadLevel == SectorLoadLevel::Loaded) {
    if (worldStorage->sectorGenerationLevel(sector) == SectorGenerationLevel::Complete)
      m_worldServer->activateLiquidRegion(worldStorage->tileArray()->sectorRegion(sector));
  }
}

void WorldGenerator::terraformSector(WorldStorage* worldStorage, Sector const& sector) {
  // Logger::info("terraforming sector {}...", sector);
  reapplyBiome(worldStorage, sector);
}

void WorldGenerator::initEntity(WorldStorage*, EntityId entityId, EntityPtr const& entity) {
  entity->init(m_worldServer, entityId, EntityMode::Master);
  if (auto tileEntity = as<TileEntity>(entity))
    m_worldServer->updateTileEntityTiles(tileEntity, false, false);
}

void WorldGenerator::destructEntity(WorldStorage*, EntityPtr const& entity) {
  if (entity->isSlave())
    throw StarException("Cannot destruct slave entity in WorldStorage, something has gone wrong!");
  if (auto tileEntity = as<TileEntity>(entity))
    m_worldServer->updateTileEntityTiles(tileEntity, true, false);
  entity->uninit();
}

bool WorldGenerator::entityKeepAlive(WorldStorage*, EntityPtr const& entity) const {
  return entity->isSlave() || (entity->isMaster() && entity->keepAlive());
}

bool WorldGenerator::entityPersistent(WorldStorage*, EntityPtr const& entity) const {
  return entity->isMaster() && entity->persistent();
}

RpcPromise<Vec2I> WorldGenerator::enqueuePlacement(List<BiomeItemDistribution> distributions, Maybe<DungeonId> id) {
  auto promise = RpcPromise<Vec2I>::createPair();
  m_queuedPlacements.append(QueuedPlacement {
    std::move(distributions),
    id,
    promise.second,
    false,
  });
  return promise.first;
}

void WorldGenerator::replaceBiomeBlocks(ServerTile* tile) {
  if (auto blockBiome = m_worldServer->worldTemplate()->biome(tile->blockBiomeIndex)) {
    if (tile->foreground == BiomeMaterialId) {
      tile->foreground = blockBiome->mainBlock;
      tile->foregroundHueShift = m_worldServer->worldTemplate()->biomeMaterialHueShift(tile->blockBiomeIndex, tile->foreground);
    } else if ((tile->foreground >= Biome1MaterialId) && (tile->foreground <= Biome5MaterialId)) {
      auto& subblocks = blockBiome->subBlocks;
      if (subblocks.size())
        tile->foreground = subblocks[(tile->foreground - Biome1MaterialId) % subblocks.size()];
      else
        tile->foreground = blockBiome->mainBlock;
      tile->foregroundHueShift = m_worldServer->worldTemplate()->biomeMaterialHueShift(tile->blockBiomeIndex, tile->foreground);
    }

    if (tile->background == BiomeMaterialId) {
      tile->background = blockBiome->mainBlock;
      tile->backgroundHueShift =
          m_worldServer->worldTemplate()->biomeMaterialHueShift(tile->blockBiomeIndex, tile->background);
    } else if ((tile->background >= Biome1MaterialId) && (tile->background <= Biome5MaterialId)) {
      auto& subblocks = blockBiome->subBlocks;
      if (subblocks.size())
        tile->background = subblocks[(tile->background - Biome1MaterialId) % subblocks.size()];
      else
        tile->background = blockBiome->mainBlock;
      tile->backgroundHueShift = m_worldServer->worldTemplate()->biomeMaterialHueShift(tile->blockBiomeIndex, tile->background);
    }
  } else {
    if (isBiomeMaterial(tile->foreground)) {
      tile->foreground = EmptyMaterialId;
      tile->foregroundHueShift = 0;
    }
    if (isBiomeMod(tile->foregroundMod)) {
      tile->foregroundMod = NoModId;
      tile->foregroundModHueShift = 0;
    }
    if (isBiomeMaterial(tile->background)) {
      tile->background = EmptyMaterialId;
      tile->backgroundHueShift = 0;
    }
    if (isBiomeMod(tile->backgroundMod)) {
      tile->backgroundMod = NoModId;
      tile->backgroundModHueShift = 0;
    }
  }
}

void WorldGenerator::prepareTiles(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  auto materialDatabase = Root::singleton().materialDatabase();
  auto planet = m_worldServer->worldTemplate();
  // Generate sector.
  auto tileArray = worldStorage->tileArray();
  RectI sectorRegion = tileArray->sectorRegion(sector);
  for (int x = sectorRegion.xMin(); x < sectorRegion.xMax(); ++x) {
    for (int y = sectorRegion.yMin(); y < sectorRegion.yMax(); ++y) {
      Vec2I pos(x, y);
      ServerTile* tile = tileArray->modifyTile(pos);
      starAssert(tile);
      if (!tile)
        continue;

      auto blockInfo = planet->blockInfo(pos[0], pos[1]);

      tile->blockBiomeIndex = blockInfo.blockBiomeIndex;
      tile->environmentBiomeIndex = blockInfo.environmentBiomeIndex;
      tile->biomeTransition = blockInfo.biomeTransition;

      if (tile->foreground == NullMaterialId) {
        tile->foreground = blockInfo.foreground;
        tile->foregroundColorVariant = DefaultMaterialColorVariant;
        tile->foregroundHueShift = planet->biomeMaterialHueShift(tile->blockBiomeIndex, tile->foreground);

        if (materialDatabase->supportsMod(tile->foreground, blockInfo.foregroundMod)) {
          tile->foregroundMod = blockInfo.foregroundMod;
          tile->foregroundModHueShift = planet->biomeModHueShift(tile->blockBiomeIndex, tile->foregroundMod);
        }
      }

      if (tile->background == NullMaterialId) {
        tile->background = blockInfo.background;
        tile->backgroundColorVariant = DefaultMaterialColorVariant;
        tile->backgroundHueShift = planet->biomeMaterialHueShift(tile->blockBiomeIndex, tile->background);

        if (materialDatabase->supportsMod(tile->background, blockInfo.backgroundMod)) {
          tile->backgroundMod = blockInfo.backgroundMod;
          tile->backgroundModHueShift = planet->biomeModHueShift(tile->blockBiomeIndex, tile->backgroundMod);
        }
      }

      if (tile->foreground != EmptyMaterialId) {
        tile->liquid = LiquidStore();
      } else if (blockInfo.oceanLiquid != EmptyLiquidId && pos[1] < blockInfo.oceanLiquidLevel) {
        float pressure = blockInfo.oceanLiquidLevel - pos[1];
        if (tile->background == EmptyMaterialId)
          tile->liquid = LiquidStore::endless(blockInfo.oceanLiquid, pressure);
        else if (blockInfo.encloseLiquids)
          tile->liquid = LiquidStore::filled(blockInfo.oceanLiquid, 1.0f, pressure);
      }
    }
  }
}

void WorldGenerator::generateMicroDungeons(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  auto facade = make_shared<DungeonGeneratorWorld>(m_worldServer, false);

  RectI sectorTiles = worldStorage->tileArray()->sectorRegion(sector);
  RectI bounds = sectorTiles.padded(WorldSectorSize - 1);

  List<pair<BiomeItemPlacement, QueuedPlacement*>> placementQueue;
  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      auto potential = m_worldServer->worldTemplate()->potentialBiomeItemsAt(x, y);
      for (auto const& placement : m_worldServer->worldTemplate()->validBiomeItems(x, y, potential))
        placementQueue.append({placement, {}});

      for (auto& p : m_queuedPlacements) {
        WorldTemplate::PotentialBiomeItems queuedItems;
        m_worldServer->worldTemplate()->addPotentialBiomeItems(x, y, queuedItems, p.distributions, BiomePlacementArea::Surface);
        m_worldServer->worldTemplate()->addPotentialBiomeItems(x, y, queuedItems, p.distributions, BiomePlacementArea::Underground);
        for (auto placement : m_worldServer->worldTemplate()->validBiomeItems(x, y, queuedItems))
          placementQueue.append({std::move(placement), &p});
      }
    }
  }

  sort(placementQueue);
  for (auto const& p : placementQueue) {
    auto& placement = p.first;
    auto queued = p.second;
    if (queued && queued->fulfilled)
      continue;

    if (placement.item.is<MicroDungeonNames>()) {
      auto seed = m_worldServer->worldTemplate()->seedFor(placement.position[0], placement.position[1]);
      auto const& dungeonName = staticRandomFrom(placement.item.get<MicroDungeonNames>(), seed);
      Maybe<DungeonId> dungeonId;
      starAssert(!dungeonName.empty());
      if (auto generateResult = m_microDungeonFactory->generate(bounds, dungeonName, placement.position, seed, m_worldServer->threatLevel(), facade)) {
        if (queued) {
          dungeonId = queued->dungeonId;
          queued->promise.fulfill(placement.position);
          queued->fulfilled = true;
        }
        for (auto position : generateResult->second) {
          if (ServerTile* tile = m_worldServer->modifyServerTile(position)) {
            replaceBiomeBlocks(tile);
            tile->dungeonId = dungeonId.value(tile->dungeonId);
          }
        }
      }
    }
  }
  
  m_queuedPlacements = m_queuedPlacements.filtered([&](QueuedPlacement& p) {
      return !p.fulfilled;
    });
}

void WorldGenerator::generateCaveLiquid(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  Set<Vec2I> openNodes = caveLiquidSeeds(worldStorage, sector);

  if (!openNodes.size())
    return;

  auto tileArray = worldStorage->tileArray();

  Vec2I dimensions(tileArray->size());

  auto wrapCoords = [=](Vec2I const& coord) -> Vec2I { return Vec2I{pmod<int>(coord[0], dimensions[0]), coord[1]}; };

  RectI sectorTiles = tileArray->sectorRegion(sector);
  RectI bounds = sectorTiles.padded(Vec2I(WorldSectorSize - 1, WorldSectorSize - 1));

  bounds.min()[1] = clamp<int>(bounds.min()[1], 0, dimensions[1] - 1);
  bounds.max()[1] = clamp<int>(bounds.max()[1], 0, dimensions[1] - 1);

  auto materialDatabase = Root::singleton().materialDatabase();

  auto samplePoint = sectorTiles.center();
  auto blockInfo = m_worldServer->worldTemplate()->blockInfo(samplePoint[0], samplePoint[1]);
  LiquidId fillLiquid = blockInfo.caveLiquid;
  bool fillMicrodungeons = blockInfo.fillMicrodungeons;
  bool encloseLiquids = blockInfo.encloseLiquids;

  Set<Vec2I> badNodes;

  for (int i = bounds.xMin(); i <= bounds.xMax(); i++) {
    badNodes.add({i, bounds.yMin()});
    badNodes.add({i, bounds.yMax()});
  }
  for (int i = bounds.yMin(); i <= bounds.yMax(); i++) {
    badNodes.add({bounds.xMin(), i});
    badNodes.add({bounds.xMax(), i});
  }

  Set<Vec2I> candidateNodes;

  auto propose = [&](Vec2I const& position) {
    if (!bounds.contains(position))
      return;
    if (candidateNodes.contains(position))
      return;
    if (badNodes.contains(position))
      return;
    auto tile = tileArray->tile(wrapCoords(position));
    starAssert(tile.foreground != NullMaterialId);
    if (tile.foreground != EmptyMaterialId) {
      // Not sure why this doesn't poison solid materials, but it does (occasionally) encounter that case
      if (!BlockCollisionSet.contains(materialDatabase->materialCollisionKind(tile.foreground)))
        badNodes.add(position);
      return;
    }
    if ((tile.dungeonId != NoDungeonId && (!fillMicrodungeons || tile.dungeonId != BiomeMicroDungeonId))
        || (!encloseLiquids && tile.background == EmptyMaterialId)
        || (tile.liquid.liquid != fillLiquid && tile.liquid.liquid != EmptyLiquidId)) {
      badNodes.add(position);
      return;
    }
    candidateNodes.add(position);
    openNodes.add(position);
  };

  while (openNodes.size()) {
    auto node = *openNodes.begin();
    openNodes.remove(node);
    propose(node + Vec2I(-1, 0));
    propose(node + Vec2I(1, 0));
    propose(node + Vec2I(0, -1));
  }

  Set<Vec2I> visitedNodes;

  auto poison = [&](Vec2I position) {
    if (!bounds.contains(position))
      return;
    if (visitedNodes.contains(position))
      return;
    visitedNodes.add(position);
    auto tile = tileArray->tile(wrapCoords(position));
    starAssert(tile.foreground != NullMaterialId);
    if (tile.foreground != EmptyMaterialId)
      return;
    badNodes.add(position);
  };

  while (badNodes.size()) {
    auto node = *badNodes.begin();
    badNodes.remove(node);
    candidateNodes.remove(node);
    poison(node + Vec2I(-1, 0));
    poison(node + Vec2I(1, 0));
    poison(node + Vec2I(0, 1)); // upwards, not downwards
  }

  Set<Vec2I> solidSurroundings(candidateNodes);

  auto solids = [&](Vec2I position) {
    auto tile = tileArray->tile(wrapCoords(position));
    starAssert(tile.foreground != NullMaterialId);
    if (tile.foreground != EmptyMaterialId)
      solidSurroundings.add(position);
  };

  for (auto position : candidateNodes) {
    solids(position + Vec2I(1, 0));
    solids(position + Vec2I(-1, 0));
    solids(position + Vec2I(0, 1));
    solids(position + Vec2I(0, -1));
  }

  MaterialId biomeBlock = m_worldServer->worldTemplate()->biome(tileArray->tile(samplePoint).blockBiomeIndex)->mainBlock;
  Map<Vec2I, float> drops = determineLiquidLevel(candidateNodes, solidSurroundings);
  for (auto iter = drops.begin(); iter != drops.end(); ++iter) {
    auto tile = tileArray->modifyTile(wrapCoords(iter->first));
    starAssert(tile);
    if (!tile)
      continue;
    if (iter->second)
      tile->liquid = LiquidStore::filled(fillLiquid, 1.0f, iter->second);
    if (encloseLiquids && tile->background == EmptyMaterialId)
      tile->background = biomeBlock;
  }
}

void WorldGenerator::prepareSector(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  auto materialDatabase = Root::singleton().materialDatabase();
  auto planet = m_worldServer->worldTemplate();
  auto tileArray = worldStorage->tileArray();
  RectI sectorTiles = tileArray->sectorRegion(sector);

  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      Vec2I position(x, y);
      ServerTile* tile = tileArray->modifyTile(position);
      starAssert(tile);
      if (!tile)
        continue;
      starAssert(tile->foreground != NullMaterialId);

      if (tile->liquid.source) {
        auto blockInfo = planet->blockInfo(position[0], position[1]);
        // make sure that ocean liquid never exists on tiles without empty
        // background (except in real dungeons)
        if (!isRealDungeon(tile->dungeonId) && tile->background != EmptyMaterialId)
          tile->liquid.source = false;
        // pressurize liquid under the ocean
        if (blockInfo.oceanLiquid != EmptyLiquidId && position[1] < blockInfo.oceanLiquidLevel) {
          float pressure = blockInfo.oceanLiquidLevel - position[1];
          tile->liquid.pressure = pressure;
        }
      }

      if (!isRealMaterial(tile->foreground))
        tile->foregroundColorVariant = DefaultMaterialColorVariant;
      if (!isRealMaterial(tile->background))
        tile->backgroundColorVariant = DefaultMaterialColorVariant;

      replaceBiomeBlocks(tile);
      placeBiomeGrass(worldStorage, tile, position);

      tile->collision = maxCollision(tile->collision, materialDatabase->materialCollisionKind(tile->foreground));
    }
  }

  List<BiomeItemPlacement> placementQueue;
  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      auto tile = tileArray->tile(Vec2I(x, y));
      if (tile.dungeonId == NoDungeonId) {
        auto potential = m_worldServer->worldTemplate()->potentialBiomeItemsAt(x, y);
        for (auto const& placement : m_worldServer->worldTemplate()->validBiomeItems(x, y, potential))
          placementQueue.append(placement);
      }
    }
  }

  sort(placementQueue);
  for (auto const& placement : placementQueue) {
    auto seed = m_worldServer->worldTemplate()->seedFor(placement.position[0], placement.position[1]);
    if (placement.item.is<GrassVariant>()) {
      auto& grass = placement.item.get<GrassVariant>();
      placePlant(worldStorage, Root::singleton().plantDatabase()->createPlant(grass, seed), placement.position);
    } else if (placement.item.is<BushVariant>()) {
      auto& bush = placement.item.get<BushVariant>();
      placePlant(worldStorage, Root::singleton().plantDatabase()->createPlant(bush, seed), placement.position);
    } else if (placement.item.is<TreePair>()) {
      auto& treePair = placement.item.get<TreePair>();
      TreeVariant treeVariant;
      if (seed % 2 == 0)
        treeVariant = treePair.first;
      else
        treeVariant = treePair.second;

      placePlant(worldStorage, Root::singleton().plantDatabase()->createPlant(treeVariant, seed), placement.position);
    } else if (placement.item.is<ObjectPool>()) {
      auto& objectPool = placement.item.get<ObjectPool>();
      auto direction = seed % 2 ? Direction::Left : Direction::Right;
      auto objectPair = objectPool.select(seed);
      if (auto object = Root::singleton().objectDatabase()->createForPlacement(
              m_worldServer, objectPair.first, placement.position, direction, objectPair.second))
        m_worldServer->addEntity(object);
    } else if (placement.item.is<TreasureBoxSet>()) {
      auto& treasureBoxSet = placement.item.get<TreasureBoxSet>();
      auto direction = seed % 2 ? Direction::Left : Direction::Right;
      if (auto treasureContainer = Root::singleton().treasureDatabase()->createTreasureChest(
              m_worldServer, treasureBoxSet, placement.position, direction, seed))
        m_worldServer->addEntity(treasureContainer);
    }
  }

  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      if (auto* tile = worldStorage->tileArray()->modifyTile({x, y}))
        tile->collisionCacheDirty = true;
    }
  }
}

void WorldGenerator::prepareSectorBiomeBlocks(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  auto tileArray = worldStorage->tileArray();
  auto materialDatabase = Root::singleton().materialDatabase();
  RectI sectorTiles = tileArray->sectorRegion(sector);

  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      Vec2I position(x, y);
      ServerTile* tile = tileArray->modifyTile(position);

      replaceBiomeBlocks(tile);
      placeBiomeGrass(worldStorage, tile, position);

      tile->collision = maxCollision(tile->collision, materialDatabase->materialCollisionKind(tile->foreground));
    }
  }
}

void WorldGenerator::placeBiomeGrass(WorldStorage* worldStorage, ServerTile* tile, Vec2I const& position) {
  if (auto blockBiome = m_worldServer->worldTemplate()->biome(tile->blockBiomeIndex)) {
    // determine layer for grass mod calculation
    TileLayer modLayer = tile->foreground != EmptyMaterialId ? TileLayer::Foreground : TileLayer::Background;

    // don't place mods in dungeons unless explicitly specified, also don't
    // touch non-grass mods
    if (tile->mod(modLayer) == BiomeModId || tile->mod(modLayer) == UndergroundBiomeModId
        || (tile->dungeonId == NoDungeonId && tile->mod(modLayer) == NoModId)) {
      // check whether we're floor or ceiling
      auto tileAbove = worldStorage->tileArray()->tile(position + Vec2I(0, 1));
      auto tileBelow = worldStorage->tileArray()->tile(position + Vec2I(0, -1));
      bool isFloor = (tile->foreground != EmptyMaterialId && tileAbove.foreground == EmptyMaterialId) || (tile->background != EmptyMaterialId && tileAbove.background == EmptyMaterialId);
      bool isCeiling = !isFloor && ((tile->foreground != EmptyMaterialId && tileBelow.foreground == EmptyMaterialId) || (tile->background != EmptyMaterialId && tileBelow.background == EmptyMaterialId));

      // get the appropriate placeables for above/below ground
      BiomePlaceables const* placeables;
      if ((isFloor && tileAbove.background != EmptyMaterialId) || (isCeiling && tileBelow.background != EmptyMaterialId))
        placeables = &blockBiome->undergroundPlaceables;
      else
        placeables = &blockBiome->surfacePlaceables;

      // determine the proper grass mod or lack thereof
      ModId grassModId = NoModId;
      if (isFloor) {
        auto grassChance = staticRandomFloat(m_worldServer->worldTemplate()->worldSeed(), position[0], position[1]);
        if (isRealMod(placeables->grassMod) && grassChance <= placeables->grassModDensity)
          grassModId = placeables->grassMod;
      } else if (isCeiling) {
        auto grassChance = staticRandomFloat(m_worldServer->worldTemplate()->worldSeed(), position[0], position[1]);
        if (isRealMod(placeables->ceilingGrassMod) && grassChance <= placeables->ceilingGrassModDensity)
          grassModId = placeables->ceilingGrassMod;
      }

      // set the selected grass mod
      if (modLayer == TileLayer::Foreground) {
        tile->foregroundMod = grassModId;
        tile->backgroundMod = NoModId;
      } else {
        tile->foregroundMod = NoModId;
        tile->backgroundMod = grassModId;
      }
    }

    // update hue shifts appropriately
    tile->foregroundModHueShift = m_worldServer->worldTemplate()->biomeModHueShift(tile->blockBiomeIndex, tile->foregroundMod);
    tile->backgroundModHueShift = m_worldServer->worldTemplate()->biomeModHueShift(tile->blockBiomeIndex, tile->backgroundMod);
  }
}

void WorldGenerator::reapplyBiome(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  auto materialDatabase = Root::singleton().materialDatabase();
  auto planet = m_worldServer->worldTemplate();
  auto tileArray = worldStorage->tileArray();
  RectI sectorTiles = tileArray->sectorRegion(sector);

  // Logger::info("Reapplying biome in sector {}...", sectorTiles);

  auto entities = m_worldServer->entityQuery(RectF(sectorTiles.padded(1)));
  List<TileEntityPtr> biomeTileEntities;
  for (auto entity : entities) {
    if (auto plant = as<Plant>(entity)) {
      biomeTileEntities.append(as<TileEntity>(entity));
    } else if (auto object = as<Object>(entity)) {
      if (object->biomePlaced())
        biomeTileEntities.append(as<TileEntity>(entity));
    }
  }

  List<Vec2I> biomeItemTiles;

  for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
    for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y) {
      Vec2I position(x, y);
      ServerTile* tile = m_worldServer->modifyServerTile(position);
      starAssert(tile);
      if (!tile)
        continue;

      auto blockInfo = planet->blockBiomeInfo(position[0], position[1]);
      if (blockInfo.blockBiomeIndex != tile->blockBiomeIndex) {
        auto newBiome = planet->biome(blockInfo.blockBiomeIndex);
        auto oldBiome = planet->biome(tile->blockBiomeIndex);

        biomeTileEntities.filter([&, position](TileEntityPtr tileEntity) {
            if (tileEntity->tilePosition() == position) {
              m_worldServer->removeEntity(tileEntity->entityId(), false);
              return false;
            }
            return true;
          });

        // update biome index
        tile->blockBiomeIndex = blockInfo.blockBiomeIndex;
        tile->environmentBiomeIndex = blockInfo.environmentBiomeIndex;
        tile->biomeTransition = true;

        // replace biome blocks
        if (tile->foreground == oldBiome->mainBlock || oldBiome->subBlocks.contains(tile->foreground)) {
          tile->foreground = blockInfo.foreground;
          tile->foregroundColorVariant = DefaultMaterialColorVariant;
          if (tile->foreground == newBiome->mainBlock)
            tile->foregroundHueShift = newBiome->materialHueShift;
        }

        if (tile->background == oldBiome->mainBlock || oldBiome->subBlocks.contains(tile->background)) {
          tile->background = blockInfo.background;
          tile->backgroundColorVariant = DefaultMaterialColorVariant;
          if (tile->background == newBiome->mainBlock)
            tile->backgroundHueShift = newBiome->materialHueShift;
        }

        if (tile->foreground != EmptyMaterialId || tile->background != EmptyMaterialId) {
          // remove old biome mods
          if (tile->foregroundMod == oldBiome->surfacePlaceables.grassMod || tile->foregroundMod == oldBiome->surfacePlaceables.ceilingGrassMod ||
              tile->foregroundMod == oldBiome->undergroundPlaceables.grassMod || tile->foregroundMod == oldBiome->undergroundPlaceables.ceilingGrassMod) {

            tile->foregroundMod = NoModId;
            tile->foregroundModHueShift = 0;
          }

          if (tile->backgroundMod == oldBiome->surfacePlaceables.grassMod || tile->backgroundMod == oldBiome->surfacePlaceables.ceilingGrassMod ||
              tile->backgroundMod == oldBiome->undergroundPlaceables.grassMod || tile->backgroundMod == oldBiome->undergroundPlaceables.ceilingGrassMod) {

            tile->backgroundMod = NoModId;
            tile->backgroundModHueShift = 0;
          }

          // apply new biome mods
          TileLayer modLayer = tile->foreground != EmptyMaterialId ? TileLayer::Foreground : TileLayer::Background;

          if (tile->mod(modLayer) == NoModId) {
            // check whether we're floor or ceiling
            auto tileAbove = worldStorage->tileArray()->tile(position + Vec2I(0, 1));
            auto tileBelow = worldStorage->tileArray()->tile(position + Vec2I(0, -1));
            bool isFloor = tile->foreground != EmptyMaterialId && tileAbove.foreground == EmptyMaterialId;
            bool isCeiling = !isFloor && tile->foreground != EmptyMaterialId && tileBelow.foreground == EmptyMaterialId;
            bool isModFloor, isModCeiling;
            if (modLayer == TileLayer::Foreground) {
              isModFloor = isFloor;
              isModCeiling = isCeiling;
            } else {
              isModFloor = tile->background != EmptyMaterialId && tileAbove.background == EmptyMaterialId;
              isModCeiling = !isModFloor && tile->background != EmptyMaterialId && tileBelow.background == EmptyMaterialId;
            }

            // get the appropriate placeables for above/below ground
            BiomePlaceables const* placeables;
            if ((isFloor && tileAbove.background != EmptyMaterialId) || (isCeiling && tileBelow.background != EmptyMaterialId))
              placeables = &newBiome->undergroundPlaceables;
            else
              placeables = &newBiome->surfacePlaceables;

            // determine the proper grass mod or lack thereof
            ModId grassModId = NoModId;
            if (isModFloor) {
              auto grassChance = staticRandomFloat(m_worldServer->worldTemplate()->worldSeed(), position[0], position[1]);
              if (isRealMod(placeables->grassMod) && grassChance <= placeables->grassModDensity)
                grassModId = placeables->grassMod;
            } else if (isModCeiling) {
              auto grassChance = staticRandomFloat(m_worldServer->worldTemplate()->worldSeed(), position[0], position[1]);
              if (isRealMod(placeables->ceilingGrassMod) && grassChance <= placeables->ceilingGrassModDensity)
                grassModId = placeables->ceilingGrassMod;
            }

            // set the selected grass mod
            if (modLayer == TileLayer::Foreground && materialDatabase->supportsMod(tile->foreground, grassModId)) {
              tile->foregroundMod = grassModId;
              tile->backgroundMod = NoModId;
            } else if (modLayer == TileLayer::Background && materialDatabase->supportsMod(tile->background, grassModId)) {
              tile->foregroundMod = NoModId;
              tile->backgroundMod = grassModId;
            }
          }
        } else {
          tile->foregroundMod = NoModId;
          tile->backgroundMod = NoModId;
        }

        tile->collision = maxCollision(tile->collision, materialDatabase->materialCollisionKind(tile->foreground));
      }

      if (tile->biomeTransition && !blockInfo.biomeTransition) {
        tile->biomeTransition = false;
        if (!isSolidColliding(tile->collision))
          biomeItemTiles.append(position);
      }
    }
  }

  auto simplePlacePlant = [&](PlantPtr const& plant, Vec2I const& position) {
      if (!plant)
        return false;

      auto spaces = plant->spaces();
      auto roots = plant->roots();
      auto const& primaryRoot = plant->primaryRoot();

      auto blockBiome = planet->worldLayout()->getBiome(worldStorage->tileArray()->tile(position).blockBiomeIndex);

      auto positionValid = [&](Vec2I const& pos) {
          auto primaryTile = worldStorage->tileArray()->tile(pos);
          auto primaryRootTile = worldStorage->tileArray()->tile(pos + primaryRoot);
          if (isConnectableMaterial(primaryTile.foreground) || !isConnectableMaterial(primaryRootTile.foreground))
            return false;

          for (auto root : roots) {
            auto rootTile = worldStorage->tileArray()->tile(root + pos);
            if (!isConnectableMaterial(rootTile.foreground) || rootTile.blockBiomeIndex != primaryTile.blockBiomeIndex ||
                (rootTile.foreground != blockBiome->mainBlock && !blockBiome->subBlocks.contains(rootTile.foreground)))
              return false;
          }

          for (auto space : spaces) {
            Vec2I pspace = space + pos;

            if (!m_worldServer->atTile<TileEntity>(pspace).empty())
              return false;

            auto tile = worldStorage->tileArray()->tile(pspace);
            if (tile.foreground != EmptyMaterialId)
              return false;
          }

          return true;
        };

      List<Vec2I> tryPositions = {
          position,
          position + Vec2I{-1, 0},
          position + Vec2I{1, 0},
          position + Vec2I{-2, 0},
          position + Vec2I{2, 0},
          position + Vec2I{-1, 1},
          position + Vec2I{-1, -1},
          position + Vec2I{1, 1},
          position + Vec2I{1, -1}
        };

      for (auto pos : tryPositions) {
        if (positionValid(pos)) {
          plant->setTilePosition(pos);
          m_worldServer->addEntity(plant);
          return true;
        }
      }

      return false;
    };

  auto placeBiomeItem = [&](BiomeItemPlacement biomeItemPlacement, Vec2I position) {
      auto seed = m_worldServer->worldTemplate()->seedFor(position[0], position[1]);
      if (biomeItemPlacement.item.is<GrassVariant>()) {
        auto& grass = biomeItemPlacement.item.get<GrassVariant>();
        simplePlacePlant(Root::singleton().plantDatabase()->createPlant(grass, seed), position);
      } else if (biomeItemPlacement.item.is<BushVariant>()) {
        auto& bush = biomeItemPlacement.item.get<BushVariant>();
        simplePlacePlant(Root::singleton().plantDatabase()->createPlant(bush, seed), position);
      } else if (biomeItemPlacement.item.is<TreePair>()) {
        auto& treePair = biomeItemPlacement.item.get<TreePair>();
        TreeVariant treeVariant;
        if (seed % 2 == 0)
          treeVariant = treePair.first;
        else
          treeVariant = treePair.second;

        simplePlacePlant(Root::singleton().plantDatabase()->createPlant(treeVariant, seed), position);
      } else if (biomeItemPlacement.item.is<ObjectPool>()) {
        auto& objectPool = biomeItemPlacement.item.get<ObjectPool>();
        auto direction = seed % 2 ? Direction::Left : Direction::Right;
        auto objectPair = objectPool.select(seed);
        if (auto object = Root::singleton().objectDatabase()->createForPlacement(m_worldServer, objectPair.first, position, direction, objectPair.second)) {
          if (object->biomePlaced())
            m_worldServer->addEntity(object);
        }
      }
    };

  for (auto position : biomeItemTiles) {
    ServerTile* tile = m_worldServer->modifyServerTile(position);

    auto blockBiome = planet->worldLayout()->getBiome(tile->blockBiomeIndex);
    auto tileAbove = m_worldServer->getServerTile(position + Vec2I{0, 1});
    auto tileBelow = m_worldServer->getServerTile(position + Vec2I{0, -1});

    if (tile->background != EmptyMaterialId) {
      for (auto const& itemDistribution : blockBiome->undergroundPlaceables.itemDistributions) {
        if (itemDistribution.mode() == BiomePlacementMode::Background) {
          if (auto itemToPlace = itemDistribution.itemToPlace(position[0], position[1]))
            placeBiomeItem(*itemToPlace, position);
        }
      }

      if (isSolidColliding(tileAbove.collision)) {
        for (auto const& itemDistribution : blockBiome->undergroundPlaceables.itemDistributions) {
          if (itemDistribution.mode() == BiomePlacementMode::Ceiling) {
            if (auto itemToPlace = itemDistribution.itemToPlace(position[0], position[1]))
              placeBiomeItem(*itemToPlace, position);
          }
        }
      }

      if (isSolidColliding(tileBelow.collision)) {
        for (auto const& itemDistribution : blockBiome->undergroundPlaceables.itemDistributions) {
          if (itemDistribution.mode() == BiomePlacementMode::Floor) {
            if (auto itemToPlace = itemDistribution.itemToPlace(position[0], position[1]))
              placeBiomeItem(*itemToPlace, position);
          }
        }
      }
    } else {
      if (isSolidColliding(tileBelow.collision)) {
        for (auto const& itemDistribution : blockBiome->surfacePlaceables.itemDistributions) {
          if (itemDistribution.mode() == BiomePlacementMode::Floor) {
            if (auto itemToPlace = itemDistribution.itemToPlace(position[0], position[1]))
              placeBiomeItem(*itemToPlace, position);
          }
        }
      }
    }
  }
}

Set<Vec2I> WorldGenerator::caveLiquidSeeds(WorldStorage* worldStorage, ServerTileSectorArray::Sector const& sector) {
  RectI sectorTiles = worldStorage->tileArray()->sectorRegion(sector);
  auto samplePoint = sectorTiles.center();
  auto blockInfo = m_worldServer->worldTemplate()->blockInfo(samplePoint[0], samplePoint[1]);
  float seedDensity = blockInfo.caveLiquidSeedDensity;
  Set<Vec2I> nodes;
  if (seedDensity > 0) {
    int frequency = int(100 / seedDensity);
    for (int y = frequency * floor(sectorTiles.min()[1] / frequency); y < sectorTiles.max()[1]; y += frequency) {
      for (int x = frequency * floor(sectorTiles.min()[0] / frequency); x < sectorTiles.max()[0]; x += frequency) {
        if (sectorTiles.contains(Vec2I(x, y)))
          nodes.add(Vec2I(x, y));
      }
    }
  }
  return nodes;
}

Map<Vec2I, float> WorldGenerator::determineLiquidLevel(Set<Vec2I> const& spots, Set<Vec2I> const& filled) {
  Set<Vec2I> openSet(spots);
  Map<Vec2I, float> results;

  auto geometry = m_worldServer->geometry();

  while (openSet.size() > 0) {
    Set<Vec2I> cluster;
    Set<Vec2I> openCluster;
    openCluster.add(*(openSet.begin()));
    while (openCluster.size() > 0) {
      Vec2I node = *(openCluster.begin());
      openCluster.remove(node);
      if (openSet.contains(node)) {
        openSet.remove(node);
        cluster.add(node);
        openCluster.add(geometry.xwrap(Vec2I(node.x(), node.y() + 1)));
        openCluster.add(geometry.xwrap(Vec2I(node.x(), node.y() - 1)));
        openCluster.add(geometry.xwrap(Vec2I(node.x() + 1, node.y())));
        openCluster.add(geometry.xwrap(Vec2I(node.x() - 1, node.y())));
      }
    }
    levelCluster(cluster, filled, results);
  }
  return results;
}

void WorldGenerator::levelCluster(Set<Vec2I>& cluster, Set<Vec2I> const& filled, Map<Vec2I, float>& results) {
  int maxY = std::numeric_limits<int>::min();
  int minY = std::numeric_limits<int>::max();
  for (auto iter = cluster.begin(); iter != cluster.end(); iter++) {
    auto droplet = (*iter);
    if (filled.contains(droplet + Vec2I(1, 0)) && filled.contains(droplet + Vec2I(-1, 0))
        && filled.contains(droplet + Vec2I(0, -1))) {
      if (droplet.y() > maxY)
        maxY = droplet.y();
      if (!filled.contains(droplet + Vec2I(0, 1))) {
        if (droplet.y() <= minY)
          minY = droplet.y();
      }
    } else {
      if (droplet.y() <= minY)
        minY = droplet.y() - 1;
    }
  }
  int liquidLevel = std::min(maxY, minY);
  for (auto iter = cluster.begin(); iter != cluster.end(); iter++) {
    int pressure = (liquidLevel - (*iter).y());
    if (pressure >= 0)
      results[*iter] = 1.0f + pressure;
  }
}

bool WorldGenerator::placePlant(WorldStorage* worldStorage, PlantPtr const& plant, Vec2I const& position) {
  if (!plant)
    return false;

  auto spaces = plant->spaces();
  auto roots = plant->roots();
  auto const& primaryRoot = plant->primaryRoot();

  auto background = m_worldServer->getServerTile(position).background;
  bool adjustBackground = background == EmptyMaterialId || background == NullMaterialId;

  auto withinAdjustment = [=](Vec2I const& pos) {
    return PlantAdjustmentLimit - std::abs(pos[0]) > 0 && PlantAdjustmentLimit - std::abs(pos[1]) > 0;
  };

  // Bail out if we don't have at least one free space, and root in the primary
  // root position, or if we're in a dungeon region.
  auto primaryTile = worldStorage->tileArray()->tile(position);
  auto rootTile = worldStorage->tileArray()->tile(position + primaryRoot);
  if (primaryTile.dungeonId != NoDungeonId || rootTile.dungeonId != NoDungeonId)
    return false;
  if (isConnectableMaterial(primaryTile.foreground) || !isConnectableMaterial(rootTile.foreground))
    return false;

  // First bail out if we can't fit anything we're not adjusting
  for (auto space : spaces) {
    Vec2I pspace = space + position;

    if (withinAdjustment(space) && !m_worldServer->atTile<Plant>(pspace).empty())
      return false;

    // Bail out if we hit a different plant's root tile, or if we're not in the
    // adjustment space and we hit a non-empty tile.
    auto tile = worldStorage->tileArray()->tile(pspace);
    if (tile.rootSource || (!withinAdjustment(space) && tile.foreground != EmptyMaterialId))
      return false;
  }

  // Check all the roots outside of the adjustment limit
  for (auto root : roots) {
    root += position;
    if (!withinAdjustment(root) && !isConnectableMaterial(worldStorage->tileArray()->tile(root).foreground))
      return false;
  }

  // Clear all the necessary blocks within the adjustment limit
  for (auto space : spaces) {
    if (!withinAdjustment(space))
      continue;

    space += position;
    if (auto tile = worldStorage->tileArray()->modifyTile(space)) {
      if (isConnectableMaterial(tile->foreground))
        *tile = primaryTile;
      if (adjustBackground)
        tile->background = EmptyMaterialId;
      tile->collision = CollisionKind::None;
      tile->collisionCacheDirty = true;
    } else {
      return false;
    }
  }

  // Make all the root blocks a real material based on the primary root.
  for (auto root : roots) {
    root += position;
    if (auto tile = worldStorage->tileArray()->modifyTile(root)) {
      if (!isRealMaterial(tile->foreground)) {
        *tile = rootTile;
        tile->collision = Root::singleton().materialDatabase()->materialCollisionKind(tile->foreground);
        tile->collisionCacheDirty = true;
      }
    } else {
      return false;
    }
  }

  plant->setTilePosition(position);
  m_worldServer->addEntity(plant);
  return true;
}

}
