#include "StarMathCommon.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(MathTest, All) {
  EXPECT_EQ(countSetBits<uint32_t>(7), 3u);
  EXPECT_EQ(countSetBits<uint32_t>(15), 4u);
  EXPECT_EQ(countSetBits<uint32_t>(-1), 32u);
}

TEST(Math, CycleIncrement) {
  int a = 0;
  a = cycleIncrement(a, 10, 13);
  ASSERT_EQ(a, 10);
  a = cycleIncrement(a, 10, 13);
  ASSERT_EQ(a, 11);
  a = cycleIncrement(a, 10, 13);
  ASSERT_EQ(a, 12);
  a = cycleIncrement(a, 10, 13);
  ASSERT_EQ(a, 13);
  a = cycleIncrement(a, 10, 13);
  ASSERT_EQ(a, 10);
  int b = 14;
  b = cycleIncrement(b, 10, 13);
  ASSERT_EQ(b, 10);
}
