#include "StarFontTextureGroup.hpp"
#include "StarTime.hpp"
#include "StarImageProcessing.hpp"

namespace Star {

FontTextureGroup::FontTextureGroup(TextureGroupPtr textureGroup)
  : m_textureGroup(move(textureGroup)) {}

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

void FontTextureGroup::addFont(FontPtr const& font, String const& name, bool default) {
  m_fonts[name] = font;
  if (default)
    m_defaultFont = m_font = font;
}

const FontTextureGroup::GlyphTexture& FontTextureGroup::glyphTexture(String::Char c, unsigned size, String const& processingDirectives)
{
  auto res = m_glyphs.insert(GlyphDescriptor{c, size, processingDirectives, m_font.get() }, GlyphTexture());

  if (res.second) {
    m_font->setPixelSize(size);
    Image image = m_font->render(c);
    if (!processingDirectives.empty()) {
      try {
        Vec2F preSize = Vec2F(image.size());
        auto imageOperations = parseImageOperations(processingDirectives);
        for (auto& imageOp : imageOperations) {
          if (auto border = imageOp.ptr<BorderImageOperation>())
            border->includeTransparent = true;
        }
        image = processImageOperations(imageOperations, image);
        res.first->second.processingOffset = preSize - Vec2F(image.size());
      }
      catch (StarException&) {
        image.forEachPixel([](unsigned x, unsigned y, Vec4B& pixel) {
          pixel = ((x + y) % 2 == 0) ? Vec4B(255, 0, 255, pixel[3]) : Vec4B(0, 0, 0, pixel[3]);
        });
      }
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
