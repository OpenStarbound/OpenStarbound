#include "StarEnvironmentPainter.hpp"
#include "StarLexicalCast.hpp"
#include "StarTime.hpp"
#include "StarXXHash.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

float const EnvironmentPainter::SunriseTime = 0.072f;
float const EnvironmentPainter::SunsetTime = 0.42f;
float const EnvironmentPainter::SunFadeRate = 0.07f;
float const EnvironmentPainter::MaxFade = 0.3f;
float const EnvironmentPainter::RayPerlinFrequency = 0.005f; // Arbitrary, part of using the Perlin as a PRNG
float const EnvironmentPainter::RayPerlinAmplitude = 2;
int const EnvironmentPainter::RayCount = 60;
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
  m_renderer = move(renderer);
  m_textureGroup = make_shared<AssetTextureGroup>(m_renderer->createTextureGroup(TextureGroupSize::Large));
  m_timer = 0;
  m_lastTime = 0;
  m_rayPerlin = PerlinF(1, RayPerlinFrequency, RayPerlinAmplitude, 0, 2.0f, 2.0f, Random::randu64());
}

void EnvironmentPainter::update() {
  // Allows the rays to change alpha with time.
  int64_t currentTime = Time::monotonicMilliseconds();
  m_timer += (currentTime - m_lastTime) / 1000.0;
  m_timer = std::fmod(m_timer, Constants::pi * 100000.0);
  m_lastTime = currentTime;
}

void EnvironmentPainter::renderStars(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  if (!sky.settings)
    return;

  float nightSkyAlpha = 1.0f - min(sky.dayLevel, sky.skyAlpha);
  if (nightSkyAlpha <= 0.0f)
    return;

  Vec4B color(255, 255, 255, 255 * nightSkyAlpha);

  Vec2F viewSize = screenSize / pixelRatio;
  Vec2F viewCenter = viewSize / 2;
  Vec2F viewMin = sky.starOffset - viewCenter;

  auto newStarsHash = starsHash(sky, viewSize);
  if (newStarsHash != m_starsHash) {
    m_starsHash = newStarsHash;
    setupStars(sky);
  }

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

  for (auto star : stars) {
    Vec2F screenPos = transform.transformVec2(star.first);
    if (viewRect.contains(screenPos)) {
      size_t starFrame = (int)(sky.epochTime + star.second.second) % sky.starFrames;
      auto const& texture = m_starTextures[star.second.first * sky.starFrames + starFrame];
      m_renderer->render(renderTexturedRect(texture, screenPos * pixelRatio - Vec2F(texture->size()) / 2, 1.0, color, 0.0f));
    }
  }

  m_renderer->flush();
}

void EnvironmentPainter::renderDebrisFields(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  if (!sky.settings)
    return;

  if (sky.type == SkyType::Orbital || sky.type == SkyType::Warp) {
    Vec2F viewSize = screenSize / pixelRatio;
    Vec2F viewCenter = viewSize / 2;
    Vec2F viewMin = sky.starOffset - viewCenter;

    Mat3F rotMatrix = Mat3F::rotation(sky.starRotation, viewCenter);

    JsonArray debrisFields = sky.settings.queryArray("spaceDebrisFields");
    for (size_t i = 0; i < debrisFields.size(); ++i) {
      Json debrisField = debrisFields[i];

      Vec2F spaceDebrisVelocityRange = jsonToVec2F(debrisField.query("velocityRange"));
      float debrisXVel = staticRandomFloatRange(spaceDebrisVelocityRange[0], spaceDebrisVelocityRange[1], sky.skyParameters.seed, i, "DebrisFieldXVel");
      float debrisYVel = staticRandomFloatRange(spaceDebrisVelocityRange[0], spaceDebrisVelocityRange[1], sky.skyParameters.seed, i, "DebrisFieldYVel");

      // Translate the entire field to make the debris seem as though they are moving
      Vec2F velocityOffset = -Vec2F(debrisXVel, debrisYVel) * sky.epochTime;

      float screenBuffer = debrisField.queryFloat("screenBuffer");
      PolyF field = PolyF(RectF::withSize(viewMin, viewSize).padded(screenBuffer).translated(velocityOffset));

      Vec2F debrisAngularVelocityRange = jsonToVec2F(debrisField.query("angularVelocityRange"));
      JsonArray imageOptions = debrisField.query("list").toArray();

      auto debrisItems = m_debrisGenerators[i]->generate(field,
          [&](RandomSource& rand) {
            String debrisImage = rand.randFrom(imageOptions).toString();
            float debrisAngularVelocity = rand.randf(debrisAngularVelocityRange[0], debrisAngularVelocityRange[1]);

            return pair<String, float>(debrisImage, debrisAngularVelocity);
          });

      Vec2F debrisPositionOffset = -(sky.starOffset + velocityOffset + viewCenter);

      for (auto debrisItem : debrisItems) {
        Vec2F debrisPosition = rotMatrix.transformVec2(debrisItem.first + debrisPositionOffset);
        float debrisAngle = fmod(Constants::deg2rad * debrisItem.second.second * sky.epochTime, Constants::pi * 2) + sky.starRotation;
        drawOrbiter(pixelRatio, screenSize, sky, {SkyOrbiterType::SpaceDebris, 1.0f, debrisAngle, debrisItem.second.first, debrisPosition});
      }
    }

    m_renderer->flush();
  }
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

    m_renderer->render(RenderQuad{move(leftTexture),
        {leftImage[0], Vec2F(0, 0), {255, 255, 255, 255}, 0.0f},
        {leftImage[1], Vec2F(leftTextureSize[0], 0), {255, 255, 255, 255}, 0.0f},
        {leftImage[2], Vec2F(leftTextureSize[0], leftTextureSize[1]), {255, 255, 255, 255}, 0.0f},
        {leftImage[3], Vec2F(0, leftTextureSize[1]), {255, 255, 255, 255}, 0.0f}});

    m_renderer->render(RenderQuad{move(rightTexture),
        {rightImage[0], Vec2F(0, 0), {255, 255, 255, 255}, 0.0f},
        {rightImage[1], Vec2F(rightTextureSize[0], 0), {255, 255, 255, 255}, 0.0f},
        {rightImage[2], Vec2F(rightTextureSize[0], rightTextureSize[1]), {255, 255, 255, 255}, 0.0f},
        {rightImage[3], Vec2F(0, rightTextureSize[1]), {255, 255, 255, 255}, 0.0f}});
  }

  m_renderer->flush();
}

void EnvironmentPainter::renderFrontOrbiters(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky) {
  for (auto const& orbiter : sky.frontOrbiters(screenSize / pixelRatio))
    drawOrbiter(pixelRatio, screenSize, sky, orbiter);

  m_renderer->flush();
}

void EnvironmentPainter::renderSky(Vec2F const& screenSize, SkyRenderData const& sky) {
  m_renderer->render(RenderQuad{{},
      {Vec2F(0, 0), Vec2F(), sky.bottomRectColor.toRgba(), 0.0f},
      {Vec2F(screenSize[0], 0), Vec2F(), sky.bottomRectColor.toRgba(), 0.0f},
      {screenSize, Vec2F(), sky.topRectColor.toRgba(), 0.0f},
      {Vec2F(0, screenSize[1]), Vec2F(), sky.topRectColor.toRgba(), 0.0f}});

  // Flash overlay for Interstellar travel
  Vec4B flashColor = sky.flashColor.toRgba();
  m_renderer->render(renderFlatRect(RectF(Vec2F(), screenSize), flashColor, 0.0f));

  m_renderer->flush();
}

void EnvironmentPainter::renderParallaxLayers(
    Vec2F parallaxWorldPosition, WorldCamera const& camera, ParallaxLayers const& layers, SkyRenderData const& sky) {

  // Note: the "parallax space" referenced below is a grid where the scale of each cell is the size of the parallax image

  for (auto layer : layers) {
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
    Vec2F parallaxSize =
        Vec2F(m_textureGroup->loadTexture(String::joinWith("?", layer.textures.first(), layer.directives))->size());
    Vec2F parallaxPixels = parallaxSize * camera.pixelRatio();

    // texture offset in *screen pixel space*
    Vec2F parallaxOffset = layer.parallaxOffset * camera.pixelRatio();
    if (layer.speed != 0) {
      double drift = fmod((double)layer.speed * (sky.epochTime / (double)sky.dayLength) * camera.pixelRatio(), (double)parallaxPixels[0]);
      parallaxOffset[0] = fmod(parallaxOffset[0] + drift, parallaxPixels[0]);
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

        for (String const& textureImage : layer.textures) {
          if (auto texture = m_textureGroup->tryTexture(String::joinWith("?", textureImage, layer.directives))) {
            RectF drawRect = RectF::withSize(anchorPoint, subImage.size() * camera.pixelRatio());
            m_renderer->render(RenderQuad{move(texture),
                {{drawRect.xMin(), drawRect.yMin()}, {subImage.xMin(), subImage.yMin()}, drawColor, lightMapMultiplier},
                {{drawRect.xMax(), drawRect.yMin()}, {subImage.xMax(), subImage.yMin()}, drawColor, lightMapMultiplier},
                {{drawRect.xMax(), drawRect.yMax()}, {subImage.xMax(), subImage.yMax()}, drawColor, lightMapMultiplier},
                {{drawRect.xMin(), drawRect.yMax()}, {subImage.xMin(), subImage.yMax()}, drawColor, lightMapMultiplier}});
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
  m_renderer->render(RenderQuad{{},
      {start + Vec2F(std::cos(angle + width), std::sin(angle + width)) * length, {}, Vec4B(RayColor, 0), 0.0f},
      {start + Vec2F(std::cos(angle + width), std::sin(angle + width)) * SunRadius * pixelRatio,
          {},
          Vec4B(RayColor,
              (int)(RayMinUnscaledAlpha + std::abs(m_rayPerlin.get(angle * 896 + time * 30) * RayUnscaledAlphaVariance))
                  * sum
                  * alpha), 0.0f},
      {start + Vec2F(std::cos(angle), std::sin(angle)) * SunRadius * pixelRatio,
          {},
          Vec4B(RayColor,
              (int)(RayMinUnscaledAlpha + std::abs(m_rayPerlin.get(angle * 626 + time * 30) * RayUnscaledAlphaVariance))
                  * sum
                  * alpha), 0.0f},
      {start + Vec2F(std::cos(angle), std::sin(angle)) * length, {}, Vec4B(RayColor, 0), 0.0f}});
}

void EnvironmentPainter::drawOrbiter(float pixelRatio, Vec2F const& screenSize, SkyRenderData const& sky, SkyOrbiter const& orbiter) {
  float alpha = 1.0f;
  Vec2F position = orbiter.position * pixelRatio;

  if (orbiter.type == SkyOrbiterType::Sun) {
    alpha = sky.dayLevel;
    drawRays(pixelRatio, sky, position, std::max(screenSize[0], screenSize[1]), m_timer, sky.skyAlpha);
  }

  TexturePtr texture = m_textureGroup->loadTexture(orbiter.image);
  Vec2F texSize = Vec2F(texture->size());

  Mat3F renderMatrix = Mat3F::rotation(orbiter.angle, position);
  RectF renderRect = RectF::withCenter(position, texSize * orbiter.scale * pixelRatio);
  Vec4B renderColor = Vec4B(255, 255, 255, 255 * alpha);

  m_renderer->render(RenderQuad{move(texture),
      {renderMatrix.transformVec2(renderRect.min()), Vec2F(0, 0), renderColor, 0.0f},
      {renderMatrix.transformVec2(Vec2F{renderRect.xMax(), renderRect.yMin()}), Vec2F(texSize[0], 0), renderColor, 0.0f},
      {renderMatrix.transformVec2(renderRect.max()), Vec2F(texSize[0], texSize[1]), renderColor, 0.0f},
      {renderMatrix.transformVec2(Vec2F{renderRect.xMin(), renderRect.yMax()}), Vec2F(0, texSize[1]), renderColor, 0.0f}});
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

  StringList starTypes = sky.starTypes();
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
    m_debrisGenerators[i] = make_shared<Random2dPointGenerator<pair<String, float>>>(debrisSeed, debrisCellSize, debrisCountRange);
  }
}

}
