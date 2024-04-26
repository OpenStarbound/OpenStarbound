#pragma once

#include "StarString.hpp"
#include "StarImage.hpp"
#include "StarByteArray.hpp"
#include "StarMap.hpp"

namespace Star {

STAR_EXCEPTION(FontException, StarException);

STAR_STRUCT(FontImpl);
STAR_CLASS(Font);

class Font {
public:
  static FontPtr loadFont(String const& fileName, unsigned pixelSize = 12);
  static FontPtr loadFont(ByteArrayConstPtr const& bytes, unsigned pixelSize = 12);

  Font();

  Font(Font const&) = delete;
  Font const& operator=(Font const&) = delete;

  // Create a new font from the same data
  FontPtr clone() const;

  void setPixelSize(unsigned pixelSize);
  void setAlphaThreshold(uint8_t alphaThreshold = 0);

  unsigned height() const;
  unsigned width(String::Char c);

  // May return empty image on unrenderable character (Normally, this will
  // render a box, but if there is an internal freetype error this may return
  // an empty image).
  tuple<Image, Vec2I, bool> render(String::Char c);
  bool exists(String::Char c);

private:
  FontImplPtr m_fontImpl;
  ByteArrayConstPtr m_fontBuffer;
  unsigned m_pixelSize;
  uint8_t m_alphaThreshold;

  HashMap<pair<String::Char, unsigned>, unsigned> m_widthCache;
};

}
