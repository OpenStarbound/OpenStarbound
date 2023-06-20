#ifndef STAR_CANVAS_WIDGET_HPP
#define STAR_CANVAS_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);

// Very simple Widget that allows easy drawing to its surface, to easily tie a
// simplified rendering / input context into the regular widget / GuiReader
// system.
class CanvasWidget : public Widget {
public:
  struct ClickEvent {
    Vec2I position;
    MouseButton button;
    // True when button down, false when key up
    bool buttonDown;
  };

  struct KeyEvent {
    Key key;
    // True when key down, false when key up
    bool keyDown;
  };

  unsigned MaximumEventBuffer = 16;

  CanvasWidget();

  void setCaptureMouseEvents(bool captureMouse);
  void setCaptureKeyboardEvents(bool captureKeyboard);

  // Returns mouse position relative to the lower left of the drawing region.
  Vec2I mousePosition() const;

  // Pulls recent click events relative to the lower left of the drawing
  // region, if configured to capture mouse events
  List<ClickEvent> pullClickEvents();

  // Pulls recent key events captured by this Canvas, if configured to capture
  // key events.
  List<KeyEvent> pullKeyEvents();

  bool sendEvent(InputEvent const& event) override;
  KeyboardCaptureMode keyboardCaptured() const override;

  // Call before drawing to clear old draw data.
  void clear();

  void drawImage(String texName, Vec2F const& position, float scale = 1.0f, Vec4B const& color = Vec4B(255, 255, 255, 255));
  void drawImageRect(String texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color);
  void drawImageCentered(String texName, Vec2F const& position, float scale = 1.0f, Vec4B const& color = Vec4B(255, 255, 255, 255));

  void drawDrawable(Drawable drawable, Vec2F const& screenPos);
  void drawDrawables(List<Drawable> drawables, Vec2F const& screenPos);

  // Draw an image whose texture is applied over the entire screen rect in a
  // tiled manner, so that it wraps in X and Y.
  void drawTiledImage(String texName, float textureScale, Vec2D const& offset, RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));

  void drawLine(Vec2F const& begin, Vec2F const end, Vec4B const& color = Vec4B(255, 255, 255, 255), float lineWidth = 1.0f);
  void drawRect(RectF const& coords, Vec4B const& color = Vec4B(255, 255, 255, 255));
  void drawPoly(PolyF const& poly, Vec4B const& color = Vec4B(255, 255, 255, 255), float lineWidth = 1.0f);
  void drawTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& poly, Vec4B const& color = Vec4B(255, 255, 255, 255));

  void drawText(String s, TextPositioning position, unsigned fontSize, Vec4B const& color = Vec4B(255, 255, 255, 255), FontMode mode = FontMode::Normal, float lineSpacing = Star::DefaultLineSpacing, String processingDirectives = "");

protected:
  void renderImpl() override;

  void renderImage(Vec2F const& renderingOffset, String const& texName, Vec2F const& position, float scale, Vec4B const& color, bool centered);
  void renderImageRect(Vec2F const& renderingOffset, String const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color);
  void renderDrawable(Vec2F const& renderingOffset, Drawable drawable, Vec2F const& screenPos);
  void renderTiledImage(Vec2F const& renderingOffset, String const& texName, float textureScale, Vec2D const& offset, RectF const& screenCoords, Vec4B const& color);
  void renderLine(Vec2F const& renderingOffset, Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth);
  void renderRect(Vec2F const& renderingOffset, RectF const& coords, Vec4B const& color);
  void renderPoly(Vec2F const& renderingOffset, PolyF poly, Vec4B const& color, float lineWidth);
  void renderTriangles(Vec2F const& renderingOffset, List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color);
  void renderText(Vec2F const& renderingOffset, String const& s, TextPositioning const& position, unsigned fontSize, Vec4B const& color, FontMode mode, float lineSpacing, String const& directives);

private:
  bool m_captureKeyboard;
  bool m_captureMouse;
  Vec2I m_mousePosition;
  List<ClickEvent> m_clickEvents;
  List<KeyEvent> m_keyEvents;

  typedef tuple<RectF, Vec4B> RectOp;
  typedef tuple<String, Vec2F, float, Vec4B, bool> ImageOp;
  typedef tuple<String, RectF, RectF, Vec4B> ImageRectOp;
  typedef tuple<Drawable, Vec2F> DrawableOp;
  typedef tuple<String, float, Vec2D, RectF, Vec4B> TiledImageOp;
  typedef tuple<Vec2F, Vec2F, Vec4B, float> LineOp;
  typedef tuple<PolyF, Vec4B, float> PolyOp;
  typedef tuple<List<tuple<Vec2F, Vec2F, Vec2F>>, Vec4B> TrianglesOp;
  typedef tuple<String, TextPositioning, unsigned, Vec4B, FontMode, float, String> TextOp;

  typedef MVariant<RectOp, ImageOp, ImageRectOp, DrawableOp, TiledImageOp, LineOp, PolyOp, TrianglesOp, TextOp> RenderOp;
  List<RenderOp> m_renderOps;
};

}

#endif
