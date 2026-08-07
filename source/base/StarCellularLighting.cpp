#include "StarCellularLighting.hpp"

namespace Star {

Lightmap::Lightmap() : m_width(0), m_height(0) {}

Lightmap::Lightmap(unsigned width, unsigned height) : m_width(width), m_height(height) {
  m_data = std::make_unique<float[]>(len());
}

Lightmap::Lightmap(Lightmap const& lightMap) {
  operator=(lightMap);
}

Lightmap::Lightmap(Lightmap&& lightMap) noexcept {
  operator=(std::move(lightMap));
}

Lightmap& Lightmap::operator=(Lightmap const& lightMap) {
  m_width = lightMap.m_width;
  m_height = lightMap.m_height;
  if (lightMap.m_data) {
    m_data = std::make_unique<float[]>(len());
    memcpy(m_data.get(), lightMap.m_data.get(), len());
  }
  return *this;
}

Lightmap& Lightmap::operator=(Lightmap&& lightMap) noexcept {
  m_width = take(lightMap.m_width);
  m_height = take(lightMap.m_height);
  m_data = take(lightMap.m_data);
  return *this;
}

Lightmap::operator ImageView() {
  ImageView view;
  view.data = (uint8_t*)m_data.get();
  view.size = size();
  view.format = PixelFormat::RGB_F;
  return view;
}

CellularLightingCalculator::CellularLightingCalculator(bool monochrome)
    : m_monochrome(monochrome) {
  if (monochrome)
    m_lightArray.setRight(ScalarCellularLightArray());
  else
    m_lightArray.setLeft(ColoredCellularLightArray());
}

void CellularLightingCalculator::setMonochrome(bool monochrome) {
  if (monochrome == m_monochrome)
    return;

  m_monochrome = monochrome;
  if (monochrome)
    m_lightArray.setRight(ScalarCellularLightArray());
  else
    m_lightArray.setLeft(ColoredCellularLightArray());

  if (m_config)
    setParameters(m_config);
}

void CellularLightingCalculator::setParameters(Json const& config) {
  m_config = config;
  if (m_monochrome)
    m_lightArray.right().setParameters(
      config.getInt("spreadPasses"),
      config.getFloat("spreadMaxAir"),
      config.getFloat("spreadMaxObstacle"),
      config.getFloat("pointMaxAir"),
      config.getFloat("pointMaxObstacle"),
      config.getFloat("pointObstacleBoost"),
      config.getBool("pointAdditive", false));
  else
    m_lightArray.left().setParameters(
      config.getInt("spreadPasses"),
      config.getFloat("spreadMaxAir"),
      config.getFloat("spreadMaxObstacle"),
      config.getFloat("pointMaxAir"),
      config.getFloat("pointMaxObstacle"),
      config.getFloat("pointObstacleBoost"),
      config.getBool("pointAdditive", false));
}

void CellularLightingCalculator::begin(RectI const& queryRegion) {
  m_queryRegion = queryRegion;
  m_pendingPointLights.clear();
  if (m_monochrome) {
    m_calculationRegion = RectI(queryRegion).padded((int)m_lightArray.right().borderCells());
    m_lightArray.right().begin(m_calculationRegion.width(), m_calculationRegion.height());
  } else {
    m_calculationRegion = RectI(queryRegion).padded((int)m_lightArray.left().borderCells());
    m_lightArray.left().begin(m_calculationRegion.width(), m_calculationRegion.height());
  }
}

RectI CellularLightingCalculator::calculationRegion() const {
  return m_calculationRegion;
}

void CellularLightingCalculator::addSpreadLight(Vec2F const& position, Vec3F const& light) {
  Vec2F arrayPosition = position - Vec2F(m_calculationRegion.min());
  if (m_monochrome)
    m_lightArray.right().addSpreadLight({arrayPosition, light.max()});
  else
    m_lightArray.left().addSpreadLight({arrayPosition, light});
}

void CellularLightingCalculator::addPointLight(Vec2F const& position, Vec3F const& light, float beam, float beamAngle, float beamAmbience, bool asSpread) {
  Vec2F arrayPosition = position - Vec2F(m_calculationRegion.min());
  if (m_monochrome)
    m_lightArray.right().addPointLight({arrayPosition, light.max(), beam, beamAngle, beamAmbience, asSpread});
  else
    m_lightArray.left().addPointLight({arrayPosition, light, beam, beamAngle, beamAmbience, asSpread});
  m_pendingPointLights.append({arrayPosition, light, beam, beamAngle, beamAmbience, asSpread});
}

void CellularLightingCalculator::calculate(Image& output) {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  if (m_monochrome)
    m_lightArray.right().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);
  else
    m_lightArray.left().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);

  m_lastPointLights = std::move(m_pendingPointLights);
  m_hasLastPointLights = true;

  output.reset(arrayMax[0] - arrayMin[0], arrayMax[1] - arrayMin[1], PixelFormat::RGB24);

  if (m_monochrome) {
    for (size_t x = arrayMin[0]; x < arrayMax[0]; ++x) {
      for (size_t y = arrayMin[1]; y < arrayMax[1]; ++y) {
        output.set24(x - arrayMin[0], y - arrayMin[1], Color::grayf(m_lightArray.right().getLight(x, y)).toRgb());
      }
    }
  } else {
    for (size_t x = arrayMin[0]; x < arrayMax[0]; ++x) {
      for (size_t y = arrayMin[1]; y < arrayMax[1]; ++y) {
        output.set24(x - arrayMin[0], y - arrayMin[1], Color::v3fToByte(m_lightArray.left().getLight(x, y)));
      }
    }
  }
}

void CellularLightingCalculator::calculate(Lightmap& output) {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  if (m_monochrome)
    m_lightArray.right().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);
  else
    m_lightArray.left().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);

  m_lastPointLights = std::move(m_pendingPointLights);
  m_hasLastPointLights = true;

  writeOutput(output);
}

List<RectI> CellularLightingCalculator::scroll(RectI const& newQueryRegion) {
  Vec2I delta = newQueryRegion.min() - m_queryRegion.min();
  m_queryRegion = newQueryRegion;
  if (m_monochrome)
    m_calculationRegion = RectI(newQueryRegion).padded((int)m_lightArray.right().borderCells());
  else
    m_calculationRegion = RectI(newQueryRegion).padded((int)m_lightArray.left().borderCells());

  if (m_monochrome)
    m_lightArray.right().scroll(delta);
  else
    m_lightArray.left().scroll(delta);

  // The array coordinates of the lights shift together with the data.
  for (auto& light : m_lastPointLights)
    light.position -= Vec2F(delta);

  // The scrolled path re-fills the light lists via addLight* like begin()
  // would.
  m_pendingPointLights.clear();
  if (m_monochrome)
    m_lightArray.right().clearLights();
  else
    m_lightArray.left().clearLights();

  // Newly exposed area of the array (data landed in the overlap):
  // x/y in [max(0, -delta), min(size, size - delta)) stayed valid.
  int w = m_calculationRegion.width();
  int h = m_calculationRegion.height();
  int validX0 = std::max(0, -delta[0]);
  int validX1 = std::min(w, w - delta[0]);
  int validY0 = std::max(0, -delta[1]);
  int validY1 = std::min(h, h - delta[1]);

  List<RectI> exposed;
  auto appendStrip = [&](RectI const& arrayRect) {
    exposed.append(arrayRect.translated(m_calculationRegion.min()));
  };
  if (delta[0] > 0)
    appendStrip(RectI(validX1, 0, w, h));
  else if (delta[0] < 0)
    appendStrip(RectI(0, 0, validX0, h));
  if (delta[1] > 0)
    appendStrip(RectI(validX0, validY1, validX1, h));
  else if (delta[1] < 0)
    appendStrip(RectI(validX0, 0, validX1, validY0));

  // Window cells that entered from the border of the old calculation region:
  // their point values were never maintained by the window-scoped point
  // phase, so the scrolled path re-gathers and re-floods them like the
  // exposed strips (calculateScrolled zeroes the point channels first).
  // The bands are disjoint from the strips (they lie inside the old
  // calculation region) and from each other.
  RectI oldQuery = newQueryRegion.translated(-delta);
  if (delta[0] > 0)
    exposed.append(RectI(oldQuery.xMax(), newQueryRegion.yMin(), newQueryRegion.xMax(), newQueryRegion.yMax()));
  else if (delta[0] < 0)
    exposed.append(RectI(newQueryRegion.xMin(), newQueryRegion.yMin(), oldQuery.xMin(), newQueryRegion.yMax()));
  if (delta[1] > 0) {
    int x0 = delta[0] > 0 ? newQueryRegion.xMin() : oldQuery.xMin();
    int x1 = delta[0] > 0 ? oldQuery.xMax() : newQueryRegion.xMax();
    exposed.append(RectI(x0, oldQuery.yMax(), x1, newQueryRegion.yMax()));
  } else if (delta[1] < 0) {
    int x0 = delta[0] > 0 ? newQueryRegion.xMin() : oldQuery.xMin();
    int x1 = delta[0] > 0 ? oldQuery.xMax() : newQueryRegion.xMax();
    exposed.append(RectI(x0, newQueryRegion.yMin(), x1, oldQuery.yMin()));
  }

  return exposed;
}

void CellularLightingCalculator::calculateScrolled(Lightmap& output, List<RectI> const& exposedWorldRegions) {
  List<RectI> strips;
  for (RectI const& r : exposedWorldRegions)
    strips.append(r.translated(-m_calculationRegion.min()));

  if (m_monochrome)
    m_lightArray.right().calculateScrolled(strips);
  else
    m_lightArray.left().calculateScrolled(strips);

  m_lastPointLights = std::move(m_pendingPointLights);
  m_hasLastPointLights = true;

  writeOutput(output);
}

void CellularLightingCalculator::applyPointLightDiff(RectI const& worldRegion) {
  if (worldRegion.xMin() >= worldRegion.xMax() || worldRegion.yMin() >= worldRegion.yMax())
    return;

  RectI arrayRegion = worldRegion.translated(-m_calculationRegion.min());
  size_t x0 = std::max(0, arrayRegion.xMin());
  size_t y0 = std::max(0, arrayRegion.yMin());
  size_t x1 = std::min((size_t)m_calculationRegion.width(), (size_t)arrayRegion.xMax());
  size_t y1 = std::min((size_t)m_calculationRegion.height(), (size_t)arrayRegion.yMax());

  auto compare = [](ColoredCellularLightArray::PointLight const& a, ColoredCellularLightArray::PointLight const& b) {
    if (a.position[0] != b.position[0])
      return a.position[0] < b.position[0];
    if (a.position[1] != b.position[1])
      return a.position[1] < b.position[1];
    for (size_t i = 0; i < 3; ++i)
      if (a.value[i] != b.value[i])
        return a.value[i] < b.value[i];
    if (a.beam != b.beam)
      return a.beam < b.beam;
    if (a.beamAngle != b.beamAngle)
      return a.beamAngle < b.beamAngle;
    if (a.beamAmbience != b.beamAmbience)
      return a.beamAmbience < b.beamAmbience;
    return a.asSpread < b.asSpread;
  };

  List<ColoredCellularLightArray::PointLight> oldLights = std::move(m_lastPointLights);
  List<ColoredCellularLightArray::PointLight> removed;
  List<ColoredCellularLightArray::PointLight> added;
  oldLights.sort(compare);
  m_pendingPointLights.sort(compare);
  size_t i = 0, j = 0;
  while (i < oldLights.size() || j < m_pendingPointLights.size()) {
    if (j >= m_pendingPointLights.size() || (i < oldLights.size() && compare(oldLights[i], m_pendingPointLights[j])))
      removed.append(oldLights[i++]);
    else if (i >= oldLights.size() || compare(m_pendingPointLights[j], oldLights[i]))
      added.append(m_pendingPointLights[j++]);
    else
      ++i, ++j;
  }

  if (m_monochrome) {
    for (auto const& light : removed) {
      ScalarCellularLightArray::PointLight scalar{light.position, light.value.max(), light.beam, light.beamAngle, light.beamAmbience, light.asSpread};
      m_lightArray.right().addPointLightContribution(scalar, -1.0f, x0, y0, x1, y1);
    }
    for (auto const& light : added) {
      ScalarCellularLightArray::PointLight scalar{light.position, light.value.max(), light.beam, light.beamAngle, light.beamAmbience, light.asSpread};
      m_lightArray.right().addPointLightContribution(scalar, 1.0f, x0, y0, x1, y1);
    }
  } else {
    for (auto const& light : removed)
      m_lightArray.left().addPointLightContribution(light, -1.0f, x0, y0, x1, y1);
    for (auto const& light : added)
      m_lightArray.left().addPointLightContribution(light, 1.0f, x0, y0, x1, y1);
  }
}

void CellularLightingCalculator::calculateIncremental(Lightmap& output, RectI const& worldRegion) {
  applyPointLightDiff(worldRegion);

  m_lastPointLights = std::move(m_pendingPointLights);
  m_hasLastPointLights = true;

  writeOutput(output);
}

void CellularLightingCalculator::writeOutput(Lightmap& output) const {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  // Reuse the output buffer when the size did not change (double-buffering in
  // the caller) instead of allocating a fresh Lightmap every frame.
  if (output.width() != arrayMax[0] - arrayMin[0] || output.height() != arrayMax[1] - arrayMin[1])
    output = Lightmap(arrayMax[0] - arrayMin[0], arrayMax[1] - arrayMin[1]);

  float brightnessLimit = m_config.getFloat("brightnessLimit");

  if (m_monochrome) {
    for (size_t x = arrayMin[0]; x < arrayMax[0]; ++x) {
      for (size_t y = arrayMin[1]; y < arrayMax[1]; ++y) {
        auto light = min(m_lightArray.right().getLight(x, y), brightnessLimit);
        output.set(x - arrayMin[0], y - arrayMin[1], light);
      }
    }
  } else {
    for (size_t x = arrayMin[0]; x < arrayMax[0]; ++x) {
      for (size_t y = arrayMin[1]; y < arrayMax[1]; ++y) {
        auto light = m_lightArray.left().getLight(x, y);
        float intensity = ColoredLightTraits::maxIntensity(light);
        if (intensity > brightnessLimit)
          light *= brightnessLimit / intensity;
        output.set(x - arrayMin[0], y - arrayMin[1], light);
      }
    }
  }
}

void CellularLightingCalculator::setupImage(Image& image, PixelFormat format) const {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  image.reset(arrayMax[0] - arrayMin[0], arrayMax[1] - arrayMin[1], format);
}

void CellularLightIntensityCalculator::setParameters(Json const& config) {
  m_lightArray.setParameters(
    config.getInt("spreadPasses"),
    config.getFloat("spreadMaxAir"),
    config.getFloat("spreadMaxObstacle"),
    config.getFloat("pointMaxAir"),
    config.getFloat("pointMaxObstacle"),
    config.getFloat("pointObstacleBoost"),
    config.getBool("pointAdditive", false));
}

void CellularLightIntensityCalculator::begin(Vec2F const& queryPosition) {
  m_queryPosition = queryPosition;
  m_queryRegion = RectI::withSize(Vec2I::floor(queryPosition - Vec2F::filled(0.5f)), Vec2I(2, 2));
  m_calculationRegion = RectI(m_queryRegion).padded((int)m_lightArray.borderCells());

  m_lightArray.begin(m_calculationRegion.width(), m_calculationRegion.height());
}

RectI CellularLightIntensityCalculator::calculationRegion() const {
  return m_calculationRegion;
}

void CellularLightIntensityCalculator::setCell(Vec2I const& position, Cell const& cell) {
  setCellColumn(position, &cell, 1);
}

void CellularLightIntensityCalculator::setCellColumn(Vec2I const& position, Cell const* cells, size_t count) {
  size_t baseIndex = (position[0] - m_calculationRegion.xMin()) * m_calculationRegion.height() + position[1] - m_calculationRegion.yMin();
  for (size_t i = 0; i < count; ++i)
    m_lightArray.cellAtIndex(baseIndex + i) = cells[i];
}

void CellularLightIntensityCalculator::addSpreadLight(Vec2F const& position, float light) {
  Vec2F arrayPosition = position - Vec2F(m_calculationRegion.min());
  m_lightArray.addSpreadLight({arrayPosition, light});
}

void CellularLightIntensityCalculator::addPointLight(Vec2F const& position, float light, float beam, float beamAngle, float beamAmbience) {
  Vec2F arrayPosition = position - Vec2F(m_calculationRegion.min());
  m_lightArray.addPointLight({arrayPosition, light, beam, beamAngle, beamAmbience, false});
}

float CellularLightIntensityCalculator::calculate() {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  m_lightArray.calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);

  // Do 2d lerp to find lighting intensity

  float ll = m_lightArray.getLight(arrayMin[0], arrayMin[1]);
  float lr = m_lightArray.getLight(arrayMin[0] + 1, arrayMin[1]);
  float ul = m_lightArray.getLight(arrayMin[0], arrayMin[1] + 1);
  float ur = m_lightArray.getLight(arrayMin[0] + 1, arrayMin[1] + 1);

  float xl = m_queryPosition[0] - 0.5f - m_queryRegion.xMin();
  float yl = m_queryPosition[1] - 0.5f - m_queryRegion.yMin();

  return lerp(yl, lerp(xl, ll, lr), lerp(xl, ul, ur));
}

}// namespace Star
