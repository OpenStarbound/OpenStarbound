#pragma once

#include "StarRect.hpp"

namespace Star {

STAR_CLASS(Image);
Image scaleNearest(Image const& srcImage, Vec2F const& scale);
Image scaleBilinear(Image const& srcImage, Vec2F const& scale);
Image scaleBicubic(Image const& srcImage, Vec2F const& scale);

// A scale immediately followed by a crop only needs the pixels the crop keeps.
// These compute exactly those pixels, with the same arithmetic as scaleNearest / scaleBilinear, so the result
// equals scaling the whole image and then cropping it.
// scaledCropFits says whether the window lies inside the scaled image, the cropped scales assume it does.
bool scaledCropFits(Image const& srcImage, Vec2F const& scale, RectI const& window);
Image scaleNearestCropped(Image const& srcImage, Vec2F const& scale, RectI const& window);
Image scaleBilinearCropped(Image const& srcImage, Vec2F const& scale, RectI const& window);

}
