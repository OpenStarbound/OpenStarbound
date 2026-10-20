#include "StarImageProcessing.hpp"
#include "StarImageScaling.hpp"
#include "StarMatrix3.hpp"
#include "StarInterpolation.hpp"
#include "StarLexicalCast.hpp"
#include "StarColor.hpp"
#include "StarImage.hpp"
#include "StarStringView.hpp"
#include "StarEncode.hpp"
#include "StarLogging.hpp"

namespace Star {

StringList colorDirectivesFromConfig(JsonArray const& directives) {
  List<String> result;

  for (auto entry : directives) {
    if (entry.type() == Json::Type::String) {
      result.append(entry.toString());
    } else if (entry.type() == Json::Type::Object) {
      result.append(paletteSwapDirectivesFromConfig(entry));
    } else {
      throw StarException("Malformed color directives list.");
    }
  }
  return result;
}

String paletteSwapDirectivesFromConfig(Json const& swaps) {
  ColorReplaceImageOperation paletteSwaps;
  for (auto const& swap : swaps.iterateObject())
    paletteSwaps.colorReplaceMap[Color::fromHex(swap.first).toRgba()] = Color::fromHex(swap.second.toString()).toRgba();
  return "?" + imageOperationToString(paletteSwaps);
}

HueShiftImageOperation HueShiftImageOperation::hueShiftDegrees(float degrees) {
  return HueShiftImageOperation{degrees / 360.0f};
}

SaturationShiftImageOperation SaturationShiftImageOperation::saturationShift100(float amount) {
  return SaturationShiftImageOperation{amount / 100.0f};
}

BrightnessMultiplyImageOperation BrightnessMultiplyImageOperation::brightnessMultiply100(float amount) {
  return BrightnessMultiplyImageOperation{amount / 100.0f + 1.0f};
}

FadeToColorImageOperation::FadeToColorImageOperation(Vec3B color, float amount) {
  this->color = color;
  this->amount = amount;

  auto fcl = Color::rgb(color).toLinear();
  for (int i = 0; i <= 255; ++i) {
    auto r = Color::rgb(Vec3B(i, i, i)).toLinear().mix(fcl, amount).toSRGB().toRgb();
    rTable[i] = r[0];
    gTable[i] = r[1];
    bTable[i] = r[2];
  }
}

ImageOperation imageOperationFromString(StringView string) {
  try {
    std::string_view view = string.utf8();
    //double time = view.size() > 10000 ? Time::monotonicTime() : 0.0;
    auto firstBitEnd = view.find_first_of("=;");
    if (view.substr(0, firstBitEnd).compare("replace") == 0 && (firstBitEnd + 1) != view.size()) {
      //Perform optimized replace parse
      ColorReplaceImageOperation operation;

      std::string_view bits = view.substr(firstBitEnd + 1);
      operation.colorReplaceMap.reserve(bits.size() / 8);

      char const* hexPtr = nullptr;
      unsigned int hexLen = 0;

      char const* ptr = bits.data();
      char const* end = ptr + bits.size();

      char a[4]{}, b[4]{};
      bool which = true;

      while (true) {
        char ch = *ptr;

        if (ch == '=' || ch == ';' || ptr == end) {
          if (hexLen != 0) {
            char* c = which ? a : b;

            if (hexLen == 3) {
              nibbleDecode(hexPtr, 3, c, 4);
              c[0] |= (c[0] << 4);
              c[1] |= (c[1] << 4);
              c[2] |= (c[2] << 4);
              c[3] = static_cast<char>(255);
            }
            else if (hexLen == 4) {
              nibbleDecode(hexPtr, 4, c, 4);
              c[0] |= (c[0] << 4);
              c[1] |= (c[1] << 4);
              c[2] |= (c[2] << 4);
              c[3] |= (c[3] << 4);
            }
            else if (hexLen == 6) {
              hexDecode(hexPtr, 6, c, 4);
              c[3] = static_cast<char>(255);
            }
            else if (hexLen == 8) {
              hexDecode(hexPtr, 8, c, 4);
            }
            else if (!which || (ptr != end && ++ptr != end))
                return ErrorImageOperation{strf("Improper size for hex string '{}'", StringView(hexPtr, hexLen))};
            else // we're in A of A=B. In vanilla only A=B pairs are evaluated, so only throw an error if B is also there.
                return operation;

            if ((which = !which))
              operation.colorReplaceMap[*(Vec4B*)&a] = *(Vec4B*)&b;

            hexLen = 0;
          }
        }
        else if (!hexLen++)
          hexPtr = ptr;

        if (ptr++ == end)
          break;
      }

      //if (time != 0.0)
      //  Logger::logf(LogLevel::Debug, "Parsed %u long directives to %u replace operations in %fs", view.size(), operation.colorReplaceMap.size(), Time::monotonicTime() - time);
      return operation;
    }

    List<StringView> bits;

    string.forEachSplitAnyView("=;", [&](StringView split, size_t, size_t) {
      if (!split.empty())
        bits.emplace_back(split);
    });

    StringView const& type = bits.at(0);

    if (type == "hueshift") {
      return HueShiftImageOperation::hueShiftDegrees(lexicalCast<float>(bits.at(1)));

    } else if (type == "saturation") {
      return SaturationShiftImageOperation::saturationShift100(lexicalCast<float>(bits.at(1)));

    } else if (type == "brightness") {
      return BrightnessMultiplyImageOperation::brightnessMultiply100(lexicalCast<float>(bits.at(1)));

    } else if (type == "fade") {
      return FadeToColorImageOperation(Color::fromHex(bits.at(1)).toRgb(), lexicalCast<float>(bits.at(2)));

    } else if (type == "scanlines") {
      return ScanLinesImageOperation{
          FadeToColorImageOperation(Color::fromHex(bits.at(1)).toRgb(), lexicalCast<float>(bits.at(2))),
          FadeToColorImageOperation(Color::fromHex(bits.at(3)).toRgb(), lexicalCast<float>(bits.at(4)))};

    } else if (type == "setcolor") {
      return SetColorImageOperation{Color::fromHex(bits.at(1)).toRgb()};

    } else if (type == "replace") {
      ColorReplaceImageOperation operation;
      for (size_t i = 0; i < (bits.size() - 1) / 2; ++i)
        operation.colorReplaceMap[Color::hexToVec4B(bits[i * 2 + 1])] = Color::hexToVec4B(bits[i * 2 + 2]);

      return operation;

    } else if (type == "addmask" || type == "submask") {
      AlphaMaskImageOperation operation;
      if (type == "addmask")
        operation.mode = AlphaMaskImageOperation::Additive;
      else
        operation.mode = AlphaMaskImageOperation::Subtractive;

      operation.maskImages = String(bits.at(1)).split('+');

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else if (type == "blendmult" || type == "blendscreen") {
      BlendImageOperation operation;

      if (type == "blendmult")
        operation.mode = BlendImageOperation::Multiply;
      else
        operation.mode = BlendImageOperation::Screen;

      operation.blendImages = String(bits.at(1)).split('+');

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else if (type == "multiply") {
      return MultiplyImageOperation{Color::fromHex(bits.at(1)).toRgba()};

    } else if (type == "border" || type == "outline") {
      BorderImageOperation operation;
      operation.pixels = lexicalCast<unsigned>(bits.at(1));
      operation.startColor = Color::fromHex(bits.at(2)).toRgba();
      if (bits.size() > 3)
        operation.endColor = Color::fromHex(bits.at(3)).toRgba();
      else
        operation.endColor = operation.startColor;
      operation.outlineOnly = type == "outline";
      operation.includeTransparent = false; // Currently just here for anti-aliased fonts

      return operation;

    } else if (type == "scalenearest" || type == "scalebilinear" || type == "scalebicubic" || type == "scale") {
      Vec2F scale;
      if (bits.size() == 2)
        scale = Vec2F::filled(lexicalCast<float>(bits.at(1)));
      else
        scale = Vec2F(lexicalCast<float>(bits.at(1)), lexicalCast<float>(bits.at(2)));

      ScaleImageOperation::Mode mode;
      if (type == "scalenearest")
        mode = ScaleImageOperation::Nearest;
      else if (type == "scalebicubic")
        mode = ScaleImageOperation::Bicubic;
      else
        mode = ScaleImageOperation::Bilinear;

      return ScaleImageOperation{mode, scale};

    } else if (type == "crop") {
      return CropImageOperation{RectI(lexicalCast<float>(bits.at(1)), lexicalCast<float>(bits.at(2)),
          lexicalCast<float>(bits.at(3)), lexicalCast<float>(bits.at(4)))};

    } else if (type == "flipx") {
      return FlipImageOperation{FlipImageOperation::FlipX};

    } else if (type == "flipy") {
      return FlipImageOperation{FlipImageOperation::FlipY};

    } else if (type == "flipxy") {
      return FlipImageOperation{FlipImageOperation::FlipXY};

    } else {
      return NullImageOperation();
    }
  } catch (OutOfRangeException const& e) {
    return ErrorImageOperation{std::string(e.what())};
  } catch (BadLexicalCast const& e) {
    return ErrorImageOperation{std::string(e.what())};
  } catch (StarException const& e) {
    return ErrorImageOperation{std::string(e.what())};
  }
}

String imageOperationToString(ImageOperation const& operation) {
  if (auto op = operation.ptr<HueShiftImageOperation>()) {
    return strf("hueshift={}", op->hueShiftAmount * 360.0f);
  } else if (auto op = operation.ptr<SaturationShiftImageOperation>()) {
    return strf("saturation={}", op->saturationShiftAmount * 100.0f);
  } else if (auto op = operation.ptr<BrightnessMultiplyImageOperation>()) {
    return strf("brightness={}", (op->brightnessMultiply - 1.0f) * 100.0f);
  } else if (auto op = operation.ptr<FadeToColorImageOperation>()) {
    return strf("fade={}={}", Color::rgb(op->color).toHex(), op->amount);
  } else if (auto op = operation.ptr<ScanLinesImageOperation>()) {
    return strf("scanlines={}={}={}={}",
        Color::rgb(op->fade1.color).toHex(),
        op->fade1.amount,
        Color::rgb(op->fade2.color).toHex(),
        op->fade2.amount);
  } else if (auto op = operation.ptr<SetColorImageOperation>()) {
    return strf("setcolor={}", Color::rgb(op->color).toHex());
  } else if (auto op = operation.ptr<ColorReplaceImageOperation>()) {
    String str = "replace";
    for (auto const& pair : op->colorReplaceMap)
      str += strf(";{}={}", Color::rgba(pair.first).toHex(), Color::rgba(pair.second).toHex());
    return str;
  } else if (auto op = operation.ptr<AlphaMaskImageOperation>()) {
    if (op->mode == AlphaMaskImageOperation::Additive)
      return strf("addmask={};{};{}", op->maskImages.join("+"), op->offset[0], op->offset[1]);
    else if (op->mode == AlphaMaskImageOperation::Subtractive)
      return strf("submask={};{};{}", op->maskImages.join("+"), op->offset[0], op->offset[1]);
  } else if (auto op = operation.ptr<BlendImageOperation>()) {
    if (op->mode == BlendImageOperation::Multiply)
      return strf("blendmult={};{};{}", op->blendImages.join("+"), op->offset[0], op->offset[1]);
    else if (op->mode == BlendImageOperation::Screen)
      return strf("blendscreen={};{};{}", op->blendImages.join("+"), op->offset[0], op->offset[1]);
  } else if (auto op = operation.ptr<MultiplyImageOperation>()) {
    return strf("multiply={}", Color::rgba(op->color).toHex());
  } else if (auto op = operation.ptr<BorderImageOperation>()) {
    if (op->outlineOnly)
      return strf("outline={};{};{}", op->pixels, Color::rgba(op->startColor).toHex(), Color::rgba(op->endColor).toHex());
    else
      return strf("border={};{};{}", op->pixels, Color::rgba(op->startColor).toHex(), Color::rgba(op->endColor).toHex());
  } else if (auto op = operation.ptr<ScaleImageOperation>()) {
    if (op->mode == ScaleImageOperation::Nearest)
      return strf("scalenearest={}", op->scale);
    else if (op->mode == ScaleImageOperation::Bilinear)
      return strf("scalebilinear={}", op->scale);
    else if (op->mode == ScaleImageOperation::Bicubic)
      return strf("scalebicubic={}", op->scale);
  } else if (auto op = operation.ptr<CropImageOperation>()) {
    return strf("crop={};{};{};{}", op->subset.xMin(), op->subset.xMax(), op->subset.yMin(), op->subset.yMax());
  } else if (auto op = operation.ptr<FlipImageOperation>()) {
    if (op->mode == FlipImageOperation::FlipX)
      return "flipx";
    else if (op->mode == FlipImageOperation::FlipY)
      return "flipy";
    else if (op->mode == FlipImageOperation::FlipXY)
      return "flipxy";
  }

  return "";
}

void parseImageOperations(StringView params, function<void(ImageOperation&&)> outputter) {
  params.forEachSplitView("?", [&](StringView op, size_t, size_t) {
    if (!op.empty())
      outputter(imageOperationFromString(op));
  });
}

List<ImageOperation> parseImageOperations(StringView params) {
  List<ImageOperation> operations;
  params.forEachSplitView("?", [&](StringView op, size_t, size_t) {
    if (!op.empty())
      operations.append(imageOperationFromString(op));
    });

  return operations;
}

String printImageOperations(List<ImageOperation> const& list) {
  return StringList(list.transformed(imageOperationToString)).join("?");
}

void addImageOperationReferences(ImageOperation const& operation, StringList& out) {
  if (auto op = operation.ptr<AlphaMaskImageOperation>())
    out.appendAll(op->maskImages);
  else if (auto op = operation.ptr<BlendImageOperation>())
    out.appendAll(op->blendImages);
}

StringList imageOperationReferences(List<ImageOperation> const& operations) {
  StringList references;
  for (auto const& operation : operations)
    addImageOperationReferences(operation, references);
  return references;
}

// The hue, saturation and brightness operations below turn every pixel through floating point color math.
// However, most images have far fewer distinct colors than pixels, especially the heaviest, largest ones..
// This remembers the result for recently seen colors, the pixel that computing it again would give.
struct RecentColors {
  struct Entry {
    uint32_t in = 0;   // 0 is never looked u,: a pixel with alpha 0 is skipped before it gets here
    uint32_t out = 0;
  };

  static uint32_t pack(Vec4B const& c) {
    return (uint32_t)c[0] | (uint32_t)c[1] << 8 | (uint32_t)c[2] << 16 | (uint32_t)c[3] << 24;
  }

  static Vec4B unpack(uint32_t v) {
    return Vec4B(v & 255, (v >> 8) & 255, (v >> 16) & 255, v >> 24);
  }

  // Replaces the pixel with compute(pixel)
  template <typename Function>
  void apply(Vec4B& pixel, Function&& compute) {
    uint32_t key = pack(pixel);
    Entry& entry = entries[(key * 2654435761u) >> 24];
    if (entry.in != key) {
      entry.out = pack(compute(pixel));
      entry.in = key;
    }
    pixel = unpack(entry.out);
  }

  Entry entries[256];
};

#ifdef STAR_COMPILER_GNU
#pragma GCC push_options
#pragma GCC optimize("-fno-fast-math", "-fassociative-math", "-freciprocal-math")
#endif
static void processSaturationShift(Image& image, SaturationShiftImageOperation const* op) {
  RecentColors recent;
  image.forEachPixel([&](unsigned, unsigned, Vec4B& pixel) {
    if (pixel[3] != 0) {
      recent.apply(pixel, [&op](Vec4B const& p) { return Color::shiftSaturationVec4B(p, op->saturationShiftAmount); });
    }
  });
}
#ifdef STAR_COMPILER_GNU
#pragma GCC pop_options
#endif


// The pixels [x0, x1) x [y0, y1) of an image
struct PixelRect {
  long x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  bool empty() const { return x0 >= x1 || y0 >= y1; }
};

// Replaces the colors of the pixels inside the rectangle. The image has four bytes per pixel.
static void replaceColors(ColorReplaceMap const& replaceMap, Image& image, PixelRect rect) {
  if (replaceMap.empty() || rect.empty())
    return;

  if ((rect.x1 - rect.x0) * (rect.y1 - rect.y0) < 1024) {
    for (long y = rect.y0; y < rect.y1; ++y) {
      for (long x = rect.x0; x < rect.x1; ++x) {
        Vec4B pixel = image.get(x, y);
        if (auto m = replaceMap.maybe(pixel))
          image.set(x, y, *m);
      }
    }
    return;
  }

  // Most pixels are not a key, but each one costs a hash lookup.
  // A bitmap of the keys' hashes rejects almost all of the non-keys without touching the map.
  // A pixel is only ever replaced if it is a key, so the result is the same.
  unsigned bitCount = 4096;
  while (bitCount < replaceMap.size() * 32 && bitCount < (1u << 20))
    bitCount <<= 1;
#if defined(STAR_COMPILER_GNU) || defined(STAR_COMPILER_CLANG)
  unsigned shift = 32 - __builtin_ctz(bitCount);
#else
  unsigned shift = 32;
  for (unsigned n = bitCount; n > 1; n >>= 1)
    --shift;
#endif
  std::vector<uint64_t> bits(bitCount / 64, 0);
  auto bitFor = [shift](Vec4B const& c) -> unsigned {
    uint32_t v = (uint32_t)c[0] | (uint32_t)c[1] << 8 | (uint32_t)c[2] << 16 | (uint32_t)c[3] << 24;
    return (v * 2654435761u) >> shift;
  };
  for (auto const& entry : replaceMap) {
    unsigned bit = bitFor(entry.first);
    bits[bit >> 6] |= 1ull << (bit & 63);
  }

  // Large areas of one color are common (the backgrounds of sign and costume drawables), and a pixel that equals the one before it
  // gets the same result, so that one is remembered.
  static_assert(sizeof(Vec4B) == sizeof(uint32_t), "a pixel is four bytes");
  uint8_t* data = image.data();
  uint32_t lastIn = 0, lastOut = 0;
  bool haveLast = false;
  for (long y = rect.y0; y < rect.y1; ++y) {
    uint8_t* row = data + ((size_t)y * image.width() + rect.x0) * 4;
    for (long x = 0; x < rect.x1 - rect.x0; ++x) {
      uint32_t value;
      memcpy(&value, row + x * 4, 4);
      if (haveLast && value == lastIn) {
        memcpy(row + x * 4, &lastOut, 4);
        continue;
      }
      Vec4B pixel;
      memcpy(&pixel, &value, 4);
      uint32_t result = value;
      unsigned bit = bitFor(pixel);
      if ((bits[bit >> 6] >> (bit & 63)) & 1) {
        if (auto m = replaceMap.maybe(pixel)) {
          memcpy(&result, m->ptr(), 4);
          memcpy(row + x * 4, &result, 4);
        }
      }
      lastIn = value;
      lastOut = result;
      haveLast = true;
    }
  }
}

// Blends the images into the target and returns the rectangle of pixels that can have changed.
static PixelRect applyBlend(BlendImageOperation const& op, Image& image, ImageReferenceCallback const& refCallback) {
  if (op.blendImages.empty())
    return {};

  if (!refCallback)
    throw StarException("Missing image ref callback during BlendImageOperation in ImageProcessor::process");

  List<Image const*> blendImages;
  for (auto const& reference : op.blendImages)
    blendImages.append(refCallback(reference));

  // A pixel is only changed where one of the blend images lies under it.
  // Everywhere else, the original loop converted the pixel to float and back, which gives the same byte again.
  // Large images blended with a small one (sign tiles) therefore only need the bounding box of the places where a blend image overlaps.
  long minX = (long)image.width(), minY = (long)image.height(), maxX = 0, maxY = 0;
  for (auto blend : blendImages) {
    minX = std::min(minX, std::max(0L, -(long)op.offset[0]));
    minY = std::min(minY, std::max(0L, -(long)op.offset[1]));
    maxX = std::max(maxX, std::min((long)image.width(), (long)blend->width() - op.offset[0]));
    maxY = std::max(maxY, std::min((long)image.height(), (long)blend->height() - op.offset[1]));
  }

  for (long y = minY; y < maxY; ++y) {
    for (long x = minX; x < maxX; ++x) {
      Vec2U pos = Vec2U(Vec2I(x, y) + op.offset);
      Vec4B pixel = image.get(x, y);
      Vec4F fpixel = Color::v4bToFloat(pixel);
      for (auto blend : blendImages) {
        if (pos[0] < blend->width() && pos[1] < blend->height()) {
          Vec4F blendPixel = Color::v4bToFloat(blend->get(pos));
          if (op.mode == BlendImageOperation::Multiply)
            fpixel = fpixel.piecewiseMultiply(blendPixel);
          else if (op.mode == BlendImageOperation::Screen)
            fpixel = Vec4F::filled(1.0f) - (Vec4F::filled(1.0f) - fpixel).piecewiseMultiply(Vec4F::filled(1.0f) - blendPixel);
        }
      }
      image.set(x, y, Color::v4fToByte(fpixel));
    }
  }
  return {minX, minY, maxX, maxY};
}


void processImageOperation(ImageOperation const& operation, Image& image, ImageReferenceCallback refCallback) {
  if (image.bytesPerPixel() == 3) {
    // Convert to an image format that has alpha so certain operations function properly
    image = image.convert(image.pixelFormat() == PixelFormat::BGR24 ? PixelFormat::BGRA32 : PixelFormat::RGBA32);
  }
  // A hue shift, saturation shift or brightness multiplier of exactly zero (?hueshift=0 is in every vanilla parallax path) hands
  // back every pixel unchanged, for every color and every alpha value, so those passes are skipped.
  if (auto op = operation.ptr<HueShiftImageOperation>()) {
    if (op->hueShiftAmount == 0.0f)
      return;
    RecentColors recent;
    image.forEachPixel([&](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0)
        recent.apply(pixel, [&op](Vec4B const& p) { return Color::hueShiftVec4B(p, op->hueShiftAmount); });
    });
  } else if (auto op = operation.ptr<SaturationShiftImageOperation>()) {
    if (op->saturationShiftAmount != 0.0f)
      processSaturationShift(image, op);
  } else if (auto op = operation.ptr<BrightnessMultiplyImageOperation>()) {
    if (op->brightnessMultiply == 1.0f)
      return;
    RecentColors recent;
    image.forEachPixel([&](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0) {
        recent.apply(pixel, [&op](Vec4B const& p) { return Color::scaleValueVec4B(p, op->brightnessMultiply); });
      }
    });
  } else if (auto op = operation.ptr<FadeToColorImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel[0] = op->rTable[pixel[0]];
      pixel[1] = op->gTable[pixel[1]];
      pixel[2] = op->bTable[pixel[2]];
    });
  } else if (auto op = operation.ptr<ScanLinesImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned y, Vec4B& pixel) {
      if (y % 2 == 0) {
        pixel[0] = op->fade1.rTable[pixel[0]];
        pixel[1] = op->fade1.gTable[pixel[1]];
        pixel[2] = op->fade1.bTable[pixel[2]];
      } else {
        pixel[0] = op->fade2.rTable[pixel[0]];
        pixel[1] = op->fade2.gTable[pixel[1]];
        pixel[2] = op->fade2.bTable[pixel[2]];
      }
    });
  } else if (auto op = operation.ptr<SetColorImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel[0] = op->color[0];
      pixel[1] = op->color[1];
      pixel[2] = op->color[2];
    });
  } else if (auto op = operation.ptr<ColorReplaceImageOperation>()) {
    replaceColors(op->colorReplaceMap, image, {0, 0, (long)image.width(), (long)image.height()});

  } else if (auto op = operation.ptr<AlphaMaskImageOperation>()) {
    if (op->maskImages.empty())
      return;

    if (!refCallback)
      throw StarException("Missing image ref callback during AlphaMaskImageOperation in ImageProcessor::process");

    List<Image const*> maskImages;
    for (auto const& reference : op->maskImages)
      maskImages.append(refCallback(reference));

    image.forEachPixel([&op, &maskImages](unsigned x, unsigned y, Vec4B& pixel) {
      uint8_t maskAlpha = 0;
      Vec2U pos = Vec2U(Vec2I(x, y) + op->offset);
      for (auto mask : maskImages) {
        if (pos[0] < mask->width() && pos[1] < mask->height()) {
          if (op->mode == AlphaMaskImageOperation::Additive) {
            // We produce our mask alpha from the maximum alpha of any of
            // the
            // mask images.
            maskAlpha = std::max(maskAlpha, mask->get(pos)[3]);
          } else if (op->mode == AlphaMaskImageOperation::Subtractive) {
            // We produce our mask alpha from the minimum alpha of any of
            // the
            // mask images.
            maskAlpha = std::min(maskAlpha, mask->get(pos)[3]);
          }
        }
      }
      pixel[3] = std::min(pixel[3], maskAlpha);
    });

  } else if (auto op = operation.ptr<BlendImageOperation>()) {
    applyBlend(*op, image, refCallback);

  } else if (auto op = operation.ptr<MultiplyImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel = pixel.combine(op->color, [](uint8_t a, uint8_t b) -> uint8_t {
          return (uint8_t)(((int)a * (int)b) / 255);
        });
    });

  } else if (auto op = operation.ptr<BorderImageOperation>()) {
    Image borderImage(image.size() + Vec2U::filled(op->pixels * 2), PixelFormat::RGBA32);
    borderImage.copyInto(Vec2U::filled(op->pixels), image);
    Vec2I borderImageSize = Vec2I(borderImage.size());

    borderImage.forEachPixel([&op, &image, &borderImageSize](int x, int y, Vec4B& pixel) {
      int pixels = op->pixels;
      bool includeTransparent = op->includeTransparent;
      if (pixel[3] == 0 || (includeTransparent && pixel[3] != 255)) {
        int dist = std::numeric_limits<int>::max();
        for (int j = -pixels; j < pixels + 1; j++) {
          for (int i = -pixels; i < pixels + 1; i++) {
            if (i + x >= pixels && j + y >= pixels && i + x < borderImageSize[0] - pixels && j + y < borderImageSize[1] - pixels) {
              Vec4B remotePixel = image.get(i + x - pixels, j + y - pixels);
              if (remotePixel[3] != 0) {
                dist = std::min(dist, abs(i) + abs(j));
                if (dist == 1) // Early out, if dist is 1 it ain't getting shorter
                  break;
              }
            }
          }
        }

        if (dist < std::numeric_limits<int>::max()) {
          float percent = (dist - 1) / (2.0f * pixels - 1);
          if (pixel[3] != 0) {
            Color color = Color::rgba(op->startColor).mix(Color::rgba(op->endColor), percent);
            if (op->outlineOnly) {
              float pixelA = byteToFloat(pixel[3]);
              color.setAlphaF((1.0f - pixelA) * fminf(pixelA, 0.5f) * 2.0f);
            }
            else {
              Color pixelF = Color::rgba(pixel);
              float pixelA = pixelF.alphaF(), colorA = color.alphaF();
              colorA += pixelA * (1.0f - colorA);
              pixelF.convertToLinear(); //Mix in linear color space as it is more perceptually accurate
              color.convertToLinear();
              color = color.mix(pixelF, pixelA);
              color.convertToSRGB();
              color.setAlphaF(colorA);
            }
            pixel = color.toRgba();
          } else {
            pixel = Vec4B(Vec4F(op->startColor) * (1 - percent) + Vec4F(op->endColor) * percent);
          }
        }
      } else if (op->outlineOnly) {
        pixel = Vec4B(0, 0, 0, 0);
      }
    });

    image = borderImage;

  } else if (auto op = operation.ptr<ScaleImageOperation>()) {
    auto scale = op->scale;
    if (scale[0] < 0.0f || scale[1] < 0.0f) {
      Logger::warn("Negative scale in ScaleImageOperation ({})", scale);
      scale = scale.piecewiseMax(Vec2F::filled(0.f));
    }
    if (op->mode == ScaleImageOperation::Nearest)
      image = scaleNearest(image, scale);
    else if (op->mode == ScaleImageOperation::Bilinear)
      image = scaleBilinear(image, scale);
    else if (op->mode == ScaleImageOperation::Bicubic)
      image = scaleBicubic(image, scale);

  } else if (auto op = operation.ptr<CropImageOperation>()) {
    image = image.subImage(Vec2U(op->subset.min()), Vec2U(op->subset.size()));

  } else if (auto op = operation.ptr<FlipImageOperation>()) {
    if (op->mode == FlipImageOperation::FlipX || op->mode == FlipImageOperation::FlipXY) {
      for (size_t y = 0; y < image.height(); ++y) {
        for (size_t xLeft = 0; xLeft < image.width() / 2; ++xLeft) {
          size_t xRight = image.width() - 1 - xLeft;

          auto left = image.get(xLeft, y);
          auto right = image.get(xRight, y);

          image.set(xLeft, y, right);
          image.set(xRight, y, left);
        }
      }
    }

    if (op->mode == FlipImageOperation::FlipY || op->mode == FlipImageOperation::FlipXY) {
      for (size_t x = 0; x < image.width(); ++x) {
        for (size_t yTop = 0; yTop < image.height() / 2; ++yTop) {
          size_t yBottom = image.height() - 1 - yTop;

          auto top = image.get(x, yTop);
          auto bottom = image.get(x, yBottom);

          image.set(x, yTop, bottom);
          image.set(x, yBottom, top);
        }
      }
    }
  }
}

bool processScaleCrop(ScaleImageOperation const& scale, CropImageOperation const& crop, Image& image) {
  if (scale.mode == ScaleImageOperation::Bicubic)
    return false;
  if (scale.scale[0] < 0.0f || scale.scale[1] < 0.0f)
    return false;  // processImageOperation warns about this and clamps it

  if (image.bytesPerPixel() == 3) {
    // Same conversion processImageOperation does before any operation
    image = image.convert(image.pixelFormat() == PixelFormat::BGR24 ? PixelFormat::BGRA32 : PixelFormat::RGBA32);
  }

  if (!scaledCropFits(image, scale.scale, crop.subset))
    return false;

  if (scale.mode == ScaleImageOperation::Nearest)
    image = scaleNearestCropped(image, scale.scale, crop.subset);
  else
    image = scaleBilinearCropped(image, scale.scale, crop.subset);
  return true;
}

// Operations whose result for a pixel depends on that pixel alone.
static bool isPerPixelColorOperation(ImageOperation const& operation) {
  return operation.is<HueShiftImageOperation>()
      || operation.is<SaturationShiftImageOperation>()
      || operation.is<BrightnessMultiplyImageOperation>()
      || operation.is<FadeToColorImageOperation>()
      || operation.is<SetColorImageOperation>()
      || operation.is<ColorReplaceImageOperation>()
      || operation.is<MultiplyImageOperation>();
}

// A nearest neighbour scale that makes the image bigger by copying pixels.
// An image with no pixels has none to copy, so the scale then makes pixels of {0, 0, 0, 0} out of nothing, which later color operations would change, so that is not a case for this.
static bool scaleEnlargesByCopying(ScaleImageOperation const& scale, Image const& image) {
  if (image.width() == 0 || image.height() == 0)
    return false;
  if (scale.mode != ScaleImageOperation::Nearest || scale.scale[0] < 0.0f || scale.scale[1] < 0.0f)
    return false;
  Vec2U destSize = Vec2U::round(vmult(Vec2F(image.size()), scale.scale));
  destSize[0] = max(destSize[0], 1u);
  destSize[1] = max(destSize[1], 1u);
  return (uint64_t)destSize[0] * destSize[1] > (uint64_t)image.width() * image.height();
}

// A crop that fits in the image and keeps fewer pixels than it has
static bool cropShrinks(CropImageOperation const& crop, Image const& image) {
  Vec2I min = crop.subset.min();
  Vec2I size = Vec2I(crop.subset.size());
  if (min[0] < 0 || min[1] < 0 || size[0] < 0 || size[1] < 0 || min[0] + size[0] > (int)image.width() || min[1] + size[1] > (int)image.height())
    return false;
  return (uint64_t)size[0] * size[1] < (uint64_t)image.width() * image.height();
}

// A replace leaves no pixel that is one of its keys, as long as none of the colors it makes is a key itself.
static bool replaceSettles(ColorReplaceMap const& map) {
  for (auto const& entry : map) {
    if (map.contains(entry.second))
      return false;
  }
  return true;
}

static bool replaceKeysAreKeysOf(ColorReplaceMap const& map, ColorReplaceMap const& other) {
  for (auto const& entry : map) {
    if (!other.contains(entry.first))
      return false;
  }
  return true;
}

// Signs and costume drawables are made of a replace, blends that each touch a small area, the same replace again, and so on.
// After a replace that settles, none of the pixels are its keys. The blends only change the areas they cover, and a following replace
// with keys that are all keys of the one before can only find any in those areas, so it only needs to look there.
// Called with the index of the operation after a replace that has been applied, and returns the index of the next one to apply.
static size_t processReplaceBlendChain(List<ImageOperation const*> const& operations, size_t i, ColorReplaceImageOperation const* current, Image& image, ImageReferenceCallback const& refCallback) {
  size_t count = operations.size();
  if (image.bytesPerPixel() != 4 || i >= count || !operations[i]->is<BlendImageOperation>() || !replaceSettles(current->colorReplaceMap))
    return i;

  while (i < count && operations[i]->is<BlendImageOperation>()) {
    PixelRect changed;
    bool any = false;
    while (i < count && operations[i]->is<BlendImageOperation>()) {
      PixelRect rect = applyBlend(*operations[i]->ptr<BlendImageOperation>(), image, refCallback);
      ++i;
      if (rect.empty())
        continue;
      if (!any) {
        changed = rect;
        any = true;
      } else {
        changed.x0 = std::min(changed.x0, rect.x0);
        changed.y0 = std::min(changed.y0, rect.y0);
        changed.x1 = std::max(changed.x1, rect.x1);
        changed.y1 = std::max(changed.y1, rect.y1);
      }
    }
    if (i < count) {
      if (auto next = operations[i]->ptr<ColorReplaceImageOperation>()) {
        if (replaceKeysAreKeysOf(next->colorReplaceMap, current->colorReplaceMap) && replaceSettles(next->colorReplaceMap)) {
          replaceColors(next->colorReplaceMap, image, changed);
          current = next;
          ++i;
          continue;
        }
      }
    }
    break;
  }
  return i;
}

void processImageOperationList(List<ImageOperation const*> const& operations, Image& image, ImageReferenceCallback refCallback) {
  size_t count = operations.size();
  size_t i = 0;
  while (i < count) {
    ImageOperation const& operation = *operations[i];

    if (isPerPixelColorOperation(operation)) {
      size_t end = i;
      while (end < count && isPerPixelColorOperation(*operations[end]))
        ++end;
      // Colors then crop that makes the image smaller get swapped, so the colors are applied to fewer pixels
      if (end < count) {
        if (auto crop = operations[end]->ptr<CropImageOperation>()) {
          if (cropShrinks(*crop, image)) {
            processImageOperation(*operations[end], image, refCallback);
            for (size_t k = i; k < end; ++k)
              processImageOperation(*operations[k], image, refCallback);
            i = end + 1;
            continue;
          }
        }
      }
      for (size_t k = i; k < end; ++k)
        processImageOperation(*operations[k], image, refCallback);
      i = end;
      if (auto replace = operations[end - 1]->ptr<ColorReplaceImageOperation>())
        i = processReplaceBlendChain(operations, i, replace, image, refCallback);
      continue;
    }

    if (auto scale = operation.ptr<ScaleImageOperation>()) {
      size_t end = i + 1;
      while (end < count && isPerPixelColorOperation(*operations[end]))
        ++end;
      // Scale, then colors get swapped, so the scale only copies pixels into a bigger image
      if (end > i + 1 && scaleEnlargesByCopying(*scale, image)) {
        for (size_t k = i + 1; k < end; ++k)
          processImageOperation(*operations[k], image, refCallback);
        // then the scale itself, together with a crop right after it if there is one
        if (end < count) {
          if (auto crop = operations[end]->ptr<CropImageOperation>()) {
            if (processScaleCrop(*scale, *crop, image)) {
              i = end + 1;
              continue;
            }
          }
        }
        processImageOperation(operation, image, refCallback);
        i = end;
        continue;
      }
      if (i + 1 < count) {
        if (auto crop = operations[i + 1]->ptr<CropImageOperation>()) {
          if (processScaleCrop(*scale, *crop, image)) {
            i += 2;
            continue;
          }
        }
      }
    }

    processImageOperation(operation, image, refCallback);
    ++i;
  }
}

Image processImageOperations(List<ImageOperation> const& operations, Image image, ImageReferenceCallback refCallback) {
  List<ImageOperation const*> pointers;
  for (auto const& operation : operations)
    pointers.append(&operation);
  processImageOperationList(pointers, image, refCallback);
  return image;
}

}
