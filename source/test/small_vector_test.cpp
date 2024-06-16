#include "StarSmallVector.hpp"
#include "StarFormat.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(SmallVectorTest, InsertErase) {
  typedef SmallVector<int, 2> SV;
  SV a = {1, 2, 3, 4};
  EXPECT_EQ(a.size(), 4u);
  EXPECT_EQ(a, SV({1, 2, 3, 4}));
  EXPECT_NE(a, SV({1, 2, 3}));
  a.insert(a.begin(), 0);
  a.insert(a.begin(), -1);
  EXPECT_EQ(a, SV({-1, 0, 1, 2, 3, 4}));
  a.insert(a.begin(), {-3, -2});
  EXPECT_EQ(a, SV({-3, -2, -1, 0, 1, 2, 3, 4}));
  a.erase(a.begin() + 1);
  EXPECT_EQ(a, SV({-3, -1, 0, 1, 2, 3, 4}));
  a.erase(a.begin(), a.begin() + 3);
  EXPECT_EQ(a, SV({1, 2, 3, 4}));
  a.insert(a.end(), {5, 6, 7, 8});
  EXPECT_EQ(a, SV({1, 2, 3, 4, 5, 6, 7, 8}));
  a.erase(a.begin() + 2, a.end() - 2);
  EXPECT_EQ(a, SV({1, 2, 7, 8}));
  a.insert(a.begin() + 2, 6);
  a.insert(a.begin() + 2, 5);
  a.insert(a.begin() + 2, 4);
  a.insert(a.begin() + 2, 3);
  EXPECT_EQ(a, SV({1, 2, 3, 4, 5, 6, 7, 8}));

  SV b(a.begin(), a.end());
  EXPECT_EQ(b, SV({1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(SmallVectorTest, Comparators) {
  typedef SmallVector<int, 3> SV;

  EXPECT_TRUE(SV({1, 2, 3, 4}) < SV({1, 2, 3, 5}));
  EXPECT_FALSE(SV({1, 2, 3, 4}) < SV({1, 2, 3, 4}));
  EXPECT_FALSE(SV({1, 2, 3, 4}) < SV({1, 2, 3, 3}));
  EXPECT_TRUE(SV({1, 2, 3}) < SV({1, 2, 3, 4}));
  EXPECT_FALSE(SV({1, 2, 3, 4, 5}) < SV({1, 2, 3, 4}));
}

TEST(SmallVectorTest, Destructors) {
  auto i = make_shared<int>(0);
  SmallVector<shared_ptr<int>, 1> v;
  v.push_back(i);
  v.push_back(i);
  v.push_back(i);
  EXPECT_EQ(i.use_count(), 4);
  v.pop_back();
  EXPECT_EQ(i.use_count(), 3);
  v.pop_back();
  EXPECT_EQ(i.use_count(), 2);
  v.clear();
  EXPECT_EQ(i.use_count(), 1);
}
