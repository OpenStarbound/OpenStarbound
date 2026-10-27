#include "StarWeather.hpp"
#include "StarParallax.hpp"

#include "gtest/gtest.h"

using namespace Star;

namespace {

static Json weatherConfig() {
  return JsonObject{
    {"name", "testWeather"},
    {"particles", JsonArray()},
    {"projectiles", JsonArray()},
    {"duration", JsonArray{40, 100}}};
}

}// namespace

TEST(WeatherTest, NoParallax) {
  WeatherType weather(weatherConfig());

  EXPECT_FALSE(weather.parallax);
  EXPECT_TRUE(weather.toJson().get("parallax").isNull());
  EXPECT_FALSE(WeatherType(weather.toJson()).parallax);
}

TEST(WeatherTest, EmptyParallax) {
  WeatherType weather(weatherConfig().set("parallax", ""));

  EXPECT_FALSE(weather.parallax);
}

TEST(WeatherTest, ShorthandParallax) {
  WeatherType weather(weatherConfig().set("parallax", "rain"));

  ASSERT_TRUE(weather.parallax);
  EXPECT_EQ(*weather.parallax, "/parallax/weather/rain.parallax");

  WeatherType roundTrip(weather.toJson());
  ASSERT_TRUE(roundTrip.parallax);
  EXPECT_EQ(*roundTrip.parallax, "/parallax/weather/rain.parallax");
}

TEST(WeatherTest, AbsoluteParallax) {
  WeatherType weather(weatherConfig().set("parallax", "/custom/weather/storm.parallax"));

  ASSERT_TRUE(weather.parallax);
  EXPECT_EQ(*weather.parallax, "/custom/weather/storm.parallax");
}

TEST(WeatherTest, CompleteStateRestoresAfterLocalClear) {
  WeatherPool pool(WeatherPool::ItemsList{{1.0, "rain"}});
  WorldGeometry geometry(Vec2U(1000, 1000));

  ServerWeather server;
  server.setup(pool, 0, geometry, {});
  server.setWeather("rain", true);
  auto update = server.writeUpdate(0).first;

  ClientWeather client;
  client.setup(geometry, {});
  client.setVisibleRegion({0, 100, 100, 200});
  client.readUpdate(update, {});
  client.update(0);
  EXPECT_FALSE(client.statusEffects().empty());

  client.clear();
  EXPECT_TRUE(client.statusEffects().empty());

  client.readUpdate(update, {});
  client.setVisibleRegion({0, 100, 100, 200});
  client.update(0);
  EXPECT_FALSE(client.statusEffects().empty());
}

TEST(ParallaxTest, WindSettingsDefaultOff) {
  ParallaxLayer layer;
  ParallaxLayer restored(layer.store());

  EXPECT_FALSE(restored.followsWind);
  EXPECT_FALSE(restored.windSpeedMultiplier);
}

TEST(ParallaxTest, WindSettingsRoundTrip) {
  ParallaxLayer layer;
  layer.followsWind = true;
  layer.windSpeedMultiplier = 0.25f;
  ParallaxLayer restored(layer.store());

  EXPECT_TRUE(restored.followsWind);
  ASSERT_TRUE(restored.windSpeedMultiplier);
  EXPECT_FLOAT_EQ(*restored.windSpeedMultiplier, 0.25f);
}
