#pragma once

#include "StarString.hpp"
#include "StarVector.hpp"
#include "StarIODevice.hpp"

namespace Star {

enum class PixelFormat : uint8_t {
  RGB24,
  RGBA32,
  BGR24,
  BGRA32,
  RGB_F,
  RGBA_F
};

uint8_t bitsPerPixel(PixelFormat pf);
uint8_t bytesPerPixel(PixelFormat pf);

STAR_EXCEPTION(ImageException, StarException);

STAR_CLASS(Image);

// Holds an image of PixelFormat in row major order, with no padding, with (0,
// 0) defined to be the *lower left* corner.
class Image {
public:
  static Image readPng(IODevicePtr device);
  // Returns the size and pixel format that would be constructed from the given
  // png file, without actually loading it.
  static tuple<Vec2U, PixelFormat> readPngMetadata(IODevicePtr device);

  static Image filled(Vec2U size, Vec4B color, PixelFormat pf = PixelFormat::RGBA32);

  // Creates a zero size image
  Image(PixelFormat pf = PixelFormat::RGBA32);
  Image(Vec2U size, PixelFormat pf = PixelFormat::RGBA32);
  Image(unsigned width, unsigned height, PixelFormat pf = PixelFormat::RGBA32);
  ~Image();

  Image(Image const& image);
  Image(Image&& image);

  Image& operator=(Image const& image);
  Image& operator=(Image&& image);

  uint8_t bitsPerPixel() const;
  uint8_t bytesPerPixel() const;

  unsigned width() const;
  unsigned height() const;
  Vec2U size() const;
  // width or height is 0
  bool empty() const;

  PixelFormat pixelFormat() const;

  // If the image is empty, the data ptr will be null
  uint8_t const* data() const;
  uint8_t* data();

  // Reallocate the image with the given width, height, and pixel format.  The
  // contents of the image are always zeroed after a call to reset.
  void reset(Vec2U size, Maybe<PixelFormat> pf = {});
  void reset(unsigned width, unsigned height, Maybe<PixelFormat> pf = {});

  // Fill the image with a given color
  void fill(Vec3B const& c);
  void fill(Vec4B const& c);

  // Fill a rectangle with a given color
  void fillRect(Vec2U const& pos, Vec2U const& size, Vec3B const& c);
  void fillRect(Vec2U const& pos, Vec2U const& size, Vec4B const& c);

  // Color parameters / return values here are in whatever the internal format
  // is.  Fourth byte, if missing or not provided, is assumed to be 255.  If
  // the position is out of range, then throws an exception.
  void set(Vec2U const& pos, Vec4B const& c);
  void set(Vec2U const& pos, Vec3B const& c);
  Vec4B get(Vec2U const& pos) const;

  // Same as set / get, except color parameters / return values here are always
  // RGB[A], and converts if necessary.
  void setrgb(Vec2U const& pos, Vec4B const& c);
  void setrgb(Vec2U const& pos, Vec3B const& c);
  Vec4B getrgb(Vec2U const& pos) const;

  // Get pixel value, but if pos is out of the normal pixel range, it is
  // clamped back into the valid pixel range.  Returns (0, 0, 0, 0) if image is
  // empty.
  Vec4B clamp(Vec2I const& pos) const;
  Vec4B clamprgb(Vec2I const& pos) const;

  // x / y versions of set / get, for compatibility
  void set(unsigned x, unsigned y, Vec4B const& c);
  void set(unsigned x, unsigned y, Vec3B const& c);
  Vec4B get(unsigned x, unsigned y) const;
  void setrgb(unsigned x, unsigned y, Vec4B const& c);
  void setrgb(unsigned x, unsigned y, Vec3B const& c);
  Vec4B getrgb(unsigned x, unsigned y) const;
  Vec4B clamp(int x, int y) const;
  Vec4B clamprgb(int x, int y) const;

  // Must be 32 bitsPerPixel, no format conversion or bounds checking takes
  // place.  Very fast inline versions.
  void set32(Vec2U const& pos, Vec4B const& c);
  void set32(unsigned x, unsigned y, Vec4B const& c);
  Vec4B get32(unsigned x, unsigned y) const;

  // Must be 24 bitsPerPixel, no format conversion or bounds checking takes
  // place.  Very fast inline versions.
  void set24(Vec2U const& pos, Vec3B const& c);
  void set24(unsigned x, unsigned y, Vec3B const& c);
  Vec3B get24(unsigned x, unsigned y) const;

  // Called as callback(unsigned x, unsigned y, Vec4B const& pixel)
  template <typename CallbackType>
  void forEachPixel(CallbackType&& callback) const;

  // Called as callback(unsigned x, unsigned y, Vec4B& pixel)
  template <typename CallbackType>
  void forEachPixel(CallbackType&& callback);

  // Pixel rectangle, lower left position and size of rectangle.
  Image subImage(Vec2U const& pos, Vec2U const& size) const;

  // Copy given image into this one at pos
  void copyInto(Vec2U const& pos, Image const& image);
  // Draw given image over this one at pos (with alpha composition)
  void drawInto(Vec2U const& pos, Image const& image);

  // Convert this image into the given pixel format
  Image convert(PixelFormat pixelFormat) const;

  void writePng(IODevicePtr device) const;

private:
  uint8_t* m_data;
  unsigned m_width;
  unsigned m_height;
  PixelFormat m_pixelFormat;
};

inline uint8_t bitsPerPixel(PixelFormat pf) {
  switch (pf) {
    case PixelFormat::RGB24:
      return 24;
    case PixelFormat::RGBA32:
      return 32;
    case PixelFormat::BGR24:
      return 24;
    case PixelFormat::BGRA32:
      return 32;
    case PixelFormat::RGB_F:
      return 96;
    default:
      return 128;
  }
}

inline uint8_t bytesPerPixel(PixelFormat pf) {
  switch (pf) {
    case PixelFormat::RGB24:
      return 3;
    case PixelFormat::RGBA32:
      return 4;
    case PixelFormat::BGR24:
      return 3;
    case PixelFormat::BGRA32:
      return 4;
    case PixelFormat::RGB_F:
      return 12;
    default:
      return 16;
  }
}

inline uint8_t Image::bitsPerPixel() const {
  return Star::bitsPerPixel(m_pixelFormat);
}

inline uint8_t Image::bytesPerPixel() const {
  return Star::bytesPerPixel(m_pixelFormat);
}

inline unsigned Image::width() const {
  return m_width;
}

inline unsigned Image::height() const {
  return m_height;
}

inline bool Image::empty() const {
  return m_width == 0 || m_height == 0;
}

inline Vec2U Image::size() const {
  return {m_width, m_height};
}

inline PixelFormat Image::pixelFormat() const {
  return m_pixelFormat;
}

inline const uint8_t* Image::data() const {
  return m_data;
}

inline uint8_t* Image::data() {
  return m_data;
}

inline void Image::set(unsigned x, unsigned y, Vec4B const& c) {
  return set({x, y}, c);
}

inline void Image::set(unsigned x, unsigned y, Vec3B const& c) {
  return set({x, y}, c);
}

inline Vec4B Image::get(unsigned x, unsigned y) const {
  return get({x, y});
}

inline void Image::setrgb(unsigned x, unsigned y, Vec4B const& c) {
  return setrgb({x, y}, c);
}

inline void Image::setrgb(unsigned x, unsigned y, Vec3B const& c) {
  return setrgb({x, y}, c);
}

inline Vec4B Image::getrgb(unsigned x, unsigned y) const {
  return getrgb({x, y});
}

inline Vec4B Image::clamp(int x, int y) const {
  return clamp({x, y});
}

inline Vec4B Image::clamprgb(int x, int y) const {
  return clamprgb({x, y});
}

inline void Image::set32(Vec2U const& pos, Vec4B const& c) {
  set32(pos[0], pos[1], c);
}

inline void Image::set32(unsigned x, unsigned y, Vec4B const& c) {
  starAssert(m_data && x < m_width && y < m_height);
  starAssert(bytesPerPixel() == 4);

  size_t offset = y * m_width * 4 + x * 4;
  m_data[offset] = c[0];
  m_data[offset + 1] = c[1];
  m_data[offset + 2] = c[2];
  m_data[offset + 3] = c[3];
}

inline Vec4B Image::get32(unsigned x, unsigned y) const {
  starAssert(m_data && x < m_width && y < m_height);
  starAssert(bytesPerPixel() == 4);

  Vec4B c;
  size_t offset = y * m_width * 4 + x * 4;
  c[0] = m_data[offset];
  c[1] = m_data[offset + 1];
  c[2] = m_data[offset + 2];
  c[3] = m_data[offset + 3];
  return c;
}

inline void Image::set24(Vec2U const& pos, Vec3B const& c) {
  set24(pos[0], pos[1], c);
}

inline void Image::set24(unsigned x, unsigned y, Vec3B const& c) {
  starAssert(m_data && x < m_width && y < m_height);
  starAssert(bytesPerPixel() == 3);

  size_t offset = y * m_width * 3 + x * 3;
  m_data[offset] = c[0];
  m_data[offset + 1] = c[1];
  m_data[offset + 2] = c[2];
}

inline Vec3B Image::get24(unsigned x, unsigned y) const {
  starAssert(m_data && x < m_width && y < m_height);
  starAssert(bytesPerPixel() == 3);

  Vec3B c;
  size_t offset = y * m_width * 3 + x * 3;
  c[0] = m_data[offset];
  c[1] = m_data[offset + 1];
  c[2] = m_data[offset + 2];
  return c;
}

template <typename CallbackType>
void Image::forEachPixel(CallbackType&& callback) const {
  for (unsigned y = 0; y < m_height; y++) {
    for (unsigned x = 0; x < m_width; x++)
      callback(x, y, get(x, y));
  }
}

template <typename CallbackType>
void Image::forEachPixel(CallbackType&& callback) {
  for (unsigned y = 0; y < m_height; y++) {
    for (unsigned x = 0; x < m_width; x++) {
      Vec4B pixel = get(x, y);
      callback(x, y, pixel);
      set(x, y, pixel);
    }
  }
}

struct ImageView {
  inline bool empty() const { return size.x() == 0 || size.y() == 0; }
  ImageView() = default;
  ImageView(Image const& image);

  Vec2U size{0, 0};
  uint8_t const* data = nullptr;
  PixelFormat format = PixelFormat::RGB24;
};

}
