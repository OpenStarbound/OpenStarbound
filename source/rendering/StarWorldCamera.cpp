#include "StarWorldCamera.hpp"

namespace Star {

void WorldCamera::setCenterWorldPosition(Vec2F const& position, bool force) {
  m_rawWorldCenter = position;
  // Only actually move the world center if a half pixel distance has been
  // moved in any direction.  This is sort of arbitrary, but helps prevent
  // judder if the camera is at a boundary and floating point inaccuracy is
  // causing the focus to jitter back and forth across the boundary.
  if (fabs(position[0] - m_worldCenter[0]) < 1.0f / (TilePixels * m_pixelRatio * 2)
   && fabs(position[1] - m_worldCenter[1]) < 1.0f / (TilePixels * m_pixelRatio * 2) && !force)
    return;

  // First, make sure the camera center position is inside the main x
  // coordinate bounds, and that the top and bototm of the screen are not
  // outside of the y coordinate bounds.
  m_worldCenter = m_worldGeometry.xwrap(position);
  m_worldCenter[1] = clamp(m_worldCenter[1],
      (float)m_screenSize[1] / (TilePixels * m_pixelRatio * 2),
      m_worldGeometry.height() - (float)m_screenSize[1] / (TilePixels * m_pixelRatio * 2));

  // Then, position the camera center position so that the tile grid is as
  // close as possible aligned to whole pixel boundaries.  This is incredibly
  // important, because this means that even without any complicated rounding,
  // elements drawn in world space that are aligned with TilePixels will
  // eventually also be aligned to real screen pixels.

  float ratio = TilePixels * m_pixelRatio;

  if (m_screenSize[0] % 2 == 0)
    m_worldCenter[0] = round(m_worldCenter[0] * ratio) / ratio;
  else
    m_worldCenter[0] = (round(m_worldCenter[0] * ratio + 0.5f) - 0.5f) / ratio;

  if (m_screenSize[1] % 2 == 0)
    m_worldCenter[1] = round(m_worldCenter[1] * ratio) / ratio;
  else
    m_worldCenter[1] = (round(m_worldCenter[1] * ratio + 0.5f) - 0.5f) / ratio;
}

}
