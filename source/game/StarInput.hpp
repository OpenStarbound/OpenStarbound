#ifndef STAR_INPUT_HPP
#define STAR_INPUT_HPP

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

      // The default binds.
      List<Bind> defaultBinds;
      // The user-configured binds.
      List<Bind> customBinds;

      BindEntry(String entryId, Json const& config, BindCategory const& parentCategory);
    };

    struct BindRef {
      KeyMod mods;
      uint8_t priority;

      // Invalidated on reload, careful!
      BindEntry* entry;
    };

    struct BindRefSorter {
      inline bool operator() (BindRef const& a, BindRef const& b) {
        return a.priority > b.priority;
      }
    };

    struct BindCategory {
      String id;
      String name;
      Json config;

      StringMap<BindEntry> entries;

      BindCategory(String categoryId, Json const& categoryConfig);
    };

    struct InputState {
      size_t presses = 0;
      size_t releases = 0;

      // Calls the passed functions for each press and release.
      template <typename PressFunction, typename ReleaseFunction>
      void forEach(PressFunction&& pressed, ReleaseFunction&& released) {
        for (size_t i = 0; i != releases; ++i) {
          pressed();
          released();
        }
      }

      constexpr bool held() {
        return presses < releases;
      }
    };

    // Get pointer to the singleton root instance, if it exists.  Otherwise,
    // returns nullptr.
    static Input* singletonPtr();

    // Gets reference to GuiContext singleton, throws GuiContextException if root
    // is not initialized.
    static Input& singleton();

    Input();
    ~Input();

    Input(Input const&) = delete;
    Input& operator=(Input const&) = delete;

    // Clears input state. Should be done at the end of the client loop.
    void reset();

    // Handles an input event.
    bool handleInput(InputEvent const& event);

    // Loads input categories and their binds from Assets.
    void reload();
  private:
    static Input* s_singleton;

    // Regenerated on reload.
    StringMap<BindCategory> m_bindCategories;
    // Contains raw pointers to bind entries in categories, so also regenerated on reload.
    HashMap<InputVariant, List<BindRef>> m_inputsToBinds;

    ListenerPtr m_rootReloadListener;

    // Per-frame input state maps.

    //Raw input state
    HashMap<InputVariant, InputState> m_inputStates;
    //Bind input state
    HashMap<BindEntry*, InputState> m_bindStates;
  };
}

#endif