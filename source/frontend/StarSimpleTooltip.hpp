#pragma once

#include "StarString.hpp"

namespace Star {

STAR_CLASS(Pane);

namespace SimpleTooltipBuilder {
  PanePtr buildTooltip(String const& text);
};

}
