#pragma once

#include "StarFontTextureGroup.hpp"
#include "StarAnchorTypes.hpp"
#include "StarRoot.hpp"
#include "StarStringView.hpp"
#include "StarText.hpp"

namespace Star {

// deprecated in favor of explicit shadow color
enum class FontMode : uint8_t {
  Normal,
  Shadow
};

inline Color const& fontModeToColor(FontMode mode) {
  return mode == FontMode::Shadow ? Color::Black : Color::Clear;
}

STAR_CLASS(TextPainter);

struct TextPositioning {
  TextPositioning();

  TextPositioning(Vec2F pos,
      HorizontalAnchor hAnchor = HorizontalAnchor::LeftAnchor,
      VerticalAnchor vAnchor = VerticalAnchor::BottomAnchor,
      Maybe<unsigned> wrapWidth = {},
      Maybe<unsigned> charLimit = {});

  TextPositioning(Json const& v);
  Json toJson() const;

  TextPositioning translated(Vec2F translation) const;

  Vec2F pos;
  HorizontalAnchor hAnchor;
  VerticalAnchor vAnchor;
  Maybe<unsigned> wrapWidth;
  Maybe<unsigned> charLimit;
};

// Renders text while caching individual glyphs for fast rendering but with *no
// kerning*.
class TextPainter {
public:
  TextPainter(RendererPtr renderer, TextureGroupPtr textureGroup);

  RectF renderText(StringView s, TextPositioning const& position);
  RectF renderLine(StringView s, TextPositioning const& position);
  RectF renderGlyph(String::Char c, TextPositioning const& position);

  RectF determineTextSize(StringView s, TextPositioning const& position);
  RectF determineLineSize(StringView s, TextPositioning const& position);
  RectF determineGlyphSize(String::Char c, TextPositioning const& position);

  int glyphWidth(String::Char c);
  int stringWidth(StringView s, unsigned charLimit = 0);

    
  typedef function<bool(StringView, unsigned)> WrapTextCallback;
  bool processWrapText(StringView s, unsigned* wrapWidth, WrapTextCallback textFunc);

  List<StringView> wrapTextViews(StringView s, Maybe<unsigned> wrapWidth);
  StringList wrapText(StringView s, Maybe<unsigned> wrapWidth);

  unsigned fontSize() const;
  void setFontSize(unsigned size);
  void setLineSpacing(float lineSpacing);
  void setMode(FontMode mode);
  void setFontColor(Vec4B color);
  void setProcessingDirectives(StringView directives, bool back = false);
  void setFont(String const& font);
  TextStyle& setTextStyle(TextStyle const& textStyle);
  void clearTextStyle();
  void addFont(FontPtr const& font, String const& name);
  void reloadFonts();

  void cleanup(int64_t textureTimeout);
  void applyCommands(StringView unsplitCommands);
private:
  void modifyDirectives(Directives& directives);
  RectF doRenderText(StringView s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderLine(StringView s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderGlyph(String::Char c, TextPositioning const& position, bool reallyRender);

  void renderPrimitives();
  void renderGlyph(String::Char c, Vec2F const& screenPos, List<RenderPrimitive>& out, unsigned fontSize, float scale, Vec4B color, Directives const* processingDirectives = nullptr);
  static FontPtr loadFont(String const& fontPath, Maybe<String> fontName = {});

  RendererPtr m_renderer;
  List<RenderPrimitive> m_shadowPrimitives;
  List<RenderPrimitive> m_backPrimitives;
  List<RenderPrimitive> m_frontPrimitives;
  FontTextureGroup m_fontTextureGroup;

  TextStyle m_defaultRenderSettings;
  TextStyle m_renderSettings;
  TextStyle m_savedRenderSettings;

  String m_nonRenderedCharacters;
  TrackerListenerPtr m_reloadTracker;
};

}
