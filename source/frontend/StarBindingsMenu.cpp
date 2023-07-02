#include "StarBindingsMenu.hpp"
#include "StarInputLuaBindings.hpp"

namespace Star {

BindingsMenu::BindingsMenu(Json const& config) : BaseScriptPane(config) {
  m_script.setLuaRoot(make_shared<LuaRoot>());
  m_script.addCallbacks("input", LuaBindings::makeInputCallbacks());
}

void BindingsMenu::show() {
  BaseScriptPane::show();
}

void BindingsMenu::displayed() {
  BaseScriptPane::displayed();
}

void BindingsMenu::dismissed() {
  BaseScriptPane::dismissed();
}

}