#include "StarRenderingLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaConverters.hpp"
#include "StarClientApplication.hpp"
#include "StarRenderer.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeRenderingCallbacks(ClientApplication* app) {
  LuaCallbacks callbacks;
  
  // if the last argument is defined and true, this change will also be saved to starbound.config and read on next game start, use for things such as an interface that does this
  callbacks.registerCallbackWithSignature<unsigned>("framesSkipped", bind(mem_fn(&ClientApplication::framesSkipped), app));
  callbacks.registerCallbackWithSignature<void, String, bool, Maybe<bool>>("setPostProcessGroupEnabled", bind(mem_fn(&ClientApplication::setPostProcessGroupEnabled), app, _1, _2, _3));
  callbacks.registerCallbackWithSignature<bool, String>("postProcessGroupEnabled", bind(mem_fn(&ClientApplication::postProcessGroupEnabled), app, _1));
  
  
  // not entirely necessary (root.assetJson can achieve the same purpose) but may as well
  callbacks.registerCallbackWithSignature<Json>("postProcessGroups", bind(mem_fn(&ClientApplication::postProcessGroups), app));
  
  // typedef Variant<float, int, Vec2F, Vec3F, Vec4F, bool> RenderEffectParameter;
  // feel free to change this if there's a better way to do this
  // specifically checks if the effect parameter is an int since Lua prefers converting the values to floats
  callbacks.registerCallback("setEffectParameter", [app](String const& effectName, String const& effectParameter, RenderEffectParameter const& value) {
    auto renderer = app->renderer();
    auto mtype = renderer->getEffectScriptableParameterType(effectName, effectParameter);
    if (mtype) {
      auto type = mtype.value();
      if (type == 1 && value.is<float>()) {
        renderer->setEffectScriptableParameter(effectName, effectParameter, (int)value.get<float>());
      } else {
        renderer->setEffectScriptableParameter(effectName, effectParameter, value);
      }
    }
  });
  
  callbacks.registerCallback("getEffectParameter", [app](String const& effectName, String const& effectParameter) {
    auto renderer = app->renderer();
    return renderer->getEffectScriptableParameter(effectName, effectParameter);
  });
  
  // not saved; should be loaded by Lua again
  callbacks.registerCallbackWithSignature<void, String, unsigned>("setPostProcessLayerPasses", bind(mem_fn(&ClientApplication::setPostProcessLayerPasses), app, _1, _2));

  return callbacks;
}


}
