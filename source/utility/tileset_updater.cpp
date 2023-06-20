#include "StarLogging.hpp"
#include "tileset_updater.hpp"

using namespace Star;

String const InvalidTileImage = "../packed/invalid.png";
String const AssetsTilesetDirectory = "tilesets";
String const TileImagesDirectory = "../../../../tiled";
int const Indentation = 2;

String unixFileJoin(String const& dirname, String const& filename) {
  return (dirname.trimEnd("\\/") + '/' + filename.trimBeg("\\/")).replace("\\", "/");
}

TileDatabase::TileDatabase(String const& name) : m_name(name) {}

void TileDatabase::defineTile(TilePtr const& tile) {
  m_tiles[tile->name] = tile;
}

TilePtr TileDatabase::getTile(String const& tileName) const {
  return m_tiles.maybe(tileName).value({});
}

String TileDatabase::name() const {
  return m_name;
}

StringSet TileDatabase::tileNames() const {
  return StringSet::from(m_tiles.keys());
}

Tileset::Tileset(String const& source, String const& name, TileDatabasePtr const& database)
  : m_source(source), m_name(name), m_tiles(), m_database(database) {}

void Tileset::defineTile(TilePtr const& tile) {
  // Each tileset must be exported from a single database. When a tile switches
  // to another tileset (e.g. because an object has changed category), we allow
  // it to stay in the previous tileset to avoid breaking maps.
  // This means that if we exported a mix of, e.g. materials, liquids and
  // objects (which would cause the assertion failure below) it'd be harder to
  // check if a tile still exists in the database and should be exported
  // despite no longer belonging to the tileset.
  starAssert(m_source == tile->source);
  starAssert(m_database->name() == tile->database);

  m_tiles.append(tile);
}

Maybe<pair<String, String>> parseAssetSource(String const& source) {
  if (!File::isDirectory(source))
    return {};

  String sourcePath = source.trimEnd("/\\");
  String sourceName = sourcePath.splitAny("/\\").last();

  return make_pair(sourceName, sourcePath);
}

String tilesetExportDir(String const& sourcePath, String const& sourceName) {
  return StringList{sourcePath, AssetsTilesetDirectory, sourceName}.join("/");
}

void Tileset::exportTileset() const {
  auto parsedSource = parseAssetSource(m_source);
  if (!parsedSource)
    // Don't export tilesets into packed assets
    return;

  String sourceName, sourcePath;
  tie(sourceName, sourcePath) = *parsedSource;
  String exportDir = tilesetExportDir(sourcePath, sourceName);
  String tilesetPath = unixFileJoin(exportDir, m_name + ".json");
  File::makeDirectoryRecursive(File::dirName(tilesetPath));
  Logger::info("Updating tileset at %s", tilesetPath);

  exportTilesetImages(exportDir);

  Json root = getTilesetJson(tilesetPath);
  JsonObject tileImages = JsonObject{};
  JsonObject tileProperties = root.getObject("tileproperties", JsonObject{});

  // Scan the tiles already in the tileset
  StringMap<size_t> existingTiles;
  size_t nextId = 0;
  tie(existingTiles, nextId) = indexExistingTiles(root);

  // Add new tiles and update existing ones
  StringSet updatedTiles = updateTiles(tileProperties, tileImages, existingTiles, nextId, tilesetPath);

  // Mark all tiles that (a) already existed and (b) were not updated as invalid
  // as they are no longer in the assets database.
  StringSet invalidTiles = StringSet::from(existingTiles.keys()).difference(updatedTiles);
  invalidateTiles(invalidTiles, existingTiles, tileProperties, tileImages, tilesetPath);

  // We have some broken tile indices because of something strange happening
  // in the old .tsx files (manual editing? faulty merges?).
  // Cover up the holes so that Tiled doesn't barf on them.
  for (size_t id = 0; id < nextId; ++id) {
    String idKey = toString(id);
    if (!tileProperties.contains(idKey))
      tileProperties[idKey] = JsonObject{{"invalid", "true"}};
    if (!tileImages.contains(idKey))
      tileImages[idKey] = imageFileReference(InvalidTileImage);
  }

  root = root.set("tiles", tileImages).set("tileproperties", tileProperties);
  root = root.set("tilecount", nextId);
  File::writeFile(root.printJson(Indentation, true), tilesetPath);
}

String Tileset::name() const {
  return m_name;
}

TileDatabasePtr Tileset::database() const {
  return m_database;
}

String imageExportDirName(String const& baseExportDir, String const& assetSourceName) {
  String dir = unixFileJoin(baseExportDir, TileImagesDirectory);
  return unixFileJoin(dir, assetSourceName);
}

String Tileset::imageDirName(String const& baseExportDir) const {
  String sourceName = parseAssetSource(m_source)->first;
  return imageExportDirName(baseExportDir, sourceName);
}

String Tileset::relativePathBase() const {
  int subdirs = m_name.splitAny("\\/").size() - 1;
  String relativePathBase;
  if (subdirs == 0) {
    relativePathBase = ".";
  } else {
    StringList path;
    for (int i = 0; i < subdirs; ++i)
      path.append("..");
    relativePathBase = path.join("/");
  }
  return relativePathBase;
}

Json Tileset::imageFileReference(String const& fileName) const {
  String tileImagePath = unixFileJoin(imageDirName(relativePathBase()), fileName);
  return JsonObject{{"image", tileImagePath}};
}

Json Tileset::tileImageReference(String const& tileName, String const& database) const {
  String tileImageName = unixFileJoin(database, tileName + ".png");
  return imageFileReference(tileImageName);
}

void Tileset::exportTilesetImages(String const& exportDir) const {
  for (auto const& tile : m_tiles) {
    String imageDir = unixFileJoin(imageDirName(exportDir), tile->database);
    File::makeDirectoryRecursive(imageDir);
    String imageName = unixFileJoin(imageDir, tile->name + ".png");
    Logger::info("Updating image %s", imageName);
    tile->image->writePng(File::open(imageName, IOMode::Write));
  }
}

Json Tileset::getTilesetJson(String const& tilesetPath) const {
  if (File::exists(tilesetPath)) {
    return Json::parseJson(File::readFileString(tilesetPath));
  } else {
    Logger::warn(
        "Tileset %s wasn't already present. Creating it from scratch. Any maps already using this tileset may be "
        "broken.",
        tilesetPath);
    return JsonObject{{"margin", 0},
        {"name", m_name},
        {"properties", JsonObject{}},
        {"spacing", 0},
        {"tilecount", m_tiles.size()},
        {"tileheight", TilePixels},
        {"tilewidth", TilePixels},
        {"tiles", JsonObject{}},
        {"tileproperties", JsonObject{}}};
  }
}

pair<StringMap<size_t>, size_t> Tileset::indexExistingTiles(Json tileset) const {
  StringMap<size_t> existingTiles;
  size_t nextId = 0;
  for (auto const& entry : tileset.getObject("tileproperties")) {
    size_t id = lexicalCast<size_t>(entry.first);
    Tiled::Properties properties = entry.second;
    if (properties.contains("//name")) {
      existingTiles[properties.get<String>("//name")] = id;
      nextId = max(id + 1, nextId);
    }
  }
  return make_pair(existingTiles, nextId);
}

StringSet Tileset::updateTiles(JsonObject& tileProperties,
    JsonObject& tileImages,
    StringMap<size_t> const& existingTiles,
    size_t& nextId,
    String const& tilesetPath) const {
  StringSet updatedTiles;
  for (TilePtr const& tile : m_tiles) {
    Tiled::Properties properties = tile->properties;
    size_t id = 0;

    if (existingTiles.contains(tile->name)) {
      id = existingTiles.get(tile->name);
    } else {
      coutf("Adding '%s' to %s\n", tile->name, tilesetPath);
      id = nextId++;
    }

    tileProperties[toString(id)] = properties.toJson();
    tileImages[toString(id)] = tileImageReference(tile->name, tile->database);

    updatedTiles.add(tile->name);
  }
  return updatedTiles;
}

void Tileset::invalidateTiles(StringSet const& invalidTiles,
    StringMap<size_t> const& existingTiles,
    JsonObject& tileProperties,
    JsonObject& tileImages,
    String const& tilesetPath) const {
  for (String tileName : invalidTiles) {
    size_t id = existingTiles.get(tileName);

    if (TilePtr const& tile = m_database->getTile(tileName)) {
      // Tile has moved category, but we're leaving it in this tileset to avoid
      // breaking existing maps.
      tileProperties[toString(id)] = tile->properties.toJson();
      tileImages[toString(id)] = tileImageReference(tile->name, tile->database);
    } else {
      if (!tileProperties[toString(id)].contains("invalid"))
        coutf("Removing '%s' from %s\n", tileName, tilesetPath);
      tileProperties[toString(id)] = JsonObject{{"//name", tileName}, {"invalid", "true"}};
      tileImages[toString(id)] = imageFileReference(InvalidTileImage);
    }
  }
}

void TilesetUpdater::defineAssetSource(String const& source) {
  auto parsedSource = parseAssetSource(source);
  if (!parsedSource)
    // Don't change anything about images in packed assets
    return;

  String sourceName;
  String sourcePath;
  tie(sourceName, sourcePath) = *parsedSource;
  String tilesetDir = tilesetExportDir(sourcePath, sourceName);
  String imageDir = imageExportDirName(tilesetDir, sourceName);

  Logger::info("Scanning %s for images...", imageDir);
  if (!File::isDirectory(imageDir))
    return;

  for (pair<String, bool> entry : File::dirList(imageDir)) {
    if (entry.second) {
      String databaseName = entry.first;
      String databasePath = unixFileJoin(imageDir, databaseName);
      Logger::info("Scanning database %s...", databaseName);
      for (pair<String, bool> image : File::dirList(databasePath)) {
        starAssert(!image.second);
        starAssert(image.first.endsWith(".png"));
        String tileName = image.first.substr(0, image.first.findLast(".png"));
        m_preexistingImages[sourceName][databaseName].add(tileName);
      }
    }
  }
}

void TilesetUpdater::defineTile(TilePtr const& tile) {
  getDatabase(tile)->defineTile(tile);
  getTileset(tile)->defineTile(tile);
}

void TilesetUpdater::exportTilesets() {
  for (auto const& tilesets : m_tilesets) {
    auto parsedAssetSource = parseAssetSource(tilesets.first);
    if (!parsedAssetSource) {
      Logger::info("Not updating tilesets in %s because it is packed", tilesets.first);
      continue;
    }
    String sourceName;
    String sourcePath;
    tie(sourceName, sourcePath) = *parsedAssetSource;

    String tilesetDir = tilesetExportDir(sourcePath, sourceName);
    String imageDir = imageExportDirName(tilesetDir, sourceName);

    for (auto const& tileset : tilesets.second.values()) {
      tileset->exportTileset();
    }

    for (auto const& database : m_databases[tilesets.first].values()) {
      String databaseImagePath = unixFileJoin(imageDir, database->name());
      StringSet unusedImages = m_preexistingImages[sourceName][database->name()].difference(database->tileNames());
      for (String tileName : unusedImages) {
        String tileImagePath = unixFileJoin(databaseImagePath, tileName + ".png");
        starAssert(File::isFile(tileImagePath));
        coutf("Removing unused tile image tiled/%s/%s/%s.png\n", sourceName, database->name(), tileName);
        File::remove(tileImagePath);
      }
      m_preexistingImages[sourceName][database->name()] = database->tileNames();
    }
  }
}

TileDatabasePtr const& TilesetUpdater::getDatabase(TilePtr const& tile) {
  auto& databases = m_databases[tile->source];
  if (!databases.contains(tile->database))
    databases[tile->database] = make_shared<TileDatabase>(tile->database);
  return databases[tile->database];
}

TilesetPtr const& TilesetUpdater::getTileset(TilePtr const& tile) {
  TileDatabasePtr database = getDatabase(tile);
  auto& tilesets = m_tilesets[tile->source];
  if (!tilesets.contains(tile->tileset))
    tilesets[tile->tileset] = make_shared<Tileset>(tile->source, tile->tileset, database);
  return tilesets[tile->tileset];
}
