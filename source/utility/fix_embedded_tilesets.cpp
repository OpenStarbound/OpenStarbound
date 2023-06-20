#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarRootLoader.hpp"
#include "StarTilesetDatabase.hpp"

using namespace Star;

void removeCommonPrefix(StringList& a, StringList& b) {
  // Remove elements from a and b until there is one that differs.
  while (a.size() > 0 && b.size() > 0 && a[0] == b[0]) {
    a.eraseAt(0);
    b.eraseAt(0);
  }
}

String createRelativePath(String fromFile, String toFile) {
  if (!File::isDirectory(fromFile))
    fromFile = File::dirName(fromFile);
  fromFile = File::fullPath(fromFile);
  toFile = File::fullPath(toFile);

  StringList fromParts = fromFile.splitAny("/\\");
  StringList toParts = toFile.splitAny("/\\");
  removeCommonPrefix(fromParts, toParts);

  StringList relativeParts;
  for (String part : fromParts)
    relativeParts.append("..");
  relativeParts.appendAll(toParts);

  return relativeParts.join("/");
}

Maybe<Json> repairTileset(Json tileset, String const& mapPath, String const& tilesetPath) {
  if (tileset.contains("source"))
    return {};
  size_t firstGid = tileset.getUInt("firstgid");
  String tilesetName = tileset.getString("name");
  String tilesetFileName = File::relativeTo(tilesetPath, tilesetName + ".json");
  if (!File::exists(tilesetFileName))
    throw StarException::format("Tileset %s does not exist. Can't repair %s", tilesetFileName, mapPath);
  return {JsonObject{{"firstgid", firstGid}, {"source", createRelativePath(mapPath, tilesetFileName)}}};
}

Maybe<Json> repair(Json mapJson, String const& mapPath, String const& tilesetPath) {
  JsonArray tilesets = mapJson.getArray("tilesets");
  bool changed = false;
  for (size_t i = 0; i < tilesets.size(); ++i) {
    if (Maybe<Json> tileset = repairTileset(tilesets[i], mapPath, tilesetPath)) {
      tilesets[i] = *tileset;
      changed = true;
    }
  }
  if (!changed)
    return {};
  return mapJson.set("tilesets", tilesets);
}

void forEachRecursiveFileMatch(String const& dirName, String const& filenameSuffix, function<void(String)> func) {
  for (pair<String, bool> entry : File::dirList(dirName)) {
    if (entry.second)
      forEachRecursiveFileMatch(File::relativeTo(dirName, entry.first), filenameSuffix, func);
    else if (entry.first.endsWith(filenameSuffix))
      func(File::relativeTo(dirName, entry.first));
  }
}

void fixEmbeddedTilesets(String const& searchRoot, String const& tilesetPath) {
  forEachRecursiveFileMatch(searchRoot, ".json", [tilesetPath](String const& path) {
      Json json = Json::parseJson(File::readFileString(path));
      if (json.contains("tilesets")) {
        if (Maybe<Json> fixed = repair(json, path, tilesetPath)) {
          File::writeFile(fixed->repr(2, true), path);
          Logger::info("Repaired %s", path);
        }
      }
    });
}

int main(int argc, char* argv[]) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Info, false, {}});
    rootLoader.setSummary("Replaces embedded tilesets in Tiled JSON files with references to external tilesets. Assumes tilesets are available in the packed assets.");
    rootLoader.addArgument("searchRoot", OptionParser::Required);
    rootLoader.addArgument("tilesetsPath", OptionParser::Required);

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    String searchRoot = options.arguments[0];
    String tilesetPath = options.arguments[1];

    fixEmbeddedTilesets(searchRoot, tilesetPath);

    return 0;
  } catch (std::exception const& e) {
    cerrf("exception caught: %s\n", outputException(e, true));
    return 1;
  }
}
