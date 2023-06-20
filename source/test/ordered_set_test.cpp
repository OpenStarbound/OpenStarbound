#include "StarOrderedSet.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(OrderedSet, Insert) {
  OrderedSet<int> map{5, 4};
  map.insert(3);
  map.insert(2);
  map.insert(1);

  EXPECT_EQ(map.values(), List<int>({5, 4, 3, 2, 1}));
}

TEST(OrderedSet, AssignClear) {
  OrderedHashSet<int> map1{1, 2, 3};
  OrderedHashSet<int> map2 = map1;
  map1.clear();
  EXPECT_EQ(map1.values(), List<int>());
  EXPECT_EQ(map2.values(), List<int>({1, 2, 3}));
}

TEST(OrderedSet, AddReplace) {
  OrderedHashSet<int> map;
  EXPECT_TRUE(map.add(4));
  EXPECT_TRUE(map.add(6));
  EXPECT_TRUE(map.add(1));
  EXPECT_FALSE(map.add(1));
  EXPECT_TRUE(map.add(2));
  EXPECT_TRUE(map.add(3));
  EXPECT_FALSE(map.add(2));
  EXPECT_TRUE(map.replace(4));
  EXPECT_FALSE(map.replace(5));
  EXPECT_FALSE(map.addBack(6));

  EXPECT_EQ(map.values(), List<int>({1, 2, 3, 4, 5, 6}));
}

TEST(OrderedSet, ReverseIterate) {
  OrderedHashSet<int> map{5, 4, 3, 2, 1};

  List<int> vals;
  for (auto i = map.rbegin(); i != map.rend(); ++i)
    vals.append(*i);
  EXPECT_EQ(vals, List<int>({1, 2, 3, 4, 5}));
}
