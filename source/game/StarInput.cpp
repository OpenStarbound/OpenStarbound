#include "StarInput.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

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

}

Input::~Input() {
  s_singleton = nullptr;
}

void Input::reset() {

}

void Input::reload() {
  reset();
  
  auto assets = Root::singleton().assets();

  for (auto& bindPath : assets->scanExtension("binds")) {
    Json config = assets->json(bindPath);
  }
}

}