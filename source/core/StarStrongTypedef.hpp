#ifndef STAR_STRONG_TYPEDEF_HPP
#define STAR_STRONG_TYPEDEF_HPP

#include <type_traits>

// Defines a new type that behaves nearly identical to 'parentType', with the
// added benefit that though the new type can be implicitly converted to the
// base type, it must be explicitly converted *from* the base type, and they
// are two distinct types in the type system.
#define strong_typedef(ParentType, NewType)                                                                         \
  template <typename BaseType>                                                                                      \
  struct NewType##Wrapper : BaseType {                                                                              \
    using BaseType::BaseType;                                                                                       \
                                                                                                                    \
    NewType##Wrapper() : BaseType() {}                                                                              \
                                                                                                                    \
    NewType##Wrapper(NewType##Wrapper const& nt) : BaseType(nt) {}                                                  \
                                                                                                                    \
    NewType##Wrapper(NewType##Wrapper&& nt) : BaseType(std::move(nt)) {}                                            \
                                                                                                                    \
    explicit NewType##Wrapper(BaseType const& bt) : BaseType(bt) {}                                                 \
                                                                                                                    \
    explicit NewType##Wrapper(BaseType&& bt) : BaseType(std::move(bt)) {}                                           \
                                                                                                                    \
    NewType##Wrapper& operator=(NewType##Wrapper const& rhs) {                                                      \
      BaseType::operator=(rhs);                                                                                     \
      return *this;                                                                                                 \
    }                                                                                                               \
                                                                                                                    \
    NewType##Wrapper& operator=(NewType##Wrapper&& rhs) {                                                           \
      BaseType::operator=(std::move(rhs));                                                                          \
      return *this;                                                                                                 \
    }                                                                                                               \
                                                                                                                    \
    template <class Arg>                                                                                            \
    NewType##Wrapper& operator=(Arg&& other) {                                                                      \
      static_assert(std::is_base_of<BaseType, typename std::decay<Arg>::type>::value == false                       \
              || std::is_same<NewType##Wrapper, typename std::decay<Arg>::type>::value,                             \
          "" #NewType " can not implicitly be assigned from " #ParentType "-derived classes or strong " #ParentType \
          " typedefs");                                                                                             \
                                                                                                                    \
      BaseType::operator=(std::forward<Arg>(other));                                                                \
      return *this;                                                                                                 \
    }                                                                                                               \
  };                                                                                                                \
  typedef NewType##Wrapper<ParentType> NewType

// Version of strong_typedef for builtin types.
#define strong_typedef_builtin(Type, NewType)   \
  struct NewType {                              \
    Type t;                                     \
                                                \
    explicit NewType(const Type t_)             \
      : t(t_){};                                \
                                                \
    NewType()                                   \
      : t(Type()) {}                            \
                                                \
    NewType(const NewType& t_)                  \
      : t(t_.t) {}                              \
                                                \
    NewType& operator=(const NewType& rhs) {    \
      t = rhs.t;                                \
      return *this;                             \
    }                                           \
                                                \
    NewType& operator=(Type const& rhs) {       \
      t = rhs;                                  \
      return *this;                             \
    }                                           \
                                                \
    operator const Type&() const {              \
      return t;                                 \
    }                                           \
                                                \
    operator Type&() {                          \
      return t;                                 \
    }                                           \
                                                \
    bool operator==(NewType const& rhs) const { \
      return t == rhs.t;                        \
    }                                           \
                                                \
    bool operator!=(NewType const& rhs) const { \
      return t != rhs.t;                        \
    }                                           \
                                                \
    bool operator<(NewType const& rhs) const {  \
      return t < rhs.t;                         \
    }                                           \
                                                \
    bool operator>(NewType const& rhs) const {  \
      return t > rhs.t;                         \
    }                                           \
                                                \
    bool operator<=(NewType const& rhs) const { \
      return t <= rhs.t;                        \
    }                                           \
                                                \
    bool operator>=(NewType const& rhs) const { \
      return t >= rhs.t;                        \
    }                                           \
  }

#endif
