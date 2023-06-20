#ifndef STAR_FONT_TEXTURE_GROUP_HPP
#define STAR_FONT_TEXTURE_GROUP_HPP

#include "StarColor.hpp"
#include "StarFont.hpp"
#include "StarRenderer.hpp"

namespace Star {

STAR_CLASS(FontTextureGroup);

class FontTextureGroup {
public:
  FontTextureGroup(FontPtr font, TextureGroupPtr textureGroup);

  TexturePtr glyphTexture(String::Char, unsigned fontSize);
  TexturePtr glyphTexture(String::Char, unsigned fontSize, String const& processingDirectives);

  unsigned glyphWidth(String::Char c, unsigned fontSize);

  // Removes glyphs that haven't been used in more than the given time in
  // milliseconds
  void cleanup(int64_t timeout);

private:
  typedef tuple<String::Char, unsigned, String> GlyphDescriptor;

  struct GlyphTexture {
    TexturePtr texture;
    int64_t time;
  };

  FontPtr m_font;
  TextureGroupPtr m_textureGroup;
  HashMap<GlyphDescriptor, GlyphTexture> m_glyphs;
};

}

#endif
