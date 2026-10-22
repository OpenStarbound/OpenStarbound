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

TEST(ModPatchTest, MergesOverrideWhenPatchFails) {
  TempAssetRoot temp;

  auto baseSource = temp.makeSource("base");
  auto modSource = temp.makeSource("mod");
  auto patchSource = temp.makeSource("patch");
  temp.writeMetadata(baseSource, "base");
  temp.writeMetadata(modSource, "mod");
  temp.writeMetadata(patchSource, "patch");

  temp.write(baseSource, "test.config", "{\"a\":1,\"b\":{\"c\":2}}");
  temp.write(baseSource, "preload.config", "[]");
  temp.write(modSource, "test.config", "{\"a\":10,\"d\":4}");
  temp.write(patchSource, "test.config.patch", "[{\"op\":\"replace\",\"path\":\"/b/c\",\"value\":3}]");

  auto settings = testSettings();
  settings.autoMergePatchedOverrides = true;

  Assets assets(settings, {baseSource, modSource, patchSource});
  auto result = assets.json("/test.config");
  EXPECT_EQ(result.get("a").toInt(), 10);
  EXPECT_EQ(result.get("b").get("c").toInt(), 3);
  EXPECT_EQ(result.get("d").toInt(), 4);
}

TEST(ModPatchTest, OwnPostLoadPatchDoesNotMergeLowerPriorityBase) {
  TempAssetRoot temp;

  auto baseSource = temp.makeSource("base");
  auto modSource = temp.makeSource("mod");
  temp.writeMetadata(baseSource, "base");
  temp.write(modSource, "_metadata",
      "{\"name\":\"mymod\",\"scripts\":{\"postLoad\":[\"/postload.lua\"]}}");

  temp.write(baseSource, "test.config", "{\"a\":1,\"b\":{\"c\":2}}");
  temp.write(baseSource, "preload.config", "[]");
  temp.write(modSource, "test.config", "{\"a\":10}");
  temp.write(modSource, "postload.lua",
      "assets.add(\"/own.patch\", '[{\"op\":\"test\",\"path\":\"/b/c\",\"value\":2},"
      "{\"op\":\"replace\",\"path\":\"/a\",\"value\":20}]')\n"
      "assets.patch(\"/test.config\", \"/own.patch\")\n");

  auto settings = testSettings();
  settings.autoMergePatchedOverrides = true;

  Assets assets(settings, {baseSource, modSource});
  auto result = assets.json("/test.config");

  // The patch belongs to the same mod that provided the override, so the
  // lower priority base must not be merged back in.
  EXPECT_EQ(result.get("a").toInt(), 10);
  EXPECT_FALSE(result.contains("b"));
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

TEST(ModPatchTest, DoesNotMergeWhenDisabled) {
  TempAssetRoot temp;

  auto baseSource = temp.makeSource("base");
  auto modSource = temp.makeSource("mod");
  auto patchSource = temp.makeSource("patch");
  temp.writeMetadata(baseSource, "base");
  temp.writeMetadata(modSource, "mod");
  temp.writeMetadata(patchSource, "patch");

  temp.write(baseSource, "test.config", "{\"a\":1,\"b\":{\"c\":2}}");
  temp.write(baseSource, "preload.config", "[]");
  temp.write(modSource, "test.config", "{\"a\":10}");
  temp.write(patchSource, "test.config.patch", "[{\"op\":\"test\",\"path\":\"/b/c\",\"value\":2}]");

  auto settings = testSettings();
  settings.autoMergePatchedOverrides = false;

  Assets assets(settings, {baseSource, modSource, patchSource});
  auto result = assets.json("/test.config");
  EXPECT_EQ(result.get("a").toInt(), 10);
  EXPECT_FALSE(result.contains("b"));
}

TEST(ModPatchTest, RetryRunsLuaPatchesFromCleanState) {
  TempAssetRoot temp;

  auto baseSource = temp.makeSource("base");
  auto modSource = temp.makeSource("mod");
  auto luaPatchSource = temp.makeSource("lua_patch");
  auto jsonPatchSource = temp.makeSource("json_patch");
  temp.writeMetadata(baseSource, "base");
  temp.writeMetadata(modSource, "mod");
  temp.writeMetadata(luaPatchSource, "lua_patch");
  temp.writeMetadata(jsonPatchSource, "json_patch");

  temp.write(baseSource, "test.config", "{\"a\":1,\"b\":2,\"keepB\":true}");
  temp.write(baseSource, "preload.config", "[]");
  temp.write(modSource, "test.config", "{\"a\":10,\"b\":2}");
  temp.write(luaPatchSource, "test.config.patch.lua",
      "function patch(config)\n"
      "  counter = (counter or 0) + 1\n"
      "  config.count = counter\n"
      "  if not config.keepB then\n"
      "    config.b = nil\n"
      "  end\n"
      "  return config\n"
      "end\n");
  temp.write(jsonPatchSource, "test.config.patch",
      "[{\"op\":\"test\",\"path\":\"/b\",\"value\":2},{\"op\":\"replace\",\"path\":\"/b\",\"value\":3}]");

  auto settings = testSettings();
  settings.autoMergePatchedOverrides = true;

  Assets assets(settings, {baseSource, modSource, luaPatchSource, jsonPatchSource});
  auto result = assets.json("/test.config");

  EXPECT_EQ(result.get("a").toInt(), 10);
  EXPECT_EQ(result.get("b").toInt(), 3);
  EXPECT_TRUE(result.get("keepB").toBool());
  EXPECT_EQ(result.get("count").toInt(), 1);
}
