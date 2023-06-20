#include "StarImage.hpp"
#include "StarTilesetDatabase.hpp"

namespace Star {

STAR_STRUCT(Tile);
STAR_CLASS(TileDatabase);
STAR_CLASS(Tileset);

struct Tile {
  String source, database, tileset, name;
  ImageConstPtr image;
  Tiled::Properties properties;
};

class TileDatabase {
public:
  TileDatabase(String const& name);

  void defineTile(TilePtr const& tile);
  TilePtr getTile(String const& tileName) const;
  String name() const;
  StringSet tileNames() const;

private:
  Map<String, TilePtr> m_tiles;
  String m_name;
};

class Tileset {
public:
  Tileset(String const& source, String const& name, TileDatabasePtr const& database);

  void defineTile(TilePtr const& tile);
  void exportTileset() const;

  String name() const;
  TileDatabasePtr database() const;

private:
  String imageDirName(String const& baseExportDir) const;
  String relativePathBase() const;
  Json imageFileReference(String const& fileName) const;
  Json tileImageReference(String const& tileName, String const& database) const;

  // Exports an image for each tile into its own file. Tiles can represent
  // objects with all different sizes, so we use Tiled's "collection of images"
  // tileset feature, which puts each image in its own file.
  void exportTilesetImages(String const& exportDir) const;

  // Read the tileset from the given path, or create a new tileset root
  // structure if it doesn't already exist.
  Json getTilesetJson(String const& tilesetPath) const;

  // Determine which tiles already exist in the tileset, returning a map
  // which contains the id of each named tile, and the next available Id after
  // the highest Id seen in the tileset.
  pair<StringMap<size_t>, size_t> indexExistingTiles(Json tileset) const;

  // Update existing and insert new tile definitions in the tileProperties and
  // tileImages objects.
  StringSet updateTiles(JsonObject& tileProperties,
      JsonObject& tileImages,
      StringMap<size_t> const& existingTiles,
      size_t& nextId,
      String const& tilesetPath) const;

  // Mark the given tiles as 'invalid' so they can't be used. (Actually removing
  // them from the tileset would cause the tile indices to change and break
  // existing maps.)
  void invalidateTiles(StringSet const& invalidTiles,
      StringMap<size_t> const& existingTiles,
      JsonObject& tileProperties,
      JsonObject& tileImages,
      String const& tilesetPath) const;

  String m_source, m_name;
  List<TilePtr> m_tiles;
  TileDatabasePtr m_database;
};

class TilesetUpdater {
public:
  void defineAssetSource(String const& source);
  void defineTile(TilePtr const& tile);
  void exportTilesets();

private:
  TileDatabasePtr const& getDatabase(TilePtr const& tile);
  TilesetPtr const& getTileset(TilePtr const& tile);

  // Asset Source -> Tileset Name -> Tileset
  StringMap<StringMap<TilesetPtr>> m_tilesets;
  // Asset Source -> Database Name -> Database
  StringMap<StringMap<TileDatabasePtr>> m_databases;

  // Images that existed before running update_tilesets:
  // Asset Source -> Database Name -> Tile Name
  StringMap<StringMap<StringSet>> m_preexistingImages;
};

}
