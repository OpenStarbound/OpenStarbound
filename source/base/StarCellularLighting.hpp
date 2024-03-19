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

STAR_EXCEPTION(LightmapException, StarException);

class Lightmap {
public:
  Lightmap();
  Lightmap(unsigned width, unsigned height);
  Lightmap(Lightmap const& lightMap);
  Lightmap(Lightmap&& lightMap) noexcept;
  
  Lightmap& operator=(Lightmap const& lightMap);
  Lightmap& operator=(Lightmap&& lightMap) noexcept;

  operator ImageView();

  void set(unsigned x, unsigned y, float v);
  void set(unsigned x, unsigned y, Vec3F const& v);
  Vec3F get(unsigned x, unsigned y) const;

  bool empty() const;

  Vec2U size() const;
  unsigned width() const;
  unsigned height() const;
  float* data();

private:
  size_t len() const;

  std::unique_ptr<float[]> m_data;
  unsigned m_width;
  unsigned m_height;
};

inline void Lightmap::set(unsigned x, unsigned y, float v) {
  if (x >= m_width || y >= m_height) {
    throw LightmapException(strf("[{}, {}] out of range in Lightmap::set", x, y));
    return;
  }
  float* ptr = m_data.get() + (y * m_width * 3 + x * 3);
  ptr[0] = ptr[1] = ptr[2] = v;
}

inline void Lightmap::set(unsigned x, unsigned y, Vec3F const& v) {
  if (x >= m_width || y >= m_height) {
    throw LightmapException(strf("[{}, {}] out of range in Lightmap::set", x, y));
    return;
  }
  float* ptr = m_data.get() + (y * m_width * 3 + x * 3);
  ptr[0] = v.x();
  ptr[1] = v.y();
  ptr[2] = v.z();
}


inline Vec3F Lightmap::get(unsigned x, unsigned y) const {
  if (x >= m_width || y >= m_height) {
    throw LightmapException(strf("[{}, {}] out of range in Lightmap::get", x, y));
    return Vec3F();
  }
  float* ptr = m_data.get() + (y * m_width * 3 + x * 3);
  return Vec3F(ptr[0], ptr[1], ptr[2]);
}

inline bool Lightmap::empty() const {
  return m_width == 0 || m_height == 0;
}

inline Vec2U Lightmap::size() const {
  return { m_width, m_height };
}

inline unsigned Lightmap::width() const {
  return m_width;
}

inline unsigned Lightmap::height() const {
  return m_height;
}

inline float* Lightmap::data() {
  return m_data.get();
}

inline size_t Lightmap::len() const {
  return m_width * m_height * 3;
}

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
  // Same as above, but the color data in a float buffer instead.
  void calculate(Lightmap& output);

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
