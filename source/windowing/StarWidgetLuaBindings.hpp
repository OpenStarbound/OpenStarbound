#pragma once

#include "StarLua.hpp"
#include "StarGuiReader.hpp"

namespace Star {

STAR_CLASS(Widget);
STAR_CLASS(CanvasWidget);

template <>
struct LuaConverter<CanvasWidgetPtr> : LuaUserDataConverter<CanvasWidgetPtr> {};

template <>
struct LuaUserDataMethods<CanvasWidgetPtr> {
  static LuaMethods<CanvasWidgetPtr> make();
};

namespace LuaBindings {
  LuaCallbacks makeWidgetCallbacks(Widget* parentWidget, GuiReaderPtr reader = {});
}

}
