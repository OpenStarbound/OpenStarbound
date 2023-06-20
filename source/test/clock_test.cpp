#include "StarTime.hpp"
#include "StarThread.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ClockTest, All) {
  Clock clock;

  Thread::sleepPrecise(1000);

  // Pick wide range in case the system is acting iffy, it's just to check that
  // the clock is progressing properly.
  EXPECT_GT(clock.time(), 0.8);
  EXPECT_LT(clock.time(), 8.0);

  double time = clock.time();
  clock.stop();
  Thread::sleepPrecise(1000);
  EXPECT_EQ(clock.time(), time);

  clock.reset();
  EXPECT_EQ(clock.time(), 0.0);

  Timer nullTimer;
  EXPECT_TRUE(nullTimer.timeUp());
  EXPECT_FALSE(nullTimer.running());
}
