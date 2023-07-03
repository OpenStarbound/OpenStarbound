#ifndef STAR_FONT_TEXTURE_GROUP_HPP
#define STAR_FONT_TEXTURE_GROUP_HPP

#include "StarColor.hpp"
#include "StarFont.hpp"
#include "StarRenderer.hpp"

namespace Star {

STAR_CLASS(FontTextureGroup);

class FontTextureGroup {
public:
  // Font* is only included for key uniqueness and should not be dereferenced
  typedef tuple<String::Char, unsigned, String, Font*> GlyphDescriptor;

  struct GlyphTexture {
    TexturePtr texture;
    int64_t time;
    Vec2F offset;
  };

  FontTextureGroup(TextureGroupPtr textureGroup);

  const GlyphTexture& glyphTexture(String::Char, unsigned fontSize, String const& processingDirectives);

  TexturePtr glyphTexturePtr(String::Char, unsigned fontSize);
  TexturePtr glyphTexturePtr(String::Char, unsigned fontSize, String const& processingDirectives);

  unsigned glyphWidth(String::Char c, unsigned fontSize);

  // Removes glyphs that haven't been used in more than the given time in
  // milliseconds
  void cleanup(int64_t timeout);
  // Switches the current font
  void switchFont(String const& font);
  String const& activeFont();
  void addFont(FontPtr const& font, String const& name, bool isDefault = false);
  void clearFonts();
private:
  StringMap<FontPtr> m_fonts;
  String m_fontName;
  FontPtr m_font;
  FontPtr m_defaultFont;

  TextureGroupPtr m_textureGroup;
  HashMap<GlyphDescriptor, GlyphTexture> m_glyphs;
};

}

#endif
