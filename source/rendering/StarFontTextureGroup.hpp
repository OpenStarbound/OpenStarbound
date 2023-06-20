#ifndef STAR_FONT_TEXTURE_GROUP_HPP
#define STAR_FONT_TEXTURE_GROUP_HPP

#include "StarColor.hpp"
#include "StarFont.hpp"
#include "StarRenderer.hpp"

namespace Star {

STAR_CLASS(FontTextureGroup);

class FontTextureGroup {
public:
  typedef tuple<String::Char, unsigned, String> GlyphDescriptor;

  struct GlyphTexture {
    TexturePtr texture;
    int64_t time;
    Vec2F processingOffset;
  };

  FontTextureGroup(FontPtr font, TextureGroupPtr textureGroup);

  const GlyphTexture& glyphTexture(String::Char, unsigned fontSize, String const& processingDirectives);

  TexturePtr glyphTexturePtr(String::Char, unsigned fontSize);
  TexturePtr glyphTexturePtr(String::Char, unsigned fontSize, String const& processingDirectives);

  unsigned glyphWidth(String::Char c, unsigned fontSize);

  // Removes glyphs that haven't been used in more than the given time in
  // milliseconds
  void cleanup(int64_t timeout);
private:

  FontPtr m_font;
  TextureGroupPtr m_textureGroup;
  HashMap<GlyphDescriptor, GlyphTexture> m_glyphs;
};

}

#endif
