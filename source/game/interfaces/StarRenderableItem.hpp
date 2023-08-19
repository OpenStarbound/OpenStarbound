#ifndef STAR_RENDERABLE_ITEM_HPP
#define STAR_RENDERABLE_ITEM_HPP

#include "StarEntityRendering.hpp"

namespace Star {

  STAR_CLASS(RenderableItem);

  class RenderableItem {
  public:
    virtual ~RenderableItem() {}

    virtual void render(RenderCallback* renderCallback, EntityRenderLayer renderLayer) = 0;
  };

}

#endif
