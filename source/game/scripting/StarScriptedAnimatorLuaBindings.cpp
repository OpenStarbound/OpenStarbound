#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeScriptedAnimatorCallbacks(NetworkedAnimator* networkedAnimator, function<Json(String const&, Json const&)> getParameter) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("animationParameter", getParameter);
  callbacks.registerCallbackWithSignature<Maybe<Vec2F>, String, String>("partPoint", bind(&NetworkedAnimator::partPoint, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<PolyF>, String, String>("partPoly", bind(&NetworkedAnimator::partPoly, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String, String>("partProperty", bind(&NetworkedAnimator::partProperty, networkedAnimator, _1, _2));

  callbacks.registerCallback("transformPoint", [networkedAnimator] (Vec2F point, String const& part) -> Vec2F {
      return networkedAnimator->partTransformation(part).transformVec2(point);
    });
  callbacks.registerCallback("transformPoly", [networkedAnimator] (PolyF poly, String const& part) -> PolyF {
      poly.transform(networkedAnimator->partTransformation(part));
      return poly;
    });

  callbacks.registerCallbackWithSignature<bool, String, String, bool, bool>(
      "setLocalAnimationState", bind(&NetworkedAnimator::setLocalState, networkedAnimator, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<Json, String, String>(
      "animationStateProperty", bind(&NetworkedAnimator::stateProperty, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String, String>(
      "animationStateNextProperty", bind(&NetworkedAnimator::stateNextProperty, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<String, String>(
      "animationState", bind(&NetworkedAnimator::state, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<int, String>(
      "animationStateFrame", bind(&NetworkedAnimator::stateFrame, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "animationStateFrameProgress", bind(&NetworkedAnimator::stateFrameProgress, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "animationStateTimer", bind(&NetworkedAnimator::stateTimer, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "animationStateReverse", bind(&NetworkedAnimator::stateReverse, networkedAnimator, _1));

  callbacks.registerCallbackWithSignature<bool, String>(
      "hasTransformationGroup", bind(&NetworkedAnimator::hasTransformationGroup, networkedAnimator, _1));

  callbacks.registerCallbackWithSignature<void, String, Vec2F>("translateLocalTransformationGroup",
      bind(&NetworkedAnimator::translateLocalTransformationGroup, networkedAnimator, _1, _2));
  callbacks.registerCallback("rotateLocalTransformationGroup",
      [networkedAnimator](String const& transformationGroup, float rotation, Maybe<Vec2F> const& rotationCenter) {
        networkedAnimator->rotateLocalTransformationGroup(transformationGroup, rotation, rotationCenter.value());
      });
  callbacks.registerCallback("scaleLocalTransformationGroup",
      [networkedAnimator](LuaEngine& engine, String const& transformationGroup, LuaValue scale, Maybe<Vec2F> const& scaleCenter) {
        if (auto cs = engine.luaMaybeTo<Vec2F>(scale))
          networkedAnimator->scaleLocalTransformationGroup(transformationGroup, *cs, scaleCenter.value());
        else
          networkedAnimator->scaleLocalTransformationGroup(transformationGroup, engine.luaTo<float>(scale), scaleCenter.value());
      });
  callbacks.registerCallbackWithSignature<void, String, float, float, float, float, float, float>(
      "transformLocalTransformationGroup",
      bind(&NetworkedAnimator::transformLocalTransformationGroup, networkedAnimator, _1, _2, _3, _4, _5, _6, _7));
  callbacks.registerCallbackWithSignature<void, String>(
      "resetLocalTransformationGroup", bind(&NetworkedAnimator::resetLocalTransformationGroup, networkedAnimator, _1));

  callbacks.registerCallbackWithSignature<void, String, List<Drawable>>(
      "addPartDrawables", bind(&NetworkedAnimator::addPartDrawables, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, List<Drawable>>(
      "setPartDrawables", bind(&NetworkedAnimator::setPartDrawables, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<String, String, String>(
      "applyPartTags", bind(&NetworkedAnimator::applyPartTags, networkedAnimator, _1, _2));


  return callbacks;
}

}
