#ifndef STAR_CONFIG_HPP
#define STAR_CONFIG_HPP

#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <tuple>
#include <memory>
#include <functional>
#include <algorithm>
#include <iostream>
#include <initializer_list>
#include <exception>
#include <stdexcept>
#include <atomic>
#include <string>
#include <iterator>

namespace Star {

// Some really common std namespace includes

using std::size_t;

using std::swap;
using std::move;

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;
using std::make_unique;
using std::static_pointer_cast;
using std::dynamic_pointer_cast;
using std::const_pointer_cast;
using std::enable_shared_from_this;

using std::pair;
using std::make_pair;

using std::tuple;
using std::make_tuple;
using std::tuple_element;
using std::get;
using std::tie;
using std::ignore;

using std::initializer_list;

using std::min;
using std::max;

using std::bind;
using std::function;
using std::forward;
using std::mem_fn;
using std::ref;
using std::cref;
using namespace std::placeholders;

using std::prev;
// using std::next;

using std::atomic;
using std::atomic_flag;
using std::atomic_load;
using std::atomic_store;

#ifndef NDEBUG
#define STAR_DEBUG 1
constexpr bool DebugEnabled = true;
#else
constexpr bool DebugEnabled = false;
#endif

// A version of string::npos that's used in general to mean "not a position"
// and is the largest value for size_t.
size_t const NPos = (size_t)(-1);

typedef int64_t StreamOffset;

// Convenient way to purposefully mark a variable as unused to avoid warning
#define _unused(x) ((void)x)

// Forward declare a class or struct, and define a lot of typedefs for
// different pointer types all at once.

#define STAR_CLASS(ClassName)                                     \
  class ClassName;                                                \
  typedef std::shared_ptr<ClassName> ClassName##Ptr;              \
  typedef std::shared_ptr<const ClassName> ClassName##ConstPtr;   \
  typedef std::weak_ptr<ClassName> ClassName##WeakPtr;            \
  typedef std::weak_ptr<const ClassName> ClassName##ConstWeakPtr; \
  typedef std::unique_ptr<ClassName> ClassName##UPtr;             \
  typedef std::unique_ptr<const ClassName> ClassName##ConstUPtr

#define STAR_STRUCT(StructName)                                     \
  struct StructName;                                                \
  typedef std::shared_ptr<StructName> StructName##Ptr;              \
  typedef std::shared_ptr<const StructName> StructName##ConstPtr;   \
  typedef std::weak_ptr<StructName> StructName##WeakPtr;            \
  typedef std::weak_ptr<const StructName> StructName##ConstWeakPtr; \
  typedef std::unique_ptr<StructName> StructName##UPtr;             \
  typedef std::unique_ptr<const StructName> StructName##ConstUPtr

#define STAR_QUOTE(name) #name
#define STAR_STR(macro) STAR_QUOTE(macro)

}

#endif
