#ifndef STAR_TEXT_PAINTER_HPP
#define STAR_TEXT_PAINTER_HPP

#include "StarFontTextureGroup.hpp"
#include "StarAnchorTypes.hpp"
#include "StarRoot.hpp"

namespace Star {

STAR_CLASS(TextPainter);

namespace Text {
  unsigned char const StartEsc = '\x1b';
  unsigned char const EndEsc = ';';
  unsigned char const CmdEsc = '^';
  unsigned char const SpecialCharLimit = ' ';

  String stripEscapeCodes(String const& s);
  String preprocessEscapeCodes(String const& s);
  String extractCodes(String const& s);
}

enum class FontMode {
  Normal,
  Shadow
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

  RectF renderText(String const& s, TextPositioning const& position);
  RectF renderLine(String const& s, TextPositioning const& position);
  RectF renderGlyph(String::Char c, TextPositioning const& position);

  RectF determineTextSize(String const& s, TextPositioning const& position);
  RectF determineLineSize(String const& s, TextPositioning const& position);
  RectF determineGlyphSize(String::Char c, TextPositioning const& position);

  int glyphWidth(String::Char c);
  int stringWidth(String const& s);

  StringList wrapText(String const& s, Maybe<unsigned> wrapWidth);

  unsigned fontSize() const;
  void setFontSize(unsigned size);
  void setLineSpacing(float lineSpacing);
  void setMode(FontMode mode);
  void setSplitIgnore(String const& splitIgnore);
  void setFontColor(Vec4B color);
  void setProcessingDirectives(String directives);
  void setFont(String const& font);
  void addFont(FontPtr const& font, String const& name);
  void reloadFonts();

  void cleanup(int64_t textureTimeout);
  void applyCommands(String const& unsplitCommands);
private:
  struct RenderSettings {
    FontMode mode;
    Vec4B color;
    String font;
    String directives;
  };

  RectF doRenderText(String const& s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderLine(String const& s, TextPositioning const& position, bool reallyRender, unsigned* charLimit);
  RectF doRenderGlyph(String::Char c, TextPositioning const& position, bool reallyRender);

  void renderGlyph(String::Char c, Vec2F const& screenPos, unsigned fontSize, float scale, Vec4B const& color, String const& processingDirectives);

  RendererPtr m_renderer;
  FontTextureGroup m_fontTextureGroup;

  unsigned m_fontSize;
  float m_lineSpacing;

  RenderSettings m_renderSettings;
  RenderSettings m_savedRenderSettings;

  String m_splitIgnore;
  String m_splitForce;
  String m_nonRenderedCharacters;

  TrackerListenerPtr m_reloadTracker;
};

}

#endif
