#pragma once

#include "StarInputEvent.hpp"
#include "StarSet.hpp"
#include "StarJson.hpp"

namespace Star {

enum class InterfaceAction {
  None,
  PlayerUp,
  PlayerDown,
  PlayerLeft,
  PlayerRight,
  PlayerJump,
  PlayerMainItem,
  PlayerAltItem,
  PlayerDropItem,
  PlayerInteract,
  PlayerShifting,
  PlayerTechAction1,
  PlayerTechAction2,
  PlayerTechAction3,
  EmoteBlabbering,
  EmoteShouting,
  EmoteHappy,
  EmoteSad,
  EmoteNeutral,
  EmoteLaugh,
  EmoteAnnoyed,
  EmoteOh,
  EmoteOooh,
  EmoteBlink,
  EmoteWink,
  EmoteEat,
  EmoteSleep,
  ShowLabels,
  CameraShift,
  TitleBack,
  CinematicSkip,
  CinematicNext,
  GuiClose,
  GuiShifting,
  KeybindingClear,
  KeybindingCancel,
  ChatPageUp,
  ChatPageDown,
  ChatPreviousLine,
  ChatNextLine,
  ChatSendLine,
  ChatBegin,
  ChatBeginCommand,
  ChatStop,
  InterfaceShowHelp,
  InterfaceHideHud,
  InterfaceChangeBarGroup,
  InterfaceDeselectHands,
  InterfaceBar1,
  InterfaceBar2,
  InterfaceBar3,
  InterfaceBar4,
  InterfaceBar5,
  InterfaceBar6,
  InterfaceBar7,
  InterfaceBar8,
  InterfaceBar9,
  InterfaceBar10,
  EssentialBar1,
  EssentialBar2,
  EssentialBar3,
  EssentialBar4,
  InterfaceRepeatCommand,
  InterfaceToggleFullscreen,
  InterfaceReload,
  InterfaceEscapeMenu,
  InterfaceInventory,
  InterfaceCodex,
  InterfaceQuest,
  InterfaceCrafting,
};
extern EnumMap<InterfaceAction> const InterfaceActionNames;

// Maps the mod keys that can used in key chords to its associated KeyMod.
extern HashMap<Key, KeyMod> const KeyChordMods;

struct KeyChord {
  Key key;
  KeyMod mods;

  bool operator<(KeyChord const& rhs) const;
};

KeyChord inputDescriptorFromJson(Json const& json);
Json inputDescriptorToJson(KeyChord const& chord);

String printInputDescriptor(KeyChord chord);

STAR_CLASS(KeyBindings);

class KeyBindings {
public:
  KeyBindings();
  explicit KeyBindings(Json const& json);

  Set<InterfaceAction> actions(Key key) const;
  Set<InterfaceAction> actions(InputEvent const& event) const;
  Set<InterfaceAction> actions(KeyChord chord) const;
  Set<InterfaceAction> actionsForKey(Key key) const;

private:
  // Maps the primary key to a list of InterfaceActions, and any mods that they
  // require to be held.
  Map<Key, List<pair<KeyMod, InterfaceAction>>> m_actions;
};

}
