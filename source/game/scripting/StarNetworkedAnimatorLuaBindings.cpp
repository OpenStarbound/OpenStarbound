#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeNetworkedAnimatorCallbacks(NetworkedAnimator* networkedAnimator) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<bool, String, String, bool, bool>(
      "setAnimationState", bind(&NetworkedAnimator::setState, networkedAnimator, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<bool, String, String, bool, bool>(
      "setLocalAnimationState", bind(&NetworkedAnimator::setLocalState, networkedAnimator, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<String, String>(
      "animationState", bind(&NetworkedAnimator::state, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<Json, String, String>(
      "animationStateProperty", bind(&NetworkedAnimator::stateProperty, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String, String>(
      "animationStateNextProperty", bind(&NetworkedAnimator::stateNextProperty, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<int, String>(
      "animationStateFrame", bind(&NetworkedAnimator::stateFrame, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "animationStateFrameProgress", bind(&NetworkedAnimator::stateFrameProgress, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "animationStateTimer", bind(&NetworkedAnimator::stateTimer, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "animationStateReverse", bind(&NetworkedAnimator::stateReverse, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<bool, String, Maybe<String>>(
      "hasState", bind(&NetworkedAnimator::hasState, networkedAnimator, _1, _2));

  callbacks.registerCallbackWithSignature<void, String, String>(
      "setGlobalTag", bind(&NetworkedAnimator::setGlobalTag, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, String, String>(
      "setPartTag", bind(&NetworkedAnimator::setPartTag, networkedAnimator, _1, _2, _3));
  callbacks.registerCallback("setFlipped",
      [networkedAnimator](bool flipped, Maybe<float> relativeCenterLine) {
        networkedAnimator->setFlipped(flipped, relativeCenterLine.value());
      });
  callbacks.registerCallbackWithSignature<void, float>(
      "setAnimationRate", bind(&NetworkedAnimator::setAnimationRate, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, float, bool>(
      "rotateGroup", bind(&NetworkedAnimator::rotateGroup, networkedAnimator, _1, _2, _3));
  callbacks.registerCallbackWithSignature<float, String>(
      "currentRotationAngle", bind(&NetworkedAnimator::currentRotationAngle, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "hasTransformationGroup", bind(&NetworkedAnimator::hasTransformationGroup, networkedAnimator, _1));

  callbacks.registerCallbackWithSignature<void, String, Vec2F>("translateTransformationGroup",
      bind(&NetworkedAnimator::translateTransformationGroup, networkedAnimator, _1, _2));
  callbacks.registerCallback("rotateTransformationGroup",
      [networkedAnimator](String const& transformationGroup, float rotation, Maybe<Vec2F> const& rotationCenter) {
        networkedAnimator->rotateTransformationGroup(transformationGroup, rotation, rotationCenter.value());
      });
  callbacks.registerCallback("scaleTransformationGroup",
      [networkedAnimator](LuaEngine& engine, String const& transformationGroup, LuaValue scale, Maybe<Vec2F> const& scaleCenter) {
        if (auto cs = engine.luaMaybeTo<Vec2F>(scale))
          networkedAnimator->scaleTransformationGroup(transformationGroup, *cs, scaleCenter.value());
        else
          networkedAnimator->scaleTransformationGroup(transformationGroup, engine.luaTo<float>(scale), scaleCenter.value());
      });
  callbacks.registerCallbackWithSignature<void, String, float, float, float, float, float, float>(
      "transformTransformationGroup",
      bind(&NetworkedAnimator::transformTransformationGroup, networkedAnimator, _1, _2, _3, _4, _5, _6, _7));
  callbacks.registerCallbackWithSignature<void, String>(
      "resetTransformationGroup", bind(&NetworkedAnimator::resetTransformationGroup, networkedAnimator, _1));

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


  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setParticleEmitterActive", bind(&NetworkedAnimator::setParticleEmitterActive, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>("setParticleEmitterEmissionRate",
      bind(&NetworkedAnimator::setParticleEmitterEmissionRate, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, unsigned>("setParticleEmitterBurstCount",
      bind(&NetworkedAnimator::setParticleEmitterBurstCount, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, RectF>("setParticleEmitterOffsetRegion",
      bind(&NetworkedAnimator::setParticleEmitterOffsetRegion, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "burstParticleEmitter", bind(&NetworkedAnimator::burstParticleEmitter, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setLightActive", bind(&NetworkedAnimator::setLightActive, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>(
      "setLightPosition", bind(&NetworkedAnimator::setLightPosition, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Color>(
      "setLightColor", bind(&NetworkedAnimator::setLightColor, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "setLightPointAngle", bind(&NetworkedAnimator::setLightPointAngle, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>(
      "hasSound", bind(&NetworkedAnimator::hasSound, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, StringList>(
      "setSoundPool", bind(&NetworkedAnimator::setSoundPool, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>(
      "setSoundPosition", bind(&NetworkedAnimator::setSoundPosition, networkedAnimator, _1, _2));
  callbacks.registerCallback("playSound",
      [networkedAnimator](String const& sound, Maybe<int> loops) {
        networkedAnimator->playSound(sound, loops.value());
      });

  callbacks.registerCallback("setSoundVolume",
      [networkedAnimator](String const& sound, float targetVolume, Maybe<float> rampTime) {
        networkedAnimator->setSoundVolume(sound, targetVolume, rampTime.value(0));
      });
  callbacks.registerCallback("setSoundPitch",
      [networkedAnimator](String const& sound, float targetPitch, Maybe<float> rampTime) {
        networkedAnimator->setSoundPitchMultiplier(sound, targetPitch, rampTime.value(0));
      });

  callbacks.registerCallback("stopAllSounds",
      [networkedAnimator](String const& sound, Maybe<float> rampTime) {
        networkedAnimator->stopAllSounds(sound, rampTime.value());
      });

  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setEffectActive", bind(&NetworkedAnimator::setEffectEnabled, networkedAnimator, _1, _2));
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

  callbacks.registerCallbackWithSignature<void, String, List<Drawable>>(
      "addPartDrawables", bind(&NetworkedAnimator::addPartDrawables, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, List<Drawable>>(
      "setPartDrawables", bind(&NetworkedAnimator::setPartDrawables, networkedAnimator, _1, _2));

  callbacks.registerCallbackWithSignature<String, String, String>(
      "applyPartTags", bind(&NetworkedAnimator::applyPartTags, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, String>(
      "setLocalTag", bind(&NetworkedAnimator::setLocalTag, networkedAnimator, _1, _2));

  return callbacks;
}

}
