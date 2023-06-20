#include "StarRect.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(RectTest, TranslateToInclude) {
  RectF rect;
  rect = {0, 0, 10, 10};
  rect.translateToInclude(20, 20, 2, 2);
  EXPECT_TRUE(rect.xMax() < 22.01 && rect.xMax() > 21.99);
  EXPECT_TRUE(rect.yMax() < 22.01 && rect.yMax() > 21.99);

  rect = {0, 0, 10, 10};
  rect.translateToInclude(-20, -20, 2, 2);
  EXPECT_TRUE(rect.xMin() > -22.01 && rect.xMin() < -21.99);
  EXPECT_TRUE(rect.yMin() > -22.01 && rect.yMin() < -21.99);

  rect = {0, 0, 10, 10};
  rect.translateToInclude(5, 5, 3, 3);
  EXPECT_TRUE(rect.xMin() < 0.01 && rect.xMin() > -0.01);
  EXPECT_TRUE(rect.yMin() < 0.01 && rect.yMin() > -0.01);
}

TEST(RectTest, BoxGlanceCorner) {
  RectF rect1 = {0, 0, 10, 10};
  RectF rect2 = {-10, -10, 0, 0};
  auto res = rect1.intersection(rect2);

  EXPECT_TRUE(!res.intersects);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(vmag(res.overlap) < 0.0001f);
}

TEST(RectTest, BoxGlanceEdge) {
  RectF rect1 = {0, 0, 10, 10};
  RectF rect2 = {-10, 0, 0, 10};
  auto res = rect1.intersection(rect2);

  EXPECT_TRUE(!res.intersects);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(vmag(res.overlap) < 0.0001f);
}

TEST(RectTest, BoxIntersectionNone) {
  RectF rect1 = {0, 0, 10, 10};
  RectF rect2 = {-10, 0, -1, 10};
  auto res = rect1.intersection(rect2);

  EXPECT_TRUE(!res.intersects);
  EXPECT_TRUE(!res.glances);
}

TEST(RectTest, BoxIntersectionOverlapX) {
  RectF rect1 = {0, 0, 10, 10};
  RectF rect2 = {7, 6, 10, 10};
  auto res = rect1.intersection(rect2);

  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(!res.glances);
  EXPECT_TRUE(fabs(res.overlap[0] + 3) < 0.0001f);
  EXPECT_TRUE(fabs(res.overlap[1]) < 0.0001f);
}

TEST(RectTest, BoxIntersectionOverlapY) {
  RectF rect1 = {0, 0, 10, 10};
  RectF rect2 = {5, 6, 10, 10};
  auto res = rect1.intersection(rect2);

  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(!res.glances);
  EXPECT_TRUE(fabs(res.overlap[0]) < 0.0001f);
  EXPECT_TRUE(fabs(res.overlap[1] + 4) < 0.0001f);
}

TEST(RectTest, ContainsPoint) {
  Box<float, 4> rect1 = {{0, 0, 0, 0}, {10, 10, 10, 10}};
  Vec4F point1 = {5, 5, 5, 10};
  Vec4F point2 = {-10, 0, 0, 0};
  Vec4F point3 = {5, 4, 3, 2};
  Vec4F point4 = {5, 4, 3, -2};

  EXPECT_TRUE(rect1.contains(point1));
  EXPECT_FALSE(rect1.contains(point1, false));
  EXPECT_FALSE(rect1.contains(point2));
  EXPECT_FALSE(rect1.contains(point2, false));
  EXPECT_TRUE(rect1.contains(point3));
  EXPECT_TRUE(rect1.contains(point3, false));
  EXPECT_FALSE(rect1.contains(point4));
  EXPECT_FALSE(rect1.contains(point4, false));
}

TEST(RectTest, EdgeIntersection) {
  RectF rect1 = {10, 10, 20, 20};

  Line2F line0_1 = {{3, 3}, {4, 4}};
  auto res = rect1.edgeIntersection(line0_1);
  EXPECT_FALSE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);

  Line2F line1_1 = {{9, 12}, {10, 12}};
  res = rect1.edgeIntersection(line1_1);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 12) < 0.0001f);
  EXPECT_TRUE(fabs(res.t - 1.0f) < 0.0001f);

  Line2F line1_2 = {{9, 12}, {11, 12}};
  res = rect1.edgeIntersection(line1_2);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 12) < 0.0001f);
  EXPECT_TRUE(fabs(res.t - 0.5f) < 0.0001f);

  Line2F line1_3 = {{10, 12}, {11, 12}};
  res = rect1.edgeIntersection(line1_3);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 12) < 0.0001f);
  EXPECT_TRUE(fabs(res.t) < 0.0001f);

  Line2F line1_4 = {{10, 12}, {10, 13}};
  res = rect1.edgeIntersection(line1_4);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 12) < 0.0001f);

  Line2F line2_1 = {{10, 10}, {11, 10}};
  res = rect1.edgeIntersection(line2_1);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_2 = {{15, 9}, {20, 15}};
  res = rect1.edgeIntersection(line2_2);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_3 = {{15, 9}, {21, 15}};
  res = rect1.edgeIntersection(line2_3);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_4 = {{15, 10}, {21, 15}};
  res = rect1.edgeIntersection(line2_4);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_5 = {{15, 11}, {20, 15}};
  res = rect1.edgeIntersection(line2_5);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 20) < 0.0001f);

  Line2F line2_6 = {{9, 9}, {11, 11}};
  res = rect1.edgeIntersection(line2_6);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_7 = {{9, 11}, {11, 9}};
  res = rect1.edgeIntersection(line2_7);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line2_8 = {{10, 10.5}, {10, 10}};
  res = rect1.edgeIntersection(line2_8);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10.5) < 0.0001f);

  Line2F line2_9 = {{10, 10}, {10, 10.5}};
  res = rect1.edgeIntersection(line2_9);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line3_1 = {{10, 10}, {20, 10}};
  res = rect1.edgeIntersection(line3_1);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line3_2 = {{9, 10}, {21, 10}};
  res = rect1.edgeIntersection(line3_2);
  EXPECT_TRUE(res.intersects);
  EXPECT_TRUE(res.coincides);
  EXPECT_TRUE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line3_3 = {{9, 8}, {15, 20}};
  res = rect1.edgeIntersection(line3_3);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line3_4 = {{9, 8}, {16, 22}};
  res = rect1.edgeIntersection(line3_4);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line4_1 = {{9, 9}, {21, 21}};
  res = rect1.edgeIntersection(line4_1);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 10) < 0.0001f);

  Line2F line4_2 = {{9, 21}, {21, 9}};
  res = rect1.edgeIntersection(line4_2);
  EXPECT_TRUE(res.intersects);
  EXPECT_FALSE(res.coincides);
  EXPECT_FALSE(res.glances);
  EXPECT_TRUE(fabs(res.point[0] - 10) < 0.0001f);
  EXPECT_TRUE(fabs(res.point[1] - 20) < 0.0001f);
}

TEST(RectTest, Center) {
  RectU a(0, 0, 10, 10);

  a.setCenter(Vec2U(5, 5));
  EXPECT_EQ(a, RectU(0, 0, 10, 10));

  a.setCenter(Vec2U(10, 10));
  EXPECT_EQ(a, RectU(5, 5, 15, 15));

  a.setCenter(Vec2U(5, 5));
  EXPECT_EQ(a, RectU(0, 0, 10, 10));
}
