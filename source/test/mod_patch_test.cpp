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

  void writeMetadata(String const& source, String const& name) {
    write(source, "_metadata", strf("{{\"name\":\"{}\"}}", name));
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

TEST(ModPatchTest, DefersPatchesThatLoadBeforeTheirTarget) {
  TempAssetRoot temp;

  auto patchSource = temp.makeSource("patch_only");
  auto baseSource = temp.makeSource("base");
  temp.writeMetadata(patchSource, "patch_only");
  temp.writeMetadata(baseSource, "base");
  temp.write(patchSource, "test.config.patch", "[{\"op\":\"add\",\"path\":\"/patched\",\"value\":true}]");
  temp.write(baseSource, "test.config", "{\"base\":true}");
  temp.write(baseSource, "preload.config", "[]");

  Assets assets(testSettings(), {patchSource, baseSource});
  auto result = assets.json("/test.config");
  EXPECT_TRUE(result.get("base").toBool());
  EXPECT_TRUE(result.get("patched").toBool());
}

TEST(ModPatchTest, DeferredPatchesKeepLoadOrder) {
  TempAssetRoot temp;

  auto firstPatch = temp.makeSource("first_patch");
  auto secondPatch = temp.makeSource("second_patch");
  auto baseSource = temp.makeSource("base");
  temp.writeMetadata(firstPatch, "first_patch");
  temp.writeMetadata(secondPatch, "second_patch");
  temp.writeMetadata(baseSource, "base");
  temp.write(firstPatch, "test.config.patch", "[{\"op\":\"add\",\"path\":\"/value\",\"value\":\"first\"}]");
  temp.write(secondPatch, "test.config.patch", "[{\"op\":\"add\",\"path\":\"/value\",\"value\":\"second\"}]");
  temp.write(baseSource, "test.config", "{}");
  temp.write(baseSource, "preload.config", "[]");

  Assets assets(testSettings(), {firstPatch, secondPatch, baseSource});
  EXPECT_EQ(assets.json("/test.config").get("value").toString(), "second");
}
