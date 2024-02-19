#ifndef STAR_VARIANT_HPP
#define STAR_VARIANT_HPP

#include <type_traits>
#include <utility>
#include <initializer_list>

#include "StarAlgorithm.hpp"
#include "StarMaybe.hpp"

namespace Star {

STAR_EXCEPTION(BadVariantCast, StarException);
STAR_EXCEPTION(BadVariantType, StarException);

typedef uint8_t VariantTypeIndex;
VariantTypeIndex const InvalidVariantType = 255;

namespace detail {
  template <typename T, typename... Args>
  struct HasType;

  template <typename T>
  struct HasType<T> : std::false_type {};

  template <typename T, typename Head, typename... Args>
  struct HasType<T, Head, Args...> {
    static constexpr bool value = std::is_same<T, Head>::value || HasType<T, Args...>::value;
  };

  template <typename... Args>
  struct IsNothrowMoveConstructible;

  template <>
  struct IsNothrowMoveConstructible<> : std::true_type {};

  template <typename Head, typename... Args>
  struct IsNothrowMoveConstructible<Head, Args...> {
    static constexpr bool value = std::is_nothrow_move_constructible<Head>::value && IsNothrowMoveConstructible<Args...>::value;
  };

  template <typename... Args>
  struct IsNothrowMoveAssignable;

  template <>
  struct IsNothrowMoveAssignable<> : std::true_type {};

  template <typename Head, typename... Args>
  struct IsNothrowMoveAssignable<Head, Args...> {
    static constexpr bool value = std::is_nothrow_move_assignable<Head>::value && IsNothrowMoveAssignable<Args...>::value;
  };
}

// Stack based variant type container that can be inhabited by one of a limited
// number of types.
template <typename FirstType, typename... RestTypes>
class Variant {
public:
  template <typename T>
  using ValidateType = typename std::enable_if<detail::HasType<T, FirstType, RestTypes...>::value, void>::type;

  template <typename T, typename = ValidateType<T>>
  static constexpr VariantTypeIndex typeIndexOf();

  // If the first type has a default constructor, constructs an Variant which
  // contains a default constructed value of that type.
  Variant();

  template <typename T, typename = ValidateType<T>>
  Variant(T const& x);
  template <typename T, typename = ValidateType<T>>
  Variant(T&& x);

  template <typename T, typename = ValidateType<T>, typename... Args,
    typename std::enable_if< std::is_constructible<T, Args...>::value, int >::type = 0
  >
  Variant(std::in_place_type_t<T>, Args&&... args) {
    new (&m_buffer) T(std::forward<Args>(args)...);
    m_typeIndex = TypeIndex<T>::value;
  }

  template <typename T, typename U, typename = ValidateType<T>, typename... Args,
    typename std::enable_if< std::is_constructible<T, std::initializer_list<U>&, Args...>::value, int >::type = 0
  >
  Variant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) {
    new (&m_buffer) T(il, std::forward<Args>(args)...);
    m_typeIndex = TypeIndex<T>::value;
  }

  Variant(Variant const& x);
  Variant(Variant&& x) noexcept(detail::IsNothrowMoveConstructible<FirstType, RestTypes...>::value);

  ~Variant();

  // Implementations of operator= may invalidate the Variant if the copy or
  // move constructor of the assigned value throws.
  Variant& operator=(Variant const& x);
  Variant& operator=(Variant&& x) noexcept(detail::IsNothrowMoveAssignable<FirstType, RestTypes...>::value);
  template <typename T, typename = ValidateType<T>>
  Variant& operator=(T const& x);
  template <typename T, typename = ValidateType<T>>
  Variant& operator=(T&& x);

  // Returns true if this Variant contains the given type.
  template <typename T, typename = ValidateType<T>>
  bool is() const;

  // get throws BadVariantCast on bad casts

  template <typename T, typename = ValidateType<T>>
  T const& get() const;

  template <typename T, typename = ValidateType<T>>
  T& get();

  template <typename T, typename = ValidateType<T>>
  Maybe<T> maybe() const;

  // ptr() does not throw if this Variant does not hold the given type, instead
  // simply returns nullptr.

  template <typename T, typename = ValidateType<T>>
  T const* ptr() const;

  template <typename T, typename = ValidateType<T>>
  T* ptr();

  // Calls the given function with the type currently being held, and returns
  // the value returned by that function.  Will throw if this Variant has been
  // invalidated.
  template <typename Function>
  decltype(auto) call(Function&& function);
  template <typename Function>
  decltype(auto) call(Function&& function) const;

  // Returns an index for the held type, which can be passed into makeType to
  // make this Variant hold a specific type.  Returns InvalidVariantType if
  // invalidated.
  VariantTypeIndex typeIndex() const;

  // Make this Variant hold a new default constructed type of the given type
  // index.  Can only be used if every alternative type has a default
  // constructor.  Throws if given an out of range type index or
  // InvalidVariantType.
  void makeType(VariantTypeIndex typeIndex);

  // True if this Variant has been invalidated.  If the copy or move
  // constructor of a type throws an exception during assignment, there is no
  // *good* way to ensure that the Variant has a valid type, so it may become
  // invalidated.  It is not possible to directly construct an invalidated
  // Variant.
  bool invalid() const;

  // Requires that every type included in this Variant has operator==
  bool operator==(Variant const& x) const;
  bool operator!=(Variant const& x) const;

  // Requires that every type included in this Variant has operator<
  bool operator<(Variant const& x) const;

  template <typename T, typename = ValidateType<T>>
  bool operator==(T const& x) const;
  template <typename T, typename = ValidateType<T>>
  bool operator!=(T const& x) const;
  template <typename T, typename = ValidateType<T>>
  bool operator<(T const& x) const;

private:
  template <typename MatchType, VariantTypeIndex Index, typename... Rest>
  struct LookupTypeIndex;

  template <typename MatchType, VariantTypeIndex Index>
  struct LookupTypeIndex<MatchType, Index> {
    static VariantTypeIndex const value = InvalidVariantType;
  };

  template <typename MatchType, VariantTypeIndex Index, typename Head, typename... Rest>
  struct LookupTypeIndex<MatchType, Index, Head, Rest...> {
    static VariantTypeIndex const value = std::is_same<MatchType, Head>::value ? Index : LookupTypeIndex<MatchType, Index + 1, Rest...>::value;
  };

  template <typename MatchType>
  struct TypeIndex {
    static VariantTypeIndex const value = LookupTypeIndex<MatchType, 0, FirstType, RestTypes...>::value;
  };

  void destruct();

  template <typename T>
  void assign(T&& x);

  template <typename Function, typename T>
  decltype(auto) doCall(Function&& function);
  template <typename Function, typename T1, typename T2, typename... TL>
  decltype(auto) doCall(Function&& function);

  template <typename Function, typename T>
  decltype(auto) doCall(Function&& function) const;
  template <typename Function, typename T1, typename T2, typename... TL>
  decltype(auto) doCall(Function&& function) const;

  template <typename First>
  void doMakeType(VariantTypeIndex);
  template <typename First, typename Second, typename... Rest>
  void doMakeType(VariantTypeIndex typeIndex);

  typename std::aligned_union<0, FirstType, RestTypes...>::type m_buffer;
  VariantTypeIndex m_typeIndex = InvalidVariantType;
};

// A version of Variant that has always has a default "empty" state, useful
// when there is no good default type for a Variant but it needs to be default
// constructed, and is slightly more convenient than Maybe<Variant<Types...>>.
template <typename... Types>
class MVariant {
public:
  template <typename T>
  using ValidateType = typename std::enable_if<detail::HasType<T, Types...>::value, void>::type;

  template <typename T, typename = ValidateType<T>>
  static constexpr VariantTypeIndex typeIndexOf();

  MVariant();
  MVariant(MVariant const& x);
  MVariant(MVariant&& x);

  template <typename T, typename = ValidateType<T>>
  MVariant(T const& x);
  template <typename T, typename = ValidateType<T>>
  MVariant(T&& x);

  MVariant(Variant<Types...> const& x);
  MVariant(Variant<Types...>&& x);

  ~MVariant();

  // MVariant::operator= will never invalidate the MVariant, instead it will
  // just become empty.
  MVariant& operator=(MVariant const& x);
  MVariant& operator=(MVariant&& x);

  template <typename T, typename = ValidateType<T>>
  MVariant& operator=(T const& x);
  template <typename T, typename = ValidateType<T>>
  MVariant& operator=(T&& x);

  MVariant& operator=(Variant<Types...> const& x);
  MVariant& operator=(Variant<Types...>&& x);

  // Requires that every type included in this MVariant has operator==
  bool operator==(MVariant const& x) const;
  bool operator!=(MVariant const& x) const;

  // Requires that every type included in this MVariant has operator<
  bool operator<(MVariant const& x) const;

  template <typename T, typename = ValidateType<T>>
  bool operator==(T const& x) const;
  template <typename T, typename = ValidateType<T>>
  bool operator!=(T const& x) const;
  template <typename T, typename = ValidateType<T>>
  bool operator<(T const& x) const;

  // get throws BadVariantCast on bad casts

  template <typename T, typename = ValidateType<T>>
  T const& get() const;

  template <typename T, typename = ValidateType<T>>
  T& get();

  // maybe() and ptr() do not throw if this MVariant does not hold the given
  // type, instead simply returns Nothing / nullptr.

  template <typename T, typename = ValidateType<T>>
  Maybe<T> maybe() const;

  template <typename T, typename = ValidateType<T>>
  T const* ptr() const;

  template <typename T, typename = ValidateType<T>>
  T* ptr();

  template <typename T, typename = ValidateType<T>>
  bool is() const;

  // Takes the given value out and leaves this empty
  template <typename T, typename = ValidateType<T>>
  T take();

  // Returns a Variant of all the allowed types if non-empty, throws
  // BadVariantCast if empty.
  Variant<Types...> value() const;

  // Moves the contents of this MVariant into the given Variant if non-empty,
  // throws BadVariantCast if empty.
  Variant<Types...> takeValue();

  bool empty() const;
  void reset();

  // Equivalent to !empty()
  explicit operator bool() const;

  // If this MVariant holds a type, calls the given function with the type
  // being held.  If nothing is currently held, the function is not called.
  template <typename Function>
  void call(Function&& function);

  template <typename Function>
  void call(Function&& function) const;

  // Returns an index for the held type, which can be passed into makeType to
  // make this MVariant hold a specific type.  Types are always indexed in the
  // order they are specified starting from 1.  A type index of 0 indicates an
  // empty MVariant.
  VariantTypeIndex typeIndex() const;

  // Make this MVariant hold a new default constructed type of the given type
  // index.  Can only be used if every alternative type has a default
  // constructor.
  void makeType(VariantTypeIndex typeIndex);

private:
  struct MVariantEmpty {
    bool operator==(MVariantEmpty const& rhs) const;
    bool operator<(MVariantEmpty const& rhs) const;
  };

  template <typename Function>
  struct RefCaller {
    Function&& function;

    RefCaller(Function&& function);

    void operator()(MVariantEmpty& empty);

    template <typename T>
    void operator()(T& t);
  };

  template <typename Function>
  struct ConstRefCaller {
    Function&& function;

    ConstRefCaller(Function&& function);

    void operator()(MVariantEmpty const& empty);

    template <typename T>
    void operator()(T const& t);
  };

  Variant<MVariantEmpty, Types...> m_variant;
};

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
constexpr VariantTypeIndex Variant<FirstType, RestTypes...>::typeIndexOf() {
  return TypeIndex<T>::value;
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>::Variant()
  : Variant(FirstType()) {}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
Variant<FirstType, RestTypes...>::Variant(T const& x) {
  assign(x);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
Variant<FirstType, RestTypes...>::Variant(T&& x) {
  assign(std::forward<T>(x));
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>::Variant(Variant const& x) {
  x.call([&](auto const& t) {
      assign(t);
    });
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>::Variant(Variant&& x)
  noexcept(detail::IsNothrowMoveConstructible<FirstType, RestTypes...>::value) {
  x.call([&](auto& t) {
      assign(std::move(t));
    });
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>::~Variant() {
  destruct();
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>& Variant<FirstType, RestTypes...>::operator=(Variant const& x) {
  if (&x == this)
    return *this;

  x.call([&](auto const& t) {
      assign(t);
    });

  return *this;
}

template <typename FirstType, typename... RestTypes>
Variant<FirstType, RestTypes...>& Variant<FirstType, RestTypes...>::operator=(Variant&& x)
  noexcept(detail::IsNothrowMoveAssignable<FirstType, RestTypes...>::value) {
  if (&x == this)
    return *this;

  x.call([&](auto& t) {
      assign(std::move(t));
    });

  return *this;
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
Variant<FirstType, RestTypes...>& Variant<FirstType, RestTypes...>::operator=(T const& x) {
  assign(x);
  return *this;
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
Variant<FirstType, RestTypes...>& Variant<FirstType, RestTypes...>::operator=(T&& x) {
  assign(std::forward<T>(x));
  return *this;
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
T const& Variant<FirstType, RestTypes...>::get() const {
  if (!is<T>())
    throw BadVariantCast();
  return *(T*)(&m_buffer);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
T& Variant<FirstType, RestTypes...>::get() {
  if (!is<T>())
    throw BadVariantCast();
  return *(T*)(&m_buffer);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
Maybe<T> Variant<FirstType, RestTypes...>::maybe() const {
  if (!is<T>())
    return {};
  return *(T*)(&m_buffer);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
T const* Variant<FirstType, RestTypes...>::ptr() const {
  if (!is<T>())
    return nullptr;
  return (T*)(&m_buffer);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
T* Variant<FirstType, RestTypes...>::ptr() {
  if (!is<T>())
    return nullptr;
  return (T*)(&m_buffer);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
bool Variant<FirstType, RestTypes...>::is() const {
  return m_typeIndex == TypeIndex<T>::value;
}

template <typename FirstType, typename... RestTypes>
template <typename Function>
decltype(auto) Variant<FirstType, RestTypes...>::call(Function&& function) {
  return doCall<Function, FirstType, RestTypes...>(std::forward<Function>(function));
}

template <typename FirstType, typename... RestTypes>
template <typename Function>
decltype(auto) Variant<FirstType, RestTypes...>::call(Function&& function) const {
  return doCall<Function, FirstType, RestTypes...>(std::forward<Function>(function));
}

template <typename FirstType, typename... RestTypes>
VariantTypeIndex Variant<FirstType, RestTypes...>::typeIndex() const {
  return m_typeIndex;
}

template <typename FirstType, typename... RestTypes>
void Variant<FirstType, RestTypes...>::makeType(VariantTypeIndex typeIndex) {
  return doMakeType<FirstType, RestTypes...>(typeIndex);
}

template <typename FirstType, typename... RestTypes>
bool Variant<FirstType, RestTypes...>::invalid() const {
  return m_typeIndex == InvalidVariantType;
}

template <typename FirstType, typename... RestTypes>
bool Variant<FirstType, RestTypes...>::operator==(Variant const& x) const {
  if (this == &x) {
    return true;
  } else if (typeIndex() != x.typeIndex()) {
    return false;
  } else {
    return call([&x](auto const& t) {
        typedef typename std::decay<decltype(t)>::type T;
        return t == x.template get<T>();
      });
  }
}

template <typename FirstType, typename... RestTypes>
bool Variant<FirstType, RestTypes...>::operator!=(Variant const& x) const {
  return !operator==(x);
}

template <typename FirstType, typename... RestTypes>
bool Variant<FirstType, RestTypes...>::operator<(Variant const& x) const {
  if (this == &x) {
    return false;
  } else {
    auto sti = typeIndex();
    auto xti = x.typeIndex();
    if (sti != xti) {
      return sti < xti;
    } else {
      return call([&x](auto const& t) {
          typedef typename std::decay<decltype(t)>::type T;
          return t < x.template get<T>();
        });
    }
  }
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
bool Variant<FirstType, RestTypes...>::operator==(T const& x) const {
  if (auto p = ptr<T>())
    return *p == x;
  return false;
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
bool Variant<FirstType, RestTypes...>::operator!=(T const& x) const {
  return !operator==(x);
}

template <typename FirstType, typename... RestTypes>
template <typename T, typename>
bool Variant<FirstType, RestTypes...>::operator<(T const& x) const {
  if (auto p = ptr<T>())
    return *p == x;
  return m_typeIndex < TypeIndex<T>::value;
}

template <typename FirstType, typename... RestTypes>
void Variant<FirstType, RestTypes...>::destruct() {
  if (m_typeIndex != InvalidVariantType) {
    try {
      call([](auto& t) {
          typedef typename std::decay<decltype(t)>::type T;
          t.~T();
        });
      m_typeIndex = InvalidVariantType;
    } catch (...) {
      m_typeIndex = InvalidVariantType;
      throw;
    }
  }
}

template <typename FirstType, typename... RestTypes>
template <typename T>
void Variant<FirstType, RestTypes...>::assign(T&& x) {
  typedef typename std::decay<T>::type AssignType;
  if (auto p = ptr<AssignType>()) {
    *p = std::forward<T>(x);
  } else {
    destruct();
    new (&m_buffer) AssignType(std::forward<T>(x));
    m_typeIndex = TypeIndex<AssignType>::value;
  }
}

template <typename FirstType, typename... RestTypes>
template <typename Function, typename T>
decltype(auto) Variant<FirstType, RestTypes...>::doCall(Function&& function) {
  if (T* p = ptr<T>())
    return function(*p);
  else
    throw BadVariantType();
}

template <typename FirstType, typename... RestTypes>
template <typename Function, typename T1, typename T2, typename... TL>
decltype(auto) Variant<FirstType, RestTypes...>::doCall(Function&& function) {
  if (T1* p = ptr<T1>())
    return function(*p);
  else
    return doCall<Function, T2, TL...>(std::forward<Function>(function));
}

template <typename FirstType, typename... RestTypes>
template <typename Function, typename T>
decltype(auto) Variant<FirstType, RestTypes...>::doCall(Function&& function) const {
  if (T const* p = ptr<T>())
    return function(*p);
  else
    throw BadVariantType();
}

template <typename FirstType, typename... RestTypes>
template <typename Function, typename T1, typename T2, typename... TL>
decltype(auto) Variant<FirstType, RestTypes...>::doCall(Function&& function) const {
  if (T1 const* p = ptr<T1>())
    return function(*p);
  else
    return doCall<Function, T2, TL...>(std::forward<Function>(function));
}

template <typename FirstType, typename... RestTypes>
template <typename First>
void Variant<FirstType, RestTypes...>::doMakeType(VariantTypeIndex typeIndex) {
  if (typeIndex == 0)
    *this = First();
  else
    throw BadVariantType();
}

template <typename FirstType, typename... RestTypes>
template <typename First, typename Second, typename... Rest>
void Variant<FirstType, RestTypes...>::doMakeType(VariantTypeIndex typeIndex) {
  if (typeIndex == 0)
    *this = First();
  else
    return doMakeType<Second, Rest...>(typeIndex - 1);
}

template <typename... Types>
template <typename T, typename>
constexpr VariantTypeIndex MVariant<Types...>::typeIndexOf() {
  return Variant<MVariantEmpty, Types...>::template typeIndexOf<T>();
}

template <typename... Types>
MVariant<Types...>::MVariant() {}

template <typename... Types>
MVariant<Types...>::MVariant(MVariant const& x)
  : m_variant(x.m_variant) {}

template <typename... Types>
MVariant<Types...>::MVariant(MVariant&& x) {
  m_variant = std::move(x.m_variant);
  x.m_variant = MVariantEmpty();
}

template <typename... Types>
MVariant<Types...>::MVariant(Variant<Types...> const& x) {
  operator=(x);
}

template <typename... Types>
MVariant<Types...>::MVariant(Variant<Types...>&& x) {
  operator=(std::move(x));
}

template <typename... Types>
template <typename T, typename>
MVariant<Types...>::MVariant(T const& x)
  : m_variant(x) {}

template <typename... Types>
template <typename T, typename>
MVariant<Types...>::MVariant(T&& x)
  : m_variant(std::forward<T>(x)) {}

template <typename... Types>
MVariant<Types...>::~MVariant() {}

template <typename... Types>
MVariant<Types...>& MVariant<Types...>::operator=(MVariant const& x) {
  try {
    m_variant = x.m_variant;
  } catch (...) {
    if (m_variant.invalid())
      m_variant = MVariantEmpty();
    throw;
  }
  return *this;
}

template <typename... Types>
MVariant<Types...>& MVariant<Types...>::operator=(MVariant&& x) {
  try {
    m_variant = std::move(x.m_variant);
  } catch (...) {
    if (m_variant.invalid())
      m_variant = MVariantEmpty();
    throw;
  }
  return *this;
}

template <typename... Types>
template <typename T, typename>
MVariant<Types...>& MVariant<Types...>::operator=(T const& x) {
  try {
    m_variant = x;
  } catch (...) {
    if (m_variant.invalid())
      m_variant = MVariantEmpty();
    throw;
  }
  return *this;
}

template <typename... Types>
template <typename T, typename>
MVariant<Types...>& MVariant<Types...>::operator=(T&& x) {
  try {
    m_variant = std::forward<T>(x);
  } catch (...) {
    if (m_variant.invalid())
      m_variant = MVariantEmpty();
    throw;
  }
  return *this;
}

template <typename... Types>
MVariant<Types...>& MVariant<Types...>::operator=(Variant<Types...> const& x) {
  x.call([this](auto const& t) {
      *this = t;
    });
  return *this;
}

template <typename... Types>
MVariant<Types...>& MVariant<Types...>::operator=(Variant<Types...>&& x) {
  x.call([this](auto& t) {
      *this = std::move(t);
    });
  return *this;
}

template <typename... Types>
bool MVariant<Types...>::operator==(MVariant const& x) const {
  return m_variant == x.m_variant;
}

template <typename... Types>
bool MVariant<Types...>::operator!=(MVariant const& x) const {
  return m_variant != x.m_variant;
}

template <typename... Types>
bool MVariant<Types...>::operator<(MVariant const& x) const {
  return m_variant < x.m_variant;
}

template <typename... Types>
template <typename T, typename>
bool MVariant<Types...>::operator==(T const& x) const {
  return m_variant == x;
}

template <typename... Types>
template <typename T, typename>
bool MVariant<Types...>::operator!=(T const& x) const {
  return m_variant != x;
}

template <typename... Types>
template <typename T, typename>
bool MVariant<Types...>::operator<(T const& x) const {
  return m_variant < x;
}

template <typename... Types>
template <typename T, typename>
T const& MVariant<Types...>::get() const {
  return m_variant.template get<T>();
}

template <typename... Types>
template <typename T, typename>
T& MVariant<Types...>::get() {
  return m_variant.template get<T>();
}

template <typename... Types>
template <typename T, typename>
Maybe<T> MVariant<Types...>::maybe() const {
  return m_variant.template maybe<T>();
}

template <typename... Types>
template <typename T, typename>
T const* MVariant<Types...>::ptr() const {
  return m_variant.template ptr<T>();
}

template <typename... Types>
template <typename T, typename>
T* MVariant<Types...>::ptr() {
  return m_variant.template ptr<T>();
}

template <typename... Types>
template <typename T, typename>
bool MVariant<Types...>::is() const {
  return m_variant.template is<T>();
}

template <typename... Types>
template <typename T, typename>
T MVariant<Types...>::take() {
  T t = std::move(m_variant.template get<T>());
  m_variant = MVariantEmpty();
  return t;
}

template <typename... Types>
Variant<Types...> MVariant<Types...>::value() const {
  if (empty())
    throw BadVariantCast();

  Variant<Types...> r;
  call([&r](auto const& v) {
      r = v;
    });
  return r;
}

template <typename... Types>
Variant<Types...> MVariant<Types...>::takeValue() {
  if (empty())
    throw BadVariantCast();

  Variant<Types...> r;
  call([&r](auto& v) {
      r = std::move(v);
    });
  m_variant = MVariantEmpty();
  return r;
}

template <typename... Types>
bool MVariant<Types...>::empty() const {
  return m_variant.template is<MVariantEmpty>();
}

template <typename... Types>
void MVariant<Types...>::reset() {
  m_variant = MVariantEmpty();
}

template <typename... Types>
MVariant<Types...>::operator bool() const {
  return !empty();
}

template <typename... Types>
template <typename Function>
void MVariant<Types...>::call(Function&& function) {
  m_variant.call(RefCaller<Function>(std::forward<Function>(function)));
}

template <typename... Types>
template <typename Function>
void MVariant<Types...>::call(Function&& function) const {
  m_variant.call(ConstRefCaller<Function>(std::forward<Function>(function)));
}

template <typename... Types>
VariantTypeIndex MVariant<Types...>::typeIndex() const {
  return m_variant.typeIndex();
}

template <typename... Types>
void MVariant<Types...>::makeType(VariantTypeIndex typeIndex) {
  m_variant.makeType(typeIndex);
}

template <typename... Types>
bool MVariant<Types...>::MVariantEmpty::operator==(MVariantEmpty const&) const {
  return true;
}

template <typename... Types>
bool MVariant<Types...>::MVariantEmpty::operator<(MVariantEmpty const&) const {
  return false;
}

template <typename... Types>
template <typename Function>
MVariant<Types...>::RefCaller<Function>::RefCaller(Function&& function)
  : function(std::forward<Function>(function)) {}

template <typename... Types>
template <typename Function>
void MVariant<Types...>::RefCaller<Function>::operator()(MVariantEmpty&) {}

template <typename... Types>
template <typename Function>
template <typename T>
void MVariant<Types...>::RefCaller<Function>::operator()(T& t) {
  function(t);
}

template <typename... Types>
template <typename Function>
MVariant<Types...>::ConstRefCaller<Function>::ConstRefCaller(Function&& function)
  : function(std::forward<Function>(function)) {}

template <typename... Types>
template <typename Function>
void MVariant<Types...>::ConstRefCaller<Function>::operator()(MVariantEmpty const&) {}

template <typename... Types>
template <typename Function>
template <typename T>
void MVariant<Types...>::ConstRefCaller<Function>::operator()(T const& t) {
  function(t);
}

}

template <typename FirstType, typename... RestTypes>
struct fmt::formatter<Star::Variant<FirstType, RestTypes...>> : ostream_formatter {};

#endif
