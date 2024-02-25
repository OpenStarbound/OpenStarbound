#pragma once

#include "StarDrawable.hpp"
#include "StarRenderer.hpp"
#include "StarAssetTextureGroup.hpp"

namespace Star {

STAR_CLASS(DrawablePainter);

class DrawablePainter {
public:
  DrawablePainter(RendererPtr renderer, AssetTextureGroupPtr textureGroup);

  void drawDrawable(Drawable const& drawable);

  void cleanup(int64_t textureTimeout);

private:
  RendererPtr m_renderer;
  AssetTextureGroupPtr m_textureGroup;
};

}
