#include "StarFontTextureGroup.hpp"
#include "StarTime.hpp"
#include "StarImageProcessing.hpp"

namespace Star {

FontTextureGroup::FontTextureGroup(FontPtr font, TextureGroupPtr textureGroup)
  : m_font(move(font)), m_textureGroup(move(textureGroup)) {}

void FontTextureGroup::cleanup(int64_t timeout) {
  int64_t currentTime = Time::monotonicMilliseconds();
  eraseWhere(m_glyphs, [&](auto const& p) { return currentTime - p.second.time > timeout; });
}

const FontTextureGroup::GlyphTexture& FontTextureGroup::glyphTexture(String::Char c, unsigned size, String const& processingDirectives)
{
  auto res = m_glyphs.insert(GlyphDescriptor{c, size, processingDirectives}, GlyphTexture());

  if (res.second) {
    m_font->setPixelSize(size);
    Image image = m_font->render(c);
    if (!processingDirectives.empty()) {
      Vec2F preSize = Vec2F(image.size());
      image = processImageOperations(parseImageOperations(processingDirectives), image);
      res.first->second.processingOffset = preSize - Vec2F(image.size());
    }
    else
      res.first->second.processingOffset = Vec2F();

    res.first->second.texture = m_textureGroup->create(image);
  }

  res.first->second.time = Time::monotonicMilliseconds();
  return res.first->second;
}

TexturePtr FontTextureGroup::glyphTexturePtr(String::Char c, unsigned size) {
  return glyphTexture(c, size, "").texture;
}

TexturePtr FontTextureGroup::glyphTexturePtr(String::Char c, unsigned size, String const& processingDirectives) {
  return glyphTexture(c, size, processingDirectives).texture;
}

unsigned FontTextureGroup::glyphWidth(String::Char c, unsigned fontSize) {
  m_font->setPixelSize(fontSize);
  return m_font->width(c);
}

}
