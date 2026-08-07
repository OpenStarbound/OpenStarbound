#include "StarCellularLightArray.hpp"
#include "StarCellularLighting.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace Star;

namespace {

ScalarCellularLightArray makeArray() {
  ScalarCellularLightArray array;
  array.setParameters(0, 10.0f, 4.0f, 20.0f, 4.0f, 0.0f, true);
  array.begin(21, 21);
  return array;
}

ScalarCellularLightArray::PointLight lightAt(size_t x, size_t y) {
  return {Vec2F(x, y), 1.0f, 0.0f, 0.0f, 0.0f, false};
}

// Scene builder matching the in-game lighting config
// (spreadPasses 3, spreadMaxAir 32, spreadMaxObstacle 8, pointMaxAir 48,
// pointMaxObstacle 9, pointObstacleBoost 3, additive).
ColoredCellularLightArray makeGameScene(size_t w, size_t h, bool additive = true) {
  ColoredCellularLightArray array;
  array.setParameters(3, 32.0f, 8.0f, 48.0f, 9.0f, 3.0f, additive);
  array.begin(w, h);
  return array;
}

void drawWall(ColoredCellularLightArray& array, size_t x0, size_t y0, size_t x1, size_t y1) {
  for (size_t x = x0; x <= x1; ++x)
    for (size_t y = y0; y <= y1; ++y)
      array.setObstacle(x, y, true);
}

}// namespace

TEST(cellularLightFlood, lightAttenuatesWithDistance) {
  auto array = makeArray();
  array.addPointLight(lightAt(10, 10));
  array.calculate(0, 0, 21, 21);

  // Attenuation = distance(cell center, light position) / pointMaxAir, so a
  // light at integer (10,10) sits on the corner of cell (10,10).
  EXPECT_NEAR(array.getLight(10, 10), 1.0f - std::sqrt(0.5f) / 20.0f, 1e-6f);
  EXPECT_NEAR(array.getLight(13, 10), 1.0f - std::sqrt(12.5f) / 20.0f, 1e-6f);
  EXPECT_NEAR(array.getLight(15, 10), 1.0f - std::sqrt(30.5f) / 20.0f, 1e-6f);
}

TEST(cellularLightFlood, wallAttenuatesLight) {
  auto array = makeArray();
  // Full-height wall: the flood cannot route around it, so any path to the
  // far side crosses exactly one obstacle block.
  for (size_t y = 0; y < 21; ++y)
    array.setObstacle(11, y, true);
  array.addPointLight(lightAt(10, 10));
  array.calculate(0, 0, 21, 21);

  // One obstacle block on the ray costs perBlockObstacleAttenuation = 0.25,
  // plus a small anti-aliasing contribution from the Xiaolin Wu endpoint
  // weights (verified against an independent reference implementation).
  EXPECT_NEAR(array.getLight(13, 10), 0.570685f, 1e-4f);
  EXPECT_NEAR(array.getLight(15, 10), 0.472835f, 1e-4f);
}

TEST(cellularLightFlood, pointLightCastsSharpRayShadow) {
  // In-game config (pointMaxAir 48, pointMaxObstacle 9, boost 3).  A wall
  // with a doorway near the top: a cell in the lower part has no line of
  // sight to the source, so the ray walk must leave it dark even though a
  // flood fill would route around the wall through the doorway.
  auto array = makeGameScene(41, 31);
  drawWall(array, 20, 0, 20, 15);
  ColoredCellularLightArray::PointLight light;
  light.position = Vec2F(10, 15);
  light.value = Vec3F(1.0f, 1.0f, 1.0f);
  light.beam = 0.0f;
  light.beamAngle = 0.0f;
  light.beamAmbience = 0.0f;
  light.asSpread = false;
  array.addPointLight(light);
  array.calculate(0, 0, 41, 31);

  // Behind the wall, far from the doorway: line of sight is blocked, the
  // accumulated obstacle attenuation plus boost exceeds 1.0 -> no light.
  EXPECT_LT(array.getLight(32, 4)[0], 0.01f);
  // Same side of the wall as the doorway: clear line of sight -> lit.
  EXPECT_GT(array.getLight(28, 20)[0], 0.3f);
}

TEST(cellularLightFlood, coloredChannelsStaySeparate) {
  ColoredCellularLightArray array;
  array.setParameters(0, 10.0f, 4.0f, 20.0f, 4.0f, 0.0f, true);
  array.begin(21, 21);
  ColoredCellularLightArray::PointLight light;
  light.position = Vec2F(10, 10);
  light.value = Vec3F(0.9f, 0.5f, 0.25f);
  light.beam = 0.0f;
  light.beamAngle = 0.0f;
  light.beamAmbience = 0.0f;
  light.asSpread = false;
  array.addPointLight(light);
  array.calculate(0, 0, 21, 21);

  // Each channel is attenuated proportionally to the same accumulated
  // attenuation.  A channel mix-up (writing G/B into the R array's neighbor
  // cells) leaves G and B zero here, so all three must be checked.
  Vec3F atSource = array.getLight(10, 10);
  float attSource = std::sqrt(0.5f) / 20.0f;
  EXPECT_NEAR(atSource[0], 0.9f - attSource, 1e-6f);
  EXPECT_NEAR(atSource[1], 0.5f - attSource * 0.5f / 0.9f, 1e-6f);
  EXPECT_NEAR(atSource[2], 0.25f - attSource * 0.25f / 0.9f, 1e-6f);

  Vec3F farCell = array.getLight(13, 10);
  float attFar = std::sqrt(12.5f) / 20.0f;
  EXPECT_NEAR(farCell[0], 0.9f - attFar, 1e-6f);
  EXPECT_NEAR(farCell[1], 0.5f - attFar * 0.5f / 0.9f, 1e-6f);
  EXPECT_NEAR(farCell[2], 0.25f - attFar * 0.25f / 0.9f, 1e-6f);
}

TEST(cellularLightFlood, incrementalAddRemoveIsSymmetric) {
  auto array = makeArray();
  array.addPointLight(lightAt(10, 10));
  array.calculate(0, 0, 21, 21);

  auto afterCalc = array.getLight(13, 10);

  array.addPointLightContribution(lightAt(10, 10), -1.0f);
  EXPECT_NEAR(array.getLight(10, 10), 0.0f, 1e-6f);
  EXPECT_NEAR(array.getLight(13, 10), 0.0f, 1e-6f);

  array.addPointLightContribution(lightAt(10, 10), 1.0f);
  EXPECT_FLOAT_EQ(array.getLight(13, 10), afterCalc);
}

TEST(cellularLightFlood, parallelPointPhaseIsConsistent) {
  auto array = makeArray();
  // 9 lights: >= the parallel-path threshold (8), so on multi-core machines
  // calculate() floods them on worker threads into private buffers and merges.
  for (size_t y = 5; y <= 17; y += 6)
    for (size_t x = 5; x <= 17; x += 6)
      array.addPointLight(lightAt(x, y));
  array.calculate(0, 0, 21, 21);

  auto full = array.getLight(11, 11);
  EXPECT_GT(full, 0.0f);

  // Subtracting one light's contribution must exactly cancel its share of the
  // merged result.
  array.addPointLightContribution(lightAt(5, 5), -1.0f);
  auto without = array.getLight(11, 11);
  EXPECT_LT(without, full);

  array.addPointLightContribution(lightAt(5, 5), 1.0f);
  EXPECT_NEAR(array.getLight(11, 11), full, 1e-6f);
}

TEST(cellularLightFlood, incrementalMovingLightMatchesFullRecalc) {
  // Simulates the in-game incremental point-light diff: a light moving by one
  // cell per step is removed from its old position and added at the new one
  // (addPointLightContribution with scale -1/+1, as calculateIncremental
  // does).  After many steps the accumulated result must match a full
  // recalculate of the same scene; divergence here shows up in-game as
  // light accumulating / flickering between full recalculations.
  auto array = makeGameScene(100, 100);
  ColoredCellularLightArray::PointLight moving;
  moving.position = Vec2F(10, 50);
  moving.value = Vec3F(0.9f, 0.5f, 0.3f);
  moving.beam = 0.0f;
  moving.beamAngle = 0.0f;
  moving.beamAmbience = 0.0f;
  moving.asSpread = false;
  ColoredCellularLightArray::PointLight torch;
  torch.position = Vec2F(90, 20);
  torch.value = Vec3F(0.8f, 0.7f, 0.5f);
  torch.beam = 0.0f;
  torch.beamAngle = 0.0f;
  torch.beamAmbience = 0.0f;
  torch.asSpread = false;

  array.addPointLight(moving);
  array.addPointLight(torch);
  array.calculate(0, 0, 100, 100);

  for (int step = 0; step < 240; ++step) {
    ColoredCellularLightArray::PointLight oldLight = moving;
    moving.position[0] += 1.0f;
    array.addPointLightContribution(oldLight, -1.0f);
    array.addPointLightContribution(moving, 1.0f);
  }

  // Full recalc of the final scene (spread passes re-run on the same base
  // light gathered by setLight, matching what calculate() does).
  ColoredCellularLightArray ref = makeGameScene(100, 100);
  ref.setLight(10, 50, Vec3F());// no-op: base light comes from setSpreadLightingPoints
  ref.addPointLight(moving);
  ref.addPointLight(torch);
  ref.calculate(0, 0, 100, 100);

  float maxDiff = 0.0f;
  for (size_t x = 0; x < 100; ++x)
    for (size_t y = 0; y < 100; ++y) {
      Vec3F a = array.getLight(x, y);
      Vec3F b = ref.getLight(x, y);
      for (size_t c = 0; c < 3; ++c)
        maxDiff = std::max(maxDiff, std::fabs(a[c] - b[c]));
    }
  EXPECT_LT(maxDiff, 1e-3f) << "incremental diff diverged from full recalc";
}

TEST(cellularLightFlood, scrolledStripsMatchFullRecalc) {
  // Calculator-level equivalence for the scrolled (dirty-region) path: a
  // camera scroll followed by re-gathering only the newly exposed strips and
  // diffing a moved point light over the overlap must produce the same
  // lightmap as a full recalculation of the new window.
  Json config = Json(JsonObject{
    {"spreadPasses", 3},
    {"spreadMaxAir", 32},
    {"spreadMaxObstacle", 8},
    {"pointMaxAir", 48},
    {"pointMaxObstacle", 9},
    {"pointObstacleBoost", 3},
    {"pointAdditive", true},
    {"brightnessLimit", 1.4}});

  auto worldLight = [](Vec2I const& p) -> Vec3F {
    Vec3F base(0.08f, 0.1f, 0.12f);
    if (p[0] % 17 == 0 && p[1] % 13 == 0)
      base += Vec3F(0.3f, 0.25f, 0.1f);
    return base;
  };
  // A wall with a doorway: obstacles matter for both the ray walk and the
  // spread phase.
  auto worldObstacle = [](Vec2I const& p) {
    return p[0] == 20 && (p[1] < 10 || p[1] > 14);
  };

  RectI query(0, 0, 60, 40);
  Vec2F lightPos(25, 20);
  Vec2F spreadPos(10, 30);

  auto gatherAll = [&](CellularLightingCalculator& calc) {
    RectI calcRegion = calc.calculationRegion();
    for (int x = calcRegion.xMin(); x < calcRegion.xMax(); ++x)
      for (int y = calcRegion.yMin(); y < calcRegion.yMax(); ++y) {
        Vec2I p(x, y);
        calc.setCellIndex(calc.baseIndexFor(p), worldLight(p), worldObstacle(p));
      }
  };
  auto addLights = [&](CellularLightingCalculator& calc) {
    calc.addPointLight(lightPos, Vec3F(1.0f, 0.7f, 0.5f), 0.5f, 0.0f, 0.0f);
    calc.addSpreadLight(spreadPos, Vec3F(0.4f, 0.4f, 0.6f));
  };

  CellularLightingCalculator calc;
  calc.setParameters(config);
  calc.begin(query);
  gatherAll(calc);
  addLights(calc);
  Lightmap fullMap;
  calc.calculate(fullMap);

  static const int steps[][2] = {{3, 0}, {2, 1}, {0, -2}, {-4, 0}, {1, -1}};
  RectI current = query;
  for (auto const& s : steps) {
    current = current.translated(Vec2I(s[0], s[1]));
    lightPos += Vec2F(s[0] * 0.5f, s[1] * 0.5f);

    // Reference: full recalculation of the new window.
    CellularLightingCalculator ref;
    ref.setParameters(config);
    ref.begin(current);
    gatherAll(ref);
    addLights(ref);
    Lightmap refMap;
    ref.calculate(refMap);

    // Scrolled path: shift the atlas, re-gather only the exposed strips,
    // diff the moved point light over the overlap.
    List<RectI> exposed = calc.scroll(current);
    for (RectI const& region : exposed)
      for (int x = region.xMin(); x < region.xMax(); ++x)
        for (int y = region.yMin(); y < region.yMax(); ++y) {
          Vec2I p(x, y);
          calc.setCellIndex(calc.baseIndexFor(p), worldLight(p), worldObstacle(p));
        }
    addLights(calc);
    // Diff the moved point light over the window overlap; border cells are
    // not maintained by the diff - the re-gathered strips returned by
    // scroll() (which include window cells that entered from the border) are
    // zeroed and re-flooded with the current lights by calculateScrolled.
    RectI lastCurrent = current.translated(Vec2I(-s[0], -s[1]));
    RectI overlap(
      std::max(lastCurrent.xMin(), current.xMin()),
      std::max(lastCurrent.yMin(), current.yMin()),
      std::min(lastCurrent.xMax(), current.xMax()),
      std::min(lastCurrent.yMax(), current.yMax()));
    calc.applyPointLightDiff(overlap);
    Lightmap scrolledMap;
    calc.calculateScrolled(scrolledMap, exposed);

    for (int x = 0; x < 60; ++x)
      for (int y = 0; y < 40; ++y) {
        Vec3F a = scrolledMap.get(x, y);
        Vec3F b = refMap.get(x, y);
        for (size_t c = 0; c < 3; ++c) {
          EXPECT_NEAR(a[c], b[c], 1e-3f) << "step " << s[0] << "," << s[1] << " cell " << x << "," << y;
        }
      }
  }
}

TEST(cellularLightFlood, dumpGameScene) {
  // Visual inspection dump: writes /tmp/opencode/lmap_scene.ppm so the actual
  // lightmap (shadows, beams, spread) can be looked at instead of guessed at.
  auto array = makeGameScene(200, 150);
  // Two rooms split by a wall with a doorway.
  drawWall(array, 100, 0, 100, 59);
  drawWall(array, 100, 80, 100, 149);
  drawWall(array, 0, 0, 199, 0);
  drawWall(array, 0, 149, 199, 149);
  drawWall(array, 0, 0, 0, 149);
  drawWall(array, 199, 0, 199, 149);

  ColoredCellularLightArray::PointLight torchA;
  torchA.position = Vec2F(20, 100);
  torchA.value = Vec3F(0.9f, 0.55f, 0.3f);
  torchA.beam = 0.9f;
  torchA.beamAngle = 0.0f;
  torchA.beamAmbience = 0.0f;
  torchA.asSpread = false;
  ColoredCellularLightArray::PointLight torchB;
  torchB.position = Vec2F(180, 40);
  torchB.value = Vec3F(0.5f, 0.6f, 0.9f);
  torchB.beam = 0.0f;
  torchB.beamAngle = 0.0f;
  torchB.beamAmbience = 0.0f;
  torchB.asSpread = false;
  ColoredCellularLightArray::SpreadLight sparkle;
  sparkle.position = Vec2F(50, 40);
  sparkle.value = Vec3F(0.4f, 0.4f, 0.5f);

  array.addPointLight(torchA);
  array.addPointLight(torchB);
  array.addSpreadLight(sparkle);
  array.calculate(0, 0, 200, 150);

  FILE* f = fopen("lmap_scene.ppm", "wb");
  ASSERT_NE(f, nullptr);
  fprintf(f, "P6\n200 150\n255\n");
  for (size_t y = 0; y < 150; ++y)
    for (size_t x = 0; x < 200; ++x) {
      Vec3F v = array.getLight(x, y);
      unsigned char px[3];
      for (size_t c = 0; c < 3; ++c)
        px[c] = (unsigned char)std::min(255.0f, std::max(0.0f, v[c] * 255.0f));
      fwrite(px, 1, 3, f);
    }
  fclose(f);
}
