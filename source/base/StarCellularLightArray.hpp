#pragma once

#include "StarList.hpp"
#include "StarMaybe.hpp"
#include "StarVector.hpp"
#include "StarWorkerPool.hpp"

#include <atomic>
#include <limits>
#include <queue>
#include <thread>

namespace Star {

// Operations for simple scalar lighting.
struct ScalarLightTraits {
  typedef float Value;
  static constexpr size_t ComponentCount = 1;

  // Per-channel drops for spreading the source cell's channels by `drop`.
  static void spreadDrops(float const* src, float drop, float* drops) {
    drops[0] = drop;
  }

  // Per-channel proportional subtract (contribution = value - attenuation).
  static void subtractChannels(float const* src, float attenuation, float* out) {
    out[0] = std::max(src[0] - attenuation, 0.0f);
  }

  // Pointer to the Value's channels as a contiguous float array.
  static float const* channels(float const& value) {
    return &value;
  }

  static float subtract(float value, float drop);
  static float maxIntensity(float value);
  static float minIntensity(float value);

  static float max(float v1, float v2);
};

// Operations for 3 component (colored) lighting.  Spread and subtract are
// applied proportionally, so that color ratios stay the same, to prevent hues
// changing as light spreads.
struct ColoredLightTraits {
  typedef Vec3F Value;
  static constexpr size_t ComponentCount = 3;

  static void spreadDrops(float const* src, float drop, float* drops) {
    float maxChannel = std::max(src[0], std::max(src[1], src[2]));
    if (maxChannel <= 0.0f) {
      drops[0] = drops[1] = drops[2] = 0.0f;
      return;
    }
    float s = drop / maxChannel;
    drops[0] = src[0] * s;
    drops[1] = src[1] * s;
    drops[2] = src[2] * s;
  }

  static void subtractChannels(float const* src, float attenuation, float* out) {
    float maxChannel = std::max(src[0], std::max(src[1], src[2]));
    if (maxChannel <= 0.0f) {
      out[0] = out[1] = out[2] = 0.0f;
      return;
    }
    for (size_t c = 0; c < 3; ++c) {
      float pdrop = (attenuation * src[c]) / maxChannel;
      out[c] = src[c] > pdrop ? src[c] - pdrop : 0.0f;
    }
  }

  static float const* channels(Vec3F const& value) {
    return &value[0];
  }

  static Vec3F subtract(Vec3F value, float drop);
  static float maxIntensity(Vec3F const& value);
  static float minIntensity(Vec3F const& value);

  static Vec3F max(Vec3F const& v1, Vec3F const& v2);
};

template <typename LightTraits>
class CellularLightArray {
public:
  typedef typename LightTraits::Value LightValue;
  static constexpr size_t ComponentCount = LightTraits::ComponentCount;

  struct Cell {
    LightValue light;
    bool obstacle;
  };

  // Write-through view of a single cell over the SoA storage, returned by
  // cell()/cellAtIndex().  Only assignment from Cell is supported (used by
  // CellularLightingCalculator::setCellIndex / setCellColumn).
  class CellRef {
  public:
    CellRef(CellularLightArray& array, size_t index) : m_array(array), m_index(index) {}

    CellRef& operator=(Cell const& cell) {
      m_array.setLightAtIndex(m_index, cell.light);
      m_array.setObstacleAtIndex(m_index, cell.obstacle);
      return *this;
    }

    operator Cell() const {
      return Cell{m_array.lightAtIndex(m_index), m_array.obstacleAtIndex(m_index)};
    }

  private:
    CellularLightArray& m_array;
    size_t m_index;
  };

  struct SpreadLight {
    Vec2F position;
    LightValue value;
  };

  struct PointLight {
    Vec2F position;
    LightValue value;
    float beam;
    float beamAngle;
    float beamAmbience;
    bool asSpread;
  };

  void setParameters(unsigned spreadPasses, float spreadMaxAir, float spreadMaxObstacle,
                     float pointMaxAir, float pointMaxObstacle, float pointObstacleBoost, bool pointAdditive);

  // The border around the target lighting array where initial lighting / light
  // source data is required.  Based on parameters.
  size_t borderCells() const;

  // Begin a new calculation, setting internal storage to new width and height
  // (if these are the same as last time this is cheap).  Always clears all
  // existing light and collision data.
  void begin(size_t newWidth, size_t newHeight);

  // Position is in index space, spread lights will have no effect if they are
  // outside of the array.  Integer points are assumed to be on the corners of
  // the grid (not the center)
  void addSpreadLight(SpreadLight const& spreadLight);
  void addPointLight(PointLight const& pointLight);

  // Directly set the lighting values for this position.
  void setLight(size_t x, size_t y, LightValue const& light);

  // Get current light value.  Call after calling calculate() to pull final
  // data out.
  LightValue getLight(size_t x, size_t y) const;

  // Set obstacle values for this position
  void setObstacle(size_t x, size_t y, bool obstacle);
  bool getObstacle(size_t x, size_t y) const;

  Cell cell(size_t x, size_t y) const;
  CellRef cell(size_t x, size_t y);

  Cell cellAtIndex(size_t index) const;
  CellRef cellAtIndex(size_t index);

  // Calculate lighting in the given sub-rect, in order to properly do spread
  // lighting, and initial lighting must be given for the ambient border this
  // given rect, and the array size must be at least that large.  xMax / yMax
  // are not inclusive, the range is [xMin, xMax) and [yMin, yMax).
  void calculate(size_t xMin, size_t yMin, size_t xMax, size_t yMax);

  // Incrementally add (scale = 1) or remove (scale = -1) a point light's
  // contribution in additive mode. No-op in non-additive mode, where callers
  // must do a full recalculate instead.
  void addPointLightContribution(PointLight const& light, float scale);

private:
  // Set 4 points based on interpolated light position and free space
  // attenuation.
  void setSpreadLightingPoints();

  // Spreads light out in an octagonal based cellular automata
  void calculateLightSpread(size_t xmin, size_t ymin, size_t xmax, size_t ymax);

  // Loops through each light and adds light strength based on distance and
  // obstacle attenuation.  Calculates within the given sub-rect
  void calculatePointLighting(size_t xmin, size_t ymin, size_t xmax, size_t ymax);

  // Ray-traced point lighting: for each cell in the light's bounding box a
  // straight ray is cast from the source (Xiaolin Wu anti-aliased line walk)
  // and the obstacle attenuation along it is summed.  This is the original
  // Starbound algorithm: walls cast sharp shadows and light never flows
  // around corners or through doorways the way a flood fill does.  Applies
  // each cell's contribution (distance air attenuation, beam, obstacle
  // attenuation) to the array: additively scaled by `scale` (used both by
  // the full calculation and by incremental add/remove, so subtracting a
  // moved light's old contribution exactly cancels what was added) or by
  // max() in non-additive mode.  writeTargets, if non-null, is ComponentCount
  // per-channel float arrays (e.g. a worker buffer) the contributions are
  // written to instead of the cell light channels.
  void pointLightFlood(PointLight const& light, float scale, size_t xmin, size_t ymin, size_t xmax, size_t ymax,
                       float* const* writeTargets);

  // Xiaolin Wu's anti-aliased line walk from start to end, summing the
  // attenuation of every obstacle block the line would draw to.
  float lineAttenuation(Vec2F const& start, Vec2F const& end, float perObstacleAttenuation, float maxAttenuation);

  // SoA accessors over the channel arrays, index = x * m_height + y.
  LightValue lightAtIndex(size_t index) const;
  void setLightAtIndex(size_t index, LightValue const& light);
  bool obstacleAtIndex(size_t index) const;
  void setObstacleAtIndex(size_t index, bool obstacle);

  size_t m_width;
  size_t m_height;

  // Structure-of-arrays storage: one contiguous float array per light channel
  // (scalar lighting uses channel 0 only) plus an obstacle byte per cell.
  std::vector<float> m_lightChannels[3];
  std::vector<uint8_t> m_obstacles;

  List<SpreadLight> m_spreadLights;
  List<PointLight> m_pointLights;

  // Per-worker contribution buffers for the parallel point phase
  // (calculatePointLighting), reused across calls.  Each worker buffer holds
  // cellCount * ComponentCount floats, channel-major
  // (channel c at c * cellCount + index).
  std::vector<std::vector<float>> m_workerBuffers;

  unsigned m_spreadPasses;
  float m_spreadMaxAir;
  float m_spreadMaxObstacle;
  float m_pointMaxAir;
  float m_pointMaxObstacle;
  float m_pointObstacleBoost;
  bool m_pointAdditive;
};

typedef CellularLightArray<ColoredLightTraits> ColoredCellularLightArray;
typedef CellularLightArray<ScalarLightTraits> ScalarCellularLightArray;

// Shared worker pool for the parallel point phase (calculatePointLighting).
// One pool for all CellularLightArray instantiations.
inline WorkerPool& cellularLightWorkerPool() {
  static WorkerPool pool("CellularLightWorkerPool", std::thread::hardware_concurrency());
  return pool;
}

inline float ScalarLightTraits::subtract(float c, float drop) {
  return std::max(c - drop, 0.0f);
}

inline float ScalarLightTraits::maxIntensity(float value) {
  return value;
}

inline float ScalarLightTraits::minIntensity(float value) {
  return value;
}

inline float ScalarLightTraits::max(float v1, float v2) {
  return std::max(v1, v2);
}

inline Vec3F ColoredLightTraits::subtract(Vec3F c, float drop) {
  float max = std::max(std::max(c[0], c[1]), c[2]);
  if (max <= 0.0f)
    return c;

  for (size_t i = 0; i < 3; ++i) {
    float pdrop = (drop * c[i]) / max;
    if (c[i] > pdrop)
      c[i] -= pdrop;
    else
      c[i] = 0;
  }
  return c;
}

inline float ColoredLightTraits::maxIntensity(Vec3F const& value) {
  return value.max();
}

inline float ColoredLightTraits::minIntensity(Vec3F const& value) {
  return value.min();
}

inline Vec3F ColoredLightTraits::max(Vec3F const& v1, Vec3F const& v2) {
  return vmax(v1, v2);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setParameters(unsigned spreadPasses, float spreadMaxAir, float spreadMaxObstacle,
                                                    float pointMaxAir, float pointMaxObstacle, float pointObstacleBoost, bool pointAdditive) {
  m_spreadPasses = spreadPasses;
  m_spreadMaxAir = spreadMaxAir;
  m_spreadMaxObstacle = spreadMaxObstacle;
  m_pointMaxAir = pointMaxAir;
  m_pointMaxObstacle = pointMaxObstacle;
  m_pointObstacleBoost = pointObstacleBoost;
  m_pointAdditive = pointAdditive;
}

template <typename LightTraits>
size_t CellularLightArray<LightTraits>::borderCells() const {
  return (size_t)ceil(max(0.0f, max(m_spreadMaxAir, m_pointMaxAir)));
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::begin(size_t newWidth, size_t newHeight) {
  m_spreadLights.clear();
  m_pointLights.clear();
  starAssert(newWidth > 0 && newHeight > 0);

  m_width = newWidth;
  m_height = newHeight;

  size_t cellCount = newWidth * newHeight;
  for (size_t c = 0; c < ComponentCount; ++c)
    m_lightChannels[c].assign(cellCount, 0.0f);
  m_obstacles.assign(cellCount, 0);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::addSpreadLight(SpreadLight const& spreadLight) {
  m_spreadLights.append(spreadLight);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::addPointLight(PointLight const& pointLight) {
  m_pointLights.append(pointLight);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setLight(size_t x, size_t y, LightValue const& lightValue) {
  setLightAtIndex(x * m_height + y, lightValue);
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::lightAtIndex(size_t index) const -> LightValue {
  if constexpr (ComponentCount == 1)
    return m_lightChannels[0][index];
  else
    return LightValue(m_lightChannels[0][index], m_lightChannels[1][index], m_lightChannels[2][index]);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setLightAtIndex(size_t index, LightValue const& light) {
  if constexpr (ComponentCount == 1)
    m_lightChannels[0][index] = light;
  else {
    m_lightChannels[0][index] = light[0];
    m_lightChannels[1][index] = light[1];
    m_lightChannels[2][index] = light[2];
  }
}

template <typename LightTraits>
bool CellularLightArray<LightTraits>::obstacleAtIndex(size_t index) const {
  return m_obstacles[index] != 0;
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setObstacleAtIndex(size_t index, bool obstacle) {
  m_obstacles[index] = obstacle ? 1 : 0;
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::pointLightFlood(PointLight const& light, float scale,
                                                      size_t xmin, size_t ymin, size_t xmax, size_t ymax,
                                                      float* const* writeTargets) {
  if (light.position[0] < 0 || light.position[0] > m_width - 1 || light.position[1] < 0 || light.position[1] > m_height - 1)
    return;

  float maxIntensity = LightTraits::maxIntensity(light.value);
  Vec2F beamDirection = Vec2F(1, 0).rotate(light.beamAngle);
  float perBlockObstacleAttenuation = light.asSpread ? 1.0f / m_spreadMaxObstacle : 1.0f / m_pointMaxObstacle;
  float perBlockAirAttenuation = light.asSpread ? 1.0f / m_spreadMaxAir : 1.0f / m_pointMaxAir;

  float maxRange = maxIntensity * (light.asSpread ? m_spreadMaxAir : m_pointMaxAir);
  size_t lxmin = std::floor(std::max(0.0f, light.position[0] - maxRange));
  size_t lymin = std::floor(std::max(0.0f, light.position[1] - maxRange));
  size_t lxmax = std::ceil(std::min<float>(m_width, light.position[0] + maxRange));
  size_t lymax = std::ceil(std::min<float>(m_height, light.position[1] + maxRange));

  for (size_t x = lxmin; x < lxmax; ++x) {
    for (size_t y = lymin; y < lymax; ++y) {
      Vec2F blockPos = Vec2F(x + 0.5f, y + 0.5f);
      Vec2F relativeLightPosition = blockPos - light.position;
      float distance = relativeLightPosition.magnitude();

      float attenuation = 0.0f;
      Vec2F direction = relativeLightPosition / distance;
      if (distance != 0.0f) {
        attenuation = distance * perBlockAirAttenuation;
        if (attenuation < 1.0f && light.beam > 0.0001f) {
          attenuation += (1.0f - light.beamAmbience) * clamp(light.beam * (1.0f - direction * beamDirection), 0.0f, 1.0f);
        }
      }

      float remainingAttenuation = maxIntensity - attenuation;
      if (remainingAttenuation > 0.0f && distance != 0.0f) {
        float circularizedPerBlockObstacleAttenuation = perBlockObstacleAttenuation / std::max(std::fabs(direction[0]), std::fabs(direction[1]));
        float blockAttenuation = lineAttenuation(blockPos, light.position, circularizedPerBlockObstacleAttenuation, remainingAttenuation);
        attenuation += blockAttenuation;
        // Apply single obstacle boost (determine single obstacle by one
        // block unit of attenuation).
        if (!light.asSpread)
          attenuation += std::min(blockAttenuation, circularizedPerBlockObstacleAttenuation) * m_pointObstacleBoost;
      }

      if (attenuation < 1.0f) {
        size_t index = x * m_height + y;
        float contrib[ComponentCount];
        LightTraits::subtractChannels(LightTraits::channels(light.value), attenuation, contrib);
        float* dst[ComponentCount];
        for (size_t c = 0; c < ComponentCount; ++c)
          dst[c] = (writeTargets ? writeTargets[c] : m_lightChannels[c].data()) + index;
        if (m_pointAdditive) {
          // Original semantics: check max intensity of the (0.15-scaled for
          // asSpread) contribution before applying the signed scale.
          float factor = light.asSpread ? 0.15f : 1.0f;
          float maxChannel = contrib[0];
          for (size_t c = 1; c < ComponentCount; ++c)
            maxChannel = std::max(maxChannel, contrib[c]);
          if (maxChannel * factor > 0.0001f)
            for (size_t c = 0; c < ComponentCount; ++c)
              dst[c][0] += contrib[c] * scale * factor;
        } else {
          for (size_t c = 0; c < ComponentCount; ++c)
            dst[c][0] = std::max(dst[c][0], contrib[c]);
        }
      }
    }
  }
}

template <typename LightTraits>
float CellularLightArray<LightTraits>::lineAttenuation(Vec2F const& start, Vec2F const& end,
                                                       float perObstacleAttenuation, float maxAttenuation) {
  // Run Xiaolin Wu's line algorithm from start to end, summing over colliding
  // blocks using perObstacleAttenuation.
  float obstacleAttenuation = 0.0;

  // Apply correction because integer coordinates are lower left corner.
  float x1 = start[0] - 0.5;
  float y1 = start[1] - 0.5;
  float x2 = end[0] - 0.5;
  float y2 = end[1] - 0.5;

  float dx = x2 - x1;
  float dy = y2 - y1;

  if (fabs(dx) < fabs(dy)) {
    if (y2 < y1) {
      swap(y1, y2);
      swap(x1, x2);
    }

    float gradient = dx / dy;

    // first end point
    float yend = round(y1);
    float xend = x1 + gradient * (yend - y1);
    float ygap = rfpart(y1 + 0.5);
    int ypxl1 = yend;
    int xpxl1 = ipart(xend);

    if (obstacleAtIndex(xpxl1 * m_height + ypxl1))
      obstacleAttenuation += rfpart(xend) * ygap * perObstacleAttenuation;

    if (obstacleAtIndex((xpxl1 + 1) * m_height + ypxl1))
      obstacleAttenuation += fpart(xend) * ygap * perObstacleAttenuation;

    if (obstacleAttenuation >= maxAttenuation)
      return maxAttenuation;

    float interx = xend + gradient;

    // second end point
    yend = round(y2);
    xend = x2 + gradient * (yend - y2);
    ygap = fpart(y2 + 0.5);
    int ypxl2 = yend;
    int xpxl2 = ipart(xend);

    if (obstacleAtIndex(xpxl2 * m_height + ypxl2))
      obstacleAttenuation += rfpart(xend) * ygap * perObstacleAttenuation;

    if (obstacleAtIndex((xpxl2 + 1) * m_height + ypxl2))
      obstacleAttenuation += fpart(xend) * ygap * perObstacleAttenuation;

    if (obstacleAttenuation >= maxAttenuation)
      return maxAttenuation;

    for (int y = ypxl1 + 1; y < ypxl2; ++y) {
      int interxIpart = ipart(interx);
      float interxFpart = interx - interxIpart;
      float interxRFpart = 1.0 - interxFpart;

      if (obstacleAtIndex(interxIpart * m_height + y))
        obstacleAttenuation += interxRFpart * perObstacleAttenuation;
      if (obstacleAtIndex((interxIpart + 1) * m_height + y))
        obstacleAttenuation += interxFpart * perObstacleAttenuation;

      if (obstacleAttenuation >= maxAttenuation)
        return maxAttenuation;

      interx += gradient;
    }
  } else {
    if (x2 < x1) {
      swap(x1, x2);
      swap(y1, y2);
    }

    float gradient = dy / dx;

    // first end point
    float xend = round(x1);
    float yend = y1 + gradient * (xend - x1);
    float xgap = rfpart(x1 + 0.5);
    int xpxl1 = xend;
    int ypxl1 = ipart(yend);

    if (obstacleAtIndex(xpxl1 * m_height + ypxl1))
      obstacleAttenuation += rfpart(yend) * xgap * perObstacleAttenuation;

    if (obstacleAtIndex(xpxl1 * m_height + (ypxl1 + 1)))
      obstacleAttenuation += fpart(yend) * xgap * perObstacleAttenuation;

    if (obstacleAttenuation >= maxAttenuation)
      return maxAttenuation;

    float intery = yend + gradient;

    // second end point
    xend = round(x2);
    yend = y2 + gradient * (xend - x2);
    xgap = fpart(x2 + 0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart(yend);

    if (obstacleAtIndex(xpxl2 * m_height + ypxl2))
      obstacleAttenuation += rfpart(yend) * xgap * perObstacleAttenuation;

    if (obstacleAtIndex(xpxl2 * m_height + (ypxl2 + 1)))
      obstacleAttenuation += fpart(yend) * xgap * perObstacleAttenuation;

    if (obstacleAttenuation >= maxAttenuation)
      return maxAttenuation;

    for (int x = xpxl1 + 1; x < xpxl2; ++x) {
      int interyIpart = ipart(intery);
      float interyFpart = intery - interyIpart;
      float interyRFpart = 1.0 - interyFpart;

      if (obstacleAtIndex(x * m_height + interyIpart))
        obstacleAttenuation += interyRFpart * perObstacleAttenuation;
      if (obstacleAtIndex(x * m_height + (interyIpart + 1)))
        obstacleAttenuation += interyFpart * perObstacleAttenuation;

      if (obstacleAttenuation >= maxAttenuation)
        return maxAttenuation;

      intery += gradient;
    }
  }

  return min(obstacleAttenuation, maxAttenuation);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::calculatePointLighting(size_t xmin, size_t ymin, size_t xmax, size_t ymax) {
  size_t lightCount = m_pointLights.size();
  if (lightCount == 0)
    return;

  unsigned hardwareConcurrency = std::max(1u, std::thread::hardware_concurrency());

  // Serial in-place for few lights: a worker-pool roundtrip costs more than
  // the floods themselves.  ponytail: threshold 8, tune with measurements.
  if (lightCount < 8 || hardwareConcurrency <= 1) {
    for (PointLight const& light : m_pointLights)
      pointLightFlood(light, 1.0f, xmin, ymin, xmax, ymax, nullptr);
    return;
  }

  // Parallel: each worker floods its share of lights into a private
  // contribution buffer (light boxes overlap, so writes cannot go into the
  // shared cell array), then the buffers are merged into the cells.
  unsigned workerCount = std::min<size_t>(lightCount, hardwareConcurrency);
  size_t cellCount = m_width * m_height;
  if (m_workerBuffers.size() < workerCount) {
    m_workerBuffers.resize(workerCount);
  }
  for (unsigned w = 0; w < workerCount; ++w) {
    m_workerBuffers[w].assign(cellCount * ComponentCount, 0.0f);
  }

  auto& pool = cellularLightWorkerPool();
  std::vector<WorkerPoolHandle> futures;
  futures.reserve(workerCount);
  size_t chunk = (lightCount + workerCount - 1) / workerCount;
  for (unsigned w = 0; w < workerCount; ++w) {
    size_t start = w * chunk;
    size_t end = std::min(lightCount, start + chunk);
    if (start >= end)
      break;
    futures.push_back(pool.addWork([&, w, start, end]() {
      float* targets[ComponentCount];
      for (size_t c = 0; c < ComponentCount; ++c)
        targets[c] = m_workerBuffers[w].data() + c * cellCount;
      for (size_t i = start; i < end; ++i)
        pointLightFlood(m_pointLights[i], 1.0f, xmin, ymin, xmax, ymax, targets);
    }));
  }
  for (auto const& future : futures)
    future.finish();

  for (size_t y = ymin; y < ymax; ++y) {
    for (size_t x = xmin; x < xmax; ++x) {
      size_t index = x * m_height + y;
      for (size_t c = 0; c < ComponentCount; ++c) {
        float acc = m_lightChannels[c][index];
        for (unsigned w = 0; w < workerCount; ++w) {
          float v = m_workerBuffers[w][c * cellCount + index];
          if (m_pointAdditive)
            acc += v;
          else
            acc = std::max(acc, v);
        }
        m_lightChannels[c][index] = acc;
      }
    }
  }
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::addPointLightContribution(PointLight const& light, float scale) {
  // Incremental point light update, additive mode only.
  if (!m_pointAdditive)
    return;

  pointLightFlood(light, scale, 0, 0, m_width, m_height, nullptr);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setObstacle(size_t x, size_t y, bool obstacle) {
  setObstacleAtIndex(x * m_height + y, obstacle);
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::getLight(size_t x, size_t y) const -> LightValue {
  return lightAtIndex(x * m_height + y);
}

template <typename LightTraits>
bool CellularLightArray<LightTraits>::getObstacle(size_t x, size_t y) const {
  return obstacleAtIndex(x * m_height + y);
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::cell(size_t x, size_t y) const -> Cell {
  return cellAtIndex(x * m_height + y);
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::cell(size_t x, size_t y) -> CellRef {
  return cellAtIndex(x * m_height + y);
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::cellAtIndex(size_t index) const -> Cell {
  starAssert(index < m_width * m_height);
  return Cell{lightAtIndex(index), obstacleAtIndex(index)};
}

template <typename LightTraits>
auto CellularLightArray<LightTraits>::cellAtIndex(size_t index) -> CellRef {
  starAssert(index < m_width * m_height);
  return CellRef(*this, index);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::calculate(size_t xMin, size_t yMin, size_t xMax, size_t yMax) {
  setSpreadLightingPoints();
  calculateLightSpread(xMin, yMin, xMax, yMax);
  calculatePointLighting(xMin, yMin, xMax, yMax);
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::setSpreadLightingPoints() {
  for (SpreadLight const& light : m_spreadLights) {
    // - 0.5f to correct for lights being on the grid corners and not center
    int minX = floor(light.position[0] - 0.5f);
    int minY = floor(light.position[1] - 0.5f);
    int maxX = minX + 1;
    int maxY = minY + 1;

    float xdist = light.position[0] - minX - 0.5f;
    float ydist = light.position[1] - minY - 0.5f;

    // Pick falloff here based on closest block obstacle value (probably not
    // best)
    Vec2I pos(light.position.floor());
    float oneBlockAtt;
    if (pos[0] >= 0 && pos[0] < (int)m_width && pos[1] >= 0 && pos[1] < (int)m_height && getObstacle(pos[0], pos[1]))
      oneBlockAtt = 1.0f / m_spreadMaxObstacle;
    else
      oneBlockAtt = 1.0f / m_spreadMaxAir;

    // "pre fall-off" a 2x2 area of blocks to smooth out floating point
    // positions using the cellular algorithm

    if (minX >= 0 && minX < (int)m_width && minY >= 0 && minY < (int)m_height)
      setLight(minX, minY, LightTraits::max(getLight(minX, minY), LightTraits::subtract(light.value, oneBlockAtt * (2.0f - (1.0f - xdist) - (1.0f - ydist)))));

    if (minX >= 0 && minX < (int)m_width && maxY >= 0 && maxY < (int)m_height)
      setLight(minX, maxY, LightTraits::max(getLight(minX, maxY), LightTraits::subtract(light.value, oneBlockAtt * (2.0f - (1.0f - xdist) - (ydist)))));

    if (maxX >= 0 && maxX < (int)m_width && minY >= 0 && minY < (int)m_height)
      setLight(maxX, minY, LightTraits::max(getLight(maxX, minY), LightTraits::subtract(light.value, oneBlockAtt * (2.0f - (xdist) - (1.0f - ydist)))));

    if (maxX >= 0 && maxX < (int)m_width && maxY >= 0 && maxY < (int)m_height)
      setLight(maxX, maxY, LightTraits::max(getLight(maxX, maxY), LightTraits::subtract(light.value, oneBlockAtt * (2.0f - (xdist) - (ydist)))));
  }
}

template <typename LightTraits>
void CellularLightArray<LightTraits>::calculateLightSpread(size_t xMin, size_t yMin, size_t xMax, size_t yMax) {
  starAssert(m_width > 0 && m_height > 0);

  float dropoffAir = 1.0f / m_spreadMaxAir;
  float dropoffObstacle = 1.0f / m_spreadMaxObstacle;
  float dropoffAirDiag = 1.0f / m_spreadMaxAir * Constants::sqrt2;
  float dropoffObstacleDiag = 1.0f / m_spreadMaxObstacle * Constants::sqrt2;

  // enlarge x/y min/max taking into ambient spread of light
  xMin = xMin - min(xMin, (size_t)ceil(m_spreadMaxAir));
  yMin = yMin - min(yMin, (size_t)ceil(m_spreadMaxAir));
  xMax = min(m_width, xMax + (size_t)ceil(m_spreadMaxAir));
  yMax = min(m_height, yMax + (size_t)ceil(m_spreadMaxAir));

  for (unsigned p = 0; p < m_spreadPasses; ++p) {
    // Drop factors are computed once per source cell from all channels, then
    // applied per channel (SoA arrays).
    // Spread right and up and diag up right / diag down right
    for (size_t x = xMin + 1; x < xMax - 1; ++x) {
      size_t xCellOffset = x * m_height;
      size_t xRightCellOffset = (x + 1) * m_height;

      for (size_t y = yMin + 1; y < yMax - 1; ++y) {
        size_t i = xCellOffset + y;
        float src[ComponentCount];
        for (size_t cc = 0; cc < ComponentCount; ++cc)
          src[cc] = m_lightChannels[cc][i];
        bool obstacle = m_obstacles[i] != 0;
        float drops[ComponentCount];
        LightTraits::spreadDrops(src, obstacle ? dropoffObstacle : dropoffAir, drops);
        float diagDrops[ComponentCount];
        LightTraits::spreadDrops(src, obstacle ? dropoffObstacleDiag : dropoffAirDiag, diagDrops);

        for (size_t c = 0; c < ComponentCount; ++c) {
          float* light = m_lightChannels[c].data();
          float s = src[c];
          light[i + 1] = std::max(s - drops[c], light[i + 1]);
          light[i + m_height] = std::max(s - drops[c], light[i + m_height]);
          light[i + m_height + 1] = std::max(s - diagDrops[c], light[i + m_height + 1]);
          light[i + m_height - 1] = std::max(s - diagDrops[c], light[i + m_height - 1]);
        }
      }
    }

    // Spread left and down and diag up left / diag down left
    for (size_t x = xMax - 2; x > xMin; --x) {
      size_t xCellOffset = x * m_height;
      size_t xLeftCellOffset = (x - 1) * m_height;

      for (size_t y = yMax - 2; y > yMin; --y) {
        size_t i = xCellOffset + y;
        float src[ComponentCount];
        for (size_t cc = 0; cc < ComponentCount; ++cc)
          src[cc] = m_lightChannels[cc][i];
        bool obstacle = m_obstacles[i] != 0;
        float drops[ComponentCount];
        LightTraits::spreadDrops(src, obstacle ? dropoffObstacle : dropoffAir, drops);
        float diagDrops[ComponentCount];
        LightTraits::spreadDrops(src, obstacle ? dropoffObstacleDiag : dropoffAirDiag, diagDrops);

        for (size_t c = 0; c < ComponentCount; ++c) {
          float* light = m_lightChannels[c].data();
          float s = src[c];
          light[i - 1] = std::max(s - drops[c], light[i - 1]);
          light[i - m_height] = std::max(s - drops[c], light[i - m_height]);
          light[i - m_height + 1] = std::max(s - diagDrops[c], light[i - m_height + 1]);
          light[i - m_height - 1] = std::max(s - diagDrops[c], light[i - m_height - 1]);
        }
      }
    }
  }
}

}// namespace Star
