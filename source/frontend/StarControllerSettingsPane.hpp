#pragma once

#include "StarBaseScriptPane.hpp"

namespace Star {

STAR_CLASS(ControllerSettingsPane);

class ControllerSettingsPane : public BaseScriptPane {
public:
  ControllerSettingsPane(Json const& config);
};

}
