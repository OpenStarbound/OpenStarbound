#pragma once

#include "StarWorldGeometry.hpp"
#include "StarGameTypes.hpp"
#include "StarInterpolation.hpp"

namespace Star {

class WorldCamera {
public:
  void setScreenSize(Vec2U screenSize);
  Vec2U screenSize() const;

  void setTargetPixelRatio(float targetPixelRatio);
  void setPixelRatio(float pixelRatio);
  float pixelRatio() const;

  void setWorldGeometry(WorldGeometry geometry);
  WorldGeometry worldGeometry() const;

  // Set the camera center position (in world space) to as close to the given
  // location as possible while keeping the screen within world bounds.
  void setCenterWorldPosition(Vec2F const& position, bool force = false);
  // Returns the actual camera position.
  Vec2F centerWorldPosition() const;

  // Transforms world coordinates into one set of screen coordinates.  Since
  // the world is non-euclidean, one world coordinate can transform to
  // potentially an infinite number of screen coordinates.  This will retrun
  // the closest to the center of the screen.
  Vec2F worldToScreen(Vec2F const& worldCoord) const;

  // Assumes top left corner of screen is (0, 0) in screen coordinates.
  Vec2F screenToWorld(Vec2F const& screen) const;

  // Returns screen dimensions in world space.
  RectF worldScreenRect() const;

  // Returns tile dimensions of the tiles that overlap with the screen
  RectI worldTileRect() const;

  // Returns the position of the lower left corner of the lower left tile of
  // worldTileRect, in screen coordinates.
  Vec2F tileMinScreen() const;

  void update(float dt);

private:
  WorldGeometry m_worldGeometry;
  Vec2U m_screenSize;
  float m_pixelRatio = 1.0f;
  float m_targetPixelRatio = 1.0f;
  Vec2F m_worldCenter;
};

inline void WorldCamera::setScreenSize(Vec2U screenSize) {
  m_screenSize = screenSize;
}

inline Vec2U WorldCamera::screenSize() const {
  return m_screenSize;
}

inline void WorldCamera::setTargetPixelRatio(float targetPixelRatio) {
  m_targetPixelRatio = targetPixelRatio;
}

inline void WorldCamera::setPixelRatio(float pixelRatio) {
  m_pixelRatio = m_targetPixelRatio = pixelRatio;
}

inline float WorldCamera::pixelRatio() const {
  return m_pixelRatio;
}

inline void WorldCamera::setWorldGeometry(WorldGeometry geometry) {
  m_worldGeometry = std::move(geometry);
}

inline WorldGeometry WorldCamera::worldGeometry() const {
  return m_worldGeometry;
}

inline Vec2F WorldCamera::centerWorldPosition() const {
  return Vec2F(m_worldCenter);
}

inline Vec2F WorldCamera::worldToScreen(Vec2F const& worldCoord) const {
  Vec2F wrappedCoord = m_worldGeometry.nearestTo(Vec2F(m_worldCenter), worldCoord);
  return Vec2F(
      (wrappedCoord[0] - m_worldCenter[0]) * (TilePixels * m_pixelRatio) + (float)m_screenSize[0] / 2.0,
      (wrappedCoord[1] - m_worldCenter[1]) * (TilePixels * m_pixelRatio) + (float)m_screenSize[1] / 2.0
    );
}

inline Vec2F WorldCamera::screenToWorld(Vec2F const& screen) const {
  return Vec2F(
      (screen[0] - (float)m_screenSize[0] / 2.0) / (TilePixels * m_pixelRatio) + m_worldCenter[0],
      (screen[1] - (float)m_screenSize[1] / 2.0) / (TilePixels * m_pixelRatio) + m_worldCenter[1]
    );
}

inline RectF WorldCamera::worldScreenRect() const {
  // screen dimensions in world space
  float w = (float)m_screenSize[0] / (TilePixels * m_pixelRatio);
  float h = (float)m_screenSize[1] / (TilePixels * m_pixelRatio);
  return RectF::withSize(Vec2F(m_worldCenter[0] - w / 2, m_worldCenter[1] - h / 2), Vec2F(w, h));
}

inline RectI WorldCamera::worldTileRect() const {
  RectF screen = worldScreenRect();
  Vec2I min = Vec2I::floor(screen.min());
  Vec2I size = Vec2I::ceil(Vec2F(m_screenSize) / (TilePixels * m_pixelRatio) + (screen.min() - Vec2F(min)));
  return RectI::withSize(min, size);
}

inline Vec2F WorldCamera::tileMinScreen() const {
  RectF screenRect = worldScreenRect();
  RectI tileRect = worldTileRect();
  return (Vec2F(tileRect.min()) - screenRect.min()) * (TilePixels * m_pixelRatio);
}

inline void WorldCamera::update(float dt) {
  float newPixelRatio = lerp(exp(-20.0f * dt), m_targetPixelRatio, m_pixelRatio);
  if (m_pixelRatio != newPixelRatio) {
    m_pixelRatio = newPixelRatio;
    setCenterWorldPosition(m_worldCenter, true);
  }
}

}
