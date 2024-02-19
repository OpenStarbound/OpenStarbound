#include "StarImage.hpp"
#include "StarLogging.hpp"

#include <png.h>

namespace Star {

void logPngError(png_structp png_ptr, png_const_charp c) {
  Logger::debug("PNG error in file: '{}', {}", ((IODevice*)png_get_error_ptr(png_ptr))->deviceName(), c);
};

void logPngWarning(png_structp png_ptr, png_const_charp c) {
  Logger::debug("PNG warning in file: '{}', {}", ((IODevice*)png_get_error_ptr(png_ptr))->deviceName(), c);
};

void readPngData(png_structp pngPtr, png_bytep data, png_size_t length) {
  ((IODevice*)png_get_io_ptr(pngPtr))->readFull((char*)data, length);
};

Image Image::readPng(IODevicePtr device) {
  png_byte header[8];
  device->readFull((char*)header, sizeof(header));

  if (png_sig_cmp(header, 0, sizeof(header)))
    throw ImageException(strf("File {} is not a png image!", device->deviceName()));

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    throw ImageException("Internal libPNG error");

  // Use custom warning function to suppress cerr warnings
  png_set_error_fn(png_ptr, (png_voidp)device.get(), logPngError, logPngWarning);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    throw ImageException("Internal libPNG error");
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    throw ImageException("Internal libPNG error");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    throw ImageException("Internal error reading png.");
  }

  png_set_read_fn(png_ptr, device.get(), readPngData);

  // Tell libPNG that we read some of the header.
  png_set_sig_bytes(png_ptr, sizeof(header));

  png_read_info(png_ptr, info_ptr);

  png_uint_32 img_width = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 img_height = png_get_image_height(png_ptr, info_ptr);

  png_uint_32 bitdepth = png_get_bit_depth(png_ptr, info_ptr);
  png_uint_32 channels = png_get_channels(png_ptr, info_ptr);

  // Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
  png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    channels = 3;
    bitdepth = 8;
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bitdepth < 8) {
      png_set_expand_gray_1_2_4_to_8(png_ptr);
      bitdepth = 8;
    }
    png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      channels = 4;
    else
      channels = 3;
  }

  // If the image has a transperancy set, convert it to a full alpha channel
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
    channels += 1;
  }

  // We don't support 16 bit precision.. so if the image Has 16 bits per channel
  // precision... round it down to 8.
  if (bitdepth == 16) {
    png_set_strip_16(png_ptr);
    bitdepth = 8;
  }

  if (bitdepth != 8 || (channels != 3 && channels != 4)) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    throw ImageException(strf("Unsupported PNG pixel format in file {}", device->deviceName()));
  }

  Image image(img_width, img_height, channels == 3 ? PixelFormat::RGB24 : PixelFormat::RGBA32);

  std::unique_ptr<png_bytep[]> row_ptrs(new png_bytep[img_height]);
  size_t stride = img_width * channels;
  for (size_t i = 0; i < img_height; ++i)
    row_ptrs[i] = (png_bytep)image.data() + (img_height - i - 1) * stride;

  png_read_image(png_ptr, row_ptrs.get());
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  return image;
}

tuple<Vec2U, PixelFormat> Image::readPngMetadata(IODevicePtr device) {
  png_byte header[8];
  device->readFull((char*)header, sizeof(header));

  if (png_sig_cmp(header, 0, sizeof(header)))
    throw ImageException(strf("File {} is not a png image!", device->deviceName()));

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    throw ImageException("Internal libPNG error");

  // Use custom warning function to suppress cerr warnings
  png_set_error_fn(png_ptr, (png_voidp)device.get(), logPngError, logPngWarning);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    throw ImageException("Internal libPNG error");
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    throw ImageException("Internal libPNG error");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    throw ImageException("Internal error reading png.");
  }

  png_set_read_fn(png_ptr, device.get(), readPngData);

  // Tell libPNG that we read some of the header.
  png_set_sig_bytes(png_ptr, sizeof(header));

  png_read_info(png_ptr, info_ptr);

  png_uint_32 img_width = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 img_height = png_get_image_height(png_ptr, info_ptr);

  png_uint_32 bitdepth = png_get_bit_depth(png_ptr, info_ptr);
  png_uint_32 channels = png_get_channels(png_ptr, info_ptr);

  // Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
  png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    channels = 3;
    bitdepth = 8;
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bitdepth < 8) {
      png_set_expand_gray_1_2_4_to_8(png_ptr);
      bitdepth = 8;
    }
    png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      channels = 4;
    else
      channels = 3;
  }

  // If the image has a transperancy set, convert it to a full alpha channel
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
    channels += 1;
  }

  Vec2U imageSize{img_width, img_height};
  PixelFormat pixelFormat = channels == 3 ? PixelFormat::RGB24 : PixelFormat::RGBA32;

  return make_tuple(imageSize, pixelFormat);
}

Image Image::filled(Vec2U size, Vec4B color, PixelFormat pf) {
  Image image(size, pf);
  image.fill(color);
  return image;
}

Image::Image(PixelFormat pf)
  : m_data(nullptr), m_width(0), m_height(0), m_pixelFormat(pf) {}

Image::Image(Vec2U size, PixelFormat pf)
  : Image(size[0], size[1], pf) {}

Image::Image(unsigned width, unsigned height, PixelFormat pf)
  : Image(pf) {
  reset(width, height, pf);
}

Image::~Image() {
  if (m_data)
    Star::free(m_data);
}

Image::Image(Image const& image) : Image() {
  operator=(image);
}

Image::Image(Image&& image) : Image() {
  operator=(std::move(image));
}

Image& Image::operator=(Image const& image) {
  reset(image.m_width, image.m_height, image.m_pixelFormat);
  memcpy(data(), image.data(), m_width * m_height * bytesPerPixel());
  return *this;
}

Image& Image::operator=(Image&& image) {
  reset(0, 0, m_pixelFormat);

  m_data = take(image.m_data);
  m_width = take(image.m_width);
  m_height = take(image.m_height);
  m_pixelFormat = take(image.m_pixelFormat);
  return *this;
}

void Image::reset(Vec2U size, Maybe<PixelFormat> pf) {
  reset(size[0], size[1], pf);
}

void Image::reset(unsigned width, unsigned height, Maybe<PixelFormat> pf) {
  if (!pf)
    pf = m_pixelFormat;

  if (m_data && m_width == width && m_height == height && m_pixelFormat == *pf)
    return;

  size_t imageSize = width * height * Star::bytesPerPixel(*pf);
  if (imageSize == 0) {
    if (m_data) {
      Star::free(m_data);
      m_data = nullptr;
    }
  } else {
    uint8_t* newData = nullptr;
    if (!m_data)
      newData = (uint8_t*)Star::malloc(imageSize);
    else
      newData = (uint8_t*)Star::realloc(m_data, imageSize);

    if (!newData)
      throw MemoryException::format("Could not allocate memory for new Image size {}\n", imageSize);

    m_data = newData;
    memset(m_data, 0, imageSize);
  }

  m_pixelFormat = *pf;
  m_width = width;
  m_height = height;
}

void Image::fill(Vec3B const& c) {
  if (bitsPerPixel() == 24) {
    for (unsigned y = 0; y < m_height; ++y)
      for (unsigned x = 0; x < m_width; ++x)
        set24(x, y, c);
  } else {
    for (unsigned y = 0; y < m_height; ++y)
      for (unsigned x = 0; x < m_width; ++x)
        set32(x, y, Vec4B(c, 255));
  }
}

void Image::fill(Vec4B const& c) {
  if (bitsPerPixel() == 24) {
    for (unsigned y = 0; y < m_height; ++y)
      for (unsigned x = 0; x < m_width; ++x)
        set24(x, y, c.vec3());
  } else {
    for (unsigned y = 0; y < m_height; ++y)
      for (unsigned x = 0; x < m_width; ++x)
        set32(x, y, c);
  }
}

void Image::fillRect(Vec2U const& pos, Vec2U const& size, Vec3B const& c) {
  for (unsigned y = pos[1]; y < pos[1] + size[1] && y < m_height; ++y)
    for (unsigned x = pos[0]; x < pos[0] + size[0] && x < m_width; ++x)
      set(Vec2U(x, y), c);
}

void Image::fillRect(Vec2U const& pos, Vec2U const& size, Vec4B const& c) {
  for (unsigned y = pos[1]; y < pos[1] + size[1] && y < m_height; ++y)
    for (unsigned x = pos[0]; x < pos[0] + size[0] && x < m_width; ++x)
      set(Vec2U(x, y), c);
}

void Image::set(Vec2U const& pos, Vec4B const& c) {
  if (pos[0] >= m_width || pos[1] >= m_height) {
    throw ImageException(strf("{} out of range in Image::set", pos));
  } else if (bytesPerPixel() == 4) {
    size_t offset = pos[1] * m_width * 4 + pos[0] * 4;
    m_data[offset] = c[0];
    m_data[offset + 1] = c[1];
    m_data[offset + 2] = c[2];
    m_data[offset + 3] = c[3];
  } else if (bytesPerPixel() == 3) {
    size_t offset = pos[1] * m_width * 3 + pos[0] * 3;
    m_data[offset] = c[0];
    m_data[offset + 1] = c[1];
    m_data[offset + 2] = c[2];
  }
}

void Image::set(Vec2U const& pos, Vec3B const& c) {
  if (pos[0] >= m_width || pos[1] >= m_height) {
    throw ImageException(strf("{} out of range in Image::set", pos));
  } else if (bytesPerPixel() == 4) {
    size_t offset = pos[1] * m_width * 4 + pos[0] * 4;
    m_data[offset] = c[0];
    m_data[offset + 1] = c[1];
    m_data[offset + 2] = c[2];
    m_data[offset + 3] = 255;
  } else if (bytesPerPixel() == 3) {
    size_t offset = pos[1] * m_width * 3 + pos[0] * 3;
    m_data[offset] = c[0];
    m_data[offset + 1] = c[1];
    m_data[offset + 2] = c[2];
  }
}

Vec4B Image::get(Vec2U const& pos) const {
  Vec4B c;
  if (pos[0] >= m_width || pos[1] >= m_height) {
    throw ImageException(strf("{} out of range in Image::get", pos));
  } else if (bytesPerPixel() == 4) {
    size_t offset = pos[1] * m_width * 4 + pos[0] * 4;
    c[0] = m_data[offset];
    c[1] = m_data[offset + 1];
    c[2] = m_data[offset + 2];
    c[3] = m_data[offset + 3];
  } else if (bytesPerPixel() == 3) {
    size_t offset = pos[1] * m_width * 3 + pos[0] * 3;
    c[0] = m_data[offset];
    c[1] = m_data[offset + 1];
    c[2] = m_data[offset + 2];
    c[3] = 255;
  }
  return c;
}

void Image::setrgb(Vec2U const& pos, Vec4B const& c) {
  if (m_pixelFormat == PixelFormat::BGR24 || m_pixelFormat == PixelFormat::BGRA32)
    set(pos, Vec4B{c[2], c[1], c[0], c[3]});
  else
    set(pos, c);
}

void Image::setrgb(Vec2U const& pos, Vec3B const& c) {
  if (m_pixelFormat == PixelFormat::BGR24 || m_pixelFormat == PixelFormat::BGRA32)
    set(pos, Vec3B{c[2], c[1], c[0]});
  else
    set(pos, c);
}

Vec4B Image::getrgb(Vec2U const& pos) const {
  auto c = get(pos);
  if (m_pixelFormat == PixelFormat::BGR24 || m_pixelFormat == PixelFormat::BGRA32)
    return Vec4B{c[2], c[1], c[0], c[3]};
  else
    return c;
}

Vec4B Image::clamp(Vec2I const& pos) const {
  Vec4B c;
  unsigned x = (unsigned)Star::clamp<int>(pos[0], 0, m_width - 1);
  unsigned y = (unsigned)Star::clamp<int>(pos[1], 0, m_height - 1);
  if (m_width == 0 || m_height == 0) {
    return {0, 0, 0, 0};
  } else if (bytesPerPixel() == 4) {
    size_t offset = y * m_width * 4 + x * 4;
    c[0] = m_data[offset];
    c[1] = m_data[offset + 1];
    c[2] = m_data[offset + 2];
    c[3] = m_data[offset + 3];
  } else if (bytesPerPixel() == 3) {
    size_t offset = y * m_width * 3 + x * 3;
    c[0] = m_data[offset];
    c[1] = m_data[offset + 1];
    c[2] = m_data[offset + 2];
    c[3] = 255;
  }
  return c;
}

Vec4B Image::clamprgb(Vec2I const& pos) const {
  auto c = clamp(pos);
  if (m_pixelFormat == PixelFormat::BGR24 || m_pixelFormat == PixelFormat::BGRA32)
    return Vec4B{c[2], c[1], c[0], c[3]};
  else
    return c;
}

Image Image::subImage(Vec2U const& pos, Vec2U const& size) const {
  if (pos[0] + size[0] > m_width || pos[1] + size[1] > m_height)
    throw ImageException(strf("call to subImage with pos {} size {} out of image bounds ({}, {})", pos, size, m_width, m_height));

  Image sub(size[0], size[1], m_pixelFormat);

  for (unsigned y = 0; y < size[1]; ++y) {
    for (unsigned x = 0; x < size[0]; ++x) {
      sub.set({x, y}, get(pos + Vec2U(x, y)));
    }
  }

  return sub;
}

void Image::copyInto(Vec2U const& min, Image const& image) {
  Vec2U max = (min + image.size()).piecewiseMin(size());

  for (unsigned y = min[1]; y < max[1]; ++y) {
    for (unsigned x = min[0]; x < max[0]; ++x)
      set(x, y, image.get(Vec2U(x, y) - min));
  }
}

void Image::drawInto(Vec2U const& min, Image const& image) {
  Vec2U max = (min + image.size()).piecewiseMin(size());

  for (unsigned y = min[1]; y < max[1]; ++y) {
    for (unsigned x = min[0]; x < max[0]; ++x) {
      Vec4B dest = get(Vec2U(x, y));
      Vec4B src = image.get(Vec2U(x, y) - min);

      Vec3U destMultiplied = Vec3U(dest[0], dest[1], dest[2]) * dest[3] / 255;
      Vec3U srcMultiplied = Vec3U(src[0], src[1], src[2]) * src[3] / 255;

      // Src over dest alpha composition
      Vec3U over = srcMultiplied + destMultiplied * (255 - src[3]) / 255;
      unsigned alpha = src[3] + dest[3] * (255 - src[3]) / 255;

      set(x, y, Vec4B(over[0], over[1], over[2], alpha));
    }
  }
}

Image Image::convert(PixelFormat pixelFormat) const {
  Image converted(m_width, m_height, pixelFormat);
  converted.copyInto(Vec2U(), *this);
  return converted;
}

void Image::writePng(IODevicePtr device) const {
  auto writePngData = [](png_structp pngPtr, png_bytep data, png_size_t length) {
    IODevice* device = (IODevice*)png_get_io_ptr(pngPtr);
    device->writeFull((char*)data, length);
  };

  auto flushPngData = [](png_structp) {};

  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    throw ImageException("Internal libPNG error");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, nullptr);
    throw ImageException("Internal libPNG error");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    throw ImageException("Internal error reading png.");
  }

  unsigned channels = m_pixelFormat == PixelFormat::RGB24 ? 3 : 4;

  png_set_IHDR(png_ptr,
      info_ptr,
      m_width,
      m_height,
      8,
      channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
      PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);

  unique_ptr<png_bytep[]> row_ptrs(new png_bytep[m_height]);
  size_t stride = m_width * 8 * channels / 8;
  for (size_t i = 0; i < m_height; ++i) {
    size_t q = (m_height - i - 1) * stride;
    row_ptrs[i] = (png_bytep)m_data + q;
  }

  png_set_write_fn(png_ptr, device.get(), writePngData, flushPngData);
  png_set_rows(png_ptr, info_ptr, row_ptrs.get());
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);
}

}
