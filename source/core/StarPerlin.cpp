#include "StarPerlin.hpp"

namespace Star {

EnumMap<PerlinType> const PerlinTypeNames{
  {PerlinType::Uninitialized, "uninitialized"},
  {PerlinType::Perlin, "perlin"},
  {PerlinType::Billow, "billow"},
  {PerlinType::RidgedMulti, "ridgedMulti"},
};

}
