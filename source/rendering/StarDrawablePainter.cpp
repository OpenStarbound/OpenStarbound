#include "StarDrawablePainter.hpp"

namespace Star {

DrawablePainter::DrawablePainter(RendererPtr renderer, AssetTextureGroupPtr textureGroup) {
  m_renderer = move(renderer);
  m_textureGroup = move(textureGroup);
}

void DrawablePainter::drawDrawable(Drawable const& drawable) {
  Vec4B color = drawable.color.toRgba();

  if (auto linePart = drawable.part.ptr<Drawable::LinePart>()) {
    auto line = linePart->line;
    line.translate(drawable.position);

    Vec2F left = Vec2F(vnorm(line.diff())).rot90() * linePart->width / 2.0f;
    m_renderer->render(RenderQuad{{},
        RenderVertex{Vec2F(line.min()) + left, Vec2F(), color, drawable.fullbright ? 0.0f : 1.0f},
        RenderVertex{Vec2F(line.min()) - left, Vec2F(), color, drawable.fullbright ? 0.0f : 1.0f},
        RenderVertex{Vec2F(line.max()) - left, Vec2F(), color, drawable.fullbright ? 0.0f : 1.0f},
        RenderVertex{Vec2F(line.max()) + left, Vec2F(), color, drawable.fullbright ? 0.0f : 1.0f}
      });

  } else if (auto polyPart = drawable.part.ptr<Drawable::PolyPart>()) {
    auto poly = polyPart->poly;
    poly.translate(drawable.position);

    m_renderer->render(renderFlatPoly(poly, color, 0.0f));

  } else if (auto imagePart = drawable.part.ptr<Drawable::ImagePart>()) {
    TexturePtr texture = m_textureGroup->loadTexture(imagePart->image);

    Vec2F textureSize(texture->size());
    RectF imageRect(Vec2F(), textureSize);

    Mat3F transformation = Mat3F::translation(drawable.position) * imagePart->transformation;

    Vec2F lowerLeft = transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMin()));
    Vec2F lowerRight = transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMin()));
    Vec2F upperRight = transformation.transformVec2(Vec2F(imageRect.xMax(), imageRect.yMax()));
    Vec2F upperLeft = transformation.transformVec2(Vec2F(imageRect.xMin(), imageRect.yMax()));

    m_renderer->render(RenderQuad{move(texture),
        {lowerLeft, {0, 0}, color, drawable.fullbright ? 0.0f : 1.0f},
        {lowerRight, {textureSize[0], 0}, color, drawable.fullbright ? 0.0f : 1.0f},
        {upperRight, {textureSize[0], textureSize[1]}, color, drawable.fullbright ? 0.0f : 1.0f},
        {upperLeft, {0, textureSize[1]}, color, drawable.fullbright ? 0.0f : 1.0f}
      });
  }
}

void DrawablePainter::cleanup(int64_t textureTimeout) {
  m_textureGroup->cleanup(textureTimeout);
}

}
