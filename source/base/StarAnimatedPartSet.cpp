#include "StarAnimatedPartSet.hpp"
#include "StarMathCommon.hpp"
#include "StarJsonExtra.hpp"
#include "StarInterpolation.hpp"

namespace Star {

AnimatedPartSet::AnimatedPartSet() {}

AnimatedPartSet::AnimatedPartSet(Json config, uint8_t animatorVersion) {
  m_animatorVersion = animatorVersion;
  for (auto const& stateTypePair : config.get("stateTypes", JsonObject()).iterateObject()) {
    auto const& stateTypeName = stateTypePair.first;
    auto const& stateTypeConfig = stateTypePair.second;
    if ((version() > 0) && !stateTypeConfig.isType(Json::Type::Object)) // guard just incase any merges use false to override and remove entries from inherited configs
      continue;

    StateType newStateType;
    newStateType.priority = stateTypeConfig.getFloat("priority", 0.0f);
    newStateType.enabled = stateTypeConfig.getBool("enabled", true);
    newStateType.defaultState = stateTypeConfig.getString("default", "");
    newStateType.stateTypeProperties = stateTypeConfig.getObject("properties", {});

    for (auto const& statePair : stateTypeConfig.get("states", JsonObject()).iterateObject()) {
      auto const& stateName = statePair.first;
      auto const& stateConfig = statePair.second;
      if ((version() > 0) && !stateConfig.isType(Json::Type::Object)) // guard just incase any merges use false to override and remove entries from inherited configs
        continue;

      auto newState = make_shared<State>();
      newState->frames = stateConfig.getInt("frames", 1);
      newState->cycle = stateConfig.getFloat("cycle", 1.0f);
      newState->animationMode = stringToAnimationMode(stateConfig.getString("mode", "end"));
      newState->transitionState = stateConfig.getString("transition", "");
      newState->stateProperties = stateConfig.getObject("properties", {});
      newState->stateFrameProperties = stateConfig.getObject("frameProperties", {});
      newStateType.states[stateName] = std::move(newState);
    }

    newStateType.states.sortByKey();

    newStateType.activeState.stateTypeName = stateTypeName;
    newStateType.activeState.reverse = false;
    newStateType.activeStateDirty = true;

    if (newStateType.defaultState.empty() && !newStateType.states.empty())
      newStateType.defaultState = newStateType.states.firstKey();

    m_stateTypes[stateTypeName] = std::move(newStateType);
  }

  // Sort state types by decreasing priority.
  m_stateTypes.sort([](pair<String, StateType> const& a, pair<String, StateType> const& b) {
      return b.second.priority < a.second.priority;
    });

  for (auto const& partPair : config.get("parts", JsonObject()).iterateObject()) {
    auto const& partName = partPair.first;
    auto const& partConfig = partPair.second;
    if ((version() > 0) && !partConfig.isType(Json::Type::Object)) // guard just incase any merges use false to override and remove entries from inherited configs
      continue;

    Part newPart;
    newPart.partProperties = partConfig.getObject("properties", {});

    for (auto const& partStateTypePair : partConfig.get("partStates", JsonObject()).iterateObject()) {
      auto const& stateTypeName = partStateTypePair.first;

      for (auto const& partStatePair : partStateTypePair.second.toObject()) {
        auto const& stateName = partStatePair.first;
        auto stateConfig = partStatePair.second;
        if ((version() > 0) && stateConfig.isType(Json::Type::String))
          stateConfig = partStateTypePair.second.get(stateConfig.toString());

        if ((version() > 0) && !stateConfig.isType(Json::Type::Object)) // guard just incase any merges use false to override and remove entries from inherited configs
          continue;

        PartState partState = {stateConfig.getObject("properties", {}), stateConfig.getObject("frameProperties", {})};
        newPart.partStates[stateTypeName][stateName] = std::move(partState);
      }
    }
    newPart.activePart.partName = partPair.first;
    newPart.activePart.setAnimationAffineTransform(Mat3F::identity());
    newPart.activePartDirty = true;

    m_parts[partName] = std::move(newPart);
  }

  for (auto const& pair : m_stateTypes)
    setActiveState(pair.first, pair.second.defaultState, true, false);
}

StringList AnimatedPartSet::stateTypes() const {
  return m_stateTypes.keys();
}

void AnimatedPartSet::setStateTypeEnabled(String const& stateTypeName, bool enabled) {
  auto& stateType = m_stateTypes.get(stateTypeName);
  if (stateType.enabled != enabled) {
    stateType.enabled = enabled;
    for (auto& pair : m_parts)
      pair.second.activePartDirty = true;
  }
}

void AnimatedPartSet::setEnabledStateTypes(StringList const& stateTypeNames) {
  for (auto& pair : m_stateTypes)
    pair.second.enabled = false;

  for (auto const& stateTypeName : stateTypeNames)
    m_stateTypes.get(stateTypeName).enabled = true;

  for (auto& pair : m_parts)
    pair.second.activePartDirty = true;
}

bool AnimatedPartSet::stateTypeEnabled(String const& stateTypeName) const {
  return m_stateTypes.get(stateTypeName).enabled;
}

StringList AnimatedPartSet::states(String const& stateTypeName) const {
  return m_stateTypes.get(stateTypeName).states.keys();
}

StringList AnimatedPartSet::partNames() const {
  return m_parts.keys();
}

bool AnimatedPartSet::setActiveState(String const& stateTypeName, String const& stateName, bool alwaysStart, bool reverse) {
  auto& stateType = m_stateTypes.get(stateTypeName);
  if (stateType.activeState.stateName != stateName || alwaysStart || stateType.activeState.reverse != reverse) {
    stateType.activeState.stateName = stateName;
    stateType.activeState.timer = 0.0f;
    stateType.activeState.frameProgress = 0.0f;
    stateType.activeState.reverse = reverse;
    stateType.activeStatePointer = stateType.states.get(stateName).get();

    stateType.activeStateDirty = true;
    for (auto& pair : m_parts)
      pair.second.activePartDirty = true;

    return true;
  } else {
    return false;
  }
}

void AnimatedPartSet::restartState(String const& stateTypeName) {
  auto& stateType = m_stateTypes.get(stateTypeName);
  stateType.activeState.timer = 0.0f;

  stateType.activeStateDirty = true;
  for (auto& pair : m_parts)
    pair.second.activePartDirty = true;
}

AnimatedPartSet::ActiveStateInformation const& AnimatedPartSet::activeState(String const& stateTypeName) const {
  auto& stateType = const_cast<StateType&>(m_stateTypes.get(stateTypeName));
  const_cast<AnimatedPartSet*>(this)->freshenActiveState(stateType);
  return stateType.activeState;
}

AnimatedPartSet::ActivePartInformation const& AnimatedPartSet::activePart(String const& partName) const {
  auto& part = const_cast<Part&>(m_parts.get(partName));
  const_cast<AnimatedPartSet*>(this)->freshenActivePart(part);
  return part.activePart;
}

AnimatedPartSet::State const& AnimatedPartSet::getState(String const& stateTypeName, String const& stateName) const {
  return *m_stateTypes.get(stateTypeName).states.get(stateName);
}

StringMap<AnimatedPartSet::Part> const& AnimatedPartSet::constParts() const {
  return m_parts;
}

StringMap<AnimatedPartSet::Part>& AnimatedPartSet::parts() {
  return m_parts;
}

void AnimatedPartSet::forEachActiveState(function<void(String const&, ActiveStateInformation const&)> callback) const {
  for (auto const& p : m_stateTypes) {
    const_cast<AnimatedPartSet*>(this)->freshenActiveState(const_cast<StateType&>(p.second));
    callback(p.first, p.second.activeState);
  }
}

void AnimatedPartSet::forEachActivePart(function<void(String const&, ActivePartInformation const&)> callback) const {
  for (auto const& p : m_parts) {
    const_cast<AnimatedPartSet*>(this)->freshenActivePart(const_cast<Part&>(p.second));
    callback(p.first, p.second.activePart);
  }
}

size_t AnimatedPartSet::activeStateIndex(String const& stateTypeName) const {
  auto const& stateType = m_stateTypes.get(stateTypeName);
  return *stateType.states.indexOf(stateType.activeState.stateName);
}
bool AnimatedPartSet::activeStateReverse(String const& stateTypeName) const {
  auto const& stateType = m_stateTypes.get(stateTypeName);
  return stateType.activeState.reverse;
}

bool AnimatedPartSet::setActiveStateIndex(String const& stateTypeName, size_t stateIndex, bool alwaysStart, bool reverse) {
  auto const& stateType = m_stateTypes.get(stateTypeName);
  String const& stateName = stateType.states.keyAt(stateIndex);
  return setActiveState(stateTypeName, stateName, alwaysStart, reverse);
}

void AnimatedPartSet::update(float dt) {
  for (auto& pair : m_stateTypes) {
    auto& stateType = pair.second;
    auto const& state = *stateType.activeStatePointer;

    stateType.activeState.timer += dt;
    if (stateType.activeState.timer > state.cycle) {
      if (state.animationMode == End) {
        stateType.activeState.timer = state.cycle;
      } else if (state.animationMode == Loop) {
        stateType.activeState.timer = std::fmod(stateType.activeState.timer, state.cycle);
      } else if (state.animationMode == Transition) {
        stateType.activeState.stateName = state.transitionState;
        stateType.activeState.timer = 0.0f;
        stateType.activeStatePointer = stateType.states.get(state.transitionState).get();
      }
    }

    stateType.activeStateDirty = true;
  }

  for (auto& pair : m_parts)
    pair.second.activePartDirty = true;
}

void AnimatedPartSet::finishAnimations() {
  for (auto& pair : m_stateTypes) {
    auto& stateType = pair.second;

    while (true) {
      auto const& state = *stateType.activeStatePointer;

      if (state.animationMode == End) {
        stateType.activeState.timer = state.cycle;
      } else if (state.animationMode == Transition) {
        stateType.activeState.stateName = state.transitionState;
        stateType.activeState.timer = 0.0f;
        stateType.activeStatePointer = stateType.states.get(state.transitionState).get();
        continue;
      }
      break;
    }

    stateType.activeStateDirty = true;
  }

  for (auto& pair : m_parts)
    pair.second.activePartDirty = true;
}

AnimatedPartSet::AnimationMode AnimatedPartSet::stringToAnimationMode(String const& string) {
  if (string.equals("end", String::CaseInsensitive)) {
    return End;
  } else if (string.equals("loop", String::CaseInsensitive)) {
    return Loop;
  } else if (string.equals("transition", String::CaseInsensitive)) {
    return Transition;
  } else {
    throw AnimatedPartSetException(strf("No such AnimationMode '{}'", string));
  }
}

void AnimatedPartSet::freshenActiveState(StateType& stateType) {
  if (stateType.activeStateDirty) {
    auto const& state = *stateType.activeStatePointer;
    auto& activeState = stateType.activeState;

    double progress = (activeState.timer / state.cycle * state.frames);
    activeState.frameProgress = std::fmod(progress, 1);
    activeState.frame = clamp<int>(progress, 0, state.frames - 1);
    if (activeState.reverse) {
      activeState.frame = (state.frames - 1) - activeState.frame;
      if ((state.animationMode == Loop) && (activeState.frame <= 0)) {
        activeState.nextFrame = state.frames - 1;
      } else {
        activeState.nextFrame = clamp<int>(activeState.frame - 1, 0, state.frames - 1);
      }
    } else {
      if ((state.animationMode == Loop) && (activeState.frame >= (state.frames - 1))) {
        activeState.nextFrame = 0;
      } else {
        activeState.nextFrame = clamp<int>(activeState.frame + 1, 0, state.frames - 1);
      }
    }

    activeState.properties = stateType.stateTypeProperties;
    activeState.properties.merge(state.stateProperties, true);

    activeState.nextProperties = stateType.stateTypeProperties;
    activeState.nextProperties.merge(state.stateProperties, true);

    for (auto const& pair : state.stateFrameProperties) {
      if (activeState.frame < pair.second.size())
        activeState.properties[pair.first] = pair.second.get(activeState.frame);
      if (activeState.nextFrame < pair.second.size())
        activeState.nextProperties[pair.first] = pair.second.get(activeState.nextFrame);
    }

    stateType.activeStateDirty = false;
  }
}

void AnimatedPartSet::freshenActivePart(Part& part) {
  if (part.activePartDirty) {
    // First reset all the active part information assuming that no state type
    // x state match exists.
    auto& activePart = part.activePart;
    activePart.activeState = {};
    activePart.properties = part.partProperties;
    activePart.nextProperties = part.partProperties;

    // Then go through each of the state types and states and look for a part
    // state match in order of priority.
    for (auto& stateTypePair : m_stateTypes) {
      auto const& stateTypeName = stateTypePair.first;
      auto& stateType = stateTypePair.second;

      // Skip disabled state types
      if (!stateType.enabled)
        continue;

      auto partStateType = part.partStates.ptr(stateTypeName);
      if (!partStateType)
        continue;

      auto const& stateName = stateType.activeState.stateName;
      auto partState = partStateType->ptr(stateName);
      if (!partState)
        continue;

      // If we have a partState match, then set the active state information.
      freshenActiveState(stateType);
      activePart.activeState = stateType.activeState;
      unsigned frame = stateType.activeState.frame;
      unsigned nextFrame = stateType.activeState.nextFrame;

      // Then set the part state data, as well as any part state frame data if
      // the current frame is within the list size.
      activePart.properties.merge(partState->partStateProperties, true);

      activePart.nextProperties.merge(partState->partStateProperties, true);

      for (auto const& pair : partState->partStateFrameProperties) {
        if (frame < pair.second.size())
          activePart.properties[pair.first] = pair.second.get(frame);
        if (nextFrame < pair.second.size())
          activePart.nextProperties[pair.first] = pair.second.get(nextFrame);
      }

      // Each part can only have one state type x state match, so we are done.
      break;
    }
    if (version() > 0) {
      auto processTransforms = [](Mat3F mat, JsonArray transforms, JsonObject properties) -> Mat3F {
        for (auto const& v : transforms) {
          auto action = v.getString(0);
          if (action == "reset") {
            mat = Mat3F::identity();
          } else if (action == "translate") {
            mat.translate(jsonToVec2F(v.getArray(1)));
          } else if (action == "rotate") {
            mat.rotate(v.getFloat(1), jsonToVec2F(v.getArray(2, properties.maybe("rotationCenter").value(JsonArray({0,0})).toArray())));
          } else if (action == "rotateDegrees") { // because radians are fucking annoying
            mat.rotate(v.getFloat(1) * Star::Constants::pi / 180, jsonToVec2F(v.getArray(2, properties.maybe("rotationCenter").value(JsonArray({0,0})).toArray())));
          } else if (action == "scale") {
            mat.scale(jsonToVec2F(v.getArray(1)), jsonToVec2F(v.getArray(2, properties.maybe("scalingCenter").value(JsonArray({0,0})).toArray())));
          } else if (action == "transform") {
            mat = Mat3F(v.getFloat(1), v.getFloat(2), v.getFloat(3), v.getFloat(4), v.getFloat(5), v.getFloat(6), 0, 0, 1) * mat;
          }
        }
        return mat;
      };


      if (auto transforms = activePart.properties.ptr("transforms")) {
        auto mat = processTransforms(activePart.animationAffineTransform(), transforms->toArray(), activePart.properties);
        if (activePart.properties.maybe("interpolated").value(false).toBool()) {
          if (auto nextTransforms = activePart.nextProperties.ptr("transforms")) {
            auto nextMat = processTransforms(activePart.animationAffineTransform(), nextTransforms->toArray(), activePart.nextProperties);
            activePart.setAnimationAffineTransform(mat, nextMat, activePart.activeState ? activePart.activeState->frameProgress : 1);
          } else {
            activePart.setAnimationAffineTransform(mat);
          }
        } else {
          activePart.setAnimationAffineTransform(mat);
        }
      }
    }

    part.activePartDirty = false;
  }
}

void AnimatedPartSet::ActivePartInformation::setAnimationAffineTransform(Mat3F const& matrix) {
  xTranslationAnimation = matrix[0][2];
  yTranslationAnimation = matrix[1][2];
  xScaleAnimation = sqrt(square(matrix[0][0]) + square(matrix[0][1]));
  yScaleAnimation = sqrt(square(matrix[1][0]) + square(matrix[1][1]));
  xShearAnimation = atan2(matrix[0][1], matrix[0][0]);
  yShearAnimation = atan2(matrix[1][0], matrix[1][1]);
}
void AnimatedPartSet::ActivePartInformation::setAnimationAffineTransform(Mat3F const& mat1, Mat3F const& mat2, float progress) {
  xTranslationAnimation = lerp(progress, mat1[0][2], mat2[0][2]);
  yTranslationAnimation = lerp(progress, mat1[1][2], mat2[1][2]);
  xScaleAnimation = lerp(progress, sqrt(square(mat1[0][0]) + square(mat1[0][1])), sqrt(square(mat2[0][0]) + square(mat2[0][1])));
  yScaleAnimation = lerp(progress, sqrt(square(mat1[1][0]) + square(mat1[1][1])), sqrt(square(mat2[1][0]) + square(mat2[1][1])));
  xShearAnimation = angleLerp(progress, atan2(mat1[0][1], mat1[0][0]), atan2(mat2[0][1], mat2[0][0]));
  yShearAnimation = angleLerp(progress, atan2(mat1[1][0], mat1[1][1]), atan2(mat2[1][0], mat2[1][1]));
}

Mat3F AnimatedPartSet::ActivePartInformation::animationAffineTransform() const {
  return Mat3F(
      xScaleAnimation * cos(xShearAnimation), xScaleAnimation * sin(xShearAnimation), xTranslationAnimation,
      yScaleAnimation * sin(yShearAnimation), yScaleAnimation * cos(yShearAnimation), yTranslationAnimation,
      0, 0, 1
    );
}

uint8_t AnimatedPartSet::version() const {
  return m_animatorVersion;
}

Json AnimatedPartSet::getStateFrameProperty(String const & stateTypeName, String const & propertyName, String stateName, int frame) const {
  auto stateType = m_stateTypes.get(stateTypeName);
  auto state = stateType.states.get(stateName);
  if (auto frameProperty = state->stateFrameProperties.maybe(propertyName))
    if (frame < frameProperty.value().size())
      return frameProperty.value().get(frame);
  return state->stateProperties.maybe(propertyName).value(stateType.stateTypeProperties.maybe(propertyName).value(Json()));
}

Json AnimatedPartSet::getPartStateFrameProperty(String const & partName, String const & propertyName, String const & stateTypeName, String stateName, int frame) const {
  auto part = m_parts.get(partName);
  auto state = part.partStates.get(stateTypeName).get(stateName);
  if (auto frameProperty = state.partStateFrameProperties.maybe(propertyName))
    if (frame < frameProperty.value().size())
      return frameProperty.value().get(frame);
  return state.partStateProperties.maybe(propertyName).value(part.partProperties.maybe(propertyName).value(Json()));
}


}
