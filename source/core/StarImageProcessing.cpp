#include "StarImageProcessing.hpp"
#include "StarMatrix3.hpp"
#include "StarInterpolation.hpp"
#include "StarLexicalCast.hpp"
#include "StarColor.hpp"
#include "StarImage.hpp"
#include "StarStringView.hpp"
#include "StarEncode.hpp"
//#include "StarTime.hpp"
//#include "StarLogging.hpp"

namespace Star {

Image scaleNearest(Image const& srcImage, Vec2F const& scale) {
  Vec2U srcSize = srcImage.size();
  Vec2U destSize = Vec2U::round(vmult(Vec2F(srcSize), scale));
  destSize[0] = max(destSize[0], 1u);
  destSize[1] = max(destSize[1], 1u);

  Image destImage(destSize, srcImage.pixelFormat());

  for (unsigned y = 0; y < destSize[1]; ++y) {
    for (unsigned x = 0; x < destSize[0]; ++x)
      destImage.set({x, y}, srcImage.clamp(Vec2I::round(vdiv(Vec2F(x, y), scale))));
  }
  return destImage;
}

Image scaleBilinear(Image const& srcImage, Vec2F const& scale) {
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

      auto result = lerp(fpart[1], lerp(fpart[0], Vec4F(srcImage.clamp(ipart[0], ipart[1])), Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1]))), lerp(fpart[0],
            Vec4F(srcImage.clamp(ipart[0], ipart[1] + 1)), Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 1))));

      destImage.set({x, y}, Vec4B(result));
    }
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
              
            if (which = !which)
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
    return ErrorImageOperation{std::exception_ptr()};
  } catch (BadLexicalCast const& e) {
    return ErrorImageOperation{std::exception_ptr()};
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

void processImageOperation(ImageOperation const& operation, Image& image, ImageReferenceCallback refCallback) {
  if (image.bytesPerPixel() == 3) {
    // Convert to an image format that has alpha so certain operations function properly
    image = image.convert(image.pixelFormat() == PixelFormat::BGR24 ? PixelFormat::BGRA32 : PixelFormat::RGBA32);
  }
  if (auto op = operation.ptr<HueShiftImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0)
        pixel = Color::hueShiftVec4B(pixel, op->hueShiftAmount);
    });
  } else if (auto op = operation.ptr<SaturationShiftImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0) {
        Color color = Color::rgba(pixel);
        color.setSaturation(clamp(color.saturation() + op->saturationShiftAmount, 0.0f, 1.0f));
        pixel = color.toRgba();
      }
    });
  } else if (auto op = operation.ptr<BrightnessMultiplyImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0) {
        Color color = Color::rgba(pixel);
        color.setValue(clamp(color.value() * op->brightnessMultiply, 0.0f, 1.0f));
        pixel = color.toRgba();
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
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (auto m = op->colorReplaceMap.maybe(pixel))
        pixel = *m;
    });

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
    if (op->blendImages.empty())
      return;

    if (!refCallback)
      throw StarException("Missing image ref callback during BlendImageOperation in ImageProcessor::process");

    List<Image const*> blendImages;
    for (auto const& reference : op->blendImages)
      blendImages.append(refCallback(reference));

    image.forEachPixel([&op, &blendImages](unsigned x, unsigned y, Vec4B& pixel) {
      Vec2U pos = Vec2U(Vec2I(x, y) + op->offset);
      Vec4F fpixel = Color::v4bToFloat(pixel);
      for (auto blend : blendImages) {
        if (pos[0] < blend->width() && pos[1] < blend->height()) {
          Vec4F blendPixel = Color::v4bToFloat(blend->get(pos));
          if (op->mode == BlendImageOperation::Multiply)
            fpixel = fpixel.piecewiseMultiply(blendPixel);
          else if (op->mode == BlendImageOperation::Screen)
            fpixel = Vec4F::filled(1.0f) - (Vec4F::filled(1.0f) - fpixel).piecewiseMultiply(Vec4F::filled(1.0f) - blendPixel);
        }
      }
      pixel = Color::v4fToByte(fpixel);
    });

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
    if (op->mode == ScaleImageOperation::Nearest)
      image = scaleNearest(image, op->scale);
    else if (op->mode == ScaleImageOperation::Bilinear)
      image = scaleBilinear(image, op->scale);
    else if (op->mode == ScaleImageOperation::Bicubic)
      image = scaleBicubic(image, op->scale);

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

Image processImageOperations(List<ImageOperation> const& operations, Image image, ImageReferenceCallback refCallback) {
  for (auto const& operation : operations)
    processImageOperation(operation, image, refCallback);

  return image;
}

}
