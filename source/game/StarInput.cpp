#include "StarInput.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

const KeyMod KeyModOptional = KeyMod::Num | KeyMod::Caps | KeyMod::Scroll;

inline bool compareKeyModLenient(KeyMod input, KeyMod test) {
	input |= KeyModOptional;
	test |= KeyModOptional;
	return (test & input) == test;
}

inline bool compareKeyMod(KeyMod input, KeyMod test) {
	return (input | (KeyModOptional & ~test)) == (test | KeyModOptional);
}

Json keyModsToJson(KeyMod mod) {
  JsonArray array;
  
  if ((bool)(mod & KeyMod::LShift)) array.emplace_back("LShift");
  if ((bool)(mod & KeyMod::RShift)) array.emplace_back("RShift");
  if ((bool)(mod & KeyMod::LCtrl )) array.emplace_back("LCtrl" );
  if ((bool)(mod & KeyMod::RCtrl )) array.emplace_back("RCtrl" );
  if ((bool)(mod & KeyMod::LAlt  )) array.emplace_back("LAlt"  );
  if ((bool)(mod & KeyMod::RAlt  )) array.emplace_back("RAlt"  );
  if ((bool)(mod & KeyMod::LGui  )) array.emplace_back("LGui"  );
  if ((bool)(mod & KeyMod::RGui  )) array.emplace_back("RGui"  );
  if ((bool)(mod & KeyMod::Num   )) array.emplace_back("Num"   );
  if ((bool)(mod & KeyMod::Caps  )) array.emplace_back("Caps"  );
  if ((bool)(mod & KeyMod::AltGr )) array.emplace_back("AltGr" );
  if ((bool)(mod & KeyMod::Scroll)) array.emplace_back("Scroll");

  return move(array);
}

// Optional pointer argument to output calculated priority
KeyMod keyModsFromJson(Json const& json, uint8_t* priority = nullptr) {
  KeyMod mod = KeyMod::NoMod;
  if (!json.isType(Json::Type::Array))
    return mod;

  for (Json const& jMod : json.toArray()) {
    if (mod != (mod |= KeyModNames.getLeft(jMod.toString())) && priority)
      ++*priority;
  }

  return mod;
}

size_t hash<InputVariant>::operator()(InputVariant const& v) const {
  size_t indexHash = hashOf(v.typeIndex());
  if (auto key = v.ptr<Key>())
    hashCombine(indexHash, hashOf(*key));
  else if (auto mButton = v.ptr<MouseButton>())
    hashCombine(indexHash, hashOf(*mButton));
  else if (auto cButton = v.ptr<ControllerButton>())
    hashCombine(indexHash, hashOf(*cButton));

  return indexHash;
}

Json Input::inputEventToJson(InputEvent const& input) {
  String type;
  Json data;

  if (auto keyDown = input.ptr<KeyDownEvent>()) {
    type = "KeyDown";
    data = JsonObject{
      {"key", KeyNames.getRight(keyDown->key)},
      {"mods", keyModsToJson(keyDown->mods)}
    };
  } else if (auto keyUp = input.ptr<KeyUpEvent>()) {
    type = "KeyUp";
    data = JsonObject{
      {"key", KeyNames.getRight(keyUp->key)}
    };
  } else if (auto mouseDown = input.ptr<MouseButtonDownEvent>()) {
    type = "MouseButtonDown";
    data = JsonObject{
      {"mouseButton", MouseButtonNames.getRight(mouseDown->mouseButton)},
      {"mousePosition", jsonFromVec2I(mouseDown->mousePosition)}
    };
  } else if (auto mouseUp = input.ptr<MouseButtonUpEvent>()) {
    type = "MouseButtonUp";
    data = JsonObject{
      {"mouseButton", MouseButtonNames.getRight(mouseUp->mouseButton)},
      {"mousePosition", jsonFromVec2I(mouseUp->mousePosition)}
    };
  } else if (auto mouseWheel = input.ptr<MouseWheelEvent>()) {
    type = "MouseWheel";
    data = JsonObject{
      {"mouseWheel", MouseWheelNames.getRight(mouseWheel->mouseWheel)},
      {"mousePosition", jsonFromVec2I(mouseWheel->mousePosition)}
    };
  } else if (auto mouseMove = input.ptr<MouseMoveEvent>()) {
    type = "MouseMove";
    data = JsonObject{
      {"mouseMove", jsonFromVec2I(mouseMove->mouseMove)},
      {"mousePosition", jsonFromVec2I(mouseMove->mousePosition)}
    };
  }

  if (data) {
    return JsonObject{
      {"type", type},
      {"data", data}
    };
  }

  return data;
}

Input::Bind Input::bindFromJson(Json const& json) {
  Bind bind;

  String type = json.getString("type");
  Json value = json.get("value", {});

  if (type == "key") {
    KeyBind keyBind;
    keyBind.key = KeyNames.getLeft(value.toString());
    keyBind.mods = keyModsFromJson(json.getArray("mods", {}), &keyBind.priority);
    bind = move(keyBind);
  }
  else if (type == "mouse") {
    MouseBind mouseBind;
    mouseBind.button = MouseButtonNames.getLeft(value.toString());
    mouseBind.mods = keyModsFromJson(json.getArray("mods", {}), &mouseBind.priority);
    bind = move(mouseBind);
  }
  else if (type == "controller") {
    ControllerBind controllerBind;
    controllerBind.button = ControllerButtonNames.getLeft(value.toString());
    controllerBind.controller = json.getUInt("controller", 0);
    bind = move(controllerBind);
  }

  return bind;
}

Json Input::bindToJson(Bind const& bind) {
  if (auto keyBind = bind.ptr<KeyBind>()) {
    return JsonObject{
      {"type", "key"},
      {"value", KeyNames.getRight(keyBind->key)},
      {"mods", keyModsToJson(keyBind->mods)}
    };
  }
  else if (auto mouseBind = bind.ptr<MouseBind>()) {
    return JsonObject{
      {"type", "mouse"},
      {"value", MouseButtonNames.getRight(mouseBind->button)},
      {"mods", keyModsToJson(mouseBind->mods)}
    };
  }
  else if (auto controllerBind = bind.ptr<ControllerBind>()) {
    return JsonObject{
      {"type", "controller"},
      {"value", ControllerButtonNames.getRight(controllerBind->button)}
    };
  }

  return Json();
}

Input::BindEntry::BindEntry(String entryId, Json const& config, BindCategory const& parentCategory) {
  category = &parentCategory;
  id = move(entryId);
  name = config.getString("name", id);

  for (Json const& jBind : config.getArray("default", {})) {
    try
      { defaultBinds.emplace_back(bindFromJson(jBind)); }
    catch (JsonException const& e)
      { Logger::error("Binds: Error loading default bind in {}.{}: {}", parentCategory.id, id, e.what()); }
  }
}

Input::BindCategory::BindCategory(String categoryId, Json const& categoryConfig) {
  id = move(categoryId);
  config = categoryConfig;
  name = config.getString("name", id);

  ConfigurationPtr userConfig = Root::singletonPtr()->configuration();
  auto userBindings = userConfig->get("modBindings");

  for (auto& pair : config.getObject("binds", {})) {
    String const& bindId = pair.first;
    Json const& bindConfig = pair.second;
    if (!bindConfig.isType(Json::Type::Object))
      continue;

    BindEntry& entry = entries.insert(bindId, BindEntry(bindId, bindConfig, *this)).first->second;

    if (userBindings.isType(Json::Type::Object)) {
      for (auto& jBind : userBindings.queryArray(strf("{}.{}", id, bindId), {})) {
        try
          { entry.customBinds.emplace_back(bindFromJson(jBind)); }
        catch (JsonException const& e)
          { Logger::error("Binds: Error loading user bind in {}.{}: {}", id, bindId, e.what()); }
      }
    }
  }
}

List<Input::BindEntry*> Input::filterBindEntries(List<Input::BindRef> const& binds, KeyMod mods) const {
  uint8_t maxPriority = 0;
  List<BindEntry*> result{};
  for (const BindRef& bind : binds) {
    if (bind.priority < maxPriority)
      break;
    else if (compareKeyModLenient(mods, bind.mods)) {
      maxPriority = bind.priority;
      result.emplace_back(bind.entry);
    }
  }
  return result;
}

Input* Input::s_singleton;

Input* Input::singletonPtr() {
  return s_singleton;
}

Input& Input::singleton() {
  if (!s_singleton)
    throw InputException("Input::singleton() called with no Input instance available");
  else
    return *s_singleton;
}

Input::Input() {
  if (s_singleton)
    throw InputException("Singleton Input has been constructed twice");

  s_singleton = this;

  reload();

  m_rootReloadListener = make_shared<CallbackListener>([&]() {
    reload();
  });

  Root::singletonPtr()->registerReloadListener(m_rootReloadListener);
}

Input::~Input() {
  s_singleton = nullptr;
}

List<std::pair<InputEvent, bool>> const& Input::inputEventsThisFrame() const {
  return m_inputEvents;
}

void Input::reset() {
  m_inputEvents.resize(0); // keeps reserved memory
  m_inputStates.clear();
  m_bindStates.clear();
}

void Input::update() {
  reset();
}

bool Input::handleInput(InputEvent const& input, bool gameProcessed) {
  m_inputEvents.emplace_back(input, gameProcessed);


  return false;
}

void Input::rebuildMappings() {
  m_bindMappings.clear();
}

void Input::reload() {
  reset();
  m_bindCategories.clear();

  auto assets = Root::singleton().assets();

  for (auto& bindPath : assets->scanExtension("binds")) {
    for (auto& pair : assets->json(bindPath).toObject()) {
      String const& categoryId = pair.first;
      Json const& categoryConfig = pair.second;
      if (!categoryConfig.isType(Json::Type::Object))
        continue;

      m_bindCategories.insert(categoryId, BindCategory(categoryId, categoryConfig));
    }
  }

  size_t count = 0;
  for (auto& pair : m_bindCategories)
    count += pair.second.entries.size();

  Logger::info("Binds: Loaded {} bind{}", count, count == 1 ? "" : "s");

  rebuildMappings();
}

}