#include "StarWorldServer.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarFile.hpp"
#include "StarRoot.hpp"
#include "StarLua.hpp"
#include "StarWorldLuaBindings.hpp"

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

TEST(WorldHueTest, PaintThenHue) {
  auto materialDatabase = Root::singleton().materialDatabase();
  MaterialId dirt = materialDatabase->materialId("dirt");
  MaterialId colorful = NullMaterialId;
  for (auto const& name : materialDatabase->materialNames()) {
    auto id = materialDatabase->materialId(name);
    if (isRealMaterial(id) && materialDatabase->isMultiColor(id) && materialDatabase->canPlaceInLayer(id, TileLayer::Foreground)) {
      colorful = id;
      break;
    }
  }
  ASSERT_NE(colorful, NullMaterialId);

  WorldServer world(Vec2U(1024, 1024), File::ephemeralFile());
  Vec2I pos(100, 0);
  world.generateRegion(RectI(90, 0, 120, 20));

  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMaterial{TileLayer::Background, dirt, MaterialHue(20)}}}, true).empty());
  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMaterial{TileLayer::Foreground, colorful, MaterialHue(10)}}}, true).empty());

  // Painting resets the hue, which is what scripts have always got.
  ASSERT_TRUE(world.modifyTile(pos, PlaceMaterialColor{TileLayer::Foreground, 1}, true));
  EXPECT_EQ(world.colorVariant(pos, TileLayer::Foreground), 1);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 0);

  // A hue written afterwards leaves the paint alone.
  EXPECT_TRUE(world.setTileHueShift(pos, TileLayer::Foreground, false, 100));
  EXPECT_EQ(world.colorVariant(pos, TileLayer::Foreground), 1);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 100);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Background), 20);
}

TEST(WorldHueTest, LuaCallbacks) {
  auto materialDatabase = Root::singleton().materialDatabase();
  MaterialId dirt = materialDatabase->materialId("dirt");
  ModId grass = materialDatabase->modId("grass");

  WorldServer world(Vec2U(1024, 1024), File::ephemeralFile());
  Vec2I pos(100, 0);
  world.generateRegion(RectI(90, 0, 120, 20));
  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMaterial{TileLayer::Foreground, dirt, MaterialHue(0)}}}, true).empty());
  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMod{TileLayer::Foreground, grass, MaterialHue(0)}}}, true).empty());

  auto luaEngine = LuaEngine::create();
  auto context = luaEngine->createContext();
  context.setCallbacks("world", LuaBindings::makeWorldCallbacks(&world));

  // Hue is clamped to the 0-255 the tile can hold.
  EXPECT_TRUE(context.eval<bool>("return world.setMaterialHue({100, 0}, 'foreground', 300)"));
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 255);
  EXPECT_TRUE(context.eval<bool>("return world.setMaterialHue({100, 0}, 'foreground', -20)"));
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 0);
  EXPECT_TRUE(context.eval<bool>("return world.setModHue({100, 0}, 'foreground', 77)"));
  EXPECT_EQ(world.modHueShift(pos, TileLayer::Foreground), 77);

  // Nothing there, or nothing to shift: false rather than an error.
  EXPECT_FALSE(context.eval<bool>("return world.setMaterialHue({103, 0}, 'foreground', 5)"));
  EXPECT_FALSE(context.eval<bool>("return world.setModHue({100, 0}, 'background', 5)"));

  // An unknown layer is a script error.
  EXPECT_ANY_THROW(context.eval<bool>("return world.setMaterialHue({100, 0}, 'middle', 5)"));

  // setMaterialColor resets the hue as before, unless a hue is passed.
  context.eval<Json>("world.setMaterialColor({100, 0}, 'foreground', 0); return 0");
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 0);
  context.eval<Json>("world.setMaterialColor({100, 0}, 'foreground', 0, 9000); return 0");
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 255);
  context.eval<Json>("world.setMaterialColor({100, 0}, 'foreground', 0, 40); return 0");
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 40);
}

TEST(WorldHueTest, SetHueShiftModification) {
  auto materialDatabase = Root::singleton().materialDatabase();
  MaterialId dirt = materialDatabase->materialId("dirt");
  ModId grass = materialDatabase->modId("grass");

  WorldServer world(Vec2U(1024, 1024), File::ephemeralFile());
  Vec2I pos(100, 0);
  Vec2I empty(103, 0);
  world.generateRegion(RectI(90, 0, 120, 20));
  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMaterial{TileLayer::Foreground, dirt, MaterialHue(10)}}}, true).empty());
  ASSERT_TRUE(world.applyTileModifications({{pos, PlaceMod{TileLayer::Foreground, grass, MaterialHue(30)}}}, true).empty());

  // What a client sends survives the wire.
  TileModification sent = SetHueShift{TileLayer::Foreground, true, 77};
  DataStreamBuffer buffer;
  buffer.write(sent);
  buffer.seek(0);
  auto received = buffer.read<TileModification>();
  ASSERT_TRUE(received.is<SetHueShift>());
  EXPECT_EQ(received.get<SetHueShift>().layer, TileLayer::Foreground);
  EXPECT_TRUE(received.get<SetHueShift>().mod);
  EXPECT_EQ(received.get<SetHueShift>().hueShift, 77);

  // The server applies it like any other tile modification.
  EXPECT_TRUE(world.applyTileModifications({{pos, received}}, true).empty());
  EXPECT_EQ(world.modHueShift(pos, TileLayer::Foreground), 77);
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 10);

  EXPECT_TRUE(world.applyTileModifications({{pos, SetHueShift{TileLayer::Foreground, false, 200}}}, true).empty());
  EXPECT_EQ(world.materialHueShift(pos, TileLayer::Foreground), 200);
  EXPECT_EQ(world.modHueShift(pos, TileLayer::Foreground), 77);

  // Nothing to shift: it is handed back as unapplied.
  EXPECT_EQ(world.applyTileModifications({{empty, SetHueShift{TileLayer::Foreground, false, 5}}}, true).size(), 1u);
  EXPECT_EQ(world.applyTileModifications({{pos, SetHueShift{TileLayer::Background, false, 5}}}, true).size(), 1u);
  EXPECT_FALSE(world.canModifyTile(empty, SetHueShift{TileLayer::Foreground, true, 5}, true));
  EXPECT_TRUE(world.canModifyTile(pos, SetHueShift{TileLayer::Foreground, true, 5}, true));
}
