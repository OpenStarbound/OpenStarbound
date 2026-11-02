#include "StarLuaChain.hpp"

#include "StarAssetPathCache.hpp"

#include "StarLuaConverters.hpp"
#include "StarLuaGameConverters.hpp"

#include <cmath>

namespace Star {

namespace {

// Reads an optional number field.  nil and false are "absent", like lua's
// truthiness tests in chain.lua.  Returns false if the value is anything else
// that is not a number.
bool readNumber(LuaTable const& table, char const* key, Maybe<double>& out) {
  auto value = table.get<LuaValue>(key);
  if (value == LuaNil || value == LuaValue(false)) {
    out = {};
    return true;
  }
  if (auto i = value.ptr<LuaInt>()) {
    out = (double)*i;
    return true;
  }
  if (auto f = value.ptr<LuaFloat>()) {
    out = *f;
    return true;
  }
  return false;
}

bool readString(LuaTable const& table, char const* key, Maybe<String>& out) {
  auto value = table.get<LuaValue>(key);
  if (value == LuaNil || value == LuaValue(false)) {
    out = {};
    return true;
  }
  if (auto s = value.ptr<LuaString>()) {
    out = s->toString();
    return true;
  }
  return false;
}

// Rotates a vector like vec2.rotate (an angle of exactly zero is a copy)
Vec2D rotate(Vec2D const& v, double angle) {
  if (angle == 0)
    return v;
  double s = std::sin(angle);
  double c = std::cos(angle);
  return {v[0] * c - v[1] * s, v[0] * s + v[1] * c};
}

// tostring() of a lua float
String luaFloatString(double d) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.14g", d);
  String result(buffer);
  if (!result.contains(".") && !result.contains("e") && !result.contains("n") && !result.contains("i"))
    result += ".0";
  return result;
}

}

bool LuaChain::addChain(Vec2D startPosition, Vec2D chainVec, LuaTable const& chain,
    List<pair<Drawable, Maybe<EntityRenderLayer>>>& drawables, List<LightSource>& lightSources) {
  Maybe<String> segmentImage, startSegmentImage, endSegmentImage, renderLayerName;
  Maybe<double> segmentSizeOpt, overdrawLength, arcRadius, drawPercentage, taper, jitter;
  if (!readString(chain, "segmentImage", segmentImage) || !segmentImage
      || !readString(chain, "startSegmentImage", startSegmentImage)
      || !readString(chain, "endSegmentImage", endSegmentImage)
      || !readString(chain, "renderLayer", renderLayerName)
      || !readNumber(chain, "segmentSize", segmentSizeOpt) || !segmentSizeOpt
      || !readNumber(chain, "overdrawLength", overdrawLength)
      || !readNumber(chain, "arcRadius", arcRadius)
      || !readNumber(chain, "drawPercentage", drawPercentage)
      || !readNumber(chain, "taper", taper)
      || !readNumber(chain, "jitter", jitter))
    return false;
  if (jitter || chain.get<LuaValue>("waveform") != LuaNil)
    return false;

  auto fullbrightValue = chain.get<LuaValue>("fullbright");
  bool fullbright = false;
  if (auto b = fullbrightValue.ptr<LuaBoolean>())
    fullbright = *b;
  else if (fullbrightValue != LuaNil)
    fullbright = true; // lua truthiness

  Maybe<Vec3F> lightColor;
  auto lightValue = chain.get<LuaValue>("light");
  if (lightValue != LuaNil && lightValue != LuaValue(false)) {
    auto color = chain.engine().luaMaybeTo<Color>(lightValue);
    if (!color)
      return false;
    lightColor = color->toRgbF();
  }

  double segmentSize = *segmentSizeOpt;
  double chainDirection = chainVec[0] < 0 ? -1 : 1;
  double chainLength = chainVec.magnitude();

  double arcAngle = 0;
  if (arcRadius) {
    arcAngle = chainDirection * 2 * std::asin(chainLength / (2 * *arcRadius));
    chainLength = chainDirection * arcAngle * *arcRadius;
  }

  double segmentCountD = std::floor((chainLength + overdrawLength.value(0)) / segmentSize + 0.5);
  if (!std::isfinite(segmentCountD) || segmentCountD > 1e7)
    return false;
  int64_t segmentCount = (int64_t)segmentCountD;
  if (segmentCount <= 0)
    return true;

  Maybe<EntityRenderLayer> renderLayer;
  if (renderLayerName)
    renderLayer = parseRenderLayer(*renderLayerName);

  double chainStartAngle = std::atan2(chainVec[1], chainVec[0]);
  if (chainStartAngle < 0)
    chainStartAngle += 2 * 3.141592653589793238462643383279502884;
  chainStartAngle -= arcAngle / 2;
  if (chainVec[0] < 0)
    chainStartAngle = 3.141592653589793238462643383279502884 - chainStartAngle;

  double magnitude = chainVec.magnitude();
  Vec2D segmentOffset = magnitude == 0 ? chainVec : chainVec / magnitude;
  segmentOffset = segmentOffset * segmentSize;
  segmentOffset = rotate(segmentOffset, -arcAngle / 2);
  Vec2D currentBaseOffset = startPosition + segmentOffset * 0.5;
  double lastDrawnSegment = drawPercentage ? std::ceil(segmentCount * *drawPercentage) : segmentCount;
  double segmentStep = arcAngle / segmentCount;
  bool mirrored = chainVec[0] < 0;

  drawables.reserve(drawables.size() + (size_t)std::max(lastDrawnSegment, 0.0));
  for (int64_t i = 1; i <= (int64_t)lastDrawnSegment; ++i) {
    String image = *segmentImage;
    if (i == 1 && startSegmentImage)
      image = *startSegmentImage;
    else if (i == lastDrawnSegment && endSegmentImage)
      image = *endSegmentImage;

    if (taper) {
      double taperFactor = 1 - ((double)(i - 1) / lastDrawnSegment) * *taper;
      double rounded = std::floor(taperFactor * 10.0 + 0.5) / 10.0;
      image = image + "?scale=1.0=" + luaFloatString(rounded);
    }

    double segmentAngle = chainStartAngle + (double)(i - 1) * chainDirection * segmentStep;

    // a zero offset rotated by any angle is still zero
    Vec2D segmentPos = currentBaseOffset;

    Drawable drawable = Drawable::makeImage(cachedAssetPath(image), 1.0f, true, Vec2F(), Color::White);
    drawable.rotate((float)segmentAngle);
    if (mirrored)
      drawable.scale(Vec2F(-1, 1));
    drawable.translate(Vec2F(segmentPos));
    drawable.fullbright = fullbright;
    if (auto imagePart = drawable.part.ptr<Drawable::ImagePart>())
      imagePart->transformation.scale(0.125f);
    drawables.append({std::move(drawable), renderLayer});

    if (lightColor)
      lightSources.append({Vec2F(segmentPos), *lightColor, (LightType)false, 0.0f, 0.0f, 0.0f});

    segmentOffset = rotate(segmentOffset, segmentStep);
    currentBaseOffset = currentBaseOffset + segmentOffset;
  }
  return true;
}

}
