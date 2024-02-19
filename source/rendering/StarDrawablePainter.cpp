#include "StarDrawablePainter.hpp"

namespace Star {

DrawablePainter::DrawablePainter(RendererPtr renderer, AssetTextureGroupPtr textureGroup) {
  m_renderer = std::move(renderer);
  m_textureGroup = std::move(textureGroup);
}

void DrawablePainter::drawDrawable(Drawable const& drawable) {
  Vec4B color = drawable.color.toRgba();
  auto& primitives = m_renderer->immediatePrimitives();

  if (auto linePart = drawable.part.ptr<Drawable::LinePart>()) {
    auto line = linePart->line;
    line.translate(drawable.position);
    Vec2F left = Vec2F(vnorm(line.diff())).rot90() * linePart->width / 2.0f;
    
    float fullbright = drawable.fullbright ? 0.0f : 1.0f;
    auto& primitive = primitives.emplace_back(std::in_place_type_t<RenderQuad>(),
      line.min() + left,
      line.min() - left,
      line.max() - left,
      line.max() + left,
      color, fullbright);
    if (auto* endColor = linePart->endColor.ptr()) {
      RenderQuad& quad = primitive.get<RenderQuad>();
      quad.c.color = quad.d.color = endColor->toRgba();
    }
  } else if (auto polyPart = drawable.part.ptr<Drawable::PolyPart>()) {
    PolyF poly = polyPart->poly;
    poly.translate(drawable.position);

    primitives.emplace_back(std::in_place_type_t<RenderPoly>(), poly.vertexes(), color, 0.0f);

  } else if (auto imagePart = drawable.part.ptr<Drawable::ImagePart>()) {
    TexturePtr texture = m_textureGroup->loadTexture(imagePart->image);

    Vec2F textureSize(texture->size());
    RectF imageRect(Vec2F(), textureSize);

    Mat3F transformation = Mat3F::translation(drawable.position) * imagePart->transformation;

    Vec2F lowerLeft = transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMin()));
    Vec2F lowerRight = transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMin()));
    Vec2F upperRight = transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMax()));
    Vec2F upperLeft = transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMax()));

    float param1 = drawable.fullbright ? 0.0f : 1.0f;

    primitives.emplace_back(std::in_place_type_t<RenderQuad>(), std::move(texture),
        lowerLeft,  Vec2F{0, 0},
        lowerRight, Vec2F{textureSize[0], 0},
        upperRight, Vec2F{textureSize[0], textureSize[1]},
        upperLeft,  Vec2F{0, textureSize[1]},
      color, param1);
  }
}

void DrawablePainter::cleanup(int64_t textureTimeout) {
  m_textureGroup->cleanup(textureTimeout);
}

}
