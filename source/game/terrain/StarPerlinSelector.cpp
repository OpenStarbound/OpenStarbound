#include "StarPerlinSelector.hpp"
#include "StarRandom.hpp"

namespace Star {

char const* const PerlinSelector::Name = "perlin";

PerlinSelector::PerlinSelector(Json const& config, TerrainSelectorParameters const& parameters)
  : TerrainSelector(Name, config, parameters) {
  RandomSource random(parameters.seed);

  auto type = PerlinTypeNames.getLeft(config.getString("function"));
  auto octaves = config.getFloat("octaves");
  auto freq = config.getFloat("freq");
  auto amp = config.getFloat("amp");
  auto bias = config.getFloat("bias", 0.0f);
  auto alpha = config.getFloat("alpha", 2.0f);
  auto beta = config.getFloat("beta", 2.0f);

  function = PerlinF(type, octaves, freq, amp, bias, alpha, beta, random.randu64());

  xInfluence = config.getFloat("xInfluence", 1.0f);
  yInfluence = config.getFloat("yInfluence", 1.0f);
}

float PerlinSelector::get(int x, int y) const {
  return function.get(x * xInfluence, y * yInfluence);
}

}
