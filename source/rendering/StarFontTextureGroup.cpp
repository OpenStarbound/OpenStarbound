#include "StarFontTextureGroup.hpp"
#include "StarTime.hpp"
#include "StarImageProcessing.hpp"

namespace Star {

FontTextureGroup::FontTextureGroup(TextureGroupPtr textureGroup)
  : m_textureGroup(std::move(textureGroup)) {}

void FontTextureGroup::cleanup(int64_t timeout) {
  int64_t currentTime = Time::monotonicMilliseconds();
  eraseWhere(m_glyphs, [&](auto const& p) { return currentTime - p.second.time > timeout; });
}

void FontTextureGroup::switchFont(String const& font) {
  if (font.empty()) {
    m_font = m_defaultFont;
    m_fontName.clear();
  }
  else if (m_fontName != font) {
    m_fontName = font;
    auto find = m_fonts.find(font);
    m_font = find != m_fonts.end() ? find->second : m_defaultFont;
  }
}

String const& FontTextureGroup::activeFont() {
  return m_fontName;
}

void FontTextureGroup::addFont(FontPtr const& font, String const& name, bool isDefault) {
  m_fonts[name] = font;
  if (isDefault)
    m_defaultFont = m_font = font;
}

void FontTextureGroup::clearFonts() {
  m_fonts.clear();
  m_font = m_defaultFont;
}

void FontTextureGroup::setFallbackFont(String const& fontName) {
  if (auto font = m_fonts.ptr(fontName))
    m_fallbackFont = *font;
}

const FontTextureGroup::GlyphTexture& FontTextureGroup::glyphTexture(String::Char c, unsigned size, Directives const* processingDirectives)
{
  Font* font = (m_font->exists(c) || !m_fallbackFont) ? m_font.get() : m_fallbackFont.get();
  auto res = m_glyphs.insert(GlyphDescriptor{c, size, processingDirectives ? processingDirectives->hash() : 0, font}, GlyphTexture());

  if (res.second) {
    font->setPixelSize(size);
    auto pair = font->render(c);
    Image& image = pair.first;
    if (processingDirectives) {
      try {
        Directives const& directives = *processingDirectives;
        Vec2F preSize = Vec2F(image.size());

        for (auto& entry : directives->entries)
          processImageOperation(entry.operation, image);

        res.first->second.offset = (preSize - Vec2F(image.size())) / 2;
      }
      catch (StarException const&) {
        image.forEachPixel([](unsigned x, unsigned y, Vec4B& pixel) {
          pixel = ((x + y) % 2 == 0) ? Vec4B(255, 0, 255, pixel[3]) : Vec4B(0, 0, 0, pixel[3]);
        });
      }
    }
    else
      res.first->second.offset = Vec2F();

    res.first->second.offset += Vec2F(pair.second);
    res.first->second.texture = m_textureGroup->create(image);
  }

  res.first->second.time = Time::monotonicMilliseconds();
  return res.first->second;
}

TexturePtr FontTextureGroup::glyphTexturePtr(String::Char c, unsigned size) {
  return glyphTexture(c, size, nullptr).texture;
}

TexturePtr FontTextureGroup::glyphTexturePtr(String::Char c, unsigned size, Directives const* processingDirectives) {
  return glyphTexture(c, size, processingDirectives).texture;
}

unsigned FontTextureGroup::glyphWidth(String::Char c, unsigned fontSize) {
  Font* font = (m_font->exists(c) || !m_fallbackFont) ? m_font.get() : m_fallbackFont.get();
  font->setPixelSize(fontSize);
  return font->width(c);
}

}
