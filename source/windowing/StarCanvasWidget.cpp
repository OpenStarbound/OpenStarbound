#include "StarCanvasWidget.hpp"

namespace Star {

CanvasWidget::CanvasWidget() {
  m_ignoreInterfaceScale = m_captureKeyboard = m_captureMouse = false;
}

void CanvasWidget::setCaptureMouseEvents(bool captureMouse) {
  m_captureMouse = captureMouse;
}

void CanvasWidget::setCaptureKeyboardEvents(bool captureKeyboard) {
  m_captureKeyboard = captureKeyboard;
}

void CanvasWidget::setIgnoreInterfaceScale(bool ignoreInterfaceScale) {
  m_ignoreInterfaceScale = ignoreInterfaceScale;
}

bool CanvasWidget::ignoreInterfaceScale() const {
  return m_ignoreInterfaceScale;
}

void CanvasWidget::clear() {
  m_renderOps.clear();
}

void CanvasWidget::drawImage(String texName, Vec2F const& position, float scale, Vec4B const& color) {
  m_renderOps.append(make_tuple(move(texName), position, scale, color, false));
}

void CanvasWidget::drawImageCentered(String texName, Vec2F const& position, float scale, Vec4B const& color) {
  m_renderOps.append(make_tuple(move(texName), position, scale, color, true));
}

void CanvasWidget::drawImageRect(String texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color) {
  m_renderOps.append(make_tuple(move(texName), texCoords, screenCoords, color));
}

void CanvasWidget::drawDrawable(Drawable drawable, Vec2F const& screenPos) {
  m_renderOps.append(make_tuple(move(drawable), screenPos));
}

void CanvasWidget::drawDrawables(List<Drawable> drawables, Vec2F const& screenPos) {
  for (auto& drawable : drawables)
    drawDrawable(move(drawable), screenPos);
}

void CanvasWidget::drawTiledImage(String texName, float textureScale, Vec2D const& offset, RectF const& screenCoords, Vec4B const& color) {
  m_renderOps.append(make_tuple(move(texName), textureScale, offset, screenCoords, color));
}

void CanvasWidget::drawLine(Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth) {
  m_renderOps.append(make_tuple(begin, end, color, lineWidth));
}

void CanvasWidget::drawRect(RectF const& coords, Vec4B const& color) {
  m_renderOps.append(make_tuple(coords, color));
}

void CanvasWidget::drawPoly(PolyF const& poly, Vec4B const& color, float lineWidth) {
  m_renderOps.append(make_tuple(poly, color, lineWidth));
}

void CanvasWidget::drawTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color) {
  m_renderOps.append(make_tuple(triangles, color));
}

void CanvasWidget::drawText(String s, TextPositioning position, unsigned fontSize, Vec4B const& color, FontMode mode, float lineSpacing, String font, String processingDirectives) {
  m_renderOps.append(make_tuple(move(s), move(position), fontSize, color, mode, lineSpacing, move(font), move(processingDirectives)));
}

Vec2I CanvasWidget::mousePosition() const {
  return m_mousePosition;
}

List<CanvasWidget::ClickEvent> CanvasWidget::pullClickEvents() {
  return take(m_clickEvents);
}

List<CanvasWidget::KeyEvent> CanvasWidget::pullKeyEvents() {
  return take(m_keyEvents);
}

bool CanvasWidget::sendEvent(InputEvent const& event) {
  if (!m_visible)
    return false;

  auto& context = GuiContext::singleton();
  int interfaceScale = m_ignoreInterfaceScale ? 1 : context.interfaceScale();
  if (auto mouseButtonDown = event.ptr<MouseButtonDownEvent>()) {
    if (inMember(*context.mousePosition(event, interfaceScale)) && m_captureMouse) {
      m_clickEvents.append({*context.mousePosition(event, interfaceScale) - screenPosition(), mouseButtonDown->mouseButton, true});
      m_clickEvents.limitSizeBack(MaximumEventBuffer);
      return true;
    }
  } else if (auto mouseButtonUp = event.ptr<MouseButtonUpEvent>()) {
    if (m_captureMouse) {
      m_clickEvents.append({*context.mousePosition(event, interfaceScale) - screenPosition(), mouseButtonUp->mouseButton, false});
      m_clickEvents.limitSizeBack(MaximumEventBuffer);
      return true;
    }
  } else if (event.is<MouseMoveEvent>()) {
    m_mousePosition = *context.mousePosition(event, interfaceScale) - screenPosition();
    return false;
  } else if (auto keyDown = event.ptr<KeyDownEvent>()) {
    if (m_captureKeyboard) {
      m_keyEvents.append({keyDown->key, true});
      return true;
    }
  } else if (auto keyUp = event.ptr<KeyUpEvent>()) {
    if (m_captureKeyboard) {
      m_keyEvents.append({keyUp->key, false});
      m_keyEvents.limitSizeBack(MaximumEventBuffer);
      return true;
    }
  }

  return Widget::sendEvent(event);
}

KeyboardCaptureMode CanvasWidget::keyboardCaptured() const {
  return m_captureKeyboard ? KeyboardCaptureMode::KeyEvents : KeyboardCaptureMode::None;
}

void CanvasWidget::renderImpl() {
  auto renderingOffset = Vec2F(screenPosition());

  for (auto const& op : m_renderOps) {
    if (auto args = op.ptr<ImageOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderImage, this, renderingOffset, _1, _2, _3, _4, _5), *args);
    if (auto args = op.ptr<ImageRectOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderImageRect, this, renderingOffset, _1, _2, _3, _4), *args);
    if (auto args = op.ptr<DrawableOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderDrawable, this, renderingOffset, _1, _2), *args);
    if (auto args = op.ptr<TiledImageOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderTiledImage, this, renderingOffset, _1, _2, _3, _4, _5), *args);
    if (auto args = op.ptr<LineOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderLine, this, renderingOffset, _1, _2, _3, _4), *args);
    if (auto args = op.ptr<RectOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderRect, this, renderingOffset, _1, _2), *args);
    if (auto args = op.ptr<PolyOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderPoly, this, renderingOffset, _1, _2, _3), *args);
    if (auto args = op.ptr<TrianglesOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderTriangles, this, renderingOffset, _1, _2), *args);
    if (auto args = op.ptr<TextOp>())
      tupleUnpackFunction(bind(&CanvasWidget::renderText, this, renderingOffset, _1, _2, _3, _4, _5, _6, _7, _8), *args);
  }
}

void CanvasWidget::renderImage(Vec2F const& renderingOffset, String const& texName, Vec2F const& position, float scale, Vec4B const& color, bool centered) {
  auto& context = GuiContext::singleton();
  auto texSize = Vec2F(context.textureSize(texName));
  Vec2F pos = centered ? (position - scale * texSize / 2.0f) : position;

  RectF screenCoords;
  if (m_ignoreInterfaceScale)
    screenCoords = RectF::withSize(renderingOffset + pos, texSize * scale);
  else
    screenCoords = RectF::withSize(renderingOffset * context.interfaceScale() + pos * context.interfaceScale(), texSize * scale * context.interfaceScale());

  context.drawQuad(texName, screenCoords, color);
}

void CanvasWidget::renderImageRect(Vec2F const& renderingOffset, String const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color) {
  auto& context = GuiContext::singleton();
  if (m_ignoreInterfaceScale)
    context.drawQuad(texName, texCoords, screenCoords.translated(renderingOffset), color);
  else
    context.drawQuad(texName, texCoords, screenCoords.scaled(context.interfaceScale()).translated(renderingOffset * context.interfaceScale()), color);
}

void CanvasWidget::renderDrawable(Vec2F const& renderingOffset, Drawable drawable, Vec2F const& screenPos) {
  auto& context = GuiContext::singleton();
  if (m_ignoreInterfaceScale)
    context.drawDrawable(move(drawable), renderingOffset + screenPos, 1);
  else {
    drawable.scale(context.interfaceScale());
    context.drawDrawable(move(drawable), renderingOffset * context.interfaceScale() + screenPos * context.interfaceScale(), 1);
  }
}

void CanvasWidget::renderTiledImage(Vec2F const& renderingOffset, String const& texName, float textureScale, Vec2D const& offset, RectF const& screenCoords, Vec4B const& color) {
  auto& context = GuiContext::singleton();

  Vec2F texSize = Vec2F(context.textureSize(texName));
  Vec2F texScaledSize = texSize * textureScale;
  Vec2I textureCount = Vec2I::ceil(screenCoords.size().piecewiseDivide(texScaledSize)) + Vec2I(2, 2);
  Vec2F screenLowerLeft = screenCoords.min() - Vec2F(pfmod<double>(texScaledSize[0] - offset[0], texScaledSize[0]), pfmod<double>(texScaledSize[1] - offset[1], texScaledSize[1]));

  for (int x = 0; x < textureCount[0]; ++x) {
    for (int y = 0; y < textureCount[1]; ++y) {
      Vec2F screenPos = screenLowerLeft + texScaledSize.piecewiseMultiply({(float)x, (float)y});
      RectF screenRect = RectF::withSize(screenPos, texScaledSize);

      RectF limitedScreenRect;
      limitedScreenRect.setXMin(max(screenRect.xMin(), screenCoords.xMin()));
      limitedScreenRect.setYMin(max(screenRect.yMin(), screenCoords.yMin()));
      limitedScreenRect.setXMax(min(screenRect.xMax(), screenCoords.xMax()));
      limitedScreenRect.setYMax(min(screenRect.yMax(), screenCoords.yMax()));

      RectF limitedTexRect = limitedScreenRect.translated(-screenPos).scaled(1 / textureScale);

      if (limitedScreenRect.isEmpty())
        continue;

      if (m_ignoreInterfaceScale)
        context.drawQuad(texName, limitedTexRect, limitedScreenRect.translated(renderingOffset), color);
      else
        context.drawQuad(texName, limitedTexRect, limitedScreenRect.translated(renderingOffset).scaled(context.interfaceScale()), color);
    }
  }
}

void CanvasWidget::renderLine(Vec2F const& renderingOffset, Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth) {
  auto& context = GuiContext::singleton();
  if (m_ignoreInterfaceScale)
    context.drawLine(renderingOffset + begin, renderingOffset + end, color, lineWidth);
  else {
    context.drawLine(
      renderingOffset * context.interfaceScale() + begin * context.interfaceScale(),
      renderingOffset * context.interfaceScale() + end * context.interfaceScale(),
      color, lineWidth);
  }
}

void CanvasWidget::renderRect(Vec2F const& renderingOffset, RectF const& coords, Vec4B const& color) {
  auto& context = GuiContext::singleton();

  if (m_ignoreInterfaceScale)
    context.drawQuad(coords.translated(renderingOffset), color);
  else
    context.drawQuad(coords.scaled(context.interfaceScale()).translated(renderingOffset * context.interfaceScale()), color);
}

void CanvasWidget::renderPoly(Vec2F const& renderingOffset, PolyF poly, Vec4B const& color, float lineWidth) {
  auto& context = GuiContext::singleton();
  poly.translate(renderingOffset);
  if (m_ignoreInterfaceScale)
    context.drawPolyLines(poly, color, lineWidth);
  else
    context.drawInterfacePolyLines(poly, color, lineWidth);
}

void CanvasWidget::renderTriangles(Vec2F const& renderingOffset, List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color) {
  auto& context = GuiContext::singleton();
  auto translated = triangles.transformed([&renderingOffset](tuple<Vec2F, Vec2F, Vec2F> const& poly) {
      return tuple<Vec2F, Vec2F, Vec2F>(get<0>(poly) + renderingOffset,
        get<1>(poly) + renderingOffset,
        get<2>(poly) + renderingOffset);
    });
  if (m_ignoreInterfaceScale)
    context.drawTriangles(translated, color);
  else
    context.drawInterfaceTriangles(translated, color);
}

void CanvasWidget::renderText(Vec2F const& renderingOffset, String const& s, TextPositioning const& position, unsigned fontSize, Vec4B const& color, FontMode mode, float lineSpacing, String const& font, String const& directives) {
  auto& context = GuiContext::singleton();
  context.setFontProcessingDirectives(directives);
  context.setFontSize(fontSize, m_ignoreInterfaceScale ? 1 : context.interfaceScale());
  context.setFontColor(color);
  context.setFontMode(mode);
  context.setFont(font);
  context.setLineSpacing(lineSpacing);

  TextPositioning translatedPosition = position;
  translatedPosition.pos += renderingOffset;
  if (m_ignoreInterfaceScale)
    context.renderText(s, translatedPosition);
  else
    context.renderInterfaceText(s, translatedPosition);

  context.setDefaultLineSpacing();
  context.setDefaultFont();
  context.setFontMode(FontMode::Normal);
  context.setFontProcessingDirectives("");
}

}
