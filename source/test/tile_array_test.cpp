#include "StarTileSectorArray.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(TileSectorArrayTest, All) {
  typedef TileSectorArray<int, 32> TileArray;
  TileArray tileSectorArray({100, 100}, -1);

  EXPECT_TRUE(tileSectorArray.sectorValid(TileArray::Sector(1, 1)));
  EXPECT_TRUE(tileSectorArray.sectorValid(TileArray::Sector(3, 3)));
  EXPECT_FALSE(tileSectorArray.sectorValid(TileArray::Sector(4, 4)));

  EXPECT_TRUE(List<TileArray::Sector>({{0, 0}, {1, 0}}) == tileSectorArray.validSectorsFor(RectI(0, -32, 64, 32)));

  EXPECT_TRUE(TileArray::Sector(0, 0) == tileSectorArray.sectorFor({0, 0}));
  EXPECT_TRUE(TileArray::Sector(3, 1) == tileSectorArray.sectorFor({-1, 33}));
  EXPECT_TRUE(-1 == tileSectorArray.tile({-1, -1}));
  EXPECT_TRUE(nullptr == tileSectorArray.modifyTile({-1, -1}));
  EXPECT_TRUE(RectI(32, 32, 64, 64) == tileSectorArray.sectorRegion({1, 1}));
  EXPECT_TRUE(TileArray::Sector(0, 3) == tileSectorArray.adjacentSector({3, 3}, {1, 0}));

  tileSectorArray.loadSector({0, 0}, make_unique<TileArray::Array>(1));
  tileSectorArray.loadSector({1, 0}, make_unique<TileArray::Array>(1));
  tileSectorArray.loadSector({2, 0}, make_unique<TileArray::Array>(1));
  tileSectorArray.loadSector({3, 0}, make_unique<TileArray::Array>(1));
  tileSectorArray.loadSector({0, 1}, make_unique<TileArray::Array>(2));
  tileSectorArray.loadSector({1, 1}, make_unique<TileArray::Array>(2));
  tileSectorArray.loadSector({2, 1}, make_unique<TileArray::Array>(2));
  tileSectorArray.loadSector({3, 1}, make_unique<TileArray::Array>(2));

  Set<Vec2I> found;
  tileSectorArray.tileEach(RectI(-2, 0, 3, 1),
      [&found](Vec2I const& pos, int tile) {
        found.add(pos);
        EXPECT_TRUE(pos[0] >= -2 && pos[0] < 3);
        EXPECT_TRUE(pos[1] == 0);
        EXPECT_EQ(1, tile);
      });
  EXPECT_TRUE(found.contains(Vec2I(0, 0)));
  EXPECT_TRUE(found.contains(Vec2I(-1, 0)));
  EXPECT_TRUE(found.contains(Vec2I(-2, 0)));
  EXPECT_TRUE(found.contains(Vec2I(1, 0)));

  tileSectorArray.tileEach(RectI(-10, 0, -1, 1),
      [](Vec2I const& pos, int tile) {
        EXPECT_TRUE(pos[0] >= -10 && pos[0] < -1);
        EXPECT_EQ(1, tile);
      });

  tileSectorArray.tileEach(RectI(-10, -1, -1, 0),
      [](Vec2I const& pos, int tile) {
        EXPECT_TRUE(pos[0] >= -10 && pos[0] < -1);
        EXPECT_TRUE(pos[1] == -1);
        EXPECT_EQ(-1, tile);
      });

  found.clear();
  tileSectorArray.tileEach(RectI(110, 101, 120, 102),
      [&found](Vec2I const& pos, int tile) {
        found.add(pos);
        EXPECT_TRUE(pos[0] >= 110 && pos[0] < 120);
        EXPECT_TRUE(pos[1] == 101);
        EXPECT_EQ(-1, tile);
      });
  EXPECT_TRUE(found.contains(Vec2I(110, 101)));
  EXPECT_TRUE(found.contains(Vec2I(119, 101)));

  auto res1 = tileSectorArray.tileEachResult(RectI(110, 110, 120, 120),
      [](Vec2I const& pos, int tile) -> int {
        return (pos[0] >= 110 && pos[0] < 120 && pos[1] >= 110 && pos[1] < 120 && tile == -1) ? 1 : 0;
      });
  MultiArray<int, 2> res1comp({10, 10}, 1);

  EXPECT_TRUE(res1.size() == res1comp.size());
  res1.forEach([](Array2S const&, int elem) { EXPECT_TRUE(elem == 1); });

  auto res2 = tileSectorArray.tileEachResult(RectI(32, 32, 64, 64),
      [](Vec2I const& pos, int tile) -> int {
        return (pos[0] >= 32 && pos[0] < 64 && pos[1] >= 32 && pos[1] < 64 && tile == 2) ? 1 : 0;
      });
  MultiArray<int, 2> res2comp({32, 32}, 1);

  EXPECT_TRUE(res2.size() == res2comp.size());
  res2.forEach([](Array2S const&, int elem) { EXPECT_TRUE(elem == 1); });

  auto res3 = tileSectorArray.tileEachResult(RectI(-10, -10, 1, 1),
      [](Vec2I const& pos, int tile) -> int {
        if (pos[1] < 0)
          return tile == -1;
        else
          return tile == 1;
      });
  MultiArray<int, 2> res3comp({11, 11}, 1);

  EXPECT_TRUE(res3.size() == res3comp.size());
  res3.forEach([](Array2S const&, int elem) { EXPECT_TRUE(elem == 1); });
}
