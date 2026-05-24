#include "StarControllerSettingsPane.hpp"
#include "StarRootLuaBindings.hpp"

namespace Star {

ControllerSettingsPane::ControllerSettingsPane(Json const& config) : BaseScriptPane(config) {
  m_script.setLuaRoot(make_shared<LuaRoot>());
  m_script.addCallbacks("root", LuaBindings::makeRootCallbacks());
}

}
