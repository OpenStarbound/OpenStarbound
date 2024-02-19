#include "StarKeyBindings.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarLogging.hpp"
#include "StarJsonExtra.hpp"

#include <bitset>

namespace Star {

HashMap<Key, KeyMod> const KeyChordMods{
  {Key::LShift, KeyMod::LShift},
  {Key::RShift, KeyMod::RShift},
  {Key::LCtrl, KeyMod::LCtrl},
  {Key::RCtrl, KeyMod::RCtrl},
  {Key::LAlt, KeyMod::LAlt},
  {Key::RAlt, KeyMod::RAlt},
  {Key::LGui, KeyMod::LGui},
  {Key::RGui, KeyMod::RGui},
  {Key::AltGr, KeyMod::AltGr}
};

EnumMap<InterfaceAction> const InterfaceActionNames{
    {InterfaceAction::None, "None"},
    {InterfaceAction::PlayerUp, "PlayerUp"},
    {InterfaceAction::PlayerDown, "PlayerDown"},
    {InterfaceAction::PlayerLeft, "PlayerLeft"},
    {InterfaceAction::PlayerRight, "PlayerRight"},
    {InterfaceAction::PlayerJump, "PlayerJump"},
    {InterfaceAction::PlayerMainItem, "PlayerMainItem"},
    {InterfaceAction::PlayerAltItem, "PlayerAltItem"},
    {InterfaceAction::PlayerDropItem, "PlayerDropItem"},
    {InterfaceAction::PlayerInteract, "PlayerInteract"},
    {InterfaceAction::PlayerShifting, "PlayerShifting"},
    {InterfaceAction::PlayerTechAction1, "PlayerTechAction1"},
    {InterfaceAction::PlayerTechAction2, "PlayerTechAction2"},
    {InterfaceAction::PlayerTechAction3, "PlayerTechAction3"},
    {InterfaceAction::EmoteBlabbering, "EmoteBlabbering"},
    {InterfaceAction::EmoteShouting, "EmoteShouting"},
    {InterfaceAction::EmoteHappy, "EmoteHappy"},
    {InterfaceAction::EmoteSad, "EmoteSad"},
    {InterfaceAction::EmoteNeutral, "EmoteNeutral"},
    {InterfaceAction::EmoteLaugh, "EmoteLaugh"},
    {InterfaceAction::EmoteAnnoyed, "EmoteAnnoyed"},
    {InterfaceAction::EmoteOh, "EmoteOh"},
    {InterfaceAction::EmoteOooh, "EmoteOooh"},
    {InterfaceAction::EmoteBlink, "EmoteBlink"},
    {InterfaceAction::EmoteWink, "EmoteWink"},
    {InterfaceAction::EmoteEat, "EmoteEat"},
    {InterfaceAction::EmoteSleep, "EmoteSleep"},
    {InterfaceAction::ShowLabels, "ShowLabels"},
    {InterfaceAction::CameraShift, "CameraShift"},
    {InterfaceAction::TitleBack, "TitleBack"},
    {InterfaceAction::CinematicSkip, "CinematicSkip"},
    {InterfaceAction::CinematicNext, "CinematicNext"},
    {InterfaceAction::GuiClose, "GuiClose"},
    {InterfaceAction::GuiShifting, "GuiShifting"},
    {InterfaceAction::KeybindingClear, "KeybindingClear"},
    {InterfaceAction::KeybindingCancel, "KeybindingCancel"},
    {InterfaceAction::ChatPageUp, "ChatPageUp"},
    {InterfaceAction::ChatPageDown, "ChatPageDown"},
    {InterfaceAction::ChatPreviousLine, "ChatPreviousLine"},
    {InterfaceAction::ChatNextLine, "ChatNextLine"},
    {InterfaceAction::ChatSendLine, "ChatSendLine"},
    {InterfaceAction::ChatBegin, "ChatBegin"},
    {InterfaceAction::ChatBeginCommand, "ChatBeginCommand"},
    {InterfaceAction::ChatStop, "ChatStop"},
    {InterfaceAction::InterfaceShowHelp, "InterfaceShowHelp"},
    {InterfaceAction::InterfaceHideHud, "InterfaceHideHud"},
    {InterfaceAction::InterfaceChangeBarGroup, "InterfaceChangeBarGroup"},
    {InterfaceAction::InterfaceDeselectHands, "InterfaceDeselectHands"},
    {InterfaceAction::InterfaceBar1, "InterfaceBar1"},
    {InterfaceAction::InterfaceBar2, "InterfaceBar2"},
    {InterfaceAction::InterfaceBar3, "InterfaceBar3"},
    {InterfaceAction::InterfaceBar4, "InterfaceBar4"},
    {InterfaceAction::InterfaceBar5, "InterfaceBar5"},
    {InterfaceAction::InterfaceBar6, "InterfaceBar6"},
    {InterfaceAction::InterfaceBar7, "InterfaceBar7"},
    {InterfaceAction::InterfaceBar8, "InterfaceBar8"},
    {InterfaceAction::InterfaceBar9, "InterfaceBar9"},
    {InterfaceAction::InterfaceBar10, "InterfaceBar10"},
    {InterfaceAction::EssentialBar1, "EssentialBar1"},
    {InterfaceAction::EssentialBar2, "EssentialBar2"},
    {InterfaceAction::EssentialBar3, "EssentialBar3"},
    {InterfaceAction::EssentialBar4, "EssentialBar4"},
    {InterfaceAction::InterfaceRepeatCommand, "InterfaceRepeatCommand"},
    {InterfaceAction::InterfaceToggleFullscreen, "InterfaceToggleFullscreen"},
    {InterfaceAction::InterfaceReload, "InterfaceReload"},
    {InterfaceAction::InterfaceEscapeMenu, "InterfaceEscapeMenu"},
    {InterfaceAction::InterfaceInventory, "InterfaceInventory"},
    {InterfaceAction::InterfaceCodex, "InterfaceCodex"},
    {InterfaceAction::InterfaceQuest, "InterfaceQuest"},
    {InterfaceAction::InterfaceCrafting, "InterfaceCrafting"},
};

bool KeyChord::operator<(KeyChord const& rhs) const {
  return tie(key, mods) < tie(rhs.key, rhs.mods);
}

KeyChord inputDescriptorFromJson(Json const& json) {
  Key key;
  auto type = json.getString("type");
  if (type == "key") {
    auto value = json.get("value");
    if (value.isType(Json::Type::String)) {
      key = KeyNames.getLeft(value.toString());
    } else if (value.canConvert(Json::Type::Int)) {
      key = (Key)value.toUInt();
    } else {
      throw StarException::format("Improper key value '{}'", value);
    }
  } else {
    throw StarException::format("Improper bindings type '{}'", type);
  }

  KeyMod mods = KeyMod::NoMod;
  for (auto mod : json.get("mods").iterateArray())
    mods |= KeyModNames.getLeft(mod.toString());

  return {key, mods};
}

Json inputDescriptorToJson(KeyChord const& chord) {
  JsonArray modNames;
  for (auto const& p : KeyModNames) {
    if ((chord.mods & p.first) != KeyMod::NoMod)
      modNames.append(p.second);
  }
  return JsonObject{
    {"type", "key"},
    {"value", KeyNames.getRight(chord.key)},
    {"mods", modNames}
  };
}

String printInputDescriptor(KeyChord chord) {
  StringList modNames;
  for (auto const& p : KeyModNames) {
    if ((chord.mods & p.first) != KeyMod::NoMod)
      modNames.append(p.second);
  }

  return String::joinWith(" + ", modNames.join(" + "), KeyNames.getRight(chord.key));
}

KeyBindings::KeyBindings() {}

KeyBindings::KeyBindings(Json const& json) {
  Map<Key, List<pair<KeyMod, InterfaceAction>>> actions;
  try {
    for (auto const& kvpair : json.iterateObject()) {
      InterfaceAction action = InterfaceActionNames.getLeft(kvpair.first);

      for (auto const& input : kvpair.second.iterateArray()) {
        try {
          auto chord = inputDescriptorFromJson(input);
          actions[chord.key].append({chord.mods, action});
        } catch (StarException const& e) {
          Logger::warn("Could not load keybinding for {}: {}\n",
              InterfaceActionNames.getRight(action),
              outputException(e, false));
        }
      }
    }

    m_actions = std::move(actions);
  } catch (StarException const& e) {
    throw StarException(strf("Could not set keybindings from configuration. {}", outputException(e, false)));
  }
}

Set<InterfaceAction> KeyBindings::actions(Key key) const {
  return actions(KeyChord{key, KeyMod::NoMod});
}

Set<InterfaceAction> KeyBindings::actions(InputEvent const& event) const {
  if (auto keyDown = event.ptr<KeyDownEvent>())
    return actions(KeyChord{keyDown->key, keyDown->mods});
  return {};
}

Set<InterfaceAction> KeyBindings::actions(KeyChord chord) const {
  size_t mostMatchedMods = 0;
  Set<InterfaceAction> matching;
  for (auto const& pair : m_actions.value(chord.key)) {
    // first make sure that all required mods for the binding are held
    if ((pair.first & chord.mods) == pair.first) {
      // now count the number of mods in the binding
      size_t matchedMods = 0;
      for (auto modPair : KeyChordMods) {
        if ((modPair.second & pair.first) == modPair.second)
          ++matchedMods;
      }

      if (matchedMods > mostMatchedMods) {
        matching.clear();
        mostMatchedMods = matchedMods;
      }

      // only activate the binding(s) with the most mods
      if (matchedMods == mostMatchedMods)
        matching.add(pair.second);
    }
  }
  return matching;
}

Set<InterfaceAction> KeyBindings::actionsForKey(Key key) const {
  return Set<InterfaceAction>::from(m_actions.value(key).transformed([](auto p){ return p.second; }));
}

}
