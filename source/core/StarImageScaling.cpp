#include "StarImage.hpp"
#include "StarImageScaling.hpp"
#include "StarInterpolation.hpp"

namespace Star {

// Size of the scaled image, and the value of one pixel of it. The full scales and the windowed ones below both use
// these, so a window is always exactly the same pixels the full image would have there.
static Vec2U scaledSize(Image const& srcImage, Vec2F const& scale) {
  Vec2U destSize = Vec2U::round(vmult(Vec2F(srcImage.size()), scale));
  destSize[0] = max(destSize[0], 1u);
  destSize[1] = max(destSize[1], 1u);
  return destSize;
}

// The window of w by h pixels at (x0, y0) of the nearest neighbour scaled image.
// A destination pixel copies the source pixel at clamp(round(position / scale)) and the column of that source pixel
// only depends on x and its row only on y, so each column and row is worked out once.
// Then, the pixels are copied instead of dividing and rounding for every pixel.
static Image scaleNearestWindow(Image const& srcImage, Vec2F const& scale, unsigned x0, unsigned y0, unsigned w, unsigned h) {
  Image destImage(Vec2U(w, h), srcImage.pixelFormat());

  if (srcImage.width() == 0 || srcImage.height() == 0) {
    // Image::clamp gives {0, 0, 0, 0} for an image without pixels
    for (unsigned y = 0; y < h; ++y) {
      for (unsigned x = 0; x < w; ++x)
        destImage.set({x, y}, Vec4B(0, 0, 0, 0));
    }
    return destImage;
  }

  // The scale is read through volatile variables on every iteration.
  // With fast-math, a division by a value that does not change in a loop is turned into a multiplication by its reciprocal,
  // which can differ in the last bit and then land on the other side of the .5 that round() cares about (3 / 0.15384616 is 19.4999993,
  // which as a float is 19.5 and rounds up, but 3 * (1 / 0.15384616) is not).
  // The per-pixel loop this replaces kept the real division and so must this.
  volatile float scaleX = scale[0], scaleY = scale[1];
  int maxX = (int)srcImage.width() - 1, maxY = (int)srcImage.height() - 1;
  std::vector<unsigned> columns(w), rows(h);
  for (unsigned x = 0; x < w; ++x)
    columns[x] = (unsigned)Star::clamp<int>(Vec2I::round(vdiv(Vec2F(x0 + x, 0), Vec2F((float)scaleX, (float)scaleY)))[0], 0, maxX);
  for (unsigned y = 0; y < h; ++y)
    rows[y] = (unsigned)Star::clamp<int>(Vec2I::round(vdiv(Vec2F(0, y0 + y), Vec2F((float)scaleX, (float)scaleY)))[1], 0, maxY);

  size_t bpp = srcImage.bytesPerPixel();
  size_t srcStride = (size_t)srcImage.width() * bpp;
  uint8_t const* src = srcImage.data();
  uint8_t* dest = destImage.data();
  for (unsigned y = 0; y < h; ++y) {
    uint8_t const* srcRow = src + rows[y] * srcStride;
    uint8_t* destRow = dest + (size_t)y * w * bpp;
    for (unsigned x = 0; x < w; ++x)
      memcpy(destRow + (size_t)x * bpp, srcRow + columns[x] * bpp, bpp);
  }
  return destImage;
}

static inline Vec4B bilinearPixel(Image const& srcImage, Vec2F const& scale, unsigned x, unsigned y) {
  auto pos = vdiv(Vec2F(x, y), scale);
  auto ipart = Vec2I::floor(pos);
  auto fpart = pos - Vec2F(ipart);

  auto result = lerp(fpart[1], lerp(fpart[0], Vec4F(srcImage.clamp(ipart[0], ipart[1])), Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1]))), lerp(fpart[0], Vec4F(srcImage.clamp(ipart[0], ipart[1] + 1)), Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 1))));

  return Vec4B(result);
}

Image scaleNearest(Image const& srcImage, Vec2F const& scale) {
  Vec2U destSize = scaledSize(srcImage, scale);
  return scaleNearestWindow(srcImage, scale, 0, 0, destSize[0], destSize[1]);
}

Image scaleBilinear(Image const& srcImage, Vec2F const& scale) {
  Vec2U destSize = scaledSize(srcImage, scale);
  Image destImage(destSize, srcImage.pixelFormat());

  for (unsigned y = 0; y < destSize[1]; ++y) {
    for (unsigned x = 0; x < destSize[0]; ++x)
      destImage.set({x, y}, bilinearPixel(srcImage, scale, x, y));
  }

  return destImage;
}

bool scaledCropFits(Image const& srcImage, Vec2F const& scale, RectI const& window) {
  Vec2U destSize = scaledSize(srcImage, scale);
  Vec2I wmin = window.min(), wsize = Vec2I(window.size());
  return wmin[0] >= 0 && wmin[1] >= 0 && wsize[0] >= 0 && wsize[1] >= 0
      && wmin[0] + wsize[0] <= (int)destSize[0] && wmin[1] + wsize[1] <= (int)destSize[1];
}

Image scaleNearestCropped(Image const& srcImage, Vec2F const& scale, RectI const& window) {
  Vec2I wmin = window.min();
  Vec2U wsize = Vec2U(window.size());
  return scaleNearestWindow(srcImage, scale, wmin[0], wmin[1], wsize[0], wsize[1]);
}

Image scaleBilinearCropped(Image const& srcImage, Vec2F const& scale, RectI const& window) {
  Vec2I wmin = window.min();
  Vec2U wsize = Vec2U(window.size());
  Image destImage(wsize, srcImage.pixelFormat());

  for (unsigned y = 0; y < wsize[1]; ++y) {
    for (unsigned x = 0; x < wsize[0]; ++x)
      destImage.set({x, y}, bilinearPixel(srcImage, scale, wmin[0] + x, wmin[1] + y));
  }
  return destImage;
}

Image scaleBicubic(Image const& srcImage, Vec2F const& scale) {
  Vec2U srcSize = srcImage.size();
  Vec2U destSize = Vec2U::round(vmult(Vec2F(srcSize), scale));
  destSize[0] = max(destSize[0], 1u);
  destSize[1] = max(destSize[1], 1u);

  Image destImage(destSize, srcImage.pixelFormat());

  for (unsigned y = 0; y < destSize[1]; ++y) {
    for (unsigned x = 0; x < destSize[0]; ++x) {
      auto pos = vdiv(Vec2F(x, y), scale);
      auto ipart = Vec2I::floor(pos);
      auto fpart = pos - Vec2F(ipart);

      Vec4F a = cubic4(fpart[0],
          Vec4F(srcImage.clamp(ipart[0], ipart[1])),
          Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1])),
          Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1])),
          Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1])));

      Vec4F b = cubic4(fpart[0],
          Vec4F(srcImage.clamp(ipart[0], ipart[1] + 1)),
          Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 1)),
          Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 1)),
          Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 1)));

      Vec4F c = cubic4(fpart[0],
          Vec4F(srcImage.clamp(ipart[0], ipart[1] + 2)),
          Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 2)),
          Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 2)),
          Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 2)));

      Vec4F d = cubic4(fpart[0],
          Vec4F(srcImage.clamp(ipart[0], ipart[1] + 3)),
          Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 3)),
          Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 3)),
          Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 3)));

      auto result = cubic4(fpart[1], a, b, c, d);

      destImage.set({x, y}, Vec4B(
          clamp(result[0], 0.0f, 255.0f),
          clamp(result[1], 0.0f, 255.0f),
          clamp(result[2], 0.0f, 255.0f),
          clamp(result[3], 0.0f, 255.0f)
        ));
    }
  }

  return destImage;
}

}
