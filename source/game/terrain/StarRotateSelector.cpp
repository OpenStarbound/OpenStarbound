#include "StarRotateSelector.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

char const* const RotateSelector::Name = "rotate";

RotateSelector::RotateSelector(
    Json const& config, TerrainSelectorParameters const& parameters, TerrainDatabase const* database)
  : TerrainSelector(Name, config, parameters) {
  RandomSource random(parameters.seed);

  rotation = config.getFloat("rotation");
  rotationCenter = Vec2F(parameters.worldWidth / 2, 0);

  auto sourceConfig = config.get("source");
  String sourceType = sourceConfig.getString("type");
  m_source = database->createSelectorType(sourceType, sourceConfig, parameters);
}

float RotateSelector::get(int x, int y) const {
  auto pos = (Vec2F(x, y) - rotationCenter).rotate(rotation) + rotationCenter;
  return m_source->get(pos[0], pos[1]);
}

}
