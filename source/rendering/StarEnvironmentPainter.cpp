#include "StarEnvironmentPainter.hpp"
#include "StarLexicalCast.hpp"
#include "StarTime.hpp"
#include "StarXXHash.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarMathCommon.hpp"

namespace Star {

float const EnvironmentPainter::SunriseTime = 0.072f;
float const EnvironmentPainter::SunsetTime = 0.42f;
float const EnvironmentPainter::SunFadeRate = 0.07f;
float const EnvironmentPainter::MaxFade = 0.3f;
float const EnvironmentPainter::RayPerlinFrequency = 0.005f; // Arbitrary, part of using the Perlin as a PRNG
float const EnvironmentPainter::RayPerlinAmplitude = 2;
int   const EnvironmentPainter::RayCount = 60;
float const EnvironmentPainter::RayMinWidth = 0.8f; // % of its sector
float const EnvironmentPainter::RayWidthVariance = 5.0265f; // % of its sector
float const EnvironmentPainter::RayAngleVariance = 6.2832f; // Radians
float const EnvironmentPainter::SunRadius = 50;
float const EnvironmentPainter::RayColorDependenceLevel = 3.0f;
float const EnvironmentPainter::RayColorDependenceScale = 0.00625f;
float const EnvironmentPainter::RayUnscaledAlphaVariance = 2.0943f;
float const EnvironmentPainter::RayMinUnscaledAlpha = 1;
Vec3B const EnvironmentPainter::RayColor = Vec3B(255, 255, 200);

EnvironmentPainter::EnvironmentPainter(RendererPtr renderer) {
  m_renderer = std::move(renderer);
  m_textureGroup = make_shared<AssetTextureGroup>(m_renderer->createTextureGroup(TextureGroupSize::Large));
  m_timer = 0;
  m_rayPerlin = PerlinF(1, RayPerlinFrequency, RayPerlinAmplitude, 0, 2.0f, 2.0f, Random::randu64());
}

void EnvironmentPainter::update(float dt) {
  // Allows the rays to change alpha with time.
  m_timer += dt;
  m_timer = std::fmod(m_timer, Constants::pi * 100000.0);
}

void EnvironmentPainter::renderStars(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  float nightSkyAlpha = 1.0f - min(sky.dayLevel, sky.skyAlpha);
  if (nightSkyAlpha <= 0.0f)
    return;

  Vec4B color(255, 255, 255, 255 * nightSkyAlpha);

  Vec2F viewSize = screenSize / pixelRatio;
  Vec2F viewCenter = viewSize / 2;
  Vec2F viewMin = sky.starOffset - viewCenter;

  auto newStarsHash = starsHash(sky, viewSize);
  if (newStarsHash != m_starsHash || !m_starGenerator) {
    m_starsHash = newStarsHash;
    setupStars(sky);
  }

  if (!m_starGenerator || !sky.settings || sky.starFrames == 0 || sky.starTypes().empty())
    return;

  float screenBuffer = sky.settings.queryFloat("stars.screenBuffer");

  PolyF field = PolyF(RectF::withSize(viewMin, Vec2F(viewSize)).padded(screenBuffer));
  field.rotate(-sky.starRotation, Vec2F(sky.starOffset));

  Mat3F transform = Mat3F::identity();
  transform.translate(-viewMin);
  transform.rotate(sky.starRotation, viewCenter);

  int starTwinkleMin = sky.settings.queryInt("stars.twinkleMin");
  int starTwinkleMax = sky.settings.queryInt("stars.twinkleMax");
  size_t starTypesSize = sky.starTypes().size();

  auto stars = m_starGenerator->generate(field, [&](RandomSource& rand) {
      size_t starType = rand.randu32() % starTypesSize;
      float frameOffset = rand.randu32() % sky.starFrames + rand.randf(starTwinkleMin, starTwinkleMax);
      return pair<size_t, float>(starType, frameOffset);
    });

  RectF viewRect = RectF::withSize(Vec2F(), viewSize).padded(screenBuffer);

  auto& primitives = m_renderer->immediatePrimitives();

  for (auto& star : stars) {
    Vec2F screenPos = transform.transformVec2(star.first);
    if (viewRect.contains(screenPos)) {
      size_t starFrame = (size_t)(sky.epochTime + star.second.second) % sky.starFrames;
      if (auto const& texture = m_starTextures[star.second.first * sky.starFrames + starFrame])
        primitives.emplace_back(std::in_place_type_t<RenderQuad>(), texture, screenPos * pixelRatio - Vec2F(texture->size()) / 2, 1.0, color, 0.0f);
    }
  }

  m_renderer->flush();
}

void EnvironmentPainter::renderDebrisFields(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  if (!sky.settings)
    return;

    Vec2F viewSize = screenSize / pixelRatio;
    Vec2F viewCenter = viewSize / 2;
    Vec2D viewMin = Vec2D(sky.starOffset - viewCenter);

    Mat3F rotMatrix = Mat3F::rotation(sky.starRotation, viewCenter);

    JsonArray debrisFields = sky.settings.queryArray("spaceDebrisFields");
    for (size_t i = 0; i < debrisFields.size(); ++i) {
      Json debrisField = debrisFields[i];

      Vec2F spaceDebrisVelocityRange = jsonToVec2F(debrisField.query("velocityRange"));
      float debrisXVel = staticRandomFloatRange(spaceDebrisVelocityRange[0], spaceDebrisVelocityRange[1], sky.skyParameters.seed, i, "DebrisFieldXVel");
      float debrisYVel = staticRandomFloatRange(spaceDebrisVelocityRange[0], spaceDebrisVelocityRange[1], sky.skyParameters.seed, i, "DebrisFieldYVel");

      // Translate the entire field to make the debris seem as though they are moving
      Vec2D velocityOffset = -Vec2D(debrisXVel, debrisYVel) * sky.epochTime;

      JsonArray imageOptions = debrisField.query("list").toArray();
      Vec2U biggest = Vec2U();
      for (Json const& json : imageOptions) {
        TexturePtr texture = m_textureGroup->loadTexture(*json.stringPtr());
        biggest = biggest.piecewiseMax(texture->size());
      }

      float screenBuffer = ceil((float)biggest.max() * (float)Constants::sqrt2);
      PolyD field = PolyD(RectD::withSize(viewMin + velocityOffset, Vec2D(viewSize)).padded(screenBuffer));
      Vec2F debrisAngularVelocityRange = jsonToVec2F(debrisField.query("angularVelocityRange"));
      auto debrisItems = m_debrisGenerators[i]->generate(field,
          [&](RandomSource& rand) {
            StringView debrisImage = *rand.randFrom(imageOptions).stringPtr();
            float debrisAngularVelocity = rand.randf(debrisAngularVelocityRange[0], debrisAngularVelocityRange[1]);

            return pair<StringView, float>(debrisImage, debrisAngularVelocity);
          });

      Vec2D debrisPositionOffset = viewMin + velocityOffset;

      for (auto& debrisItem : debrisItems) {
        Vec2F debrisPosition = rotMatrix.transformVec2(Vec2F(debrisItem.first - debrisPositionOffset));
        float debrisAngle = fmod(Constants::deg2rad * debrisItem.second.second * sky.epochTime, Constants::pi * 2) + sky.starRotation;
        drawOrbiter(pixelRatio, screenSize, sky, {SkyOrbiterType::SpaceDebris, 1.0f, debrisAngle, debrisItem.second.first, debrisPosition});
      }
    }

    m_renderer->flush();
}

void EnvironmentPainter::renderBackOrbiters(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  for (auto const& orbiter : sky.backOrbiters(screenSize / pixelRatio))
    drawOrbiter(pixelRatio, screenSize, sky, orbiter);

  m_renderer->flush();
}

void EnvironmentPainter::renderPlanetHorizon(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  auto planetHorizon = sky.worldHorizon(screenSize / pixelRatio);
  if (planetHorizon.empty())
    return;

  // Can't bail sooner, need to queue all textures
  bool allLoaded = true;
  for (auto const& layer : planetHorizon.layers) {
    if (!m_textureGroup->tryTexture(layer.first) || !m_textureGroup->tryTexture(layer.second))
      allLoaded = false;
  }

  if (!allLoaded)
    return;

  float planetPixelRatio = pixelRatio * planetHorizon.scale;
  Vec2F center = planetHorizon.center * pixelRatio;

  auto& primitives = m_renderer->immediatePrimitives();

  for (auto const& layer : planetHorizon.layers) {
    TexturePtr leftTexture = m_textureGroup->loadTexture(layer.first);
    Vec2F leftTextureSize(leftTexture->size());
    TexturePtr rightTexture = m_textureGroup->loadTexture(layer.second);
    Vec2F rightTextureSize(rightTexture->size());

    Vec2F leftLayer = center;
    leftLayer[0] -= leftTextureSize[0] * planetPixelRatio;
    auto leftRect = RectF::withSize(leftLayer, leftTextureSize * planetPixelRatio);
    PolyF leftImage = PolyF(leftRect);
    leftImage.rotate(planetHorizon.rotation, center);

    auto rightRect = RectF::withSize(center, rightTextureSize * planetPixelRatio);
    PolyF rightImage = PolyF(rightRect);
    rightImage.rotate(planetHorizon.rotation, center);

    primitives.emplace_back(std::in_place_type_t<RenderQuad>(), std::move(leftTexture),
        leftImage[0], Vec2F(0, 0),
        leftImage[1], Vec2F(leftTextureSize[0], 0),
        leftImage[2], Vec2F(leftTextureSize[0], leftTextureSize[1]),
        leftImage[3], Vec2F(0, leftTextureSize[1]), Vec4B::filled(255), 0.0f);

    primitives.emplace_back(std::in_place_type_t<RenderQuad>(), std::move(rightTexture),
        rightImage[0], Vec2F(0, 0),
        rightImage[1], Vec2F(rightTextureSize[0], 0),
        rightImage[2], Vec2F(rightTextureSize[0], rightTextureSize[1]),
        rightImage[3], Vec2F(0, rightTextureSize[1]), Vec4B::filled(255), 0.0f);
  }

  m_renderer->flush();
}

void EnvironmentPainter::renderFrontOrbiters(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  for (auto const& orbiter : sky.frontOrbiters(screenSize / pixelRatio))
    drawOrbiter(pixelRatio, screenSize, sky, orbiter);

  m_renderer->flush();
}

void EnvironmentPainter::renderSky(Vec2F const& screenSize, SkyRenderData const& sky) {
  auto& primitives = m_renderer->immediatePrimitives();
  primitives.emplace_back(std::in_place_type_t<RenderQuad>(), TexturePtr(),
      RenderVertex{Vec2F(0, 0), Vec2F(), sky.bottomRectColor.toRgba(), 0.0f},
      RenderVertex{Vec2F(screenSize[0], 0), Vec2F(), sky.bottomRectColor.toRgba(), 0.0f},
      RenderVertex{screenSize, Vec2F(), sky.topRectColor.toRgba(), 0.0f},
      RenderVertex{Vec2F(0, screenSize[1]), Vec2F(), sky.topRectColor.toRgba(), 0.0f});

  // Flash overlay for Interstellar travel
  Vec4B flashColor = sky.flashColor.toRgba();
  primitives.emplace_back(std::in_place_type_t<RenderQuad>(), RectF(Vec2F(), screenSize), flashColor, 0.0f);

  m_renderer->flush();
}

// TODO: Fix this to work with decimal zoom levels. Currently, the clouds shake rapidly when interpolating between zoom levels.
void EnvironmentPainter::renderParallaxLayers(
    Vec2F parallaxWorldPosition, WorldCamera const& camera, ParallaxLayers const& layers, SkyRenderData const& sky) {

  // Note: the "parallax space" referenced below is a grid where the scale of each cell is the size of the parallax image

  auto& primitives = m_renderer->immediatePrimitives();

  for (auto& layer : layers) {
    if (layer.alpha == 0)
      continue;

    Vec4B drawColor;
    if (layer.unlit || layer.lightMapped)
      drawColor = Vec4B(255, 255, 255, floor(255 * layer.alpha));
    else
      drawColor = Vec4B(sky.environmentLight.toRgb(), floor(255 * layer.alpha));

    Vec2F parallaxValue = layer.parallaxValue;
    Vec2B parallaxRepeat = layer.repeat;
    Vec2F parallaxOrigin = {0.0f, layer.verticalOrigin};

    AssetPath first = layer.textures.first();
    first.directives += layer.directives;
    if (layer.frameNumber > 1)
      first.subPath.emplace("0");
    Vec2F parallaxSize = Vec2F(m_textureGroup->loadTexture(first)->size());
    Vec2F parallaxPixels = parallaxSize * camera.pixelRatio();

    // texture offset in *screen pixel space*
    Vec2F parallaxOffset = layer.parallaxOffset * camera.pixelRatio();
    if (layer.speed[0] != 0) {
      double drift = fmod((double)layer.speed[0] * (sky.epochTime / (double)sky.dayLength), (double)parallaxSize[0]);
      parallaxOffset[0] = fmod(parallaxOffset[0] + drift * camera.pixelRatio(), parallaxPixels[0]);
    }
    if (layer.speed[1] != 0) {
      double drift = fmod((double)layer.speed[1] * (sky.epochTime / (double)sky.dayLength), (double)parallaxSize[1]);
      parallaxOffset[1] = fmod(parallaxOffset[1] + drift * camera.pixelRatio(), parallaxPixels[1]);
    }

    // parallax camera world position in *parallax space*
    Vec2F parallaxCameraCenter = parallaxWorldPosition - parallaxOrigin;
    parallaxCameraCenter =
        Vec2F((((parallaxCameraCenter[0] / parallaxPixels[0]) * TilePixels) * camera.pixelRatio()) / parallaxValue[0],
            (((parallaxCameraCenter[1] / parallaxPixels[1]) * TilePixels) * camera.pixelRatio()) / parallaxValue[1]);

    // width / height of screen in *parallax space*
    float parallaxScreenWidth = camera.screenSize()[0] / parallaxPixels[0];
    float parallaxScreenHeight = camera.screenSize()[1] / parallaxPixels[1];

    // screen world position in *parallax space*
    float parallaxScreenLeft = parallaxCameraCenter[0] - parallaxScreenWidth / 2.0;
    float parallaxScreenBottom = parallaxCameraCenter[1] - parallaxScreenHeight / 2.0;

    // screen boundary world positions in *parallax space*
    Vec2F parallaxScreenOffset = parallaxOffset.piecewiseDivide(parallaxPixels);
    int left = floor(parallaxScreenLeft + parallaxScreenOffset[0]);
    int bottom = floor(parallaxScreenBottom + parallaxScreenOffset[1]);
    int right = ceil(parallaxScreenLeft + parallaxScreenWidth + parallaxScreenOffset[0]);
    int top = ceil(parallaxScreenBottom + parallaxScreenHeight + parallaxScreenOffset[1]);

    // positions to start tiling in *screen pixel space*
    float pixelLeft = (left - parallaxScreenLeft) * parallaxPixels[0] - parallaxOffset[0];
    float pixelBottom = (bottom - parallaxScreenBottom) * parallaxPixels[1] - parallaxOffset[1];

    // vertical top and bottom cutoff points in *parallax space*
    float tileLimitTop = top;
    if (layer.tileLimitTop)
      tileLimitTop = (layer.parallaxOffset[1] - layer.tileLimitTop.value()) / parallaxSize[1];
    float tileLimitBottom = bottom;
    if (layer.tileLimitBottom)
      tileLimitBottom = (layer.parallaxOffset[1] - layer.tileLimitBottom.value()) / parallaxSize[1];

    float lightMapMultiplier = (!layer.unlit && layer.lightMapped) ? 1.0f : 0.0f;

    for (int y = bottom; y <= top; ++y) {
      if (!(parallaxRepeat[1] || y == 0) || y > tileLimitTop || y + 1 < tileLimitBottom)
        continue;
      for (int x = left; x <= right; ++x) {
        if (!(parallaxRepeat[0] || x == 0))
          continue;
        float pixelTileLeft = pixelLeft + (x - left) * parallaxPixels[0];
        float pixelTileBottom = pixelBottom + (y - bottom) * parallaxPixels[1];

        Vec2F anchorPoint(pixelTileLeft, pixelTileBottom);

        RectF subImage = RectF::withSize(Vec2F(), parallaxSize);
        if (tileLimitTop != top && y + 1 > tileLimitTop)
          subImage.setYMin(parallaxSize[1] * (1.0f - fpart(tileLimitTop)));
        if (tileLimitBottom != bottom && y < tileLimitBottom)
          anchorPoint[1] += fpart(tileLimitBottom) * parallaxPixels[1];

        for (auto const& textureImage : layer.textures) {
          AssetPath withDirectives = textureImage;
          withDirectives.directives += layer.directives;
          if (layer.frameNumber > 1) {
            float time_within_cycle = (float)fmod(sky.epochTime, (double)layer.animationCycle);
            float time_per_frame = layer.animationCycle / layer.frameNumber;
            float frame_number = time_within_cycle / time_per_frame;
            int frame = (layer.frameOffset + clamp<int>(frame_number, 0, layer.frameNumber - 1)) % layer.frameNumber;
            withDirectives.subPath.emplace(toString(frame));
          }
          if (auto texture = m_textureGroup->tryTexture(withDirectives)) {
            RectF drawRect = RectF::withSize(anchorPoint, subImage.size() * camera.pixelRatio());
            primitives.emplace_back(std::in_place_type_t<RenderQuad>(), std::move(texture),
                RenderVertex{drawRect.min(), subImage.min(), drawColor, lightMapMultiplier},
                RenderVertex{{drawRect.xMax(), drawRect.yMin()}, {subImage.xMax(), subImage.yMin()}, drawColor, lightMapMultiplier},
                RenderVertex{drawRect.max(), subImage.max(), drawColor, lightMapMultiplier},
                RenderVertex{{drawRect.xMin(), drawRect.yMax()}, {subImage.xMin(), subImage.yMax()}, drawColor, lightMapMultiplier});
          }
        }
      }
    }
  }

  m_renderer->flush();
}

void EnvironmentPainter::cleanup(int64_t textureTimeout) {
  m_textureGroup->cleanup(textureTimeout);
}

void EnvironmentPainter::drawRays(
    float pixelRatio, SkyRenderData const& sky, Vec2F start, float length, double time, float alpha) {
  // All magic constants are either 2PI or arbritrary to allow the Perlin to act
  // as a PRNG
  float sectorWidth = 2 * Constants::pi / RayCount; // Radians
  Vec3B color = sky.topRectColor.toRgb();

  for (int i = 0; i < RayCount; i++)
    drawRay(pixelRatio,
        sky,
        start,
        sectorWidth * (std::abs(m_rayPerlin.get(i * 25)) * RayWidthVariance + RayMinWidth),
        length,
        i * sectorWidth + m_rayPerlin.get(i * 314) * RayAngleVariance,
        time,
        color,
        alpha);

  m_renderer->flush();
}

void EnvironmentPainter::drawRay(float pixelRatio,
    SkyRenderData const& sky,
    Vec2F start,
    float width,
    float length,
    float angle,
    double time,
    Vec3B color,
    float alpha) {
  // All magic constants are arbritrary to allow the Perlin to act as a PRNG

  float currentTime = sky.timeOfDay / sky.dayLength;
  float timeSinceSunEvent = std::min(std::abs(currentTime - SunriseTime), std::abs(currentTime - SunsetTime));
  float percentFaded = MaxFade * (1.0f - std::min(1.0f, std::pow(timeSinceSunEvent / SunFadeRate, 2.0f)));
  // Gets the current average sky color
  color = (Vec3B)((Vec3F)color * (1 - percentFaded) + (Vec3F)sky.mainSkyColor.toRgb() * percentFaded);
  // Sum is used to vary the ray intensity based on sky color
  // Rays show up more on darker backgrounds, so this scales to remove that
  float sum = std::pow((color[0] + color[1]) * RayColorDependenceScale, RayColorDependenceLevel);
  Vec3B rayColor;
  if (sky.settings.queryBool("sun.dynamicImage.enabled", false) && !sky.skyParameters.sunType.empty())
    rayColor = jsonToVec3B(sky.settings.query("sun.dynamicImage.rayColors." + sky.skyParameters.sunType, sky.settings.query("sun.rayColor", JsonArray{RayColor[0], RayColor[1], RayColor[2]})));
  else
    rayColor = jsonToVec3B(sky.settings.query("sun.rayColor", JsonArray{RayColor[0], RayColor[1], RayColor[2]}));
  float sunScale = sky.settings.queryFloat("sun.scale", 1.0f);
  m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), TexturePtr(),
      RenderVertex{start + Vec2F(std::cos(angle + width), std::sin(angle + width)) * length, {}, Vec4B(rayColor, 0), 0.0f},
      RenderVertex{start + Vec2F(std::cos(angle + width), std::sin(angle + width)) * SunRadius * sunScale * pixelRatio,
          {},
          Vec4B(rayColor,
              (int)(RayMinUnscaledAlpha + std::abs(m_rayPerlin.get(angle * 896 + time * 30) * RayUnscaledAlphaVariance))
                  * sum
                  * alpha), 0.0f},
      RenderVertex{start + Vec2F(std::cos(angle), std::sin(angle)) * SunRadius * sunScale * pixelRatio,
          {},
          Vec4B(rayColor,
              (int)(RayMinUnscaledAlpha + std::abs(m_rayPerlin.get(angle * 626 + time * 30) * RayUnscaledAlphaVariance))
                  * sum
                  * alpha), 0.0f},
      RenderVertex{start + Vec2F(std::cos(angle), std::sin(angle)) * length, {}, Vec4B(rayColor, 0), 0.0f});
}

void EnvironmentPainter::drawOrbiter(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky, SkyOrbiter const& orbiter) {
  float alpha = 1.0f;
  Vec2F position;

  // The way Starbound positions these is weird.
  // It's a random point on a 400 by 400 area from the bottom left of the screen.
  // That origin point is then multiplied by the zoom level.
  // This does not intuitively scale with higher-resolution monitors, so lets fix that.
  if (orbiter.type == SkyOrbiterType::Moon) {
    const Vec2F correctionOrigin = { 320, 180 }; 
    // correctionOrigin is 1920x1080 / default zoom level / 2, the most likely dev setup at the time.
    Vec2F offset = orbiter.position - correctionOrigin;
    position = (screenSize / 2) + offset * pixelRatio;
  }
  else
    position = orbiter.position * pixelRatio;

  if (orbiter.type == SkyOrbiterType::Sun) {
    alpha = sky.dayLevel;
    drawRays(pixelRatio, sky, position, std::max(screenSize[0], screenSize[1]), m_timer, sky.skyAlpha);
  }

  TexturePtr texture = m_textureGroup->loadTexture(orbiter.image);
  Vec2F texSize = Vec2F(texture->size());

  Mat3F renderMatrix = Mat3F::rotation(orbiter.angle, position);
  RectF renderRect = RectF::withCenter(position, texSize * orbiter.scale * pixelRatio);
  Vec4B renderColor = Vec4B(255, 255, 255, 255 * alpha);

  m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), std::move(texture),
      renderMatrix.transformVec2(renderRect.min()), Vec2F(0, 0),
      renderMatrix.transformVec2(Vec2F{renderRect.xMax(), renderRect.yMin()}), Vec2F(texSize[0], 0),
      renderMatrix.transformVec2(renderRect.max()), Vec2F(texSize[0], texSize[1]),
      renderMatrix.transformVec2(Vec2F{renderRect.xMin(), renderRect.yMax()}), Vec2F(0, texSize[1]),
    renderColor, 0.0f);
}

uint64_t EnvironmentPainter::starsHash(SkyRenderData const& sky, Vec2F const& viewSize) const {
  XXHash64 hasher;

  hasher.push(reinterpret_cast<char const*>(&viewSize[0]), sizeof(viewSize[0]));
  hasher.push(reinterpret_cast<char const*>(&viewSize[1]), sizeof(viewSize[1]));
  hasher.push(reinterpret_cast<char const*>(&sky.skyParameters.seed), sizeof(sky.skyParameters.seed));
  hasher.push(reinterpret_cast<char const*>(&sky.type), sizeof(sky.type));

  return hasher.digest();
}

void EnvironmentPainter::setupStars(SkyRenderData const& sky) {
  if (!sky.settings)
    return;

  StringList const& starTypes = sky.starTypes();
  size_t starTypesSize = starTypes.size();

  m_starTextures.resize(starTypesSize * sky.starFrames);
  for (size_t i = 0; i < starTypesSize; ++i) {
    for (size_t j = 0; j < sky.starFrames; ++j)
      m_starTextures[i * sky.starFrames + j] = m_textureGroup->loadTexture(starTypes[i] + ":" + toString(j));
  }

  int starCellSize = sky.settings.queryInt("stars.cellSize");
  Vec2I starCount = jsonToVec2I(sky.settings.query("stars.cellCount"));

  m_starGenerator = make_shared<Random2dPointGenerator<pair<size_t, float>>>(sky.skyParameters.seed, starCellSize, starCount);

  JsonArray debrisFields = sky.settings.queryArray("spaceDebrisFields");
  m_debrisGenerators.resize(debrisFields.size());
  for (size_t i = 0; i < debrisFields.size(); ++i) {
    int debrisCellSize = debrisFields[i].getInt("cellSize");
    Vec2I debrisCountRange = jsonToVec2I(debrisFields[i].get("cellCountRange"));
    uint64_t debrisSeed = staticRandomU64(sky.skyParameters.seed, i, "DebrisFieldSeed");
    m_debrisGenerators[i] = make_shared<Random2dPointGenerator<pair<String, float>, double>>(debrisSeed, debrisCellSize, debrisCountRange);
  }
}

}
