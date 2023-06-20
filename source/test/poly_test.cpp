#include "StarPoly.hpp"
#include "StarRandom.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(PolyTest, ConvexHull) {
  PolyF::VertexList inputVertexes;
  for (unsigned i = 0; i < 1000; ++i) {
    float angle = Random::randf() * 2 * Constants::pi;
    inputVertexes.append({sin(angle), cos(angle)});
  }

  PolyF convex = PolyF::convexHull(inputVertexes);

  PolyF::VertexList testVertexes;
  for (unsigned i = 0; i < 1000; ++i) {
    float angle = Random::randf() * 2 * Constants::pi;
    testVertexes.append({sin(angle) * 0.75f, cos(angle) * 0.75f});
  }

  for (auto const& vertex : testVertexes)
    EXPECT_TRUE(convex.contains(vertex * 0.75f));
}

TEST(PolyTest, Distance) {
  PolyF square = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};

  EXPECT_TRUE(fabs(square.distance({-2.0f, -2.0f}) - Constants::sqrt2) < 0.00001f);
}

TEST(PolyTest, LineCollision) {
  PolyF square = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
  EXPECT_TRUE(vmag(Vec2F(-1, 0) - square.lineIntersection(Line2F({-2, 0}, {2, 0}))->point) < 0.0001f);
  EXPECT_TRUE(vmag(Vec2F(1, 0) - square.lineIntersection(Line2F({2, 0}, {-2, 0}))->point) < 0.0001f);
}

TEST(PolyTest, ConvexArea) {
  PolyF triangle = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {0.0f, 1.0f}};
  EXPECT_TRUE(fabs(triangle.convexArea() - 2.0f) < 0.0001f);
}

TEST(PolyTest, Clipping) {
  PolyF triangle1 = {{-2.0f, -1.0f}, {2.0f, -1.0f}, {0.0f, 1.0f}};
  PolyF triangle2 = {{2.0f, 1.0f}, {-2.0f, 1.0f}, {0.0f, -1.0f}};
  PolyF overlap = PolyF::clip(triangle1, triangle2);
  overlap.deduplicateVertexes(0.0001f);
  EXPECT_TRUE(overlap.sides() == 4);
  EXPECT_TRUE(overlap.convexArea() - 2 < 0.0001f);
}
