#include "StarPoly.hpp"
#include "StarWorldGeometry.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(WorldGeometryTest, All) {
  WorldGeometry geom = WorldGeometry(Vec2U{8192, 3072});

  PolyF shapeA = {
      {19.9954f, 1603.62f}, {21.9954f, 1602.62f}, {21.9954f, 1607.62f}, {19.9954f, 1606.62f}, {19.9954f, 1605.12f}};
  Line2F lineA = {{20.9575f, 1605.13f}, {21.7853f, 1605.03f}};

  EXPECT_TRUE(geom.lineIntersectsPoly(lineA, shapeA));

  PolyF shapeA2 = {{0.9954f, 1.62f}, {2.9954f, 0.62f}, {2.9954f, 5.62f}, {0.9954f, 4.62f}, {0.9954f, 3.12f}};
  Line2F lineA2 = {{1.9575f, 3.13f}, {2.7853f, 3.03f}};

  EXPECT_TRUE(geom.lineIntersectsPoly(lineA2, shapeA2));

  PolyF shapeA3 = {{-10.0f, -10.0f}, {10, -10.0f}, {10.0f, 10.0f}, {-10.0f, 10.0f}}; //, {-10, 3.12};
  Line2F lineA3 = {{1.9575f, 3.13f}, {2.7853f, 3.03f}};

  EXPECT_TRUE(geom.lineIntersectsPoly(lineA3, shapeA3));

  RectF shapeA3R = RectF::withSize({-10, -10}, {20, 20});

  EXPECT_TRUE(geom.lineIntersectsRect(lineA3, shapeA3R));

  PolyF shapeB = {
      {20.5608f, 1603.62f}, {22.5608f, 1602.62f}, {22.5608f, 1607.62f}, {20.5608f, 1606.62f}, {20.5608f, 1605.12f}};
  Line2F lineB = {{21.7893f, 1605.07f}, {22.6173f, 1604.98f}};

  EXPECT_TRUE(geom.lineIntersectsPoly(lineB, shapeB));

  PolyF shapeC = {{-10.0f, -10.0f}, {10.0f, -10.0f}, {10.0f, 10.0f}, {-10.0f, 10.0f}};
  PolyF shapeC2 = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};

  EXPECT_TRUE(geom.polyIntersectsPoly(shapeC2, shapeC));
  EXPECT_TRUE(geom.polyIntersectsPoly(shapeC, shapeC2));
  EXPECT_TRUE(geom.polyContains(shapeC, Vec2F(0, 0)));
  EXPECT_TRUE(fabs(geom.polyOverlapArea(shapeC, shapeC2) - 4) < 0.0001f);
}
