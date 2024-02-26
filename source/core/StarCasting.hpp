#pragma once

#include "StarException.hpp"
#include "StarFormat.hpp"

namespace Star {

STAR_EXCEPTION(PointerConvertException, StarException);

template <typename Type1, typename Type2>
bool is(Type2* p) {
  return (bool)dynamic_cast<Type1*>(p);
}

template <typename Type1, typename Type2>
bool is(Type2 const* p) {
  return (bool)dynamic_cast<Type1 const*>(p);
}

template <typename Type1, typename Type2>
bool is(shared_ptr<Type2> const& p) {
  return (bool)dynamic_cast<Type1*>(p.get());
}

template <typename Type1, typename Type2>
bool is(shared_ptr<Type2 const> const& p) {
  return (bool)dynamic_cast<Type1 const*>(p.get());
}

template <typename Type1, typename Type2>
bool ris(Type2& r) {
  return (bool)dynamic_cast<Type1*>(&r);
}

template <typename Type1, typename Type2>
bool ris(Type2 const& r) {
  return (bool)dynamic_cast<Type1 const*>(&r);
}

template <typename Type1, typename Type2>
Type1* as(Type2* p) {
  return dynamic_cast<Type1*>(p);
}

template <typename Type1, typename Type2>
Type1 const* as(Type2 const* p) {
  return dynamic_cast<Type1 const*>(p);
}

template <typename Type1, typename Type2>
shared_ptr<Type1> as(shared_ptr<Type2> const& p) {
  return dynamic_pointer_cast<Type1>(p);
}

template <typename Type1, typename Type2>
shared_ptr<Type1 const> as(shared_ptr<Type2 const> const& p) {
  return dynamic_pointer_cast<Type1 const>(p);
}

template <typename Type, typename Ptr>
auto convert(Ptr const& p) -> decltype(as<Type>(p)) {
  if (!p)
    throw PointerConvertException::format("Could not convert from nullptr to {}", typeid(Type).name());
  else if (auto a = as<Type>(p))
    return a;
  else
    throw PointerConvertException::format("Could not convert from {} to {}", typeid(*p).name(), typeid(Type).name());
}

template <typename Type1, typename Type2>
Type1& rconvert(Type2& r) {
  return *dynamic_cast<Type1*>(&r);
}

template <typename Type1, typename Type2>
Type1 const& rconvert(Type2 const& r) {
  return *dynamic_cast<Type1 const*>(&r);
}

template <typename Type>
weak_ptr<Type> asWeak(shared_ptr<Type> const& p) {
  return weak_ptr<Type>(p);
}

template <typename Type>
weak_ptr<Type const> asWeak(shared_ptr<Type const> const& p) {
  return weak_ptr<Type>(p);
}

}
