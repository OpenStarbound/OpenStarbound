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

static inline Vec4B nearestPixel(Image const& srcImage, Vec2F const& scale, unsigned x, unsigned y) {
  return srcImage.clamp(Vec2I::round(vdiv(Vec2F(x, y), scale)));
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
  Image destImage(destSize, srcImage.pixelFormat());

  for (unsigned y = 0; y < destSize[1]; ++y) {
    for (unsigned x = 0; x < destSize[0]; ++x)
      destImage.set({x, y}, nearestPixel(srcImage, scale, x, y));
  }
  return destImage;
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
  Image destImage(wsize, srcImage.pixelFormat());

  for (unsigned y = 0; y < wsize[1]; ++y) {
    for (unsigned x = 0; x < wsize[0]; ++x)
      destImage.set({x, y}, nearestPixel(srcImage, scale, wmin[0] + x, wmin[1] + y));
  }
  return destImage;
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