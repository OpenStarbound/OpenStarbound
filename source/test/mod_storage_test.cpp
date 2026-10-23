#include "StarModStorage.hpp"
#include "StarFile.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ModStorageTest, SetGetRemove) {
  auto dir = File::temporaryDirectory();
  {
    ModStorage storage(File::relativeTo(dir, "modstorage"));

    EXPECT_TRUE(storage.get("mod", "a").isNull());
    storage.set("mod", "a", 1);
    storage.set("mod", "b", JsonObject{{"x", JsonArray{1, 2}}});
    EXPECT_EQ(storage.get("mod", "a"), Json(1));
    EXPECT_EQ(storage.get("mod", "b").query("x[1]"), Json(2));
    EXPECT_EQ(storage.keys("mod").size(), 2u);

    storage.set("mod", "a", Json());
    EXPECT_TRUE(storage.get("mod", "a").isNull());
    EXPECT_EQ(storage.keys("mod").size(), 1u);

    // Mods never see each other's keys.
    EXPECT_TRUE(storage.get("other", "b").isNull());
    EXPECT_TRUE(storage.keys("other").empty());

    storage.clear("mod");
    EXPECT_TRUE(storage.keys("mod").empty());
  }
  File::removeDirectoryRecursive(dir);
}

TEST(ModStorageTest, SizeLimit) {
  auto dir = File::temporaryDirectory();
  {
    ModStorage storage(File::relativeTo(dir, "modstorage"));

    String half('x', ModStorage::MaxModDataSize / 2 - 100);
    storage.set("mod", "a", half);
    storage.set("mod", "b", half);
    // A third value does not fit, and the failed set changes nothing.
    EXPECT_THROW(storage.set("mod", "c", half), JsonException);
    EXPECT_TRUE(storage.get("mod", "c").isNull());
    EXPECT_EQ(storage.keys("mod").size(), 2u);

    // Replacing a value counts the old size out, removing frees room.
    storage.set("mod", "a", "small");
    storage.set("mod", "c", String('y', ModStorage::MaxModDataSize / 4));
    storage.set("mod", "b", Json());
    storage.set("mod", "d", half);

    // The limit is per mod.
    storage.set("other", "a", half);
  }
  File::removeDirectoryRecursive(dir);
}

TEST(ModStorageTest, Persistence) {
  auto dir = File::temporaryDirectory();
  String path = File::relativeTo(dir, "modstorage");
  {
    ModStorage storage(path);
    storage.set("mod", "a", "kept");
    storage.set("mod", "b", 2);
    storage.flush();
    EXPECT_TRUE(File::isFile(File::relativeTo(path, "mod.json")));

    // Not flushed by hand, only by the destructor.
    storage.set("mod", "c", 3);
  }
  {
    ModStorage storage(path);
    EXPECT_EQ(storage.get("mod", "a"), Json("kept"));
    EXPECT_EQ(storage.get("mod", "b"), Json(2));
    EXPECT_EQ(storage.get("mod", "c"), Json(3));

    // The size of what was loaded counts towards the limit.
    storage.set("mod", "big", String('x', ModStorage::MaxModDataSize - 100));
    EXPECT_THROW(storage.set("mod", "more", String('x', 200)), JsonException);
  }
  File::removeDirectoryRecursive(dir);
}

TEST(ModStorageTest, DelayedWrite) {
  auto dir = File::temporaryDirectory();
  String path = File::relativeTo(dir, "modstorage");
  {
    ModStorage storage(path);
    storage.set("mod", "a", 1);
    // Nothing is written right away, a set is only a change in memory.
    EXPECT_FALSE(File::isFile(File::relativeTo(path, "mod.json")));

    // The background thread writes it once the delay has passed.
    Thread::sleep(ModStorage::WriteDelayMillis + 1000);
    EXPECT_TRUE(File::isFile(File::relativeTo(path, "mod.json")));
  }
  File::removeDirectoryRecursive(dir);
}

TEST(ModStorageTest, FileNames) {
  auto dir = File::temporaryDirectory();
  String path = File::relativeTo(dir, "modstorage");
  {
    ModStorage storage(path);
    storage.set("a b", "k", 1);
    storage.set("a_b", "k", 2);
    storage.set("../escape", "k", 3);
    storage.flush();

    // Ids that differ only in characters that are not safe in a file name
    // keep separate files, and none of them leaves the directory.
    EXPECT_EQ(storage.get("a b", "k"), Json(1));
    EXPECT_EQ(storage.get("a_b", "k"), Json(2));
    EXPECT_EQ(storage.get("../escape", "k"), Json(3));
    EXPECT_EQ(File::dirList(path).size(), 3u);
    EXPECT_FALSE(File::isFile(File::relativeTo(dir, "escape.json")));
  }
  File::removeDirectoryRecursive(dir);
}
