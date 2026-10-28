#include "StarSpatialHash2D.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(SpatialHash2DTest, EntrySpanningSectorsIsReturnedOnce) {
  SpatialHash2D<int, float, int> hash(16.0f);

  // Covers 3x3 sectors, so a query overlapping several of them would return the
  // entry multiple times without deduplication.
  hash.set(1, RectF::withSize({0.0f, 0.0f}, {48.0f, 48.0f}), 100);

  auto values = hash.queryValues(RectF::withSize({-8.0f, -8.0f}, {64.0f, 64.0f}));
  ASSERT_EQ(values.size(), 1u);
  EXPECT_EQ(values[0], 100);
}

TEST(SpatialHash2DTest, QueryReturnsOnlyIntersectingEntries) {
  SpatialHash2D<int, float, int> hash(16.0f);

  hash.set(1, RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}), 100);
  hash.set(2, RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}), 200);
  hash.set(3, RectF::withSize({0.0f, 100.0f}, {8.0f, 8.0f}), 300);

  auto values = hash.queryValues(RectF::withSize({-16.0f, -16.0f}, {32.0f, 32.0f}));
  ASSERT_EQ(values.size(), 1u);
  EXPECT_EQ(values[0], 100);

  EXPECT_TRUE(hash.queryValues(RectF::withSize({500.0f, 500.0f}, {16.0f, 16.0f})).empty());
}

TEST(SpatialHash2DTest, SetUpdatesEntryWithoutGrowing) {
  SpatialHash2D<int, float, int> hash(16.0f);

  hash.set(1, RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}), 100);
  ASSERT_EQ(hash.size(), 1u);

  hash.set(1, RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}), 200);

  EXPECT_EQ(hash.size(), 1u);
  EXPECT_EQ(hash.value(1), 200);

  // The old position must no longer return the entry.
  EXPECT_TRUE(hash.queryValues(RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f})).empty());
  auto values = hash.queryValues(RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}));
  ASSERT_EQ(values.size(), 1u);
  EXPECT_EQ(values[0], 200);
}

TEST(SpatialHash2DTest, RemoveClearsSpatialIndex) {
  SpatialHash2D<int, float, int> hash(16.0f);

  hash.set(1, RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}), 100);
  hash.set(2, RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}), 200);

  auto removed = hash.remove(1);
  ASSERT_TRUE(removed);
  EXPECT_EQ(*removed, 100);
  EXPECT_FALSE(hash.contains(1));
  EXPECT_TRUE(hash.queryValues(RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f})).empty());
  EXPECT_EQ(hash.size(), 1u);

  EXPECT_FALSE(hash.remove(1));
  EXPECT_EQ(hash.size(), 1u);
}

TEST(SpatialHash2DTest, ForEachVisitsEachEntryOnce) {
  SpatialHash2D<int, float, int> hash(16.0f);

  for (int i = 0; i < 8; ++i)
    hash.set(i, RectF::withSize({float(i) * 32.0f, 0.0f}, {32.0f, 32.0f}), i * 10);

  List<int> visited;
  hash.forEach(RectF::withSize({-64.0f, -64.0f}, {512.0f, 128.0f}), [&visited](int const& value) {
      visited.append(value);
    });

  ASSERT_EQ(visited.size(), 8u);
  sort(visited);
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(visited[i], i * 10);
}

TEST(SpatialHash2DTest, SetWithoutValueInsertsNewEntry) {
  SpatialHash2D<int, float, int> hash(16.0f);

  hash.set(7, RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}));
  hash.set(8, Vec2F{40.0f, 40.0f});

  EXPECT_EQ(hash.size(), 2u);
  EXPECT_TRUE(hash.contains(7));
  EXPECT_TRUE(hash.contains(8));
  EXPECT_EQ(hash.value(7), 0);
  EXPECT_EQ(hash.value(8), 0);

  auto values = hash.queryValues(RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}));
  ASSERT_EQ(values.size(), 1u);
  EXPECT_EQ(values[0], 0);
}

TEST(SpatialHash2DTest, SetSectorSizeRebuildsSpatialIndex) {
  SpatialHash2D<int, float, int> hash(16.0f);

  hash.set(1, RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}), 100);
  hash.set(2, RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}), 200);

  hash.setSectorSize(64.0f);

  EXPECT_EQ(hash.size(), 2u);
  auto first = hash.queryValues(RectF::withSize({0.0f, 0.0f}, {8.0f, 8.0f}));
  ASSERT_EQ(first.size(), 1u);
  EXPECT_EQ(first[0], 100);
  auto second = hash.queryValues(RectF::withSize({100.0f, 100.0f}, {8.0f, 8.0f}));
  ASSERT_EQ(second.size(), 1u);
  EXPECT_EQ(second[0], 200);
}
