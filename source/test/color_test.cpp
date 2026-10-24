#include "StarColor.hpp"
#include "StarJsonExtra.hpp"
#include "StarLua.hpp"
#include "StarLuaConverters.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ColorTest, ColorTemperature) {
  Color temp1000 = Color::temperature(1000);
  Color temp3000 = Color::temperature(3000);
  Color temp5000 = Color::temperature(5000);
  Color temp6000 = Color::temperature(6000);
  Color temp7000 = Color::temperature(7000);
  Color temp10000 = Color::temperature(10000);
  Color temp20000 = Color::temperature(20000);

  EXPECT_EQ(temp1000.red(), 255);
  EXPECT_EQ(temp1000.green(), 68);
  EXPECT_EQ(temp1000.blue(), 0);
  EXPECT_EQ(temp3000.red(), 255);
  EXPECT_EQ(temp3000.green(), 177);
  EXPECT_EQ(temp3000.blue(), 110);
  EXPECT_EQ(temp5000.red(), 255);
  EXPECT_EQ(temp5000.green(), 228);
  EXPECT_EQ(temp5000.blue(), 206);
  EXPECT_EQ(temp6000.red(), 255);
  EXPECT_EQ(temp6000.green(), 246);
  EXPECT_EQ(temp6000.blue(), 237);
  EXPECT_EQ(temp7000.red(), 243);
  EXPECT_EQ(temp7000.green(), 242);
  EXPECT_EQ(temp7000.blue(), 255);
  EXPECT_EQ(temp10000.red(), 202);
  EXPECT_EQ(temp10000.green(), 218);
  EXPECT_EQ(temp10000.blue(), 255);
  EXPECT_EQ(temp20000.red(), 171);
  EXPECT_EQ(temp20000.green(), 198);
  EXPECT_EQ(temp20000.blue(), 255);
}

TEST(ColorTest, OutOfRangeIsClampedTo8Bit) {
  Color c = Color::rgbaf(2.0f, -1.0f, 0.5f, 3.0f);
  EXPECT_EQ(c.red(), 255);
  EXPECT_EQ(c.green(), 0);
  EXPECT_EQ(c.alpha(), 255);
  EXPECT_FLOAT_EQ(c.redF(), 2.0f);
}

TEST(ColorTest, JsonComponentsAreAlways0To255) {
  Color a = jsonToColor(Json::parse("[255, 0, 0, 255]"));
  Color b = jsonToColor(Json::parse("[255.0, 0.0, 0.0, 255.0]"));
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.alpha(), 255);
  EXPECT_FLOAT_EQ(jsonToColor(Json::parse("[510, 0, 0]")).redF(), 2.0f);
}

TEST(ColorTest, LuaColorsStayWholeNumbers) {
  auto engineP = LuaEngine::create();
  auto& engine = *engineP;
  auto table = engine.createArrayTable(initializer_list<int>{51, 102, 204});
  Color c = engine.luaTo<Color>(table);
  auto back = engine.luaTo<Vec3F>(engine.luaFrom(c));
  EXPECT_EQ(back, Vec3F(51, 102, 204));
}
