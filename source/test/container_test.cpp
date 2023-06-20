#include "StarSet.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(SetTest, All) {
  Set<int> a = {1, 2, 3, 4};
  Set<int> b = {2, 4, 5};
  EXPECT_EQ(a.difference(b), Set<int>({1, 3}));
  EXPECT_EQ(a.intersection(b), Set<int>({2, 4}));
  EXPECT_EQ(a.combination(b), Set<int>({1, 2, 3, 4, 5}));
}
