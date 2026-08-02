// Benchmark for CellularLightArray phases (spread / serial+parallel point).
// Usage: dist/cellular_lighting_bench [width] [height] [iterations]
#include "StarCellularLightArray.hpp"

#include <chrono>
#include <cstdio>
#include <random>

using namespace Star;

static ColoredCellularLightArray makeScene(size_t w, size_t h, size_t spreadCount, size_t pointCount,
                                           unsigned spreadPasses, bool additive) {
  ColoredCellularLightArray array;
  array.setParameters(spreadPasses, 10.0f, 4.0f, 20.0f, 4.0f, 0.0f, additive);
  array.begin(w, h);

  std::mt19937 rng(42);
  for (size_t x = 0; x < w; ++x)
    for (size_t y = 0; y < h; ++y)
      if (rng() % 17 == 0)
        array.setObstacle(x, y, true);

  auto color = [&]() {
    float r = 0.4f + (rng() % 100) / 100.0f;
    float g = 0.4f + (rng() % 100) / 100.0f;
    float b = 0.4f + (rng() % 100) / 100.0f;
    return Vec3F(r, g, b);
  };

  for (size_t i = 0; i < spreadCount; ++i)
    array.addSpreadLight({Vec2F(rng() % w, rng() % h), color()});
  for (size_t i = 0; i < pointCount; ++i)
    array.addPointLight({Vec2F(rng() % w, rng() % h), color(), 0.0f, 0.0f, 0.0f, false});

  return array;
}

static double ms(std::chrono::steady_clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}

static double bench(size_t w, size_t h, size_t spreadCount, size_t pointCount, unsigned spreadPasses,
                    unsigned iterations) {
  auto array = makeScene(w, h, spreadCount, pointCount, spreadPasses, true);
  auto start = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < iterations; ++i)
    array.calculate(0, 0, w, h);
  double timeMs = ms(std::chrono::steady_clock::now() - start) / iterations;
  return timeMs;
}

int main(int argc, char** argv) {
  size_t w = argc > 1 ? strtoull(argv[1], nullptr, 10) : 280;
  size_t h = argc > 2 ? strtoull(argv[2], nullptr, 10) : 175;
  unsigned iterations = argc > 3 ? strtoul(argv[3], nullptr, 10) : 100;

  printf("region %zux%zu, %u iterations, threads=%u, hardware=%u\n",
         w, h, iterations, std::thread::hardware_concurrency(), std::thread::hardware_concurrency());

  for (unsigned passes : {2u, 4u}) {
    double spreadOnly = bench(w, h, 0, 0, passes, iterations);
    printf("spreadPasses=%u  spread-only(0 lights):   %6.3f ms\n", passes, spreadOnly);

    // Serial fast path (< 8 lights) and parallel paths (>= 8 lights).
    for (size_t points : {4u, 32u, 128u, 200u}) {
      double total = bench(w, h, 0, points, passes, iterations);
      printf("spreadPasses=%u  point=%-3zu total %6.3f ms  (point phase %6.3f ms, %s path)\n",
             passes, points, total, total - spreadOnly, points < 8 ? "serial" : "parallel");
    }
  }
  return 0;
}
