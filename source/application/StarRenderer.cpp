#include "StarRenderer.hpp"

namespace Star {

EnumMap<TextureAddressing> const TextureAddressingNames{
  {TextureAddressing::Clamp, "Clamp"},
  {TextureAddressing::Wrap, "Wrap"}
};

EnumMap<TextureFiltering> const TextureFilteringNames{
  {TextureFiltering::Nearest, "Nearest"},
  {TextureFiltering::Linear, "Linear"}
};

RenderQuad renderTexturedRect(TexturePtr texture, Vec2F minPosition, float textureScale, Vec4B color, float param1) {
  if (!texture)
    throw RendererException("renderTexturedRect called with null texture");

  auto textureSize = Vec2F(texture->size());
  return {
    move(texture),
    RenderVertex{minPosition, Vec2F(0, 0), color, param1},
    RenderVertex{minPosition + Vec2F(textureSize[0], 0) * textureScale, Vec2F(textureSize[0], 0), color, param1},
    RenderVertex{minPosition + Vec2F(textureSize[0], textureSize[1]) * textureScale, Vec2F(textureSize[0], textureSize[1]), color, param1},
    RenderVertex{minPosition + Vec2F(0, textureSize[1]) * textureScale, Vec2F(0, textureSize[1]), color, param1}
  };
}

RenderQuad renderTexturedRect(TexturePtr texture, RectF const& screenCoords, Vec4B color, float param1) {
  if (!texture)
    throw RendererException("renderTexturedRect called with null texture");

  auto textureSize = Vec2F(texture->size());
  return {
    move(texture),
    RenderVertex{{screenCoords.xMin(), screenCoords.yMin()}, Vec2F(0, 0), color, param1},
    RenderVertex{{screenCoords.xMax(), screenCoords.yMin()}, Vec2F(textureSize[0], 0), color, param1},
    RenderVertex{{screenCoords.xMax(), screenCoords.yMax()}, Vec2F(textureSize[0], textureSize[1]), color, param1},
    RenderVertex{{screenCoords.xMin(), screenCoords.yMax()}, Vec2F(0, textureSize[1]), color, param1}
  };
}

RenderQuad renderFlatRect(RectF const& rect, Vec4B color, float param1) {
  return {
    {},
    RenderVertex{{rect.xMin(), rect.yMin()}, {}, color, param1},
    RenderVertex{{rect.xMax(), rect.yMin()}, {}, color, param1},
    RenderVertex{{rect.xMax(), rect.yMax()}, {}, color, param1},
    RenderVertex{{rect.xMin(), rect.yMax()}, {}, color, param1}
  };
}

RenderPoly renderFlatPoly(PolyF const& poly, Vec4B color, float param1) {
  RenderPoly renderPoly;
  for (auto const& v : poly)
    renderPoly.vertexes.append({v, {}, color, param1});
  return renderPoly;
}

}
