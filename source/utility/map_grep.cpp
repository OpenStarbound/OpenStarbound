#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarRootLoader.hpp"
#include "StarDungeonTMXPart.hpp"

using namespace Star;
using namespace Star::Dungeon;

typedef String TileName;
typedef pair<String, String> TileProperty;

typedef MVariant<TileName, TileProperty> MatchCriteria;

struct SearchParameters {
  MatchCriteria criteria;
};

typedef function<void(String, Vec2I)> MatchReporter;

String const MapFilenameSuffix = ".json";

Maybe<String> matchTile(SearchParameters const& search, Tiled::Tile const& tile) {
  Tiled::Properties const& properties = tile.properties;
  if (search.criteria.is<TileName>()) {
    if (auto tileName = properties.opt<String>("//name"))
      if (tileName->regexMatch(search.criteria.get<TileName>()))
        return tileName;
  } else {
    String propertyName;
    String matchValue;
    tie(propertyName, matchValue) = search.criteria.get<TileProperty>();
    if (auto propertyValue = properties.opt<String>(propertyName))
      if (propertyValue->regexMatch(matchValue))
        return properties.opt<String>("//name").value("?");
  }
  return {};
}

void grepTileLayer(SearchParameters const& search, TMXMapPtr map, TMXTileLayerPtr tileLayer, MatchReporter callback) {
  tileLayer->forEachTile(map.get(),
      [&](Vec2I pos, Tile const& tile) {
        if (auto tileName = matchTile(search, static_cast<Tiled::Tile const&>(tile)))
          callback(*tileName, pos);
        return false;
      });
}

void grepObjectGroup(SearchParameters const& search, TMXObjectGroupPtr objectGroup, MatchReporter callback) {
  for (auto object : objectGroup->objects()) {
    if (auto tileName = matchTile(search, object->tile()))
      callback(*tileName, object->pos());
  }
}

void grepMap(SearchParameters const& search, String file) {
  auto map = make_shared<TMXMap>(Json::parseJson(File::readFileString(file)));

  for (auto tileLayer : map->tileLayers())
    grepTileLayer(search, map, tileLayer, [&](String const& tileName, Vec2I const& pos) {
        coutf("%s: %s: %s @ %s\n", file, tileLayer->name(), tileName, pos);
      });

  for (auto objectGroup : map->objectGroups())
    grepObjectGroup(search, objectGroup, [&](String const& tileName, Vec2I const& pos) {
        coutf("%s: %s: %s @ %s\n", file, objectGroup->name(), tileName, pos);
      });
}

void grepDirectory(SearchParameters const& search, String directory) {
  for (pair<String, bool> entry : File::dirList(directory)) {
    if (entry.second)
      grepDirectory(search, File::relativeTo(directory, entry.first));
    else if (entry.first.endsWith(MapFilenameSuffix))
      grepMap(search, File::relativeTo(directory, entry.first));
  }
}

void grepPath(SearchParameters const& search, String path) {
  if (File::isFile(path)) {
    grepMap(search, path);
  } else if (File::isDirectory(path)) {
    grepDirectory(search, path);
  }
}

MatchCriteria parseMatchCriteria(String const& criteriaStr) {
  if (criteriaStr.contains("=")) {
    StringList parts = criteriaStr.split('=', 1);
    return make_pair(parts[0], parts[1]);
  }
  return TileName(criteriaStr);
}

int main(int argc, char* argv[]) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Warn, false, {}});
    rootLoader.setSummary("Search Tiled map files for specific materials or objects.");
    rootLoader.addArgument("MaterialId|ObjectName|Property=Value", OptionParser::Required);
    rootLoader.addArgument("JsonMapFile", OptionParser::Multiple);

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    SearchParameters search = {parseMatchCriteria(options.arguments[0])};
    StringList files = options.arguments.slice(1);

    for (auto file : files)
      grepPath(search, file);

    return 0;
  } catch (std::exception const& e) {
    cerrf("exception caught: %s\n", outputException(e, true));
    return 1;
  }
}
