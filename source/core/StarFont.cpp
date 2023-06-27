#include "StarFont.hpp"
#include "StarFile.hpp"
#include "StarFormat.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Star {

struct FTContext {
  FT_Library library;

  FTContext() {
    library = nullptr;
    if (FT_Init_FreeType(&library))
      throw FontException("Could not initialize freetype library.");
  }

  ~FTContext() {
    if (library) {
      FT_Done_FreeType(library);
      library = nullptr;
    }
  }
};

FTContext ftContext;

struct FontImpl {
  FT_Face face;
};

FontPtr Font::loadTrueTypeFont(String const& fileName, unsigned pixelSize) {
  return loadTrueTypeFont(make_shared<ByteArray>(File::readFile(fileName)), pixelSize);
}

FontPtr Font::loadTrueTypeFont(ByteArrayConstPtr const& bytes, unsigned pixelSize) {
  FontPtr font = make_shared<Font>();
  font->m_fontBuffer = bytes;

  shared_ptr<FontImpl> fontImpl = make_shared<FontImpl>();
  if (FT_New_Memory_Face(
          ftContext.library, (FT_Byte const*)font->m_fontBuffer->ptr(), font->m_fontBuffer->size(), 0, &fontImpl->face))
    throw FontException("Could not load font from buffer");

  font->m_fontImpl = fontImpl;
  font->setPixelSize(pixelSize);

  return font;
}

Font::Font() {
  m_pixelSize = 0;
}

FontPtr Font::clone() const {
  return Font::loadTrueTypeFont(m_fontBuffer, m_pixelSize);
}

void Font::setPixelSize(unsigned pixelSize) {
  if (pixelSize == 0) {
    pixelSize = 1;
  }

  if (m_pixelSize == pixelSize)
    return;

  if (FT_Set_Pixel_Sizes(m_fontImpl->face, pixelSize, 0))
    throw FontException(strf("Cannot set font pixel size to: {}", pixelSize));
  m_pixelSize = pixelSize;
}

unsigned Font::height() const {
  return m_pixelSize;
}

unsigned Font::width(String::Char c) {
  if (auto width = m_widthCache.maybe({c, m_pixelSize})) {
    return *width;
  } else {
    FT_Load_Char(m_fontImpl->face, c, FT_LOAD_FORCE_AUTOHINT);
    unsigned newWidth = (m_fontImpl->face->glyph->advance.x + 32) / 64;
    m_widthCache.insert({c, m_pixelSize}, newWidth);
    return newWidth;
  }
}

Image Font::render(String::Char c) {
  if (!m_fontImpl)
    throw FontException("Font::render called on uninitialized font.");

  FT_UInt glyph_index = FT_Get_Char_Index(m_fontImpl->face, c);
  if (FT_Load_Glyph(m_fontImpl->face, glyph_index, FT_LOAD_FORCE_AUTOHINT) != 0)
    return {};

  /* convert to an anti-aliased bitmap */
  if (FT_Render_Glyph(m_fontImpl->face->glyph, FT_RENDER_MODE_NORMAL) != 0)
    return {};

  FT_GlyphSlot slot = m_fontImpl->face->glyph;

  int width = (slot->advance.x + 32) / 64;
  int height = m_pixelSize;

  Image image(width, height, PixelFormat::RGBA32);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int bx = x;
      int by = y + slot->bitmap_top - m_fontImpl->face->size->metrics.ascender / 64;
      if (bx >= 0 && by >= 0 && bx < (int)slot->bitmap.width && by < (int)slot->bitmap.rows) {
        unsigned char* p = slot->bitmap.buffer + by * slot->bitmap.pitch;
        unsigned char val = *(p + bx);
        image.set(x, height - y - 1, Vec4B(255, 255, 255, val));
      } else {
        image.set(x, height - y - 1, Vec4B(255, 255, 255, 0));
      }
    }
  }

  return image;
}

}
