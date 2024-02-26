#pragma once

#include "StarEither.hpp"
#include "StarRect.hpp"
#include "StarImage.hpp"
#include "StarJson.hpp"
#include "StarColor.hpp"
#include "StarInterpolation.hpp"
#include "StarCellularLightArray.hpp"
#include "StarThread.hpp"

namespace Star {

// Produce lighting values from an integral cellular grid.  Allows for floating
// positional point and cellular light sources, as well as pre-lighting cells
// individually.
class CellularLightingCalculator {
public:
  explicit CellularLightingCalculator(bool monochrome = false);

  typedef ColoredCellularLightArray::Cell Cell;

  void setMonochrome(bool monochrome);

  void setParameters(Json const& config);

  // Call 'begin' to start a calculation for the given region
  void begin(RectI const& queryRegion);

  // Once begin is called, this will return the region that could possibly
  // affect the target calculation region.  All lighting values should be set
  // for the given calculation region before calling 'calculate'.
  RectI calculationRegion() const;

  size_t baseIndexFor(Vec2I const& position);

  void setCellIndex(size_t cellIndex, Vec3F const& light, bool obstacle);

  void addSpreadLight(Vec2F const& position, Vec3F const& light);
  void addPointLight(Vec2F const& position, Vec3F const& light, float beam, float beamAngle, float beamAmbience);

  // Finish the calculation, and put the resulting color data in the given
  // output image.  The image will be reset to the size of the region given in
  // the call to 'begin', and formatted as RGB24.
  void calculate(Image& output);

  void setupImage(Image& image, PixelFormat format = PixelFormat::RGB24) const;
private:
  Json m_config;
  bool m_monochrome;
  Either<ColoredCellularLightArray, ScalarCellularLightArray> m_lightArray;
  RectI m_queryRegion;
  RectI m_calculationRegion;
};

// Produce light intensity values using the same algorithm as
// CellularLightingCalculator.  Only calculates a single point at a time, and
// uses scalar lights with no color calculation.
class CellularLightIntensityCalculator {
public:
  typedef ScalarCellularLightArray::Cell Cell;

  void setParameters(Json const& config);

  void begin(Vec2F const& queryPosition);

  RectI calculationRegion() const;

  void setCell(Vec2I const& position, Cell const& cell);
  void setCellColumn(Vec2I const& position, Cell const* cells, size_t count);

  void addSpreadLight(Vec2F const& position, float light);
  void addPointLight(Vec2F const& position, float light, float beam, float beamAngle, float beamAmbience);

  float calculate();

private:
  ScalarCellularLightArray m_lightArray;
  Vec2F m_queryPosition;
  RectI m_queryRegion;;
  RectI m_calculationRegion;
};

inline size_t CellularLightingCalculator::baseIndexFor(Vec2I const& position) {
  return (position[0] - m_calculationRegion.xMin()) * m_calculationRegion.height() + position[1] - m_calculationRegion.yMin();
}

inline void CellularLightingCalculator::setCellIndex(size_t cellIndex, Vec3F const& light, bool obstacle) {
  if (m_monochrome)
    m_lightArray.right().cellAtIndex(cellIndex) = ScalarCellularLightArray::Cell{light.sum() / 3, obstacle};
  else
    m_lightArray.left().cellAtIndex(cellIndex) = ColoredCellularLightArray::Cell{light, obstacle};
}

}
