#include "StarShadersMenu.hpp"

namespace Star {

ShadersMenu::ShadersMenu(Json const& config, UniverseClientPtr client) : BaseScriptPane(config) {
  m_client = std::move(client);
}

void ShadersMenu::show() {
  BaseScriptPane::show();
}

void ShadersMenu::displayed() {
  m_script.setLuaRoot(m_client->luaRoot());
  BaseScriptPane::displayed();
}

void ShadersMenu::dismissed() {
  BaseScriptPane::dismissed();
}

}
