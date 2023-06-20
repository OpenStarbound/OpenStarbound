#ifndef STAR_ANIMATED_PART_SET_HPP
#define STAR_ANIMATED_PART_SET_HPP

#include "StarOrderedMap.hpp"
#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(AnimatedPartSetException, StarException);

// Defines a "animated" data set constructed in such a way that it is very
// useful for doing generic animations with lots of additional animation data.
// It is made up of two concepts, "states" and "parts".
//
// States:
//
// There are N "state types" defined, which each defines a set of mutually
// exclusive states that each "state type" can be in.  For example, one state
// type might be "movement", and the "movement" states might be "idle", "walk",
// and "run.  Another state type might be "attack" which could have as its
// states "idle", and "melee".  Each state type will have exactly one currently
// active state, so this class may, for example,  be in the total state of
// "movement:idle" and "attack:melee".  Each state within each state type is
// animated, so that over time the state frame increases and may loop around,
// or transition into another state so that that state type without interaction
// may go from "melee" to "idle" when the "melee" state animation is finished.
// This is defined by the individual state config in the configuration passed
// into the constructor.
//
// Parts:
//
// Each instance of this class also can have N "Parts" defined, which are
// groups of properties that "listen" to active states.  Each part can "listen"
// to one or more state types, and the first matching state x state type pair
// (in order of state type priority which is specified in the config) is
// chosen, and the properties from that state type and state are merged into
// the part to produce the final active part information.  Rather than having a
// single image or image set for each part, since this class is intended to be
// as generic as possible, all of this data is assumed to be queried from the
// part properties, so that things such as image data as well as other things
// like damage or collision polys can be stored along with the animation
// frames, the part state, the base part, whichever is most applicable.
class AnimatedPartSet {
public:
  struct ActiveStateInformation {
    String stateTypeName;
    String stateName;
    float timer;
    unsigned frame;
    JsonObject properties;
  };

  struct ActivePartInformation {
    String partName;
    // If a state match is found, this will be set.
    Maybe<ActiveStateInformation> activeState;
    JsonObject properties;
  };

  AnimatedPartSet();
  AnimatedPartSet(Json config);

  // Returns the available state types.
  StringList stateTypes() const;

  // If a state type is disabled, no parts will match against it even
  // if they have entries for that state type.
  void setStateTypeEnabled(String const& stateTypeName, bool enabled);
  void setEnabledStateTypes(StringList const& stateTypeNames);
  bool stateTypeEnabled(String const& stateTypeName) const;

  // Returns the available states for the given state type.
  StringList states(String const& stateTypeName) const;

  StringList parts() const;

  // Sets the active state for this state type.  If the state is different than
  // the previously set state, will start the new states animation off at the
  // beginning.  If alwaysStart is true, then starts the state animation off at
  // the beginning even if no state change has occurred.  Returns true if a
  // state animation reset was done.
  bool setActiveState(String const& stateTypeName, String const& stateName, bool alwaysStart = false);

  // Restart this given state type's timer off at the beginning.
  void restartState(String const& stateTypeName);

  ActiveStateInformation const& activeState(String const& stateTypeName) const;
  ActivePartInformation const& activePart(String const& partName) const;

  // Function will be given the name of each state type, and the
  // ActiveStateInformation for the active state for that state type.
  void forEachActiveState(function<void(String const&, ActiveStateInformation const&)> callback) const;

  // Function will be given the name of each part, and the
  // ActivePartInformation for the active part.
  void forEachActivePart(function<void(String const&, ActivePartInformation const&)> callback) const;

  // Useful for serializing state changes.  Since each set of states for a
  // state type is ordered, it is possible to simply serialize and deserialize
  // the state index for that state type.
  size_t activeStateIndex(String const& stateTypeName) const;
  bool setActiveStateIndex(String const& stateTypeName, size_t stateIndex, bool alwaysStart = false);

  // Animate each state type forward 'dt' time, and either change state frames
  // or transition to new states, depending on the config.
  void update(float dt);

  // Pushes all the animations into their final state
  void finishAnimations();

private:
  enum AnimationMode {
    End,
    Loop,
    Transition
  };

  struct State {
    unsigned frames;
    float cycle;
    AnimationMode animationMode;
    String transitionState;
    JsonObject stateProperties;
    JsonObject stateFrameProperties;
  };

  struct StateType {
    float priority;
    bool enabled;
    String defaultState;
    JsonObject stateTypeProperties;
    OrderedHashMap<String, shared_ptr<State const>> states;

    ActiveStateInformation activeState;
    State const* activeStatePointer;
    bool activeStateDirty;
  };

  struct PartState {
    JsonObject partStateProperties;
    JsonObject partStateFrameProperties;
  };

  struct Part {
    JsonObject partProperties;
    StringMap<StringMap<PartState>> partStates;

    ActivePartInformation activePart;
    bool activePartDirty;
  };

  static AnimationMode stringToAnimationMode(String const& string);

  void freshenActiveState(StateType& stateType);
  void freshenActivePart(Part& part);

  OrderedHashMap<String, StateType> m_stateTypes;
  StringMap<Part> m_parts;
};

}

#endif
