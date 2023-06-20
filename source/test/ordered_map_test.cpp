#include "StarOrderedMap.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(OrderedMap, Insert) {
  OrderedMap<int, int> map;
  map.insert({3, 3});
  map.insert({2, 2});
  map.insert({1, 1});

  {
    List<int> comp = {3, 2, 1};
    EXPECT_EQ(comp, map.keys());
  }

  {
    List<int> comp = {3, 2, 1};
    List<int> keys;
    for (auto i = map.begin(); i != map.end(); ++i)
      keys.append(i->first);
    EXPECT_EQ(comp, keys);
  }

  {
    List<int> comp = {1, 2, 3};
    List<int> keys;
    for (auto i = map.rbegin(); i != map.rend(); ++i)
      keys.append(i->first);
    EXPECT_EQ(comp, keys);
  }
}

TEST(OrderedMap, Getters) {
  OrderedMap<int, int> map;
  map[1] = 1;
  map[2] = 2;
  map[3] = 3;

  EXPECT_EQ(map.get(1), 1);
  EXPECT_EQ(map.get(2), 2);
  EXPECT_EQ(map.get(3), 3);

  EXPECT_EQ(map.ptr(3), &map.get(3));
}

TEST(OrderedMap, ConstGetters) {
  OrderedHashMap<int, int> const map{
    {1, 1},
    {2, 2},
    {3, 3}
  };

  EXPECT_EQ(map.get(1), 1);
  EXPECT_EQ(map.get(2), 2);
  EXPECT_EQ(map.get(3), 3);

  EXPECT_EQ(map.ptr(3), &map.get(3));

  EXPECT_EQ(map.value(4, 4), 4);
  EXPECT_EQ(map.maybe(5), Maybe<int>());
}

TEST(OrderedMap, Sorting) {
  OrderedMap<int, int> map{
    {1, 5},
    {3, 3},
    {2, 4},
    {5, 1},
    {4, 2}
  };

  EXPECT_EQ(map.keys(), List<int>({1, 3, 2, 5, 4}));
  map.sortByKey();
  EXPECT_EQ(map.keys(), List<int>({1, 2, 3, 4, 5}));
  map.sortByValue();
  EXPECT_EQ(map.keys(), List<int>({5, 4, 3, 2, 1}));
}

TEST(OrderedMap, Removing) {
  OrderedHashMap<int, int> map{
    {5, 5},
    {4, 4},
    {3, 3},
    {2, 2},
    {1, 1}
  };

  map.remove(3);
  map.remove(1);
  EXPECT_EQ(map.keys(), List<int>({5, 4, 2}));
}
