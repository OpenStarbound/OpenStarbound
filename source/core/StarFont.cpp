#include "StarFont.hpp"
#include "StarFile.hpp"
#include "StarFormat.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Star {

constexpr int FontLoadFlags = FT_LOAD_NO_SVG | FT_LOAD_COLOR;

struct FTContext {
  FT_Library library;

  FTContext() {
    library = nullptr;
    if (FT_Init_FreeType(&library))
      throw FontException("Could not initialize FreeType library.");
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

FontPtr Font::loadFont(String const& fileName, unsigned pixelSize) {
  return loadFont(make_shared<ByteArray>(File::readFile(fileName)), pixelSize);
}

FontPtr Font::loadFont(ByteArrayConstPtr const& bytes, unsigned pixelSize) {
  FontPtr font = make_shared<Font>();
  font->m_fontBuffer = bytes;
  font->setPixelSize(pixelSize);

  return font;
}

Font::Font() : m_pixelSize(0), m_alphaThreshold(0) {}

Font::~Font() {
  if (m_fontImpl)
    FT_Done_Face(m_fontImpl->face);
}

FontPtr Font::clone() const {
  return Font::loadFont(m_fontBuffer, m_pixelSize);
}

void Font::setPixelSize(unsigned pixelSize) {
  if (pixelSize == 0)
    pixelSize = 1;

  if (m_pixelSize == pixelSize)
    return;

  if (m_fontImpl && FT_Set_Pixel_Sizes(m_fontImpl->face, m_pixelSize = pixelSize, 0))
    throw FontException(strf("Cannot set font pixel size to: {}", m_pixelSize));
}

void Font::setAlphaThreshold(uint8_t alphaThreshold) {
  m_alphaThreshold = alphaThreshold;
}

unsigned Font::height() const {
  return m_pixelSize;
}

unsigned Font::width(String::Char c) {
  if (auto width = m_widthCache.maybe({c, m_pixelSize})) {
    return *width;
  } else {
    loadFontImpl();
    FT_Load_Char(m_fontImpl->face, c, FontLoadFlags);
    unsigned newWidth = (m_fontImpl->face->glyph->linearHoriAdvance + 32768) / 65536;
    m_widthCache.insert({c, m_pixelSize}, newWidth);
    return newWidth;
  }
}

void Font::loadFontImpl() {
  if (!m_fontImpl) {
    if (!m_fontBuffer || m_fontBuffer->empty())
      throw FontException("Font buffer is null or empty");

    shared_ptr<FontImpl> fontImpl = make_shared<FontImpl>();
    if (FT_New_Memory_Face(ftContext.library, (FT_Byte const*)m_fontBuffer->ptr(), m_fontBuffer->size(), 0, &fontImpl->face))
      throw FontException::format("Could not load font from buffer");

    if (FT_Set_Pixel_Sizes(fontImpl->face, m_pixelSize, 0))
      throw FontException::format("Cannot set font pixel size to: {}", m_pixelSize);

    m_fontImpl = fontImpl;
  }
}

tuple<Image, Vec2I, bool> Font::render(String::Char c) {
  loadFontImpl();

  FT_Face face = m_fontImpl->face;
  if (FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FontLoadFlags) != 0)
    return {};

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
    return {};

  FT_GlyphSlot slot = face->glyph;
  if (!slot->bitmap.buffer)
    return {};

  unsigned width = slot->bitmap.width;
  unsigned height = slot->bitmap.rows;
  bool colored = false;

  Image image(width + 2, height + 2, PixelFormat::BGRA32);
  if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
    Vec4B white(255, 255, 255, 0);
    image.fill(white);
    for (unsigned y = 0; y != height; ++y) {
      uint8_t* p = slot->bitmap.buffer + y * slot->bitmap.pitch;
      for (unsigned x = 0; x != width; ++x) {
        if (x < width && y < height) {
          white[3] = m_alphaThreshold
            ? (*(p + x) >= m_alphaThreshold ? 255 : 0)
            :  *(p + x);
          image.set(x + 1, height - y, white);
        }
      }
    }
  } else if ((colored = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA))) {
    unsigned bpp = image.bytesPerPixel();
    uint8_t* data = image.data() + bpp + ((image.width() * (image.height() - 2)) * bpp); // offset by 1 pixel as it's padded
    for (size_t y = 0; y != height; ++y) {
      memcpy(data - (y * image.width() * bpp),
             slot->bitmap.buffer + y * slot->bitmap.pitch,
             slot->bitmap.pitch);
    }
    // unfortunately FreeType pre-multiplied the color channels :(
    image.forEachPixel([](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0 && pixel[3] != 255) {
        float a = byteToFloat(pixel[3]);
        pixel[0] /= a;
        pixel[1] /= a;
        pixel[2] /= a;
      }
    });
  } else {
    return {};
  }

  return make_tuple(
    std::move(image),
    Vec2I(slot->bitmap_left - 1, (slot->bitmap_top - height) + (m_pixelSize / 4) - 1),
    colored);
}

bool Font::exists(String::Char c) {
  loadFontImpl();
  return FT_Get_Char_Index(m_fontImpl->face, c);
}

}
