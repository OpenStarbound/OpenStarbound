#ifndef STAR_SIMPLE_TOOLTIP_HPP
#define STAR_SIMPLE_TOOLTIP_HPP

#include "StarString.hpp"

namespace Star {

STAR_CLASS(Pane);

namespace SimpleTooltipBuilder {
  PanePtr buildTooltip(String const& text);
};

}

#endif
