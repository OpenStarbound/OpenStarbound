#pragma once

#include "StarList.hpp"
#include "StarRect.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_CLASS(Image);

STAR_EXCEPTION(ImageOperationException, StarException);

StringList colorDirectivesFromConfig(JsonArray const& directives);
String paletteSwapDirectivesFromConfig(Json const& swaps);

struct NullImageOperation {
  bool unloaded = false;
};

struct ErrorImageOperation {
  Variant<std::string, std::exception_ptr> cause;
};

struct HueShiftImageOperation {
  // Specify hue shift angle as -360 to 360 rather than -1 to 1
  static HueShiftImageOperation hueShiftDegrees(float degrees);

  // value here is normalized to 1.0
  float hueShiftAmount;
};

struct SaturationShiftImageOperation {
  // Specify saturation shift as amount normalized to 100
  static SaturationShiftImageOperation saturationShift100(float amount);

  // value here is normalized to 1.0
  float saturationShiftAmount;
};

struct BrightnessMultiplyImageOperation {
  // Specify brightness multiply as amount where 0 means "no change" and 100
  // means "x2" and -100 means "x0"
  static BrightnessMultiplyImageOperation brightnessMultiply100(float amount);

  float brightnessMultiply;
};

// Fades R G and B channels to the given color by the given amount, ignores A
struct FadeToColorImageOperation {
  FadeToColorImageOperation(Vec3B color, float amount);

  Vec3B color;
  float amount;

  Array<uint8_t, 256> rTable;
  Array<uint8_t, 256> gTable;
  Array<uint8_t, 256> bTable;
};

// Applies two FadeToColor operations in alternating rows to produce a scanline effect
struct ScanLinesImageOperation {
  FadeToColorImageOperation fade1;
  FadeToColorImageOperation fade2;
};

// Sets RGB values to the given color, and ignores the alpha channel
struct SetColorImageOperation {
  Vec3B color;
};

typedef HashMap<Vec4B, Vec4B> ColorReplaceMap;

struct ColorReplaceImageOperation {
  ColorReplaceMap colorReplaceMap;
};

struct AlphaMaskImageOperation {
  enum MaskMode {
    Additive,
    Subtractive
  };

  MaskMode mode;
  StringList maskImages;
  Vec2I offset;
};

struct BlendImageOperation {
  enum BlendMode {
    Multiply,
    Screen
  };

  BlendMode mode;
  StringList blendImages;
  Vec2I offset;
};

struct MultiplyImageOperation {
  Vec4B color;
};

struct BorderImageOperation {
  unsigned pixels;
  Vec4B startColor;
  Vec4B endColor;
  bool outlineOnly;
  bool includeTransparent;
};

struct ScaleImageOperation {
  enum Mode {
    Nearest,
    Bilinear,
    Bicubic
  };

  Mode mode;
  Vec2F scale;
};

struct CropImageOperation {
  RectI subset;
};

struct FlipImageOperation {
  enum Mode {
    FlipX,
    FlipY,
    FlipXY
  };
  Mode mode;
};

typedef Variant<NullImageOperation, ErrorImageOperation, HueShiftImageOperation, SaturationShiftImageOperation, BrightnessMultiplyImageOperation, FadeToColorImageOperation,
  ScanLinesImageOperation, SetColorImageOperation, ColorReplaceImageOperation, AlphaMaskImageOperation, BlendImageOperation,
  MultiplyImageOperation, BorderImageOperation, ScaleImageOperation, CropImageOperation, FlipImageOperation> ImageOperation;

ImageOperation imageOperationFromString(StringView string);
String imageOperationToString(ImageOperation const& operation);

void parseImageOperations(StringView params, function<void(ImageOperation&&)> outputter);

// Each operation is assumed to be separated by '?', with parameters
// separated by ';' or '='
List<ImageOperation> parseImageOperations(StringView params);

// Each operation separated by '?', returns string with leading '?'
String printImageOperations(List<ImageOperation> const& operations);

void addImageOperationReferences(ImageOperation const& operation, StringList& out);

StringList imageOperationReferences(List<ImageOperation> const& operations);

typedef function<Image const*(String const& refName)> ImageReferenceCallback;

void processImageOperation(ImageOperation const& operation, Image& input, ImageReferenceCallback refCallback = {});

// A scale followed by a crop only needs the pixels the crop keeps, which is much cheaper when the scale makes a large image
// and the crop keeps a small part of it.
// Applies both operations with the same result as processImageOperation on each and returns true.
// Returns false without touching the image when they can't be combined (bicubic scale, negative scale, a crop
// outside the scaled image), in which case the two operations must be applied one by one.
bool processScaleCrop(ScaleImageOperation const& scale, CropImageOperation const& crop, Image& image);

// Applies the operations in order, saving work where doing it in a different order or together gives exactly the same image.
// * a scale followed by a crop only computes the pixels the crop keeps (processScaleCrop);
// * operations that look at one pixel at a time and nothing else (hue, saturation, brightness, fade, setcolor, replace, multiply)
//   come out the same before a nearest neighbour scale as after it, since that scale only copies pixels, so they are applied to
//   the smaller image before an enlarging scale;
// * the same operations come out the same after a crop as before it, so they are applied after a crop that makes the image smaller.
//
// Operations that depend on position or on neighbouring pixels (scanlines, border, blends, masks, bilinear scales...) are never moved.
void processImageOperationList(List<ImageOperation const*> const& operations, Image& image, ImageReferenceCallback refCallback = {});

Image processImageOperations(List<ImageOperation> const& operations, Image input, ImageReferenceCallback refCallback = {});

}
