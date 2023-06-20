#include "StarPeriodic.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(PeriodicTest, All) {
  Periodic periodic(2);

  EXPECT_TRUE(periodic.ready());
  EXPECT_TRUE(periodic.tick());
  EXPECT_FALSE(periodic.ready());
  EXPECT_FALSE(periodic.tick());
  EXPECT_TRUE(periodic.ready());
  EXPECT_TRUE(periodic.tick());

  periodic = Periodic(0);
  EXPECT_FALSE(periodic.tick());
  EXPECT_FALSE(periodic.tick());

  periodic = Periodic(3);
  EXPECT_TRUE(periodic.ready());
  EXPECT_TRUE(periodic.tick());
  EXPECT_FALSE(periodic.ready());
  EXPECT_FALSE(periodic.tick());
  EXPECT_FALSE(periodic.tick());
  EXPECT_TRUE(periodic.ready());
  EXPECT_TRUE(periodic.tick());
  EXPECT_FALSE(periodic.ready());
}
