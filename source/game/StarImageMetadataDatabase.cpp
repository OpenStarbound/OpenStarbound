#include "StarImageMetadataDatabase.hpp"
#include "StarFile.hpp"
#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarLogging.hpp"
#include "StarEncode.hpp"
#include "StarGameTypes.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

Vec2U ImageMetadataDatabase::imageSize(AssetPath const& path) const {
  MutexLocker locker(m_mutex);
  auto i = m_sizeCache.find(path);
  if (i != m_sizeCache.end())
    return i->second;

  locker.unlock();
  Vec2U size = calculateImageSize(path);

  locker.lock();
  m_sizeCache[path] = size;
  return size;
}

List<Vec2I> ImageMetadataDatabase::imageSpaces(AssetPath const& path, Vec2F position, float fillLimit, bool flip) const {
  SpacesEntry key = make_tuple(path, Vec2I::round(position), fillLimit, flip);

  MutexLocker locker(m_mutex);
  auto i = m_spacesCache.find(key);
  if (i != m_spacesCache.end()) {
    return i->second;
  }

  auto filteredPath = filterProcessing(path);
  SpacesEntry filteredKey = make_tuple(filteredPath, Vec2I::round(position), fillLimit, flip);

  auto j = m_spacesCache.find(filteredKey);
  if (j != m_spacesCache.end()) {
    auto spaces = j->second;
    m_spacesCache[key] = spaces;
    return spaces;
  }

  locker.unlock();

  auto image = Root::singleton().assets()->image(filteredPath);
  int imageWidth = image->width();
  int imageHeight = image->height();

  Vec2I min((position / TilePixels).floor());
  Vec2I max(((Vec2F(imageWidth, imageHeight) + position) / TilePixels).ceil());

  List<Vec2I> spaces;

  for (int yspace = min[1]; yspace < max[1]; ++yspace) {
    for (int xspace = min[0]; xspace < max[0]; ++xspace) {
      float fillRatio = 0.0f;

      for (int y = 0; y < (int)TilePixels; ++y) {
        int ypixel = round(yspace * (int)TilePixels + y - position[1]);
        if (ypixel < 0 || ypixel >= imageHeight)
          continue;

        for (int x = 0; x < (int)TilePixels; ++x) {
          int xpixel = round(xspace * (int)TilePixels + x - position[0]);
          if (flip)
            xpixel = imageWidth - 1 - xpixel;

          if (xpixel < 0 || xpixel >= imageWidth)
            continue;

          if (image->get(xpixel, ypixel)[3] > 0)
            fillRatio += 1.0f / square(TilePixels);
        }
      }

      if (fillRatio >= fillLimit)
        spaces.append(Vec2I(xspace, yspace));
    }
  }

  locker.lock();
  m_spacesCache[key] = spaces;
  m_spacesCache[filteredKey] = spaces;

  return spaces;
}

RectU ImageMetadataDatabase::nonEmptyRegion(AssetPath const& path) const {
  MutexLocker locker(m_mutex);
  auto i = m_regionCache.find(path);
  if (i != m_regionCache.end()) {
    return i->second;
  }

  auto filteredPath = filterProcessing(path);
  auto j = m_regionCache.find(filteredPath);
  if (j != m_regionCache.end()) {
    m_regionCache[path] = j->second;
    return j->second;
  }

  locker.unlock();
  auto image = Root::singleton().assets()->image(filteredPath);
  RectU region = RectU::null();
  image->forEachPixel([&region](unsigned x, unsigned y, Vec4B const& pixel) {
    if (pixel[3] > 0)
      region.combine(RectU::withSize({x, y}, {1, 1}));
  });

  locker.lock();
  m_regionCache[path] = region;
  m_regionCache[filteredPath] = region;

  return region;
}

AssetPath ImageMetadataDatabase::filterProcessing(AssetPath const& path) {
  AssetPath newPath = { path.basePath, path.subPath, {} };

  String filtered;
  for (auto& directives : path.directives.list())
    directives.loadOperations();
  path.directives.forEach([&](auto const& entry, Directives const& directives) {
    ImageOperation const& operation = entry.operation;
    if (!(operation.is<HueShiftImageOperation>()           ||
          operation.is<SaturationShiftImageOperation>()    ||
          operation.is<BrightnessMultiplyImageOperation>() ||
          operation.is<FadeToColorImageOperation>()        ||
          operation.is<ScanLinesImageOperation>()          ||
          operation.is<SetColorImageOperation>())) {
      filtered += "?";
      filtered += entry.string(*directives);
    }
    });

  newPath.directives = std::move(filtered);
  return newPath;
}

Vec2U ImageMetadataDatabase::calculateImageSize(AssetPath const& path) const {
  // Carefully calculate an image's size while trying not to actually load it.
  // In error cases, this will fall back to calling Assets::image, so that image
  // can possibly produce a missing image asset or properly report the error.

  auto assets = Root::singleton().assets();

  auto fallback = [&assets, &path]() {
    return assets->image(path)->size();
  };

  if (!assets->assetExists(path.basePath)) {
    return fallback();
  }

  Vec2U imageSize;
  if (path.subPath) {
    auto frames = assets->imageFrames(path.basePath);
    if (!frames)
      return fallback();

    if (auto rect = frames->getRect(*path.subPath))
      imageSize = rect->size();
    else
      return fallback();
  } else {
    // We ensure that the base image size is cached even when given directives,
    // so we don't have to call Image::readPngMetadata on the same file more
    // than once.
    MutexLocker locker(m_mutex);
    if (auto size = m_sizeCache.maybe(path.basePath)) {
      imageSize = *size;
    } else {
      locker.unlock();
      auto file = assets->openFile(path.basePath);
      if (Image::isPng(file))
        imageSize = get<0>(Image::readPngMetadata(file));
      else
        imageSize = fallback();
      locker.lock();
      m_sizeCache[path.basePath] = imageSize;
    }
  }

  struct OperationSizeAdjust {
    Vec2U& imageSize;
    bool hasError;

    OperationSizeAdjust(Vec2U& size) : imageSize(size), hasError(false) {};

    void operator()(NullImageOperation const&) {}

    void operator()(ErrorImageOperation const&) {}

    void operator()(HueShiftImageOperation const&) {}

    void operator()(SaturationShiftImageOperation const&) {}

    void operator()(BrightnessMultiplyImageOperation const&) {}

    void operator()(FadeToColorImageOperation const&) {}

    void operator()(ScanLinesImageOperation const&) {}

    void operator()(SetColorImageOperation const&) {}

    void operator()(ColorReplaceImageOperation const&) {}

    void operator()(AlphaMaskImageOperation const&) {}

    void operator()(BlendImageOperation const&) {}

    void operator()(MultiplyImageOperation const&) {}

    void operator()(BorderImageOperation const& bio) {
      imageSize += Vec2U::filled(bio.pixels * 2);
    }

    void operator()(ScaleImageOperation const& sio) {
      imageSize = Vec2U::round(vmult(Vec2F(imageSize), sio.scale));
    }

    void operator()(CropImageOperation const& cio) {
      if (cio.subset.isEmpty() ||
          cio.subset.xMin() < 0 ||
          cio.subset.yMin() < 0 ||
          (unsigned)cio.subset.xMax() > imageSize[0] ||
          (unsigned)cio.subset.yMax() > imageSize[1]) {
        hasError = true;
      } else {
        imageSize = Vec2U(cio.subset.size());
      }
    }

    void operator()(FlipImageOperation const&) {}
  };

  OperationSizeAdjust osa(imageSize);

  for (auto& directives : path.directives.list())
    directives.loadOperations();

  bool complete = path.directives.forEachAbortable([&](auto const& entry, Directives const&) -> bool {
    entry.operation.call(osa);
    return !osa.hasError;
  });

  if (!complete)
    return fallback();

  return imageSize;
}

}
