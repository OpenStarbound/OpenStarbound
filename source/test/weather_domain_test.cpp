#include "StarBiomeDatabase.hpp"
#include "StarRoot.hpp"
#include "StarSkyParameters.hpp"
#include "StarWorldParameters.hpp"
#include "StarWorldTemplate.hpp"

#include "gtest/gtest.h"

using namespace Star;

namespace {

static WorldTemplate makeGardenWorld(TerrestrialWorldParametersPtr parameters, uint64_t seed) {
  return WorldTemplate(parameters, SkyParameters(), seed);
}

static WorldTemplate::WeatherLayer const* layerAt(WorldTemplate const& world, int height) {
  return world.weatherLayerAt({0, height});
}

}

TEST(WeatherDomainTest, DefaultTerrestrialOwnership) {
  uint64_t seed = 1234;
  auto parameters = generateTerrestrialWorldParameters("garden", "small", seed);
  auto world = makeGardenWorld(parameters, seed);

  auto surface = layerAt(world, parameters->surfaceLayer.layerMinHeight);
  auto atmosphere = layerAt(world, parameters->atmosphereLayer.layerMinHeight);
  auto subsurface = layerAt(world, parameters->subsurfaceLayer.layerMinHeight);
  auto space = layerAt(world, parameters->spaceLayer.layerMinHeight);
  auto core = layerAt(world, parameters->coreLayer.layerMinHeight);

  ASSERT_NE(surface, nullptr);
  ASSERT_NE(atmosphere, nullptr);
  ASSERT_NE(subsurface, nullptr);
  ASSERT_NE(space, nullptr);
  ASSERT_NE(core, nullptr);
  EXPECT_EQ(surface->domain, Maybe<String>(String("surface")));
  EXPECT_EQ(atmosphere->domain, surface->domain);
  EXPECT_EQ(subsurface->domain, surface->domain);
  EXPECT_FALSE(space->domain);
  EXPECT_FALSE(core->domain);

  for (auto const& underground : parameters->undergroundLayers) {
    auto layer = layerAt(world, underground.layerMinHeight);
    ASSERT_NE(layer, nullptr);
    EXPECT_FALSE(layer->domain);
  }

  auto surfaceDomain = world.weatherDomain("surface");
  ASSERT_NE(surfaceDomain, nullptr);
  EXPECT_EQ(surfaceDomain->effectsMinHeight, parameters->subsurfaceLayer.layerMinHeight);
}

TEST(WeatherDomainTest, ExplicitLayerWeatherCreatesIndependentDomain) {
  uint64_t seed = 5678;
  auto parameters = generateTerrestrialWorldParameters("garden", "small", seed);
  parameters->atmosphereLayer.primaryRegion.biome = "garden";
  auto world = makeGardenWorld(parameters, seed);

  auto atmosphere = layerAt(world, parameters->atmosphereLayer.layerMinHeight);
  ASSERT_NE(atmosphere, nullptr);
  EXPECT_EQ(atmosphere->domain, Maybe<String>(String("atmosphere")));

  auto atmosphereDomain = world.weatherDomain("atmosphere");
  ASSERT_NE(atmosphereDomain, nullptr);
  EXPECT_FALSE(atmosphereDomain->pool.empty());
  EXPECT_EQ(atmosphereDomain->effectsMinHeight, parameters->atmosphereLayer.layerMinHeight);
}

TEST(WeatherDomainTest, WeatherPropertyPresenceIsDistinctFromOmission) {
  auto biomes = Root::singleton().biomeDatabase();
  EXPECT_TRUE(biomes->biomeHasWeather("garden"));
  EXPECT_FALSE(biomes->biomeHasWeather("atmosphere"));
}
