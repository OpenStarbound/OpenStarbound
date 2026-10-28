#include "StarWorldServer.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarFile.hpp"
#include "StarRoot.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(WorldHueTest, SetTileHueShift) {
  auto materialDatabase = Root::singleton().materialDatabase();
  MaterialId dirt = materialDatabase->materialId("dirt");
  ModId grass = materialDatabase->modId("grass");

  WorldServer world(Vec2U(1024, 1024), File::ephemeralFile());
  Vec2I pos(100, 0);
  Vec2I empty(103, 0);
  world.generateRegion(RectI(90, 0, 120, 20));

  // y = 0 so the placement rules (which want something to connect to) allow it.
  auto place = [&](Vec2I const& p, TileModification modification) {
    return world.applyTileModifications({{p, modification}}, true).empty();
  };
  ASSERT_TRUE(place(pos, PlaceMaterial{TileLayer::Background, dirt, MaterialHue(20)}));
  ASSERT_TRUE(place(pos, PlaceMaterial{TileLayer::Foreground, dirt, MaterialHue(10)}));
  ASSERT_TRUE(place(pos, PlaceMod{TileLayer::Foreground, grass, MaterialHue(30)}));
  ASSERT_EQ(world.material(empty, TileLayer::Foreground), EmptyMaterialId);
  ASSERT_EQ(world.material(pos, TileLayer::Foreground), dirt);
  ASSERT_EQ(world.mod(pos, TileLayer::Foreground), grass);
  auto color = world.colorVariant(pos, TileLayer::Foreground);

  // Only the asked layer and kind change.
  EXPECT_TRUE(world.setTileHueShift(pos, TileLayer::Foreground, false, 128));
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 128);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Background), 20);
  EXPECT_EQ(world.modHueShift(pos, TileLayer::Foreground), 30);
  EXPECT_EQ(world.colorVariant(pos, TileLayer::Foreground), color);

  EXPECT_TRUE(world.setTileHueShift(pos, TileLayer::Background, false, 64));
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Background), 64);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 128);

  EXPECT_TRUE(world.setTileHueShift(pos, TileLayer::Foreground, true, 200));
  EXPECT_EQ(world.modHueShift(pos, TileLayer::Foreground), 200);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 128);
  EXPECT_EQ(world.mod(pos, TileLayer::Foreground), grass);

  // Nothing to shift: false, and the tile is left as it was.
  EXPECT_FALSE(world.setTileHueShift(empty, TileLayer::Foreground, false, 99));
  EXPECT_FALSE(world.setTileHueShift(empty, TileLayer::Foreground, true, 99));
  EXPECT_EQ(world.materialHueShift(empty, TileLayer::Foreground), 0);
  EXPECT_EQ(world.modHueShift(empty, TileLayer::Foreground), 0);
  EXPECT_FALSE(world.setTileHueShift(pos, TileLayer::Background, true, 99));

  // Outside the world, or in a sector that is not loaded.
  EXPECT_FALSE(world.setTileHueShift(Vec2I(100, -5), TileLayer::Foreground, false, 99));
}
