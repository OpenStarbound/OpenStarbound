#pragma once

#include "StarBiMap.hpp"

namespace Star {

enum class HorizontalAnchor {
  LeftAnchor,
  HMidAnchor,
  RightAnchor
};
extern EnumMap<HorizontalAnchor> const HorizontalAnchorNames;

enum class VerticalAnchor {
  BottomAnchor,
  VMidAnchor,
  TopAnchor
};
extern EnumMap<VerticalAnchor> const VerticalAnchorNames;

}
