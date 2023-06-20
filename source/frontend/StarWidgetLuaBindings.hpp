#ifndef STAR_WIDGET_LUA_BINDINGS_HPP
#define STAR_WIDGET_LUA_BINDINGS_HPP

#include "StarLua.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(Widget);

namespace LuaBindings {
  LuaCallbacks makeWidgetCallbacks(Widget* parentWidget, GuiReader* reader);
}

}

#endif
