#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeScriptedAnimatorCallbacks(const NetworkedAnimator* animator, function<Json(String const&, Json const&)> getParameter) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("animationParameter", getParameter);
  callbacks.registerCallback("partPoint", [animator](String const& partName, String const& propertyName) {
    return animator->partPoint(partName, propertyName);
  });
  callbacks.registerCallback("partPoly", [animator](String const& partName, String const& propertyName) { return animator->partPoly(partName, propertyName); });

  callbacks.registerCallback("transformPoint", [animator] (Vec2F point, String const& part) -> Vec2F {
      return animator->partTransformation(part).transformVec2(point);
    });
  callbacks.registerCallback("transformPoly", [animator] (PolyF poly, String const& part) -> PolyF {
      poly.transform(animator->partTransformation(part));
      return poly;
    });

  return callbacks;
}

}
