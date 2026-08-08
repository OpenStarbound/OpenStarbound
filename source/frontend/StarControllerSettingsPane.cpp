#include "StarControllerSettingsPane.hpp"

namespace Star {

ControllerSettingsPane::ControllerSettingsPane(Json const& config) : BaseScriptPane(config) {
  // Setting a LuaRoot makes root.getConfiguration/setConfiguration available
  m_script.setLuaRoot(make_shared<LuaRoot>());
}

}
