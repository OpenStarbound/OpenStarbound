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

    Vec2F position = drawable.position;
    Vec2F textureSize(texture->size());
    Mat3F transformation = imagePart->transformation;
    Vec2F lowerLeft  = { transformation[0][2] += position.x(), transformation[1][2] += position.y() };
    Vec2F lowerRight = transformation * Vec2F(textureSize.x(), 0.f);
    Vec2F upperRight = transformation * textureSize;
    Vec2F upperLeft  = transformation * Vec2F(0.f, textureSize.y());

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
