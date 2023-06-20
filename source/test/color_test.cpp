#include "StarColor.hpp"

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
