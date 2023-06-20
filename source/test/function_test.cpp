#include "StarStoredFunctions.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(StoredFunctionTest, All) {
  List<pair<double, double>> values = {{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 4.0f}, {3.0f, 9.0f}, {4.0f, 16.0f}};
  ParametricFunction<double, double> function(values, InterpolationMode::Linear, BoundMode::Clamp);
  StoredFunction levelingFunction(function);

  EXPECT_LT(fabs(function.interpolate(2.5f) - 6.5f), 0.001f);

  auto result = levelingFunction.search(16.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 16.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 4.0f), 0.001f);

  result = levelingFunction.search(0.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 0.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 0.0f), 0.001f);

  result = levelingFunction.search(6.5f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 6.5f), 0.001f);
  EXPECT_LT(fabs(result.solution - 2.5f), 0.001f);

  List<pair<double, double>> swordsValues = {{0.0f, 0.0f}, {1.0f, 10.0f}, {100.0f, 500.0f}, {9999.0f, 9999999.0f}};
  ParametricFunction<double, double> swordsFunction(swordsValues, InterpolationMode::Linear, BoundMode::Clamp);
  StoredFunction swordsLevelingFunction(swordsFunction);

  result = swordsLevelingFunction.search(0.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 0.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 0.0f), 0.001f);

  result = swordsLevelingFunction.search(10.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 10.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 1.0f), 0.001f);

  result = swordsLevelingFunction.search(500.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 500.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 100.0f), 0.001f);

  result = swordsLevelingFunction.search(501.0f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 501.0f), 0.001f);
  EXPECT_LT(fabs(result.solution - 100.0f), 0.001f);

  result = swordsLevelingFunction.search(500.01f);
  EXPECT_TRUE(result.found);
  EXPECT_LT(fabs(result.value - 500.01f), 0.001f);
  EXPECT_LT(fabs(result.solution - 100.0f), 0.001f);
}
