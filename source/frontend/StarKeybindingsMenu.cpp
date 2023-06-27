#include "StarKeybindingsMenu.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarConfiguration.hpp"
#include "StarGuiReader.hpp"
#include "StarListWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarOrderedSet.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

KeybindingsMenu::KeybindingsMenu() : m_activeKeybinding(nullptr) {
  GuiReader reader;
  reader.registerCallback("cancel",
      [&](Widget*) {
        revert();
        dismiss();
      });
  reader.registerCallback("accept",
      [&](Widget*) {
        apply();
        dismiss();
      });
  reader.registerCallback("setDefault", [&](Widget*) { resetDefaults(); });

  auto assets = Root::singleton().assets();

  m_maxBindings = assets->json("/interface/windowconfig/keybindingsmenu.config:maxBindings").toUInt();

  Json paneLayout = assets->json("/interface/windowconfig/keybindingsmenu.config:paneLayout");
  reader.construct(paneLayout, this);

  buildListsFromConfig();

  m_currentMods = KeyMod::NoMod;
}

KeyboardCaptureMode KeybindingsMenu::keyboardCaptured() const {
  return m_activeKeybinding ? KeyboardCaptureMode::KeyEvents : KeyboardCaptureMode::None;
}

bool KeybindingsMenu::sendEvent(InputEvent const& event) {
  if (!m_visible)
    return false;

  if (m_activeKeybinding) {
    if (m_context->actions(event).contains(InterfaceAction::KeybindingClear)) {
      clearActive();
      return true;
    }

    if (m_context->actions(event).contains(InterfaceAction::KeybindingCancel)) {
      exitActiveMode();
      return true;
    }
  }

  if (m_activeKeybinding) {
    // HACK: I need to pass events only to the trash button first.
    if (m_activeKeybinding->parent()->fetchChild<ButtonWidget>("deleteBinding")->sendEvent(event))
      return true;

    if (auto keyUp = event.ptr<KeyUpEvent>()) {
      if (Maybe<KeyMod> modKey = KeyChordMods.maybe(keyUp->key)) {
        m_currentMods &= ~*modKey;
        setKeybinding(KeyChord{keyUp->key, m_currentMods});
        return true;
      }
    } else if (auto keyDown = event.ptr<KeyDownEvent>()) {
      Maybe<KeyMod> modKey = KeyModNames.maybeLeft(KeyNames.getRight(keyDown->key));

      if (modKey) {
        m_currentMods |= *modKey;
        return true;
      } else {
        setKeybinding(KeyChord{keyDown->key, m_currentMods});
        return true;
      }
    }
  }

  if (m_context->actions(event).contains(InterfaceAction::GuiClose)) {
    dismiss();
    return true;
  }

  if (Pane::sendEvent(event))
    return true;

  return false;
}

void KeybindingsMenu::show() {
  m_origConfiguration = Root::singleton().configuration()->get("bindings");
  Pane::show();
}

void KeybindingsMenu::dismissed() {
  exitActiveMode();
  Pane::dismissed();
}

void KeybindingsMenu::buildListsFromConfig() {
  m_playerList = fetchChild<ListWidget>("categories.tabs.player.scrollArea.keyList");
  m_toolBarList = fetchChild<ListWidget>("categories.tabs.toolbar.scrollArea.keyList");
  m_gameList = fetchChild<ListWidget>("categories.tabs.game.scrollArea.keyList");

  m_childToAction.clear();

  auto doKeybindingsFor = [&](ListWidgetPtr const& list, Json const& keybinds) {
    list->clear();

    list->registerMemberCallback("activateBinding", [this](Widget* widget) { activateBinding(widget); });

    list->registerMemberCallback("deleteBinding", [this](Widget*) { clearActive(); });

    auto config = Root::singleton().configuration();
    auto bindings = config->get("bindings");

    for (auto const& keybind : keybinds.iterateArray()) {
      auto newListMember = list->addItem();
      auto actionString = keybind.get("action").toString();
      auto action = InterfaceActionNames.getLeft(actionString);
      List<KeyChord> inputDesc;
      try {
        for (auto const& bindingEntry : bindings.get(actionString).iterateArray())
          inputDesc.append(inputDescriptorFromJson(bindingEntry));
      } catch (StarException const& e) {
        Logger::warn("Could not load keybinding for {}. {}\n", actionString, e.what());
      }

      m_childToAction.insert({newListMember->fetchChild<ButtonWidget>("boundKeys").get(), action});
      newListMember->fetchChild<LabelWidget>("actionName")->setText(keybind.getString("label"));
      newListMember->fetchChild<ButtonWidget>("boundKeys")->setText(StringList(inputDesc.transformed(printInputDescriptor)).join(", "));
      newListMember->fetchChild<ButtonWidget>("deleteBinding")->hide();
    }
  };

  auto assets = Root::singleton().assets();
  doKeybindingsFor(m_playerList, assets->json("/interface/windowconfig/keybindingsmenu.config:keyActions.player"));
  doKeybindingsFor(m_toolBarList, assets->json("/interface/windowconfig/keybindingsmenu.config:keyActions.toolbar"));
  doKeybindingsFor(m_gameList, assets->json("/interface/windowconfig/keybindingsmenu.config:keyActions.game"));
}

bool KeybindingsMenu::activateBinding(Widget* widget) {
  exitActiveMode();

  m_activeKeybinding = widget;
  m_activeKeybinding->parent()->fetchChild<ButtonWidget>("deleteBinding")->show();
  convert<ButtonWidget>(m_activeKeybinding)->setHighlighted(true);

  return false;
}

void KeybindingsMenu::setKeybinding(KeyChord desc) {
  if (!m_activeKeybinding)
    return;

  auto out = inputDescriptorToJson(desc);

  auto config = Root::singleton().configuration();
  auto base = config->get("bindings");

  auto action = m_childToAction.get(m_activeKeybinding);
  auto key = InterfaceActionNames.getRight(action);

  auto bindings = OrderedHashSet<Json>::from(base.get(key).toArray());

  if (bindings.contains(out))
    bindings.clear();

  bindings.add(out);

  if (bindings.size() > m_maxBindings)
    bindings.removeFirst();

  base = base.set(key, JsonArray::from(bindings));

  config->set("bindings", base);

  String buttonText;

  for (auto const& entry : base.get(key).iterateArray()) {
    auto stored = inputDescriptorFromJson(entry);
    buttonText = String::joinWith(", ", buttonText, printInputDescriptor(stored));
  }

  convert<ButtonWidget>(m_activeKeybinding)->setText(buttonText);

  apply();
  exitActiveMode();
}

void KeybindingsMenu::clearActive() {
  if (!m_activeKeybinding)
    return;

  auto config = Root::singleton().configuration();
  auto base = config->get("bindings").toObject();

  auto action = m_childToAction.get(m_activeKeybinding);
  auto key = InterfaceActionNames.getRight(action);

  base[key] = JsonArray{};
  config->set("bindings", base);

  convert<ButtonWidget>(m_activeKeybinding)->setText("<Unbound>");

  apply();
  exitActiveMode();
}

void KeybindingsMenu::exitActiveMode() {
  if (!m_activeKeybinding)
    return;

  m_activeKeybinding->parent()->fetchChild<ButtonWidget>("deleteBinding")->hide();
  convert<ButtonWidget>(m_activeKeybinding)->setHighlighted(false);
  m_activeKeybinding = nullptr;
  m_currentMods = KeyMod::NoMod;
}

void KeybindingsMenu::apply() {
  m_context->refreshKeybindings();
}

void KeybindingsMenu::revert() {
  Root::singleton().configuration()->set("bindings", m_origConfiguration);
  apply();

  buildListsFromConfig();
}

void KeybindingsMenu::resetDefaults() {
  auto config = Root::singleton().configuration();
  config->set("bindings", config->getDefault("bindings"));
  apply();

  buildListsFromConfig();
}

}
