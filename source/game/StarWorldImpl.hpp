#pragma once

#include "StarIterator.hpp"
#include "StarEntityMap.hpp"
#include "StarWorldTiles.hpp"
#include "StarBlocksAlongLine.hpp"
#include "StarBiome.hpp"
#include "StarSky.hpp"
#include "StarWorldTemplate.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarCellularLighting.hpp"
#include "StarRoot.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarTileModification.hpp"

namespace Star {

namespace WorldImpl {
  template <typename TileSectorArray>
  bool tileIsOccupied(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, bool includeEphemeral, bool checkCollision = false);

  template <typename TileSectorArray>
  CollisionKind tileCollisionKind(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const& entityMap,
    Vec2I const& pos);

  template <typename TileSectorArray>
  bool rectTileCollision(shared_ptr<TileSectorArray> const& tileSectorArray, RectI const& region, bool solidCollision);

  template <typename TileSectorArray>
  bool lineTileCollision(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray, Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet);

  template <typename TileSectorArray>
  Maybe<pair<Vec2F, Vec2I>> lineTileCollisionPoint(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray, Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet);

  template <typename TileSectorArray>
  List<Vec2I> collidingTilesAlongLine(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet, size_t maxSize, bool includeEdges);

  template <typename GetTileFunction>
  bool canPlaceMaterial(EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, MaterialId material, bool allowEntityOverlap, bool allowTileOverlap, GetTileFunction& getTile);
  // returns true if this material could be placed if in the same batch other
  // tiles can be placed
  // that connect to it
  template <typename GetTileFunction>
  bool perhapsCanPlaceMaterial(EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, MaterialId material, bool allowEntityOverlap, bool allowTileOverlap, GetTileFunction& getTile);
  template <typename GetTileFunction>
  bool canPlaceMaterialColorVariant(Vec2I const& pos, TileLayer layer, MaterialColorVariant color, GetTileFunction& getTile);
  template <typename GetTileFunction>
  bool canPlaceMod(Vec2I const& pos, TileLayer layer, ModId mod, GetTileFunction& getTile);
  template <typename GetTileFunction>
  pair<bool, bool> validateTileModification(EntityMapPtr const& entityMap, Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap, GetTileFunction& getTile);
  // Split modification list into good and bad
  template <typename GetTileFunction>
  pair<TileModificationList, TileModificationList> splitTileModifications(EntityMapPtr const& entityMap, TileModificationList const& modificationList,
    bool allowEntityOverlap, GetTileFunction& getTile, function<bool(Vec2I pos, TileModification modification)> extraCheck = {});

  template <typename TileSectorArray>
  float windLevel(shared_ptr<TileSectorArray> const& tileSectorArray, Vec2F const& position, float weatherWindLevel);

  template <typename TileSectorArray>
  float temperature(shared_ptr<TileSectorArray> const& tileSectorArray, WorldTemplateConstPtr const& worldTemplate,
      SkyConstPtr const& sky, Vec2F const& pos);

  template <typename TileSectorArray>
  bool breathable(World const* world, shared_ptr<TileSectorArray> const& tileSectorArray, WorldTemplateConstPtr const& worldTemplate, Vec2F const& pos);

  template <typename TileSectorArray>
  float lightLevel(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const& entityMap, WorldGeometry const& worldGeometry,
      WorldTemplateConstPtr const& worldTemplate, SkyConstPtr const& sky, CellularLightIntensityCalculator& lighting, Vec2F pos);

  InteractiveEntityPtr getInteractiveInRange(WorldGeometry const& geometry, EntityMapPtr const& entityMap,
      Vec2F const& targetPosition, Vec2F const& sourcePosition, float maxRange);
  bool isTileEntityInRange(WorldGeometry const& geometry, EntityMapPtr const& entityMap,
      EntityId targetEntity, Vec2F const& sourcePosition, float maxRange);
  template <typename TileSectorArray>
  bool canReachEntity(WorldGeometry const& geometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      EntityMapPtr const& entityMap, Vec2F const& sourcePosition, float maxRange, EntityId targetEntity, bool preferInteractive = true);

  template <typename TileSectorArray>
  bool tileIsOccupied(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, bool includeEphemeral, bool checkCollision) {
    auto& tile = tileSectorArray->tile(pos);
    if (layer == TileLayer::Foreground)
      return (checkCollision ? tile.getCollision() >= CollisionKind::Dynamic : tile.foreground != EmptyMaterialId) || entityMap->tileIsOccupied(pos, includeEphemeral);
    else
      return tile.background != EmptyMaterialId;
  }

  template <typename TileSectorArray>
  CollisionKind tileCollisionKind(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const&,
    Vec2I const& pos) {
    return tileSectorArray->tile(pos).getCollision();
  }

  template <typename TileSectorArray>
  bool rectTileCollision(shared_ptr<TileSectorArray> const& tileSectorArray, RectI const& region, CollisionSet const& collisionSet) {
    return tileSectorArray->tileSatisfies(region, [&collisionSet](Vec2I const&, typename TileSectorArray::Tile const& tile) {
        return isColliding(tile.getCollision(), collisionSet);
      });
  }

  template <typename TileSectorArray>
  bool lineTileCollision(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) {
    return !forBlocksAlongLine<float>(begin, worldGeometry.diff(end, begin), [=](int x, int y) -> bool {
        return !tileSectorArray->tile({x, y}).isColliding(collisionSet);
      });
  }

  template <typename TileSectorArray>
  Maybe<pair<Vec2F, Vec2I>> lineTileCollisionPoint(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) {
    Maybe<Vec2I> collidingBlock;
    auto clear = forBlocksAlongLine<float>(begin, worldGeometry.diff(end, begin), [=, &collidingBlock](int x, int y) -> bool {
        if (tileSectorArray->tile({x, y}).isColliding(collisionSet)) {
          collidingBlock = Vec2I(x, y);
          return false;
        } else {
          return true;
        }
      });

    if (clear)
      return {}; // no colliding blocks along the line

    Vec2F direction = worldGeometry.diff(end, begin).normalized();
    Vec2F blockPosition = Vec2F(*collidingBlock);

    // find the position of intersecting sides
    if (direction[0] < 0) blockPosition[0] = blockPosition[0] + 1;
    if (direction[1] < 0) blockPosition[1] = blockPosition[1] + 1;
    Vec2F blockDistance = worldGeometry.diff(blockPosition, begin);

    // exclude edges which are in the opposite direction of the line
    if (direction[0] * blockDistance[0] < 0) blockDistance[0] = 0.0f;
    if (direction[1] * blockDistance[1] < 0) blockDistance[1] = 0.0f;

    // line length per block in each axis, don't divide by 0
    float deltaDistX = direction[0] == 0 ? 0.0f : 1.0f / abs(direction[0]);
    float deltaDistY = direction[1] == 0 ? 0.0f : 1.0f / abs(direction[1]);

    // distance to each edge of the intersect
    Vec2F intersectDist = Vec2F(abs(blockDistance[0]) * deltaDistX, abs(blockDistance[1]) * deltaDistY);

    Vec2I normal = Vec2I(intersectDist[0] > intersectDist[1] ? 1 : 0, intersectDist[1] > intersectDist[0] ? 1 : 0);
    normal[0] *= copysign(1.0f, direction[0]);
    normal[1] *= copysign(1.0f, direction[1]);
    Vec2F position = begin + (direction * max(intersectDist[0], intersectDist[1]));
    // HACK: Keeps the end position from being right on a tile border, making line check
    // *from* this end position from colliding with the first tile
    if (intersectDist[0] > intersectDist[1])
      position[0] = blockPosition[0] + (normal[0] * -0.0001f);
    else
      position[1] = blockPosition[1] + (normal[1] * -0.0001f);
    return pair<Vec2F, Vec2I>(position, normal);
  }

  template <typename TileSectorArray>
  inline LiquidLevel liquidLevel(shared_ptr<TileSectorArray> const& tileSectorArray, RectF const& region) {
    if (region.isEmpty())
      return LiquidLevel();

    RectI sampleRect = RectI::integral(region);
    // This is not entirely accurate, even though all liquid types in the
    // region count towards the grand total liquid percentage, only the most
    // common liquid is reported.
    float totalSpace = 0;
    Map<LiquidId, float> totals;
    tileSectorArray->tileEach(sampleRect, [&region, &totalSpace, &totals](Vec2I const& pos, typename TileSectorArray::Tile const& tile) {
        float blockIncidence = RectF(pos[0], pos[1], pos[0] + 1, pos[1] + 1).overlap(region).volume();
        totalSpace += blockIncidence;
        auto liquidLevel = tile.liquid;
        if (liquidLevel.liquid != EmptyLiquidId)
          totals[liquidLevel.liquid] += min(1.0f, liquidLevel.level) * blockIncidence;
      });

    float totalLiquidLevel = 0.0f;
    float maximumLiquidLevel = 0.0f;
    LiquidId maximumLiquidId = EmptyLiquidId;
    for (auto const& p : totals) {
      totalLiquidLevel += p.second;
      if (p.second > maximumLiquidLevel) {
        maximumLiquidLevel = p.second;
        maximumLiquidId = p.first;
      }
    }
    return LiquidLevel(maximumLiquidId, totalLiquidLevel / totalSpace);
  }

  template <typename TileSectorArray>
  List<Vec2I> collidingTilesAlongLine(WorldGeometry const& worldGeometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet, size_t maxSize, bool includeEdges) {
    List<Vec2I> res;
    forBlocksAlongLine<float>(begin, worldGeometry.diff(end, begin), [&](int x, int y) {
        if (res.size() >= maxSize)
          return false;
        if (tileSectorArray->tile({x, y}).isColliding(collisionSet))
          res.append({x, y});
        return true;
      });

    if (includeEdges)
      return res;

    SMutableIterator<List<Vec2I>> tileIt{res};
    while (tileIt.hasNext()) {
      auto tileRect = RectF::withSize(Vec2F(worldGeometry.xwrap(tileIt.next())), Vec2F::filled(1.0f));
      Line2F line{worldGeometry.xwrap(begin), worldGeometry.xwrap(end)};
      auto lineSet = worldGeometry.splitLine(line);
      if (any(lineSet, [&](Line2F const& line) { return tileRect.edgeIntersection(line).glances; }))
        tileIt.remove();
    }
    return res;
  }

  template <typename GetTileFunction>
  bool canPlaceMaterial(EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, MaterialId material, bool allowEntityOverlap, bool allowTileOverlap, GetTileFunction& getTile) {
    auto materialDatabase = Root::singleton().materialDatabase();

    if (!isRealMaterial(material))
      return false;

    auto isAdjacentToConnectable = [&](Vec2I const& pos, int distance, bool foreground) {
      if (pos.y() - distance < 0)
        return true;

      int maxY = pos.y() + distance + 1;
      int maxX = pos.x() + distance + 1;
      for (int y = pos.y() - distance; y != maxY; ++y) {
        Vec2I tPos = { 0, y };
        for (int x = pos.x() - distance; x != maxX; ++x) {
          tPos[0] = x;
          if (tPos != pos) {
            const auto& tile = getTile(tPos);
            if (isConnectableMaterial(foreground ? tile.foreground : tile.background))
              return true;
          }
        }
      }
      return false;
    };

    if (!materialDatabase->canPlaceInLayer(material, layer))
      return false;

    auto& tile = getTile(pos);
    if (layer == TileLayer::Background) {
      if (tile.background != EmptyMaterialId && tile.background != ObjectPlatformMaterialId)
        return false;

      // Can attach background blocks to other background blocks, *or* the
      // foreground block in front of it.
      if (!isAdjacentToConnectable(pos, 1, false) && !isConnectableMaterial(tile.foreground))
        return false;
    } else {
      if (tile.foreground != EmptyMaterialId && tile.foreground != ObjectPlatformMaterialId)
        return false;

      if (!allowTileOverlap && entityMap->tileIsOccupied(pos))
        return false;

      if (!allowEntityOverlap && entityMap->spaceIsOccupied(RectF::withSize(Vec2F(pos), Vec2F(0.999f, 0.999f))))
        return false;

      if (!isAdjacentToConnectable(pos, 1, true) && !isConnectableMaterial(tile.background))
        return false;
    }

    return true;
  }

  template <typename GetTileFunction>
  bool perhapsCanPlaceMaterial(EntityMapPtr const& entityMap,
      Vec2I const& pos, TileLayer layer, MaterialId material, bool allowEntityOverlap, bool allowTileOverlap, GetTileFunction& getTile) {
    auto materialDatabase = Root::singleton().materialDatabase();

    if (!isRealMaterial(material))
      return false;

    if (!materialDatabase->canPlaceInLayer(material, layer))
      return false;

    const auto& tile = getTile(pos);
    if (layer == TileLayer::Background) {
      if (tile.background != EmptyMaterialId && tile.background != ObjectPlatformMaterialId)
        return false;
    } else {
      if (tile.foreground != EmptyMaterialId && tile.foreground != ObjectPlatformMaterialId)
        return false;

      if (!allowTileOverlap && entityMap->tileIsOccupied(pos))
        return false;

      if (!allowEntityOverlap && entityMap->spaceIsOccupied(RectF::withSize(Vec2F(pos), Vec2F(0.999f, 0.999f))))
        return false;
    }

    return true;
  }

  template <typename GetTileFunction>
  bool canPlaceMaterialColorVariant(Vec2I const& pos, TileLayer layer, MaterialColorVariant color, GetTileFunction& getTile) {
    auto materialDatabase = Root::singleton().materialDatabase();
    auto& tile = getTile(pos);
    auto mat = tile.material(layer);
    auto existingColor = tile.materialColor(layer);
    auto existingHue = layer == TileLayer::Foreground ? tile.foregroundHueShift : tile.backgroundHueShift;
    return existingHue != 0 || (existingColor != color && materialDatabase->isMultiColor(mat));
  }

  template <typename GetTileFunction>
  bool canPlaceMod(Vec2I const& pos, TileLayer layer, ModId mod, GetTileFunction& getTile) {
    if (!isRealMod(mod))
      return false;

    auto materialDatabase = Root::singleton().materialDatabase();
    auto mat = getTile(pos).material(layer);
    auto existingMod = getTile(pos).mod(layer);

    return existingMod != mod && materialDatabase->supportsMod(mat, mod);
  }

  template <typename GetTileFunction>
  pair<bool, bool> validateTileModification(EntityMapPtr const& entityMap, Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap, GetTileFunction& getTile) {
    bool good = false;
    bool perhaps = false;
    
    if (auto placeMaterial = modification.ptr<PlaceMaterial>()) {
      bool allowTileOverlap = placeMaterial->collisionOverride != TileCollisionOverride::None && collisionKindFromOverride(placeMaterial->collisionOverride) < CollisionKind::Dynamic;
      perhaps = WorldImpl::perhapsCanPlaceMaterial(entityMap, pos, placeMaterial->layer, placeMaterial->material, allowEntityOverlap, allowTileOverlap, getTile);
      if (perhaps)
        good = WorldImpl::canPlaceMaterial(entityMap, pos, placeMaterial->layer, placeMaterial->material, allowEntityOverlap, allowTileOverlap, getTile);
    } else if (auto placeMod = modification.ptr<PlaceMod>()) {
      good = WorldImpl::canPlaceMod(pos, placeMod->layer, placeMod->mod, getTile);
    } else if (auto placeMaterialColor = modification.ptr<PlaceMaterialColor>()) {
      good = WorldImpl::canPlaceMaterialColorVariant(pos, placeMaterialColor->layer, placeMaterialColor->color, getTile);
    } else if (modification.is<PlaceLiquid>()) {
      good = getTile(pos).collision == CollisionKind::None;
    } else {
      good = false;
    }

    return { good, perhaps };
  }

  template <typename GetTileFunction>
  pair<TileModificationList, TileModificationList> splitTileModifications(EntityMapPtr const& entityMap, TileModificationList const& modificationList,
    bool allowEntityOverlap, GetTileFunction& getTile, function<bool(Vec2I pos, TileModification modification)> extraCheck) {
    TileModificationList success;
    TileModificationList unknown;
    TileModificationList failures;
    for (auto const& pair : modificationList) {

      bool good = false, perhaps = false;
      if (!extraCheck || extraCheck(pair.first, pair.second))
        std::tie(good, perhaps) = validateTileModification(entityMap, pair.first, pair.second, allowEntityOverlap, getTile);

      if (good)
        success.append(pair);
      else if (perhaps)
        unknown.append(pair);
      else
        failures.append(pair);
    }

    if (!success.empty())
      success.appendAll(unknown);
    else
      failures.appendAll(unknown);

    return {success, failures};
  }

  template <typename TileSectorArray>
  float windLevel(shared_ptr<TileSectorArray> const& tileSectorArray, Vec2F const& position, float weatherWindLevel) {
    auto const& tile = tileSectorArray->tile(Vec2I::floor(position));
    if (tile.material(TileLayer::Background) != EmptyMaterialId)
      return 0.0f;
    if (tile.material(TileLayer::Foreground) != EmptyMaterialId)
      return 0.0f;
    return weatherWindLevel;
  }

  template <typename TileSectorArray>
  bool breathable(World const* world, shared_ptr<TileSectorArray> const& tileSectorArray, HashMap<DungeonId, bool> const& breathableMap,
      WorldTemplateConstPtr const& worldTemplate, Vec2F const& pos) {
    Vec2I ipos = Vec2I::floor(pos);
    float remainder = pos[1] - ipos[1];
    auto materialDatabase = Root::singleton().materialDatabase();
    auto liquidsDatabase = Root::singleton().liquidsDatabase();

    auto tile = tileSectorArray->tile(ipos);
    bool environmentBreathable = breathableMap.maybe(tile.dungeonId).value(worldTemplate->breathable(ipos[0], ipos[1]));
    bool liquidBreathable = remainder >= tile.liquid.level;
    bool foregroundBreathable = tile.getCollision() != CollisionKind::Block || !world->pointCollision(pos);

    return environmentBreathable && foregroundBreathable && liquidBreathable;
  }

  template <typename TileSectorArray>
  float lightLevel(shared_ptr<TileSectorArray> const& tileSectorArray, EntityMapPtr const& entityMap, WorldGeometry const& worldGeometry,
      WorldTemplateConstPtr const& worldTemplate, SkyConstPtr const& sky, CellularLightIntensityCalculator& lighting, Vec2F pos) {
    if (pos[1] < 0 || pos[1] >= worldGeometry.height())
      return 0;

    // tileEach can't handle rects that are WAY out of range.
    pos = worldGeometry.xwrap(pos);

    Vec3F environmentLight = sky->environmentLight().toRgbF();
    float undergroundLevel = worldTemplate->undergroundLevel();
    auto materialDatabase = Root::singleton().materialDatabase();
    auto liquidsDatabase = Root::singleton().liquidsDatabase();

    lighting.begin(pos);

    // Each column in tileEvalColumns is guaranteed to be no larger than the
    // sector size.
    CellularLightIntensityCalculator::Cell lightingCellColumn[WorldSectorSize];
    tileSectorArray->tileEvalColumns(lighting.calculationRegion(), [&](Vec2I const& pos, typename TileSectorArray::Tile const* column, size_t ySize) {
        for (size_t y = 0; y < ySize; ++y) {
          auto& tile = column[y];
          auto& cell = lightingCellColumn[y];

          bool backgroundTransparent = materialDatabase->backgroundLightTransparent(tile.background);
          bool foregroundTransparent = materialDatabase->foregroundLightTransparent(tile.foreground)
              && tile.getCollision() != CollisionKind::Dynamic;

          cell = {materialDatabase->radiantLight(tile.foreground, tile.foregroundMod).sum() / 3.0f, !foregroundTransparent};
          cell.light += liquidsDatabase->radiantLight(tile.liquid).sum() / 3.0f;
          if (foregroundTransparent) {
            cell.light += materialDatabase->radiantLight(tile.background, tile.backgroundMod).sum() / 3.0f;
            if (backgroundTransparent && pos[1] > undergroundLevel)
              cell.light += environmentLight.sum() / 3.0f;
          }
        }
        lighting.setCellColumn(pos, lightingCellColumn, ySize);
      });

    for (auto const& entity : entityMap->entityQuery(RectF(lighting.calculationRegion()))) {
      for (auto const& light : entity->lightSources()) {
        Vec2F position = worldGeometry.nearestTo(Vec2F(lighting.calculationRegion().min()), light.position);
        if (light.type == LightType::Spread)
          lighting.addSpreadLight(position, light.color.sum() / 3.0f);
        else
          lighting.addPointLight(position, light.color.sum() / 3.0f, light.pointBeam, light.beamAngle, light.beamAmbience);
      }
    }

    return lighting.calculate();
  }

  inline InteractiveEntityPtr getInteractiveInRange(WorldGeometry const& geometry, EntityMapPtr const& entityMap,
      Vec2F const& targetPosition, Vec2F const& sourcePosition, float maxRange) {
    if (auto entity = entityMap->interactiveEntityNear(targetPosition, maxRange)) {
      if (auto tileEntity = as<TileEntity>(entity)) {
        if (isTileEntityInRange(geometry, entityMap, tileEntity->entityId(), sourcePosition, maxRange))
          return entity;
      } else {
        if (geometry.diffToNearestCoordInBox(entity->interactiveBoundBox().translated(entity->position()), sourcePosition) .magnitude() <= maxRange)
          return entity;
      }
    }

    return {};
  }

  inline bool isTileEntityInRange(WorldGeometry const& geometry, EntityMapPtr const& entityMap, EntityId targetEntity,
      Vec2F const& sourcePosition, float maxRange) {
    if (auto entity = entityMap->get<TileEntity>(targetEntity)) {
      // If any of the entity spaces is within interaction range, then assume the
      // whole entity is in interaction range
      for (auto space : entity->spaces()) {
        if (geometry.diff(entity->position() + centerOfTile(space), sourcePosition).magnitude() <= maxRange)
          return true;
      }
    }

    return false;
  }

  template <typename TileSectorArray>
  bool canReachEntity(WorldGeometry const& geometry, shared_ptr<TileSectorArray> const& tileSectorArray,
      EntityMapPtr const& entityMap, Vec2F const& sourcePosition, float maxRange, EntityId targetEntity, bool preferInteractive) {
    auto entity = entityMap->entity(targetEntity);
    if (!entity)
      return false;

    // exclude the final tile from the collision check since many targets will be collidable tile entities, e.g. doors
    auto const canReachTile = [&](Vec2F const& end) -> bool {
      Vec2I const& endTile = Vec2I::floor(end);
      return forBlocksAlongLine<float>(sourcePosition, geometry.diff(end, sourcePosition), [=](int x, int y) -> bool {
          Vec2I diff = geometry.diff(endTile, Vec2I(x, y));
          if (diff[0] == 0 && diff[1] == 0)
            return true;
          return !tileSectorArray->tile({x, y}).isColliding(DefaultCollisionSet);
        });
    };

    if (auto tileEntity = as<TileEntity>(entity)) {
      for (auto space : preferInteractive ? tileEntity->interactiveSpaces() : tileEntity->spaces()) {
        auto spacePosition = entity->position() + centerOfTile(space);
        if (geometry.diff(spacePosition, sourcePosition).magnitude() <= maxRange) {
          if (canReachTile(spacePosition))
            return true;
        }
      }
    } else if (preferInteractive && is<InteractiveEntity>(entity)) {
      auto interactiveEntity = as<InteractiveEntity>(entity);
      auto entityBounds = interactiveEntity->interactiveBoundBox().translated(entity->position());
      if (geometry.rectContains(entityBounds, sourcePosition))
        return true;
      if (!geometry.rectIntersectsCircle(entityBounds, sourcePosition, maxRange))
        return false;

      return
          !lineTileCollision(geometry, tileSectorArray, sourcePosition, entityBounds.nearestCoordTo(sourcePosition), DefaultCollisionSet) ||
          !lineTileCollision(geometry, tileSectorArray, sourcePosition, Vec2F(entityBounds.xMin(), entityBounds.yMin()), DefaultCollisionSet) ||
          !lineTileCollision(geometry, tileSectorArray, sourcePosition, Vec2F(entityBounds.xMin(), entityBounds.yMax()), DefaultCollisionSet) ||
          !lineTileCollision(geometry, tileSectorArray, sourcePosition, Vec2F(entityBounds.xMax(), entityBounds.yMax()), DefaultCollisionSet) ||
          !lineTileCollision(geometry, tileSectorArray, sourcePosition, Vec2F(entityBounds.xMax(), entityBounds.yMin()), DefaultCollisionSet);
    } else {
      if (geometry.diff(entity->position(), sourcePosition).magnitude() <= maxRange)
        return !lineTileCollision(geometry, tileSectorArray, sourcePosition, entity->position(), DefaultCollisionSet);
    }

    return false;
  }
}

}
