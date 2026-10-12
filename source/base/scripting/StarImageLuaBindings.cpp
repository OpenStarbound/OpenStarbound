#include "StarImageLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarImage.hpp"
#include "StarRootBase.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarBuffer.hpp"
#include "StarPathEmbed.hpp"

namespace Star {

LuaMethods<Image> LuaUserDataMethods<Image>::make() {
  LuaMethods<Image> methods;

  methods.registerMethodWithSignature<Vec2U, Image&>("size", mem_fn(&Image::size));
  methods.registerMethodWithSignature<void, Image&, Vec2U, Image&>("drawInto", mem_fn(&Image::drawInto));
  methods.registerMethodWithSignature<void, Image&, Vec2U, Image&>("copyInto", mem_fn(&Image::copyInto));
  methods.registerMethod("set", [](Image& image, unsigned x, unsigned y, Color const& color) {
    image.set(x, y, color.toRgba()); 
  });

  methods.registerMethod("get", [](Image& image, unsigned x, unsigned y) {
    return Color::rgba(image.get(x, y));
  });

  methods.registerMethod("subImage", [](Image& image, Vec2U const& min, Vec2U const& size) {
    return image.subImage(min, size);
  });

  methods.registerMethod("process", [](Image& image, String const& directives) {
    return processImageOperations(parseImageOperations(directives), image, [](String const& path) -> Image const* {
      if (auto root = RootBase::singletonPtr())
        return root->assets()->image(path).get();
      else
        return nullptr;
    });
  });

  methods.registerMethod("embed", [](Image& image, String const& compression, MVariant<String,Json> const& frames, Maybe<String> const& framesCompressionMode) -> String {
    auto compressionMethod = EmbeddedCompressionMethodNames.getLeft(compression);
    BufferPtr buf = make_shared<Buffer>();
    image.writePng(buf);
    String const outType = frames ? "image" : "bin";
    String outStr = strf("/opensb_data;{};{}",outType,embedAndCompressData(buf->takeData(),compressionMethod));
    if (frames.is<String>()) {
      outStr += strf(";framespath;{}",frames.get<String>());
    } else if (frames.is<Json>()) {
      if (framesCompressionMode && framesCompressionMode->equalsIgnoreCase("text")) {
        outStr += strf(";frames;text;{}",embedEscapeText(frames.get<Json>().repr()));
      } else {
        auto framesCompressionMethod = compressionMethod;
        if (framesCompressionMode)
          framesCompressionMethod = EmbeddedCompressionMethodNames.getLeft(*framesCompressionMode);
        DataStreamBuffer ds;
        ds.write(frames.get<Json>());
        outStr += strf(";frames;json;{}",embedAndCompressData(ds.takeData(),framesCompressionMethod));
      }
    }
    return outStr;
  });

  return methods;
}

}
