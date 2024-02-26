#pragma once

#include "StarWidgetParsing.hpp"

namespace Star {

STAR_EXCEPTION(GUIBuilderException, StarException);
STAR_CLASS(GuiReader);

class GuiReader : public WidgetParser {
public:
  GuiReader();

protected:
  WidgetConstructResult titleHandler(String const& _unused, Json const& config);
  WidgetConstructResult paneFeatureHandler(String const& _unused, Json const& config);
  WidgetConstructResult backgroundHandler(String const& _unused, Json const& config);
};

}
