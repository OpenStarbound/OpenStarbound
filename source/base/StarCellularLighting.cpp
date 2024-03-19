#include "StarCellularLighting.hpp"

namespace Star {

CellularLightingCalculator::CellularLightingCalculator(bool monochrome)
    : m_monochrome(monochrome)
{
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
        config.getFloat("pointObstacleBoost")
      );
  else
    m_lightArray.left().setParameters(
        config.getInt("spreadPasses"),
        config.getFloat("spreadMaxAir"),
        config.getFloat("spreadMaxObstacle"),
        config.getFloat("pointMaxAir"),
        config.getFloat("pointMaxObstacle"),
        config.getFloat("pointObstacleBoost")
      );
}

void CellularLightingCalculator::begin(RectI const& queryRegion) {
  m_queryRegion = queryRegion;
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

void CellularLightingCalculator::addPointLight(Vec2F const& position, Vec3F const& light, float beam, float beamAngle, float beamAmbience) {
  Vec2F arrayPosition = position - Vec2F(m_calculationRegion.min());
  if (m_monochrome)
    m_lightArray.right().addPointLight({arrayPosition, light.max(), beam, beamAngle, beamAmbience});
  else
    m_lightArray.left().addPointLight({arrayPosition, light, beam, beamAngle, beamAmbience});
}

void CellularLightingCalculator::calculate(Image& output) {
  Vec2S arrayMin = Vec2S(m_queryRegion.min() - m_calculationRegion.min());
  Vec2S arrayMax = Vec2S(m_queryRegion.max() - m_calculationRegion.min());

  if (m_monochrome)
    m_lightArray.right().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);
  else
    m_lightArray.left().calculate(arrayMin[0], arrayMin[1], arrayMax[0], arrayMax[1]);

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
      config.getFloat("pointObstacleBoost")
    );
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
  m_lightArray.addPointLight({arrayPosition, light, beam, beamAngle, beamAmbience});
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

}
