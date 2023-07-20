#ifndef STAR_GUI_CONTEXT_HPP
#define STAR_GUI_CONTEXT_HPP

#include "StarApplicationController.hpp"
#include "StarTextPainter.hpp"
#include "StarDrawablePainter.hpp"
#include "StarAssetTextureGroup.hpp"
#include "StarInputEvent.hpp"
#include "StarDrawable.hpp"
#include "StarThread.hpp"
#include "StarGuiTypes.hpp"
#include "StarRenderer.hpp"
#include "StarKeyBindings.hpp"
#include "StarMixer.hpp"

namespace Star {

STAR_EXCEPTION(GuiContextException, StarException);

class GuiContext {
public:
  // Get pointer to the singleton root instance, if it exists.  Otherwise,
  // returns nullptr.
  static GuiContext* singletonPtr();

  // Gets reference to GuiContext singleton, throws GuiContextException if root
  // is not initialized.
  static GuiContext& singleton();

  GuiContext(MixerPtr mixer, ApplicationControllerPtr appController);
  ~GuiContext();

  GuiContext(GuiContext const&) = delete;
  GuiContext& operator=(GuiContext const&) = delete;

  void renderInit(RendererPtr renderer);

  MixerPtr const& mixer() const;
  ApplicationControllerPtr const& applicationController() const;
  RendererPtr const& renderer() const;
  AssetTextureGroupPtr const& assetTextureGroup() const;
  TextPainterPtr const& textPainter() const;

  unsigned windowWidth() const;
  unsigned windowHeight() const;

  Vec2U windowSize() const;
  Vec2U windowInterfaceSize() const;

  int interfaceScale() const;
  void setInterfaceScale(int interfaceScale);

  Maybe<Vec2I> mousePosition(InputEvent const& event, int pixelRatio) const;
  Maybe<Vec2I> mousePosition(InputEvent const& event) const;

  Set<InterfaceAction> actions(InputEvent const& event) const;
  // used to cancel chorded inputs on KeyUp
  Set<InterfaceAction> actionsForKey(Key key) const;
  void refreshKeybindings();

  // Drawing wrappers to internal renderers.  Automatically loads textures before drawing.

  void setInterfaceScissorRect(RectI const& scissor);
  void resetInterfaceScissorRect();

  Vec2U textureSize(AssetPath const& texName);

  void drawQuad(RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));
  void drawQuad(AssetPath const& texName, RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));
  void drawQuad(AssetPath const& texName, Vec2F const& screenPos, float pixelRatio, Vec4B const& color = Vec4B::filled(255));
  void drawQuad(AssetPath const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));

  void drawDrawable(Drawable drawable, Vec2F const& screenPos, float pixelRatio, Vec4B const& color = Vec4B::filled(255));

  void drawLine(Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth = 1);
  void drawPolyLines(PolyF const& poly, Vec4B const& color, float lineWidth = 1);

  void drawTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color);

  void drawInterfaceDrawable(Drawable drawable, Vec2F const& screenPos, Vec4B const& color = Vec4B::filled(255));

  void drawInterfaceLine(Vec2F const& begin, Vec2F const end, Vec4B const& color, float lineWidth = 1);
  void drawInterfacePolyLines(PolyF poly, Vec4B const& color, float lineWidth = 1);

  void drawInterfaceTriangles(List<tuple<Vec2F, Vec2F, Vec2F>> const& triangles, Vec4B const& color);

  void drawInterfaceQuad(RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));
  void drawInterfaceQuad(AssetPath const& texName, Vec2F const& screenPos, Vec4B const& color = Vec4B::filled(255));
  void drawInterfaceQuad(AssetPath const& texName, Vec2F const& screenPos, float scale, Vec4B const& color = Vec4B::filled(255));
  void drawInterfaceQuad(AssetPath const& texName, RectF const& texCoords, RectF const& screenCoords, Vec4B const& color = Vec4B::filled(255));

  void drawImageStretchSet(ImageStretchSet const& imageSet, RectF const& screenPos, GuiDirection direction = GuiDirection::Horizontal, Vec4B const& color = Vec4B::filled(255));

  // Returns true if the hardware cursor was successfully set to the drawable. Generally fails if the Drawable isn't an image part or the image is too big.
  bool trySetCursor(Drawable const& drawable, Vec2I const& offset, int pixelRatio);

  RectF renderText(String const& s, TextPositioning const& positioning);
  RectF renderInterfaceText(String const& s, TextPositioning const& positioning);

  RectF determineTextSize(String const& s, TextPositioning const& positioning);
  RectF determineInterfaceTextSize(String const& s, TextPositioning const& positioning);

  void setFontSize(unsigned size);
  void setFontSize(unsigned size, int pixelRatio);
  void setFontColor(Vec4B const& color);
  void setFontMode(FontMode mode);
  void setFontProcessingDirectives(String const& directives);
  void setFont(String const& font);
  void setDefaultFont();

  void setLineSpacing(float lineSpacing);
  void setDefaultLineSpacing();

  int stringWidth(String const& s);
  int stringInterfaceWidth(String const& s);

  StringList wrapText(String const& s, Maybe<unsigned> wrapWidth);
  StringList wrapInterfaceText(String const& s, Maybe<unsigned> wrapWidth);

  void playAudio(AudioInstancePtr audioInstance);
  void playAudio(String const& audioAsset, int loops = 0, float volume = 1);

  bool shiftHeld() const;
  void setShiftHeld(bool held);

  String getClipboard() const;
  void setClipboard(String text);

  void cleanup();

private:
  static GuiContext* s_singleton;

  MixerPtr m_mixer;
  ApplicationControllerPtr m_applicationController;
  RendererPtr m_renderer;

  AssetTextureGroupPtr m_textureCollection;
  TextPainterPtr m_textPainter;
  DrawablePainterPtr m_drawablePainter;

  KeyBindings m_keyBindings;

  int m_interfaceScale;

  bool m_shiftHeld;
};

}

#endif
