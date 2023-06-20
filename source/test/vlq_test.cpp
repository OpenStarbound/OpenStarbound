#include "StarVlqEncoding.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(VlqTest, All) {
  char buffer[10];
  int64_t result;

  EXPECT_EQ(writeVlqI(-1, buffer), 1u);
  EXPECT_EQ(readVlqI(result, buffer), 1u);
  EXPECT_EQ(result, -1);

  EXPECT_EQ(writeVlqI(-65, buffer), 2u);
  EXPECT_EQ(readVlqI(result, buffer), 2u);
  EXPECT_EQ(result, -65);

  EXPECT_EQ(writeVlqI(-64, buffer), 1u);
  EXPECT_EQ(readVlqI(result, buffer), 1u);
  EXPECT_EQ(result, -64);

  EXPECT_EQ(writeVlqI((int64_t)1 << 63, buffer), 10u);
  EXPECT_EQ(readVlqI(result, buffer), 10u);
  EXPECT_EQ(result, (int64_t)1 << 63);

  EXPECT_EQ(writeVlqI(0, buffer), 1u);
  EXPECT_EQ(readVlqI(result, buffer), 1u);
  EXPECT_EQ(result, 0);
}
