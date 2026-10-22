#include "StarAssets.hpp"
#include "StarFile.hpp"
#include "StarJson.hpp"

#include "gtest/gtest.h"

using namespace Star;

namespace {

struct TempAssetRoot {
  String path;

  TempAssetRoot() : path(File::temporaryDirectory()) {}

  ~TempAssetRoot() {
    try {
      File::removeDirectoryRecursive(path);
    } catch (...) {
    }
  }

  String makeSource(String const& name) {
    String source = File::relativeTo(path, name);
    File::makeDirectoryRecursive(source);
    return source;
  }

  void write(String const& source, String const& file, String const& contents) {
    File::writeFile(contents, File::relativeTo(source, file));
  }
};

Assets::Settings testSettings() {
  Assets::Settings settings;
  settings.assetTimeToLive = 60.0f;
  settings.audioDecompressLimit = 0.0f;
  settings.workerPoolSize = 1;
  return settings;
}

}

TEST(AssetSourceTest, ProtectedAssetsIgnoreModOverridesAndPatches) {
  TempAssetRoot temp;

  auto baseSource = temp.makeSource("base");
  auto modSource = temp.makeSource("mod");
  temp.write(baseSource, "_metadata",
      "{\"name\":\"base\",\"scripts\":{\"postLoad\":[\"/postload.lua\"]}}");
  temp.write(modSource, "_metadata",
      "{\"name\":\"mod\",\"scripts\":{\"postLoad\":[\"/postload.lua\"]}}");

  temp.write(baseSource, "preload.config", "[]");
  temp.write(baseSource, "protected.config", "{\"value\":1}");
  temp.write(baseSource, "postload.lua",
      "assets.add(\"/base.patch\", '[{\"op\":\"add\",\"path\":\"/fromBase\",\"value\":true}]')\n"
      "assets.patch(\"/protected.config\", \"/base.patch\")\n");

  temp.write(modSource, "protected.config", "{\"value\":2}");
  temp.write(modSource, "protected.config.patch", "[{\"op\":\"replace\",\"path\":\"/value\",\"value\":3}]");
  File::makeDirectoryRecursive(File::relativeTo(modSource, "protected"));
  temp.write(modSource, "protected/new.config", "{\"new\":true}");
  temp.write(modSource, "postload.lua",
      "assets.add(\"/mod.patch\", '[{\"op\":\"add\",\"path\":\"/fromMod\",\"value\":true}]')\n"
      "assets.patch(\"/protected.config\", \"/mod.patch\")\n");

  auto settings = testSettings();
  settings.protectedAssetPaths = StringList{"/protected.config", "/protected/"};

  Assets assets(settings, {baseSource, modSource});

  auto result = assets.json("/protected.config");
  EXPECT_EQ(result.get("value").toInt(), 1);
  EXPECT_TRUE(result.get("fromBase").toBool());
  EXPECT_FALSE(result.contains("fromMod"));

  EXPECT_TRUE(assets.json("/protected/new.config").get("new").toBool());
}
