#include "StarRenderingLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarClientApplication.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeRenderingCallbacks(ClientApplication* app) {
  LuaCallbacks callbacks;
  
  // if the last argument is defined and true, this change will also be saved to starbound.config and read on next game start, use for things such as an interface that does this
  callbacks.registerCallbackWithSignature<void, String, bool, Maybe<bool>>("setPostProcessGroupEnabled", bind(mem_fn(&ClientApplication::setPostProcessGroupEnabled), app, _1, _2, _3));
  callbacks.registerCallbackWithSignature<bool, String>("postProcessGroupEnabled", bind(mem_fn(&ClientApplication::postProcessGroupEnabled), app, _1));
  
  // not entirely necessary (root.assetJson can achieve the same purpose) but may as well
  callbacks.registerCallbackWithSignature<Json>("postProcessGroups", bind(mem_fn(&ClientApplication::postProcessGroups), app));

  return callbacks;
}


}
