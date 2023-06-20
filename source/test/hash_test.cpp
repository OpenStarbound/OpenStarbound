#include "StarHash.hpp"

#include "gtest/gtest.h"

TEST(HashTest, All) {
  enum SomeEnum { Foo, Bar };

  std::tuple<int, int, bool> testTuple(1, 2, false);
  std::pair<SomeEnum, int> testPair(SomeEnum::Bar, 10);

  // Yeah yeah, I know that it's technically possible for the hash to be zero,
  // but it's not!
  EXPECT_NE(Star::hash<decltype(testTuple)>()(testTuple), 0u);
  EXPECT_NE(Star::hash<decltype(testPair)>()(testPair), 0u);
}
