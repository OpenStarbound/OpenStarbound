#include "StarLine.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(LineTest, IntersectionEndpoint) {
  Line2F a({0, 0}, {10, 10});
  Line2F b({10, -10}, {0, 0});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  EXPECT_TRUE(intersection1.intersects);
  EXPECT_TRUE(intersection2.intersects);

  EXPECT_TRUE(vmag(intersection1.point - Vec2F(0, 0)) < 0.0001f);
  EXPECT_TRUE(vmag(intersection2.point - Vec2F(0, 0)) < 0.0001f);

  EXPECT_TRUE(intersection1.glances);
  EXPECT_TRUE(intersection2.glances);

  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);
}

TEST(LineTest, IntersectionMiddle) {
  Line2F a({-5, 0}, {5, 0});
  Line2F b({0, -2}, {0, 8});

  auto intersection1 = a.intersection(b);
  EXPECT_TRUE(intersection1.intersects);
  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection1.coincides);
  EXPECT_TRUE(vmag(intersection1.point - Vec2F(0, 0)) < 0.0001f);
  EXPECT_TRUE(fabs(intersection1.t - 0.5f) < 0.0001f);
}

TEST(LineTest, IntersectionOneEndpoint) {
  Line2F a({0, 0}, {0, 5});
  Line2F b({-1, 5}, {1, 5});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);
  EXPECT_TRUE(intersection1.glances);
  EXPECT_TRUE(intersection2.glances);
  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);

  EXPECT_TRUE(fabs(intersection1.t - 1.0f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t - 0.5f) < 0.0001f);
}

TEST(LineTest, IntersectionOneVertical) {
  Line2F a({0, 3}, {8, 5});
  Line2F b({4, 0}, {4, 8});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);
  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);
  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);
  EXPECT_TRUE(intersection1.intersects);
  EXPECT_TRUE(intersection2.intersects);

  EXPECT_TRUE(fabs(intersection1.t - 0.5f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t - 0.5f) < 0.0001f);

  EXPECT_TRUE(vmag(intersection1.point - Vec2F(4, 4)) < 0.0001f);
  EXPECT_TRUE(vmag(intersection2.point - Vec2F(4, 4)) < 0.0001f);
}

TEST(LineTest, NoIntersection) {
  Line2F a({1, 1}, {2, 2});
  Line2F b({-1, 1}, {0, 0});

  auto intersection1 = a.intersection(b);
  auto intersection1inf = a.intersection(b, true);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_TRUE(intersection1inf.intersects);
}

TEST(LineTest, ParallelHorizontal) {
  Line2F a({9, 12}, {10, 12});
  Line2F b({10, 20}, {20, 20});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);

  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);
}

TEST(LineTest, ParallelVertical) {
  Line2F a({12, 12}, {12, 14});
  Line2F b({20, 10}, {20, 20});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);

  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);
}

TEST(LineTest, ParallelOther) {
  Line2F a({3, 3}, {4, 4});
  Line2F b({5, 6}, {7, 8});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);

  EXPECT_FALSE(intersection1.coincides);
  EXPECT_FALSE(intersection2.coincides);
}

TEST(LineTest, CoincidesVertical) {
  Line2F a({3, 3}, {3, 4});
  Line2F b({3, 5}, {3, 7});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  auto intersection1inf = a.intersection(b, true);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);
  EXPECT_TRUE(intersection1inf.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);
  EXPECT_TRUE(intersection1inf.glances);

  EXPECT_TRUE(intersection1.coincides);
  EXPECT_TRUE(intersection2.coincides);
  EXPECT_TRUE(intersection1inf.coincides);

  EXPECT_TRUE(fabs(intersection1.t - 2.0f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t + 0.5f) < 0.0001f);
}

TEST(LineTest, CoincidesHorizontal) {
  Line2F a({3, 3}, {4, 3});
  Line2F b({5, 3}, {7, 3});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  auto intersection1inf = a.intersection(b, true);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);
  EXPECT_TRUE(intersection1inf.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);
  EXPECT_TRUE(intersection1inf.glances);

  EXPECT_TRUE(intersection1.coincides);
  EXPECT_TRUE(intersection2.coincides);
  EXPECT_TRUE(intersection1inf.coincides);

  EXPECT_TRUE(fabs(intersection1.t - 2.0f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t + 0.5f) < 0.0001f);
}

TEST(LineTest, CoincidesOther) {
  Line2F a({3, 3}, {4, 4});
  Line2F b({5, 5}, {7, 7});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  auto intersection1inf = a.intersection(b, true);

  EXPECT_FALSE(intersection1.intersects);
  EXPECT_FALSE(intersection2.intersects);
  EXPECT_TRUE(intersection1inf.intersects);

  EXPECT_FALSE(intersection1.glances);
  EXPECT_FALSE(intersection2.glances);
  EXPECT_TRUE(intersection1inf.glances);

  EXPECT_TRUE(intersection1.coincides);
  EXPECT_TRUE(intersection2.coincides);
  EXPECT_TRUE(intersection1inf.coincides);

  EXPECT_TRUE(fabs(intersection1.t - 2.0f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t + 0.5f) < 0.0001f);
}

TEST(LineTest, IntersectCoincides) {
  Line2F a({3, 3}, {5, 5});
  Line2F b({4, 4}, {6, 6});

  auto intersection1 = a.intersection(b);
  auto intersection2 = b.intersection(a);

  EXPECT_TRUE(intersection1.intersects);
  EXPECT_TRUE(intersection2.intersects);

  EXPECT_TRUE(intersection1.glances);
  EXPECT_TRUE(intersection2.glances);

  EXPECT_TRUE(intersection1.coincides);
  EXPECT_TRUE(intersection2.coincides);

  EXPECT_TRUE(vmag(intersection1.point - Vec2F(4, 4)) < 0.0001f);
  EXPECT_TRUE(vmag(intersection2.point - Vec2F(4, 4)) < 0.0001f);

  EXPECT_TRUE(fabs(intersection1.t - 0.5f) < 0.0001f);
  EXPECT_TRUE(fabs(intersection2.t) < 0.0001f);
}

TEST(LineTest, Closest) {
  Line2F a({0, 0}, {10, 0});

  EXPECT_TRUE(fabs(a.distanceTo(Vec2F(-1, 5), true) - 5.0f) < 0.0001f);
  EXPECT_TRUE(fabs(a.distanceTo(Vec2F(-3, 4), false) - 5.0f) < 0.0001f);
}

TEST(LineTest, MakePositive) {
  Line2F a({0, 0}, {10, 0});
  Line2F aorig = a;
  Line2F b({10, 0}, {0, 0});
  Line2F borig = b;
  Line2F c({10, 0}, {10, 1});
  Line2F corig = c;
  Line2F d({10, 1}, {10, 0});
  Line2F dorig = d;
  Line<float, 3> e({10, 0, 0}, {10, 0, 1});
  Line<float, 3> eorig = e;
  Line<float, 3> f({10, 0, 1}, {10, 0, 0});
  Line<float, 3> forig = f;

  a.makePositive();
  EXPECT_TRUE(a == aorig);
  b.makePositive();
  EXPECT_TRUE(b == aorig);
  EXPECT_FALSE(b == borig);
  c.makePositive();
  EXPECT_TRUE(c == corig);
  d.makePositive();
  EXPECT_TRUE(d == corig);
  EXPECT_FALSE(d == dorig);
  e.makePositive();
  EXPECT_TRUE(e == eorig);
  f.makePositive();
  EXPECT_TRUE(f == eorig);
  EXPECT_FALSE(f == forig);
}
