#include "StarAnimatedPartSet.hpp"
#include "StarMathCommon.hpp"

namespace Star {

AnimatedPartSet::AnimatedPartSet() {}

AnimatedPartSet::AnimatedPartSet(Json config) {
  for (auto const& stateTypePair : config.get("stateTypes", JsonObject()).iterateObject()) {
    auto const& stateTypeName = stateTypePair.first;
    auto const& stateTypeConfig = stateTypePair.second;

    StateType newStateType;
    newStateType.priority = stateTypeConfig.getFloat("priority", 0.0f);
    newStateType.enabled = stateTypeConfig.getBool("enabled", true);
    newStateType.defaultState = stateTypeConfig.getString("default", "");
    newStateType.stateTypeProperties = stateTypeConfig.getObject("properties", {});

    for (auto const& statePair : stateTypeConfig.get("states", JsonObject()).iterateObject()) {
      auto const& stateName = statePair.first;
      auto const& stateConfig = statePair.second;

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

    Part newPart;
    newPart.partProperties = partConfig.getObject("properties", {});

    for (auto const& partStateTypePair : partConfig.get("partStates", JsonObject()).iterateObject()) {
      auto const& stateTypeName = partStateTypePair.first;

      for (auto const& partStatePair : partStateTypePair.second.toObject()) {
        auto const& stateName = partStatePair.first;
        auto const& stateConfig = partStatePair.second;

        PartState partState = {stateConfig.getObject("properties", {}), stateConfig.getObject("frameProperties", {})};
        newPart.partStates[stateTypeName][stateName] = std::move(partState);
      }
    }
    newPart.activePart.partName = partPair.first;
    newPart.activePartDirty = true;

    m_parts[partName] = std::move(newPart);
  }

  for (auto const& pair : m_stateTypes)
    setActiveState(pair.first, pair.second.defaultState, true);
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

StringList AnimatedPartSet::parts() const {
  return m_parts.keys();
}

bool AnimatedPartSet::setActiveState(String const& stateTypeName, String const& stateName, bool alwaysStart) {
  auto& stateType = m_stateTypes.get(stateTypeName);
  if (stateType.activeState.stateName != stateName || alwaysStart) {
    stateType.activeState.stateName = stateName;
    stateType.activeState.timer = 0.0f;
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

bool AnimatedPartSet::setActiveStateIndex(String const& stateTypeName, size_t stateIndex, bool alwaysStart) {
  auto const& stateType = m_stateTypes.get(stateTypeName);
  String const& stateName = stateType.states.keyAt(stateIndex);
  return setActiveState(stateTypeName, stateName, alwaysStart);
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
    activeState.frame = clamp<int>(activeState.timer / state.cycle * state.frames, 0, state.frames - 1);

    activeState.properties = stateType.stateTypeProperties;
    activeState.properties.merge(state.stateProperties, true);

    for (auto const& pair : state.stateFrameProperties) {
      if (activeState.frame < pair.second.size())
        activeState.properties[pair.first] = pair.second.get(activeState.frame);
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

      // Then set the part state data, as well as any part state frame data if
      // the current frame is within the list size.
      activePart.properties.merge(partState->partStateProperties, true);

      for (auto const& pair : partState->partStateFrameProperties) {
        if (frame < pair.second.size())
          activePart.properties[pair.first] = pair.second.get(frame);
      }

      // Each part can only have one state type x state match, so we are done.
      break;
    }

    part.activePartDirty = false;
  }
}

}
