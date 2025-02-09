#pragma once

#include "StarInputEvent.hpp"
#include "StarJson.hpp"
#include "StarListener.hpp"
#include "StarHash.hpp"

namespace Star {

STAR_CLASS(Input);
STAR_EXCEPTION(InputException, StarException);

typedef Variant<Key, MouseButton, ControllerButton> InputVariant;

template <>
struct hash<InputVariant> {
  size_t operator()(InputVariant const& v) const;
};

class Input {
public:

  static Json inputEventToJson(InputEvent const& event);

  struct KeyBind {
    Key key = Key::Zero;
    KeyMod mods = KeyMod::NoMod;
    uint8_t priority = 0;

    inline bool operator<(KeyBind const& rhs) const {
      return priority < rhs.priority;
    }

    inline bool operator>(KeyBind const& rhs) const {
      return priority > rhs.priority;
    }
  };

  struct MouseBind {
    MouseButton button = MouseButton::Left;
    KeyMod mods = KeyMod::NoMod;
    uint8_t priority = 0;
  };

  struct ControllerBind {
    unsigned int controller = 0;
    ControllerButton button = ControllerButton::Invalid;
  };

  typedef MVariant<KeyBind, MouseBind, ControllerBind> Bind;

  static Bind bindFromJson(Json const& json);
  static Json bindToJson(Bind const& bind);

  struct BindCategory;

  struct BindEntry {
    // The internal ID of this entry.
    String id;
    // The user-facing name of this entry.
    String name;
    // The category this entry belongs to.
    BindCategory const* category;
    // Associated string tags that become active when this bind is pressed.
    StringList tags;

    // The default binds.
    List<Bind> defaultBinds;
    // The user-configured binds.
    List<Bind> customBinds;

    BindEntry(String entryId, Json const& config, BindCategory const& parentCategory);
    void updated();
  };

  struct BindRef {
    KeyMod mods;
    uint8_t priority = 0;  
    BindEntry* entry = nullptr; // Invalidated on reload, careful!

    BindRef(BindEntry& bindEntry, KeyBind& keyBind);
    BindRef(BindEntry& bindEntry, MouseBind& mouseBind);
    BindRef(BindEntry& bindEntry);
  };

  struct BindCategory {
    String id;
    String name;
    Json config;

    StableHashMap<String, BindEntry> entries;

    BindCategory(String categoryId, Json const& categoryConfig);
  };

  struct InputState {
    unsigned presses = 0;
    unsigned releases = 0;
    bool pressed = false;
    bool held = false;
    bool released = false;

    // Calls the passed functions for each press and release.
    template <typename PressFunction, typename ReleaseFunction>
    void forEach(PressFunction&& pressed, ReleaseFunction&& released) {
      for (size_t i = 0; i != releases; ++i) {
        pressed();
        released();
      }
    }

    inline void reset() {
      presses = releases = 0;
      pressed = released = false;
    }

    inline void press() { pressed = ++presses; held = true; }
    inline void release() { released = ++releases; held = false; }
  };

  struct KeyInputState : InputState {
    KeyMod mods = KeyMod::NoMod;
  };

  struct MouseInputState : InputState {
    List<Vec2I> pressPositions;
    List<Vec2I> releasePositions;
  };

  typedef InputState ControllerInputState;

  // Get pointer to the singleton Input instance, if it exists.  Otherwise,
  // returns nullptr.
  static Input* singletonPtr();

  // Gets reference to Input singleton, throws InputException if root
  // is not initialized.
  static Input& singleton();

  Input();
  ~Input();

  Input(Input const&) = delete;
  Input& operator=(Input const&) = delete;

  List<std::pair<InputEvent, bool>> const& inputEventsThisFrame() const;

  // Clears input state. Should be done at the very start or end of the client loop.
  void reset(bool clear = false);

  void update();

  // Handles an input event.
  bool handleInput(InputEvent const& input, bool gameProcessed);

  void rebuildMappings();

  // Loads input categories and their binds from Assets.
  void reload();

  void setTextInputActive(bool active);

  Maybe<unsigned> bindDown(String const& categoryId, String const& bindId);
  bool            bindHeld(String const& categoryId, String const& bindId);
  Maybe<unsigned> bindUp  (String const& categoryId, String const& bindId);

  Maybe<unsigned> keyDown(Key key, Maybe<KeyMod> keyMod);
  bool            keyHeld(Key key);
  Maybe<unsigned> keyUp  (Key key);

  Maybe<List<Vec2I>> mouseDown(MouseButton button);
  bool               mouseHeld(MouseButton button);
  Maybe<List<Vec2I>> mouseUp  (MouseButton button);

  Vec2I mousePosition() const;

  void resetBinds(String const& categoryId, String const& bindId);
  void setBinds(String const& categoryId, String const& bindId, Json const& binds);
  Json getDefaultBinds(String const& categoryId, String const& bindId); 
  Json getBinds(String const& categoryId, String const& bindId);
  unsigned getTag(String const& tag);

private:
  List<BindEntry*> filterBindEntries(List<BindRef> const& binds, KeyMod mods) const;

  BindEntry* bindEntryPtr(String const& categoryId, String const& bindId);
  BindEntry& bindEntry(String const& categoryId, String const& bindId);

  InputState* bindStatePtr(String const& categoryId, String const& bindId);

  InputState& addBindState(BindEntry const* bindEntry);

  static Input* s_singleton;

  // Regenerated on reload.
  StableHashMap<String, BindCategory> m_bindCategories;
  // Contains raw pointers to bind entries in categories, so also regenerated on reload.
  HashMap<InputVariant, List<BindRef>> m_bindMappings;

  ListenerPtr m_rootReloadListener;

  // Per-frame input event storage for Lua.
  List<std::pair<InputEvent, bool>> m_inputEvents;

  // Per-frame input state maps.
  //Input states
  HashMap<Key, KeyInputState> m_keyStates;
  HashMap<MouseButton, MouseInputState> m_mouseStates;
  HashMap<ControllerButton, ControllerInputState> m_controllerStates;
  //Bind states
  HashMap<BindEntry const*, InputState> m_bindStates;
  StringMap<unsigned> m_activeTags;

  KeyMod m_pressedMods;
  bool m_textInputActive;
  Vec2I m_mousePosition;
};

}
