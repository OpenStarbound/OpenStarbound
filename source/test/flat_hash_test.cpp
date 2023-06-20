#include "StarFlatHashSet.hpp"
#include "StarFlatHashMap.hpp"
#include "StarRandom.hpp"
#include "StarVector.hpp"
#include "StarIterator.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(FlatHashSet, Preset) {
  FlatHashSet<int> testSet = {42, 63};
  ASSERT_EQ(testSet.find(41), testSet.end());
  ASSERT_EQ(*testSet.find(42), 42);
  ASSERT_EQ(*testSet.find(63), 63);
  ASSERT_EQ(testSet.find(64), testSet.end());
  ASSERT_EQ(testSet.size(), 2u);

  testSet.erase(testSet.find(42));
  ASSERT_EQ(testSet.find(42), testSet.end());
  ASSERT_EQ(*testSet.find(63), 63);
  ASSERT_EQ(testSet.size(), 1u);

  testSet.erase(testSet.find(63));
  ASSERT_EQ(testSet.find(42), testSet.end());
  ASSERT_EQ(testSet.find(63), testSet.end());
  ASSERT_EQ(testSet.size(), 0u);

  testSet.insert(12);
  testSet.insert(24);
  ASSERT_EQ(*testSet.find(12), 12);
  ASSERT_EQ(testSet.size(), 2u);
  testSet.clear();

  ASSERT_EQ(testSet.find(12), testSet.end());
  ASSERT_EQ(testSet.size(), 0u);

  EXPECT_TRUE(testSet.insert(7).second);
  EXPECT_TRUE(testSet.insert(11).second);
  EXPECT_FALSE(testSet.insert(7).second);

  ASSERT_EQ(testSet.size(), 2u);

  FlatHashSet<int> testSet2(testSet.begin(), testSet.end());
  ASSERT_EQ(testSet, testSet2);

  ASSERT_EQ(testSet.erase(testSet.begin(), testSet.end()), testSet.end());
  ASSERT_EQ(testSet.size(), 0u);

  ASSERT_NE(testSet, testSet2);

  FlatHashSet<int> testSet3(testSet.begin(), testSet.end());
  ASSERT_EQ(testSet3.size(), 0u);

  testSet2 = testSet;
  ASSERT_EQ(testSet, testSet2);
}

TEST(FlatHashSet, Random) {
  List<Vec2I> keys;
  for (unsigned i = 0; i < 100000; ++i)
    keys.append(Vec2I(i * 743202097, i * 205495087));
  Random::shuffle(keys);

  FlatHashSet<Vec2I> testSet;
  for (size_t i = 0; i < keys.size(); ++i)
    testSet.insert(keys[i]);

  Random::shuffle(keys);
  for (size_t i = 0; i < keys.size() / 2; ++i)
    testSet.erase(keys[i]);

  Random::shuffle(keys);
  for (size_t i = 0; i < keys.size() / 3; ++i)
    testSet.insert(keys[i]);

  Random::shuffle(keys);
  for (size_t i = 0; i < keys.size() / 2; ++i)
    testSet.erase(keys[i]);

  Random::shuffle(keys);
  for (auto k : keys) {
    auto i = testSet.find(k);
    if (i != testSet.end())
      ASSERT_TRUE(*i == k);
  }

  Random::shuffle(keys);
  for (auto k : keys) {
    testSet.insert(k);
    ASSERT_TRUE(testSet.find(k) != testSet.end());
  }

  List<Vec2I> cmp;
  for (auto const& k : testSet)
    cmp.append(k);
  cmp.sort();
  keys.sort();
  ASSERT_TRUE(cmp == keys);

  Random::shuffle(keys);
  for (auto k : keys) {
    testSet.erase(k);
    ASSERT_TRUE(testSet.find(k) == testSet.end());
  }

  ASSERT_TRUE(testSet.empty());
}

TEST(FlatHashMap, Preset) {
  FlatHashMap<int, int> testMap = {{42, 42}, {63, 63}};
  ASSERT_EQ(testMap.find(41), testMap.end());
  ASSERT_EQ(testMap.find(42)->second, 42);
  ASSERT_EQ(testMap.find(63)->second, 63);
  ASSERT_EQ(testMap.find(64), testMap.end());
  ASSERT_EQ(testMap.size(), 2u);

  testMap.erase(testMap.find(42));
  ASSERT_EQ(testMap.find(42), testMap.end());
  ASSERT_EQ(testMap.find(63)->second, 63);
  ASSERT_EQ(testMap.size(), 1u);

  testMap.erase(testMap.find(63));
  ASSERT_EQ(testMap.find(42), testMap.end());
  ASSERT_EQ(testMap.find(63), testMap.end());
  ASSERT_EQ(testMap.size(), 0u);

  testMap.insert({12, 12});
  testMap.insert({24, 24});
  ASSERT_EQ(testMap.find(12)->second, 12);
  ASSERT_EQ(testMap.size(), 2u);
  testMap.clear();

  ASSERT_EQ(testMap.find(12), testMap.end());
  ASSERT_EQ(testMap.size(), 0u);

  EXPECT_TRUE(testMap.insert({7, 7}).second);
  EXPECT_TRUE(testMap.insert({11, 11}).second);
  EXPECT_FALSE(testMap.insert({7, 7}).second);

  ASSERT_EQ(testMap.size(), 2u);

  FlatHashMap<int, int> testMap2(testMap.begin(), testMap.end());
  ASSERT_EQ(testMap, testMap2);

  ASSERT_EQ(testMap.erase(testMap.begin(), testMap.end()), testMap.end());
  ASSERT_EQ(testMap.size(), 0u);

  ASSERT_NE(testMap, testMap2);

  FlatHashMap<int, int> testMap3(testMap.begin(), testMap.end());
  ASSERT_EQ(testMap3.size(), 0u);

  testMap2 = testMap;
  ASSERT_EQ(testMap, testMap2);
}

TEST(FlatHashMap, Random) {
  List<pair<Vec2I, int>> values;
  for (unsigned i = 0; i < 100000; ++i)
    values.append({Vec2I(i * 743202097, i * 205495087), i});
  Random::shuffle(values);

  FlatHashMap<Vec2I, int> testMap;
  for (auto v : values)
    testMap.insert(v);

  Random::shuffle(values);
  for (size_t i = 0; i < values.size() / 2; ++i)
    testMap.erase(values[i].first);

  Random::shuffle(values);
  for (size_t i = 0; i < values.size() / 3; ++i)
    testMap.insert(values[i]);

  Random::shuffle(values);
  for (size_t i = 0; i < values.size() / 2; ++i)
    testMap.erase(values[i].first);

  Random::shuffle(values);
  for (auto v : values) {
    auto i = testMap.find(v.first);
    if (i != testMap.end())
      ASSERT_TRUE(i->second == v.second);
  }

  Random::shuffle(values);
  for (auto v : values) {
    ASSERT_TRUE(testMap.insert(v).first->second == v.second);
    ASSERT_TRUE(testMap.at(v.first) == v.second);
  }

  Random::shuffle(values);
  for (auto v : values) {
    ASSERT_EQ(testMap.erase(v.first), 1u);
    ASSERT_TRUE(testMap.find(v.first) == testMap.end());
  }

  ASSERT_TRUE(testMap.empty());
}

TEST(FlatHashMap, Iterator) {
  List<pair<Vec2I, int>> values;
  for (unsigned i = 0; i < 100000; ++i)
    values.append({Vec2I(i * 743202097, i * 205495087), i});

  FlatHashMap<Vec2I, int> testMap;
  for (auto v : values)
    testMap.insert(v);

  auto it = makeSMutableMapIterator(testMap);
  while (it.hasNext()) {
    if (it.next().second % 3 == 0)
      it.remove();
  }

  auto i = values.begin();
  std::advance(i, values.size());
  ASSERT_EQ(i, values.end());
}
