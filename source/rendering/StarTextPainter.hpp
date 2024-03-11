#pragma once

#include "StarFontTextureGroup.hpp"
#include "StarAnchorTypes.hpp"
#include "StarRoot.hpp"
#include "StarStringView.hpp"

namespace Star {

STAR_CLASS(TextPainter);

enum class FontMode {
  Normal,
  Shadow,
  Rainbow
};

float const DefaultLineSpacing = 1.3f;

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
  int stringWidth(StringView s);


  typedef function<bool(StringView, int)> WrapTextCallback;
  bool processWrapText(StringView s, unsigned* wrapWidth, WrapTextCallback textFunc);

  List<StringView> wrapTextViews(StringView s, Maybe<unsigned> wrapWidth);
  StringList wrapText(StringView s, Maybe<unsigned> wrapWidth);

  unsigned fontSize() const;
  void setFontSize(unsigned size);
  void setLineSpacing(float lineSpacing);
  void setMode(FontMode mode);
  void setFontColor(Vec4B color);
  void setProcessingDirectives(StringView directives);
  void setFont(String const& font);
  void addFont(FontPtr const& font, String const& name);
  void reloadFonts();

  void cleanup(int64_t textureTimeout);
  void applyCommands(StringView unsplitCommands);
private:
  struct RenderSettings {
    FontMode mode;
    Vec4B color;
    String font;
    Directives directives;
  };

  RectF doRenderText(StringView s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderLine(StringView s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderGlyph(String::Char c, TextPositioning const& position, bool reallyRender);

  void renderGlyph(String::Char c, Vec2F const& screenPos, unsigned fontSize, float scale, Vec4B const& color, Directives const* processingDirectives = nullptr);
  static FontPtr loadFont(String const& fontPath, Maybe<String> fontName = {});

  RendererPtr m_renderer;
  FontTextureGroup m_fontTextureGroup;

  unsigned m_fontSize;
  float m_lineSpacing;

  RenderSettings m_renderSettings;
  RenderSettings m_savedRenderSettings;

  String m_nonRenderedCharacters;

  TrackerListenerPtr m_reloadTracker;
};

}
