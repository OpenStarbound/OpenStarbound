#pragma once

#include "StarGameTypes.hpp"
#include "StarXXHash.hpp"
#include "StarLiquidTypes.hpp"
#include "StarTileDamage.hpp"
#include "StarTileSectorArray.hpp"
#include "StarCollisionGenerator.hpp"
#include "StarWorldLayout.hpp"
#include "StarVersion.hpp"

namespace Star {

struct WorldTile {
  WorldTile();

  // Copy constructor and operator= do not preserve collision cache.
  WorldTile(WorldTile const& worldTile);
  WorldTile& operator=(WorldTile const& worldTile);

  MaterialId material(TileLayer layer) const;
  ModId mod(TileLayer layer) const;
  MaterialColorVariant materialColor(TileLayer layer) const;
  tuple<MaterialId, MaterialHue, MaterialColorVariant> materialAndColor(TileLayer layer) const;
  bool isConnectable(TileLayer layer, bool materialOnly) const;
  bool isColliding(CollisionSet const& collisionSet) const;

  MaterialId foreground;
  MaterialHue foregroundHueShift;
  ModId foregroundMod;
  MaterialHue foregroundModHueShift;
  MaterialColorVariant foregroundColorVariant;

  MaterialId background;
  MaterialHue backgroundHueShift;
  ModId backgroundMod;
  MaterialHue backgroundModHueShift;
  MaterialColorVariant backgroundColorVariant;

  CollisionKind collision;

  bool collisionCacheDirty;
  StaticList<CollisionBlock, CollisionGenerator::MaximumCollisionsPerSpace> collisionCache;

  BiomeIndex blockBiomeIndex;
  BiomeIndex environmentBiomeIndex;

  bool biomeTransition;

  TileDamageStatus foregroundDamage;
  TileDamageStatus backgroundDamage;

  // If block is part of a dungeon then that affects spawns/drops,
  // as well as governing block protection
  DungeonId dungeonId;
};

struct ServerTile : public WorldTile {
  static VersionNumber const CurrentSerializationVersion;

  ServerTile();

  ServerTile(ServerTile const& serverTile);
  ServerTile& operator=(ServerTile const& serverTile);

  void write(DataStream& ds) const;
  void read(DataStream& ds, VersionNumber serializationVersion);

  // Updates collision, clears cache, and if the collision kind does not
  // support liquid destroys it.
  bool updateCollision(CollisionKind kind);
  // Used for setting the second collision kind calculated by object material spaces.
  bool updateObjectCollision(CollisionKind kind);

  // Calculates the actually-used collision kind based on the tile and object collision kinds.
  CollisionKind getCollision() const;

  LiquidStore liquid;

  // If set, a plant or object is rooted to the tile and tile damage
  // should be redirected to this position
  Maybe<Vec2I> rootSource;

  // Do not serialize - calculated at runtime
  CollisionKind objectCollision;
};
typedef TileSectorArray<ServerTile, WorldSectorSize> ServerTileSectorArray;
typedef shared_ptr<ServerTileSectorArray> ServerTileSectorArrayPtr;

struct ClientTile : public WorldTile {
  ClientTile();

  ClientTile(ClientTile const& clientTile);
  ClientTile& operator=(ClientTile const& clientTile);

  bool backgroundLightTransparent;
  bool foregroundLightTransparent;

  LiquidLevel liquid;

  float gravity;
};
typedef TileSectorArray<ClientTile, WorldSectorSize> ClientTileSectorArray;
typedef shared_ptr<ClientTileSectorArray> ClientTileSectorArrayPtr;

// Tile structure to transfer all data from client to server
struct NetTile {
  NetTile();

  MaterialId background;
  MaterialHue backgroundHueShift;
  MaterialColorVariant backgroundColorVariant;
  ModId backgroundMod;
  MaterialHue backgroundModHueShift;
  MaterialId foreground;
  MaterialHue foregroundHueShift;
  MaterialColorVariant foregroundColorVariant;
  ModId foregroundMod;
  MaterialHue foregroundModHueShift;
  CollisionKind collision;
  BiomeIndex blockBiomeIndex;
  BiomeIndex environmentBiomeIndex;
  LiquidNetUpdate liquid;
  DungeonId dungeonId;
};
DataStream& operator>>(DataStream& ds, NetTile& tile);
DataStream& operator<<(DataStream& ds, NetTile const& tile);

// For storing predicted tile state.
struct PredictedTile {
  int64_t time;
  Maybe<MaterialId> background;
  Maybe<MaterialHue> backgroundHueShift;
  Maybe<MaterialColorVariant> backgroundColorVariant;
  Maybe<ModId> backgroundMod;
  Maybe<MaterialHue> backgroundModHueShift;
  Maybe<MaterialId> foreground;
  Maybe<MaterialHue> foregroundHueShift;
  Maybe<MaterialColorVariant> foregroundColorVariant;
  Maybe<ModId> foregroundMod;
  Maybe<MaterialHue> foregroundModHueShift;
  Maybe<LiquidLevel> liquid;
  Maybe<CollisionKind> collision;

  operator bool() const;
  template <typename Tile>
  void apply(Tile& tile) {
    if (foreground) tile.foreground = *foreground;
    if (foregroundMod) tile.foregroundMod = *foregroundMod;
    if (foregroundHueShift) tile.foregroundHueShift = *foregroundHueShift;
    if (foregroundModHueShift) tile.foregroundModHueShift = *foregroundModHueShift;

    if (background) tile.background = *background;
    if (backgroundMod) tile.backgroundMod = *backgroundMod;
    if (backgroundHueShift) tile.backgroundHueShift = *backgroundHueShift;
    if (backgroundModHueShift) tile.backgroundModHueShift = *backgroundModHueShift;
  }
};

// Just the parts of a tile that are used to render.  The members here are laid
// out specifically to avoid padding bytes so that a fast path can be taken
// when hashing for chunk render caching.
struct RenderTile {
  MaterialId foreground;
  ModId foregroundMod;

  MaterialId background;
  ModId backgroundMod;

  MaterialHue foregroundHueShift;
  MaterialHue foregroundModHueShift;
  MaterialColorVariant foregroundColorVariant;
  TileDamageType foregroundDamageType;
  uint8_t foregroundDamageLevel;

  MaterialHue backgroundHueShift;
  MaterialHue backgroundModHueShift;
  MaterialColorVariant backgroundColorVariant;
  TileDamageType backgroundDamageType;
  uint8_t backgroundDamageLevel;

  LiquidId liquidId;
  uint8_t liquidLevel;

  template <typename Hasher>
  void hashPushTerrain(Hasher& hasher) const;

  template <typename Hasher>
  void hashPushLiquid(Hasher& hasher) const;
};
DataStream& operator>>(DataStream& ds, RenderTile& tile);
DataStream& operator<<(DataStream& ds, RenderTile const& tile);

typedef MultiArray<RenderTile, 2> RenderTileArray;

inline WorldTile::WorldTile()
  : foreground(NullMaterialId),
    foregroundHueShift(),
    foregroundMod(NoModId),
    foregroundModHueShift(),
    foregroundColorVariant(DefaultMaterialColorVariant),
    background(NullMaterialId),
    backgroundHueShift(),
    backgroundMod(NoModId),
    backgroundModHueShift(),
    backgroundColorVariant(DefaultMaterialColorVariant),
    collision(CollisionKind::Null),
    collisionCacheDirty(true),
    blockBiomeIndex(),
    environmentBiomeIndex(),
    dungeonId(NoDungeonId) {}

inline WorldTile::WorldTile(WorldTile const& worldTile) {
  *this = worldTile;
}

inline WorldTile& WorldTile::operator=(WorldTile const& worldTile) {
  foreground = worldTile.foreground;
  foregroundHueShift = worldTile.foregroundHueShift;
  foregroundMod = worldTile.foregroundMod;
  foregroundModHueShift = worldTile.foregroundModHueShift;
  foregroundColorVariant = worldTile.foregroundColorVariant;

  background = worldTile.background;
  backgroundHueShift = worldTile.backgroundHueShift;
  backgroundMod = worldTile.backgroundMod;
  backgroundModHueShift = worldTile.backgroundModHueShift;
  backgroundColorVariant = worldTile.backgroundColorVariant;

  // Don't bother copying collision cache
  collisionCacheDirty = true;

  collision = worldTile.collision;
  blockBiomeIndex = worldTile.blockBiomeIndex;
  environmentBiomeIndex = worldTile.environmentBiomeIndex;

  foregroundDamage = worldTile.foregroundDamage;
  backgroundDamage = worldTile.backgroundDamage;

  dungeonId = worldTile.dungeonId;

  return *this;
}

inline MaterialId WorldTile::material(TileLayer layer) const {
  if (layer == TileLayer::Foreground)
    return foreground;
  else
    return background;
}

inline ModId WorldTile::mod(TileLayer layer) const {
  if (layer == TileLayer::Foreground)
    return foregroundMod;
  else
    return backgroundMod;
}

inline MaterialColorVariant WorldTile::materialColor(TileLayer layer) const {
  if (layer == TileLayer::Foreground)
    return foregroundColorVariant;
  else
    return backgroundColorVariant;
}

inline tuple<MaterialId, MaterialHue, MaterialColorVariant> WorldTile::materialAndColor(TileLayer layer) const {
  if (layer == TileLayer::Foreground)
    return std::tuple<MaterialId, MaterialHue, MaterialColorVariant>{
        foreground, foregroundHueShift, foregroundColorVariant};
  else
    return std::tuple<MaterialId, MaterialHue, MaterialColorVariant>{
        background, backgroundHueShift, backgroundColorVariant};
}

inline ClientTile::ClientTile() : backgroundLightTransparent(true), foregroundLightTransparent(true), gravity() {}

inline ClientTile::ClientTile(ClientTile const& clientTile) : WorldTile() {
  *this = clientTile;
}

inline ClientTile& ClientTile::operator=(ClientTile const& clientTile) {
  WorldTile::operator=(clientTile);

  backgroundLightTransparent = clientTile.backgroundLightTransparent;
  foregroundLightTransparent = clientTile.foregroundLightTransparent;
  liquid = clientTile.liquid;
  gravity = clientTile.gravity;

  return *this;
}

inline NetTile::NetTile()
  : background(NullMaterialId),
    backgroundHueShift(),
    backgroundColorVariant(),
    backgroundMod(NoModId),
    backgroundModHueShift(),
    foreground(NullMaterialId),
    foregroundHueShift(),
    foregroundColorVariant(),
    foregroundMod(NoModId),
    foregroundModHueShift(),
    collision(),
    blockBiomeIndex(),
    environmentBiomeIndex(),
    dungeonId(NoDungeonId) {}

template <typename Hasher>
inline void RenderTile::hashPushTerrain(Hasher& hasher) const {
  // Do the fast path hash if the last (terrain relevant) field is at byte 20, because that means
  // there are no padding bytes between any field and we can simply pass the
  // entire tile as one block of memory.
  static size_t const TerrainEndOffset = offsetof(RenderTile, liquidId);
  static size_t const TotalTerrainSize =
    sizeof(MaterialId) * 2 + sizeof(ModId) * 2 + sizeof(MaterialHue) * 4 + sizeof(MaterialColorVariant) * 2 * sizeof(TileDamageType) * 2 + 2;

#ifdef STAR_DEBUG
  static bool const FastHash = false;
#else
  static bool const FastHash = TerrainEndOffset == TotalTerrainSize;
#endif

  if (FastHash) {
    hasher.push((char const*)this, TerrainEndOffset);
  } else {
    char buffer[TotalTerrainSize];
    size_t bufferSize = 0;

    auto hashTilePart = [&](void const* data, size_t size) {
      memcpy(buffer + bufferSize, data, size);
      bufferSize += size;
    };

    hashTilePart(&foreground, sizeof(foreground));
    hashTilePart(&foregroundMod, sizeof(foregroundMod));

    hashTilePart(&background, sizeof(background));
    hashTilePart(&backgroundMod, sizeof(backgroundMod));

    hashTilePart(&foregroundHueShift, sizeof(foregroundHueShift));
    hashTilePart(&foregroundModHueShift, sizeof(foregroundModHueShift));
    hashTilePart(&foregroundColorVariant, sizeof(foregroundColorVariant));
    hashTilePart(&foregroundDamageType, sizeof(foregroundDamageType));
    hashTilePart(&foregroundDamageLevel, sizeof(foregroundDamageLevel));

    hashTilePart(&backgroundHueShift, sizeof(backgroundHueShift));
    hashTilePart(&backgroundModHueShift, sizeof(backgroundModHueShift));
    hashTilePart(&backgroundColorVariant, sizeof(backgroundColorVariant));
    hashTilePart(&backgroundDamageType, sizeof(backgroundDamageType));
    hashTilePart(&backgroundDamageLevel, sizeof(backgroundDamageLevel));

    hasher.push(buffer, TotalTerrainSize);
  }
}

template <typename Hasher>
inline void RenderTile::hashPushLiquid(Hasher& hasher) const {
  char buffer[sizeof(liquidLevel) + sizeof(liquidId)];

  memcpy(buffer, &liquidLevel, sizeof(liquidLevel));
  memcpy(buffer + sizeof(liquidLevel), &liquidId, sizeof(liquidId));

  hasher.push(buffer, sizeof(liquidLevel) + sizeof(liquidId));
}

}
