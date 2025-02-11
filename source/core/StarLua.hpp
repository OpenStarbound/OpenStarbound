#pragma once

#include <typeindex>
#include <type_traits>
#ifdef STAR_USE_RAVI
extern "C" {
  #include "ravi/lua.h"
  #include "ravi/lualib.h"
  #include "ravi/lauxlib.h"
}
#else
#include "lua.hpp"
#endif

#include "StarLexicalCast.hpp"
#include "StarString.hpp"
#include "StarJson.hpp"
#include "StarRefPtr.hpp"
#include "StarDirectives.hpp"

namespace Star {

class LuaEngine;
typedef RefPtr<LuaEngine> LuaEnginePtr;

// Basic unspecified lua exception
STAR_EXCEPTION(LuaException, StarException);

// Thrown when trying to parse an incomplete statement, useful for implementing
// REPL loops, uses the incomplete statement marker '<eof>' as the standard lua
// repl does.
STAR_EXCEPTION(LuaIncompleteStatementException, LuaException);

// Thrown when the instruction limit is reached, if the instruction limit is
// set.
STAR_EXCEPTION(LuaInstructionLimitReached, LuaException);

// Thrown when the engine recursion limit is reached, if the recursion limit is
// set.
STAR_EXCEPTION(LuaRecursionLimitReached, LuaException);

// Thrown when an incorrect lua type is passed to something in C++ expecting a
// different type.
STAR_EXCEPTION(LuaConversionException, LuaException);

typedef Empty LuaNilType;
typedef bool LuaBoolean;
typedef lua_Integer LuaInt;
typedef lua_Number LuaFloat;
class LuaString;
class LuaTable;
class LuaFunction;
class LuaThread;
class LuaUserData;
typedef Variant<LuaNilType, LuaBoolean, LuaInt, LuaFloat, LuaString, LuaTable, LuaFunction, LuaThread, LuaUserData> LuaValue;

// Used to wrap multiple return values from calling a lua function or to pass
// multiple values as arguments to a lua function from a container.  If this is
// used as an argument to a lua callback function, it must be the final
// argument of the function!
template <typename T>
class LuaVariadic : public List<T> {
public:
  using List<T>::List;
};

// Unpack a container and apply each of the arguments separately to a lua
// function, similar to lua's unpack.
template <typename Container>
LuaVariadic<typename std::decay<Container>::type::value_type> luaUnpack(Container&& c);

// Similar to LuaVariadic, but a tuple type so automatic per-entry type
// conversion is done.  This can only be used as the return value of a wrapped
// c++ function, or as a type for the return value of calling a lua function.
template <typename... Types>
class LuaTupleReturn : public tuple<Types...> {
public:
  typedef tuple<Types...> Base;

  explicit LuaTupleReturn(Types const&... args);
  template <typename... UTypes>
  explicit LuaTupleReturn(UTypes&&... args);
  template <typename... UTypes>
  explicit LuaTupleReturn(UTypes const&... args);
  LuaTupleReturn(LuaTupleReturn const& rhs);
  LuaTupleReturn(LuaTupleReturn&& rhs);
  template <typename... UTypes>
  LuaTupleReturn(LuaTupleReturn<UTypes...> const& rhs);
  template <typename... UTypes>
  LuaTupleReturn(LuaTupleReturn<UTypes...>&& rhs);

  LuaTupleReturn& operator=(LuaTupleReturn const& rhs);
  LuaTupleReturn& operator=(LuaTupleReturn&& rhs);
  template <typename... UTypes>
  LuaTupleReturn& operator=(LuaTupleReturn<UTypes...> const& rhs);
  template <typename... UTypes>
  LuaTupleReturn& operator=(LuaTupleReturn<UTypes...>&& rhs);
};

// std::tie for LuaTupleReturn
template <typename... Types>
LuaTupleReturn<Types&...> luaTie(Types&... args);

// Constructs a LuaTupleReturn from the given arguments similar to make_tuple
template <typename... Types>
LuaTupleReturn<typename std::decay<Types>::type...> luaTupleReturn(Types&&... args);

namespace LuaDetail {
  struct LuaHandle {
    LuaHandle(LuaEnginePtr engine, int handleIndex);
    ~LuaHandle();

    LuaHandle(LuaHandle const& other);
    LuaHandle(LuaHandle&& other);

    LuaHandle& operator=(LuaHandle const& other);
    LuaHandle& operator=(LuaHandle&& other);

    LuaEnginePtr engine;
    int handleIndex = 0;
  };

  // Not meant to be used directly, exposes a raw interface for wrapped C++
  // functions to be wrapped with the least amount of overhead.  Arguments are
  // passed non-const so that they can be moved into wrapped functions that
  // take values without copying.
  typedef Variant<LuaValue, LuaVariadic<LuaValue>> LuaFunctionReturn;
  typedef function<LuaFunctionReturn(LuaEngine&, size_t argc, LuaValue* argv)> LuaWrappedFunction;
}

// Prints the lua value similar to lua's print function, except it makes an
// attempt at printing tables.
std::ostream& operator<<(std::ostream& os, LuaValue const& value);

// Holds a reference to a LuaEngine and a value held internally inside the
// registry of that engine.  The lifetime of the LuaEngine will be extended
// until all LuaReferences referencing it are destroyed.
class LuaReference {
public:
  LuaReference(LuaDetail::LuaHandle handle);

  LuaReference(LuaReference&&) = default;
  LuaReference& operator=(LuaReference&&) = default;

  LuaReference(LuaReference const&) = default;
  LuaReference& operator=(LuaReference const&) = default;

  bool operator==(LuaReference const& rhs) const;
  bool operator!=(LuaReference const& rhs) const;

  LuaEngine& engine() const;
  int handleIndex() const;

private:
  LuaDetail::LuaHandle m_handle;
};

class LuaString : public LuaReference {
public:
  using LuaReference::LuaReference;

  char const* ptr() const;
  size_t length() const;

  String toString() const;
  StringView view() const;
};

bool operator==(LuaString const& s1, LuaString const& s2);
bool operator==(LuaString const& s1, char const* s2);
bool operator==(LuaString const& s1, std::string const& s2);
bool operator==(LuaString const& s1, String const& s2);
bool operator==(char const* s1, LuaString const& s2);
bool operator==(std::string const& s1, LuaString const& s2);
bool operator==(String const& s1, LuaString const& s2);

bool operator!=(LuaString const& s1, LuaString const& s2);
bool operator!=(LuaString const& s1, char const* s2);
bool operator!=(LuaString const& s1, std::string const& s2);
bool operator!=(LuaString const& s1, String const& s2);
bool operator!=(char const* s1, LuaString const& s2);
bool operator!=(std::string const& s1, LuaString const& s2);
bool operator!=(String const& s1, LuaString const& s2);

class LuaTable : public LuaReference {
public:
  using LuaReference::LuaReference;

  template <typename T = LuaValue, typename K>
  T get(K key) const;
  template <typename T = LuaValue>
  T get(char const* key) const;

  template <typename T, typename K>
  void set(K key, T t) const;
  template <typename T>
  void set(char const* key, T t) const;

  // Shorthand for get(path) != LuaNil
  template <typename K>
  bool contains(K key) const;
  bool contains(char const* key) const;

  // Shorthand for setting to LuaNil
  template <typename K>
  void remove(K key) const;
  void remove(char const* key) const;

  // Result of lua # operator
  LuaInt length() const;

  // If iteration function returns bool, returning false signals stopping.
  template <typename Function>
  void iterate(Function&& iterator) const;

  template <typename Return, typename... Args, typename Function>
  void iterateWithSignature(Function&& func) const;

  Maybe<LuaTable> getMetatable() const;
  void setMetatable(LuaTable const& table) const;

  template <typename T = LuaValue, typename K>
  T rawGet(K key) const;
  template <typename T = LuaValue>
  T rawGet(char const* key) const;

  template <typename T, typename K>
  void rawSet(K key, T t) const;
  template <typename T>
  void rawSet(char const* key, T t) const;

  LuaInt rawLength() const;
};

class LuaFunction : public LuaReference {
public:
  using LuaReference::LuaReference;

  template <typename Ret = LuaValue, typename... Args>
  Ret invoke(Args const&... args) const;
};

class LuaThread : public LuaReference {
public:
  using LuaReference::LuaReference;
  enum class Status {
    Dead,
    Active,
    Error
  };

  // Will return a value if the thread has yielded a value, and nothing if the
  // thread has finished execution
  template <typename Ret = LuaValue, typename... Args>
  Maybe<Ret> resume(Args const&... args) const;
  void pushFunction(LuaFunction const& func) const;
  Status status() const;
};

// Keeping LuaReferences in LuaUserData will lead to circular references to
// LuaEngine, in addition to circular references in Lua which the Lua
// garbage collector can't collect. Don't put LuaReferences in LuaUserData.
class LuaUserData : public LuaReference {
public:
  using LuaReference::LuaReference;

  template <typename T>
  bool is() const;

  template <typename T>
  T& get() const;
};

LuaValue const LuaNil = LuaValue();

class LuaCallbacks {
public:
  void copyCallback(String srcName, String dstName);

  template <typename Function>
  void registerCallback(String name, Function&& func);

  template <typename Return, typename... Args, typename Function>
  void registerCallbackWithSignature(String name, Function&& func);

  LuaCallbacks& merge(LuaCallbacks const& callbacks);

  StringMap<LuaDetail::LuaWrappedFunction> const& callbacks() const;

private:
  StringMap<LuaDetail::LuaWrappedFunction> m_callbacks;
};

template <typename T>
class LuaMethods {
public:
  template <typename Function>
  void registerMethod(String name, Function&& func);

  template <typename Return, typename... Args, typename Function>
  void registerMethodWithSignature(String name, Function&& func);

  StringMap<LuaDetail::LuaWrappedFunction> const& methods() const;

private:
  StringMap<LuaDetail::LuaWrappedFunction> m_methods;
};

// A single execution context from a LuaEngine that manages a (mostly) distinct
// lua environment.  Each LuaContext's global environment is separate and one
// LuaContext can (mostly) not affect any other.
class LuaContext : protected LuaTable {
public:
  typedef function<void(LuaContext&, LuaString const&)> RequireFunction;

  using LuaTable::LuaTable;

  using LuaTable::get;
  using LuaTable::set;
  using LuaTable::contains;
  using LuaTable::remove;
  using LuaTable::engine;
  using LuaTable::handleIndex;

  // Splits the path by '.' character, so can get / set values in tables inside
  // other tables.  If any table in the path is not a table but is accessed as
  // one, instead returns LuaNil.
  template <typename T = LuaValue>
  T getPath(String path) const;
  // Shorthand for getPath != LuaNil
  bool containsPath(String path) const;
  // Will create new tables if the key contains paths that are nil
  template <typename T>
  void setPath(String path, T value);

  // Load the given code (either source or bytecode) into this context as a new
  // chunk.  It is not necessary to provide the name again if given bytecode.
  void load(char const* contents, size_t size, char const* name = nullptr);
  void load(String const& contents, String const& name = String());
  void load(ByteArray const& contents, String const& name = String());

  // Evaluate a piece of lua code in this context, similar to the lua repl.
  // Can evaluate both expressions and statements.
  template <typename T = LuaValue>
  T eval(String const& lua);

  // Override the built-in require function with the given function that takes
  // this LuaContext and the module name to load.
  void setRequireFunction(RequireFunction requireFunction);

  void setCallbacks(String const& tableName, LuaCallbacks const& callbacks) const;

  // For convenience, invokePath methods are equivalent to calling getPath(key)
  // to get a function, and then invoking it.

  template <typename Ret = LuaValue, typename... Args>
  Ret invokePath(String const& key, Args const&... args) const;

  // For convenience, calls to LuaEngine conversion / create functions are
  // duplicated here.

  template <typename T>
  LuaValue luaFrom(T&& t);
  template <typename T>
  LuaValue luaFrom(T const& t);
  template <typename T>
  Maybe<T> luaMaybeTo(LuaValue&& v);
  template <typename T>
  Maybe<T> luaMaybeTo(LuaValue const& v);
  template <typename T>
  T luaTo(LuaValue const& v);
  template <typename T>
  T luaTo(LuaValue&& v);

  LuaString createString(String const& str);
  LuaString createString(char const* str);

  LuaTable createTable();

  template <typename Container>
  LuaTable createTable(Container const& map);

  template <typename Container>
  LuaTable createArrayTable(Container const& array);

  template <typename Function>
  LuaFunction createFunction(Function&& func);

  template <typename Return, typename... Args, typename Function>
  LuaFunction createFunctionWithSignature(Function&& func);

  template <typename T>
  LuaUserData createUserData(T t);
};

template <typename T>
struct LuaNullTermWrapper : T {
  LuaNullTermWrapper() : T() {}
  LuaNullTermWrapper(LuaNullTermWrapper const& nt) : T(nt) {}
  LuaNullTermWrapper(LuaNullTermWrapper&& nt) : T(std::move(nt)) {}
  LuaNullTermWrapper(T const& bt) : T(bt) {}
  LuaNullTermWrapper(T&& bt) : T(std::move(bt)) {}

  using T::T;

  LuaNullTermWrapper& operator=(LuaNullTermWrapper const& rhs) {
    T::operator=(rhs);
    return *this;
  }

  LuaNullTermWrapper& operator=(LuaNullTermWrapper&& rhs) {
    T::operator=(std::move(rhs));
    return *this;
  }

  LuaNullTermWrapper& operator=(T&& other) {
    T::operator=(std::forward<T>(other));
    return *this;
  }
};

class LuaNullEnforcer {
public:
  LuaNullEnforcer(LuaEngine& engine);
  ~LuaNullEnforcer();
private:
  LuaEngine* m_engine;
};

// Types that want to participate in automatic lua conversion should specialize
// this template and provide static to and from methods on it.  The method
// signatures will be called like:
//   LuaValue from(LuaEngine& engine, T t);
//   Maybe<T> to(LuaEngine& engine, LuaValue v);
// The methods can also take 'T const&' or 'LuaValue const&' as parameters, and
// the 'to' method can also return a bare T if conversion cannot fail.
template <typename T>
struct LuaConverter;

// UserData types that want to expose methods to lua should specialize this
// template.
template <typename T>
struct LuaUserDataMethods {
  static LuaMethods<T> make();
};

// Convenience converter that simply converts to/from LuaUserData, can be
// derived from by a declared converter.
template <typename T>
struct LuaUserDataConverter {
  static LuaValue from(LuaEngine& engine, T t);
  static Maybe<T> to(LuaEngine& engine, LuaValue const& v);
};

struct LuaProfileEntry {
  // Source name of the chunk the function was defined in
  String source;
  // Line number in the chunk of the beginning of the function definition
  unsigned sourceLine;
  // Name of the function, if it can be determined
  Maybe<String> name;
  // Scope of the function, if it can be determined
  Maybe<String> nameScope;
  // Time taken within this function itself
  int64_t selfTime;
  // Total time taken within this function or sub functions
  int64_t totalTime;
  // Calls from this function
  HashMap<tuple<String, unsigned>, shared_ptr<LuaProfileEntry>> calls;
};

// This class represents one execution engine in lua, holding a single
// lua_State.  Multiple contexts can be created, and they will have separate
// global environments and cannot affect each other.  Individual LuaEngines /
// LuaContexts are not thread safe, use one LuaEngine per thread.
class LuaEngine : public RefCounter {
public:
  // If 'safe' is true, then creates a lua engine with all builtin lua
  // functions that can affect the real world disabled.
  static LuaEnginePtr create(bool safe = true);

  ~LuaEngine();

  LuaEngine(LuaEngine const&) = delete;
  LuaEngine(LuaEngine&&) = default;

  LuaEngine& operator=(LuaEngine const&) = delete;
  LuaEngine& operator=(LuaEngine&&) = default;

  // Set the instruction limit for computation sequences in the engine.  During
  // any function invocation, thread resume, or code evaluation, an instruction
  // counter will be started.  In the event that the instruction counter
  // becomes greater than the given limit, a LuaException will be thrown.  The
  // count is only reset when the initial entry into LuaEngine is returned,
  // recursive entries into LuaEngine accumulate the same instruction counter.
  // 0 disables the instruction limit.
  void setInstructionLimit(uint64_t instructionLimit = 0);
  uint64_t instructionLimit() const;

  // If profiling is enabled, then every 'measureInterval' instructions, the
  // function call stack will be recorded, and a summary of function timing can
  // be printed using profileReport
  void setProfilingEnabled(bool profilingEnabled);
  bool profilingEnabled() const;

  // Print a summary of the profiling data gathered since profiling was last
  // enabled.
  List<LuaProfileEntry> getProfile();

  // If an instruction limit is set or profiling is neabled, this field
  // describes the resolution of instruction count measurement, and affects the
  // accuracy of profiling and the instruction count limit.  Defaults to 1000
  void setInstructionMeasureInterval(unsigned measureInterval = 1000);
  unsigned instructionMeasureInterval() const;

  // Sets the LuaEngine recursion limit, limiting the number of times a
  // LuaEngine call may directly or inderectly trigger a call back into the
  // LuaEngine, preventing a C++ stack overflow.  0 disables the limit.
  void setRecursionLimit(unsigned recursionLimit = 0);
  unsigned recursionLimit() const;

  // Compile a given script into bytecode.  If name is given, then it will be
  // used as the internal name for the resulting chunk and will provide better
  // error messages.
  //
  // Unfortunately the only way to completely ensure that a single script will
  // execute in two separate contexts and truly be isolated is to compile the
  // script to bytecode and load once in each context as a separate chunk.
  ByteArray compile(char const* contents, size_t size, char const* name = nullptr);
  ByteArray compile(String const& contents, String const& name = String());
  ByteArray compile(ByteArray const& contents, String const& name = String());
  
  // Returns the debug info of the state.
  lua_Debug const& debugInfo(int level = 1, const char* what = "nSlu");

  // Generic from/to lua conversion, calls template specialization of
  // LuaConverter for actual conversion.
  template <typename T>
  LuaValue luaFrom(T&& t);
  template <typename T>
  LuaValue luaFrom(T const& t);
  template <typename T>
  Maybe<T> luaMaybeTo(LuaValue&& v);
  template <typename T>
  Maybe<T> luaMaybeTo(LuaValue const& v);

  // Wraps luaMaybeTo, throws an exception if conversion fails.
  template <typename T>
  T luaTo(LuaValue const& v);
  template <typename T>
  T luaTo(LuaValue&& v);

  LuaString createString(String const& str);
  LuaString createString(char const* str);

  LuaTable createTable(int narr = 0, int nrec = 0);

  template <typename Container>
  LuaTable createTable(Container const& map);

  template <typename Container>
  LuaTable createArrayTable(Container const& array);

  // Creates a function and deduces the signature of the function using
  // FunctionTraits.  As a convenience, the given function may optionally take
  // a LuaEngine& parameter as the first parameter, and if it does, when called
  // the function will get a reference to the calling LuaEngine.
  template <typename Function>
  LuaFunction createFunction(Function&& func);

  // If the function signature is not deducible using FunctionTraits, you can
  // specify the return and argument types manually using this createFunction
  // version.
  template <typename Return, typename... Args, typename Function>
  LuaFunction createFunctionWithSignature(Function&& func);

  LuaFunction createWrappedFunction(LuaDetail::LuaWrappedFunction function);

  LuaFunction createRawFunction(lua_CFunction func);

  LuaFunction createFunctionFromSource(int handleIndex, char const* contents, size_t size, char const* name);

  LuaThread createThread();

  template <typename T>
  LuaUserData createUserData(T t);

  LuaContext createContext();

  // Global environment changes only affect newly created contexts

  template <typename T = LuaValue, typename K>
  T getGlobal(K key);
  template <typename T = LuaValue>
  T getGlobal(char const* key);

  template <typename T, typename K>
  void setGlobal(K key, T value);

  template <typename T>
  void setGlobal(char const* key, T value);

  // Perform either a full or incremental garbage collection.
  void collectGarbage(Maybe<unsigned> steps = {});

  // Stop / start automatic garbage collection
  void setAutoGarbageCollection(bool autoGarbageColleciton);

  // Tune the pause and step values of the lua garbage collector
  void tuneAutoGarbageCollection(float pause, float stepMultiplier);

  // Bytes in use by lua
  size_t memoryUsage() const;

  // Enforce null-terminated string conversion as long as the returned enforcer object is in scope.
  LuaNullEnforcer nullTerminate();
  // Disables null-termination enforcement
  void setNullTerminated(bool nullTerminated);

private:
  friend struct LuaDetail::LuaHandle;
  friend class LuaReference;
  friend class LuaString;
  friend class LuaTable;
  friend class LuaFunction;
  friend class LuaThread;
  friend class LuaUserData;
  friend class LuaContext;
  friend class LuaNullEnforcer;

  LuaEngine() = default;

  // Get the LuaEngine* out of the lua registry magic entry.  Uses 1 stack
  // space, and does not call lua_checkstack.
  static LuaEngine* luaEnginePtr(lua_State* state);
  // Counts instructions when instruction limiting is enabled.
  static void countHook(lua_State* state, lua_Debug* ar);

  static void* allocate(void* userdata, void* ptr, size_t oldSize, size_t newSize);

  // Pops lua error from stack and throws LuaException
  void handleError(lua_State* state, int res);

  // lua_pcall with a better message handler that includes a traceback.
  int pcallWithTraceback(lua_State* state, int nargs, int nresults);

  // override for lua coroutine resume with traceback
  static int coresumeWithTraceback(lua_State* state);
  // propagates errors from one state to another, i.e. past thread boundaries
  // pops error off the top of the from stack and pushes onto the to stack
  static void propagateErrorWithTraceback(lua_State* from, lua_State* to);

  char const* stringPtr(int handleIndex);
  size_t stringLength(int handleIndex);
  String string(int handleIndex);
  StringView stringView(int handleIndex);

  LuaValue tableGet(bool raw, int handleIndex, LuaValue const& key);
  LuaValue tableGet(bool raw, int handleIndex, char const* key);

  void tableSet(bool raw, int handleIndex, LuaValue const& key, LuaValue const& value);
  void tableSet(bool raw, int handleIndex, char const* key, LuaValue const& value);

  LuaInt tableLength(bool raw, int handleIndex);

  void tableIterate(int handleIndex, function<bool(LuaValue, LuaValue)> iterator);

  Maybe<LuaTable> tableGetMetatable(int handleIndex);
  void tableSetMetatable(int handleIndex, LuaTable const& table);

  template <typename... Args>
  LuaDetail::LuaFunctionReturn callFunction(int handleIndex, Args const&... args);

  template <typename... Args>
  Maybe<LuaDetail::LuaFunctionReturn> resumeThread(int handleIndex, Args const&... args);
  void threadPushFunction(int threadIndex, int functionIndex);
  LuaThread::Status threadStatus(int handleIndex);

  template <typename T>
  void registerUserDataType();

  template <typename T>
  bool userDataIsType(int handleIndex);

  template <typename T>
  T* getUserData(int handleIndex);

  void setContextRequire(int handleIndex, LuaContext::RequireFunction requireFunction);

  void contextLoad(int handleIndex, char const* contents, size_t size, char const* name);

  LuaDetail::LuaFunctionReturn contextEval(int handleIndex, String const& lua);

  LuaValue contextGetPath(int handleIndex, String path);
  void contextSetPath(int handleIndex, String path, LuaValue const& value);

  int popHandle(lua_State* state);
  void pushHandle(lua_State* state, int handleIndex);
  int copyHandle(int handleIndex);
  void destroyHandle(int handleIndex);

  int placeHandle();

  void pushLuaValue(lua_State* state, LuaValue const& luaValue);
  LuaValue popLuaValue(lua_State* state);

  template <typename T>
  size_t pushArgument(lua_State* state, T const& arg);

  template <typename T>
  size_t pushArgument(lua_State* state, LuaVariadic<T> const& args);

  size_t doPushArguments(lua_State*);
  template <typename First, typename... Rest>
  size_t doPushArguments(lua_State* state, First const& first, Rest const&... rest);

  template <typename... Args>
  size_t pushArguments(lua_State* state, Args const&... args);

  void incrementRecursionLevel();
  void decrementRecursionLevel();

  void updateCountHook();

  // The following fields exist to use their addresses as unique lightuserdata,
  // as is recommended by the lua docs.
  static int s_luaInstructionLimitExceptionKey;
  static int s_luaRecursionLimitExceptionKey;

  lua_State* m_state;
  int m_pcallTracebackMessageHandlerRegistryId;
  int m_scriptDefaultEnvRegistryId;
  int m_wrappedFunctionMetatableRegistryId;
  int m_requireFunctionMetatableRegistryId;
  HashMap<std::type_index, int> m_registeredUserDataTypes;

  lua_State* m_handleThread;
  int m_handleStackSize;
  int m_handleStackMax;
  List<int> m_handleFree;

  uint64_t m_instructionLimit;
  bool m_profilingEnabled;
  unsigned m_instructionMeasureInterval;
  uint64_t m_instructionCount;
  unsigned m_recursionLevel;
  unsigned m_recursionLimit;
  int m_nullTerminated;
  HashMap<tuple<String, unsigned>, shared_ptr<LuaProfileEntry>> m_profileEntries;
  lua_Debug m_debugInfo;
};

// Built in conversions

template <>
struct LuaConverter<bool> {
  static LuaValue from(LuaEngine&, bool v) {
    return v;
  }

  static Maybe<bool> to(LuaEngine&, LuaValue const& v) {
    if (auto b = v.ptr<LuaBoolean>())
      return *b;
    if (v == LuaNil)
      return false;
    return true;
  }
};

template <typename T>
struct LuaIntConverter {
  static LuaValue from(LuaEngine&, T v) {
    return LuaInt(v);
  }

  static Maybe<T> to(LuaEngine&, LuaValue const& v) {
    if (auto n = v.ptr<LuaInt>())
      return *n;
    if (auto n = v.ptr<LuaFloat>())
      return *n;
    if (auto s = v.ptr<LuaString>()) {
      if (auto n = maybeLexicalCast<LuaInt>(s->ptr()))
        return *n;
      if (auto n = maybeLexicalCast<LuaFloat>(s->ptr()))
        return *n;
    }
    return {};
  }
};

template <>
struct LuaConverter<char> : LuaIntConverter<char> {};

template <>
struct LuaConverter<unsigned char> : LuaIntConverter<unsigned char> {};

template <>
struct LuaConverter<short> : LuaIntConverter<short> {};

template <>
struct LuaConverter<unsigned short> : LuaIntConverter<unsigned short> {};

template <>
struct LuaConverter<long> : LuaIntConverter<long> {};

template <>
struct LuaConverter<unsigned long> : LuaIntConverter<unsigned long> {};

template <>
struct LuaConverter<int> : LuaIntConverter<int> {};

template <>
struct LuaConverter<unsigned int> : LuaIntConverter<unsigned int> {};

template <>
struct LuaConverter<long long> : LuaIntConverter<long long> {};

template <>
struct LuaConverter<unsigned long long> : LuaIntConverter<unsigned long long> {};

template <typename T>
struct LuaFloatConverter {
  static LuaValue from(LuaEngine&, T v) {
    return LuaFloat(v);
  }

  static Maybe<T> to(LuaEngine&, LuaValue const& v) {
    if (auto n = v.ptr<LuaFloat>())
      return *n;
    if (auto n = v.ptr<LuaInt>())
      return *n;
    if (auto s = v.ptr<LuaString>()) {
      if (auto n = maybeLexicalCast<LuaFloat>(s->ptr()))
        return *n;
      if (auto n = maybeLexicalCast<LuaInt>(s->ptr()))
        return *n;
    }
    return {};
  }
};

template <>
struct LuaConverter<float> : LuaFloatConverter<float> {};

template <>
struct LuaConverter<double> : LuaFloatConverter<double> {};

template <>
struct LuaConverter<String> {
  static LuaValue from(LuaEngine& engine, String const& v) {
    return engine.createString(v);
  }

  static Maybe<String> to(LuaEngine&, LuaValue const& v) {
    if (v.is<LuaString>())
      return v.get<LuaString>().toString();
    if (v.is<LuaInt>())
      return String(toString(v.get<LuaInt>()));
    if (v.is<LuaFloat>())
      return String(toString(v.get<LuaFloat>()));
    return {};
  }
};

template <>
struct LuaConverter<std::string> {
  static LuaValue from(LuaEngine& engine, std::string const& v) {
    return engine.createString(v.c_str());
  }

  static Maybe<std::string> to(LuaEngine& engine, LuaValue v) {
    return engine.luaTo<String>(std::move(v)).takeUtf8();
  }
};

template <>
struct LuaConverter<char const*> {
  static LuaValue from(LuaEngine& engine, char const* v) {
    return engine.createString(v);
  }
};

template <size_t s>
struct LuaConverter<char[s]> {
  static LuaValue from(LuaEngine& engine, char const v[s]) {
    return engine.createString(v);
  }
};

template <>
struct LuaConverter<Directives> {
  static LuaValue from(LuaEngine& engine, Directives const& v) {
    if (String const* ptr = v.stringPtr())
      return engine.createString(*ptr);
    else
      return engine.createString("");
  }
};

template <>
struct LuaConverter<LuaString> {
  static LuaValue from(LuaEngine&, LuaString v) {
    return LuaValue(std::move(v));
  }

  static Maybe<LuaString> to(LuaEngine& engine, LuaValue v) {
    if (v.is<LuaString>())
      return LuaString(std::move(v.get<LuaString>()));
    if (v.is<LuaInt>())
      return engine.createString(toString(v.get<LuaInt>()));
    if (v.is<LuaFloat>())
      return engine.createString(toString(v.get<LuaFloat>()));
    return {};
  }
};

template <typename T>
struct LuaValueConverter {
  static LuaValue from(LuaEngine&, T v) {
    return v;
  }

  static Maybe<T> to(LuaEngine&, LuaValue v) {
    if (auto p = v.ptr<T>()) {
      return std::move(*p);
    }
    return {};
  }
};

template <>
struct LuaConverter<LuaTable> : LuaValueConverter<LuaTable> {};

template <>
struct LuaConverter<LuaFunction> : LuaValueConverter<LuaFunction> {};

template <>
struct LuaConverter<LuaThread> : LuaValueConverter<LuaThread> {};

template <>
struct LuaConverter<LuaUserData> : LuaValueConverter<LuaUserData> {};

template <>
struct LuaConverter<LuaValue> {
  static LuaValue from(LuaEngine&, LuaValue v) {
    return v;
  }

  static LuaValue to(LuaEngine&, LuaValue v) {
    return v;
  }
};

template <typename T>
struct LuaConverter<Maybe<T>> {
  static LuaValue from(LuaEngine& engine, Maybe<T> const& v) {
    if (v)
      return engine.luaFrom<T>(*v);
    else
      return LuaNil;
  }

  static LuaValue from(LuaEngine& engine, Maybe<T>&& v) {
    if (v)
      return engine.luaFrom<T>(v.take());
    else
      return LuaNil;
  }

  static Maybe<Maybe<T>> to(LuaEngine& engine, LuaValue const& v) {
    if (v != LuaNil) {
      if (auto conv = engine.luaMaybeTo<T>(v))
        return conv;
      else
        return {};
    } else {
      return Maybe<T>();
    }
  }

  static Maybe<Maybe<T>> to(LuaEngine& engine, LuaValue&& v) {
    if (v != LuaNil) {
      if (auto conv = engine.luaMaybeTo<T>(std::move(v)))
        return conv;
      else
        return {};
    } else {
      return Maybe<T>();
    }
  }
};

template <typename T>
struct LuaMapConverter {
  static LuaValue from(LuaEngine& engine, T const& v) {
    return engine.createTable(v);
  }

  static Maybe<T> to(LuaEngine& engine, LuaValue const& v) {
    auto table = v.ptr<LuaTable>();
    if (!table)
      return {};

    T result;
    bool failed = false;
    table->iterate([&result, &failed, &engine](LuaValue key, LuaValue value) {
        auto contKey = engine.luaMaybeTo<typename T::key_type>(std::move(key));
        auto contValue = engine.luaMaybeTo<typename T::mapped_type>(std::move(value));
        if (!contKey || !contValue) {
          failed = true;
          return false;
        }
        result[contKey.take()] = contValue.take();
        return true;
      });

    if (failed)
      return {};

    return result;
  }
};

template <typename T>
struct LuaContainerConverter {
  static LuaValue from(LuaEngine& engine, T const& v) {
    return engine.createArrayTable(v);
  }

  static Maybe<T> to(LuaEngine& engine, LuaValue const& v) {
    auto table = v.ptr<LuaTable>();
    if (!table)
      return {};

    T result;
    bool failed = false;
    table->iterate([&result, &failed, &engine](LuaValue key, LuaValue value) {
        if (!key.is<LuaInt>()) {
          failed = true;
          return false;
        }
        auto contVal = engine.luaMaybeTo<typename T::value_type>(std::move(value));
        if (!contVal) {
          failed = true;
          return false;
        }
        result.insert(result.end(), contVal.take());
        return true;
      });

    if (failed)
      return {};

    return result;
  }
};

template <typename T, typename Allocator>
struct LuaConverter<List<T, Allocator>> : LuaContainerConverter<List<T, Allocator>> {};

template <typename T, size_t MaxSize>
struct LuaConverter<StaticList<T, MaxSize>> : LuaContainerConverter<StaticList<T, MaxSize>> {};

template <typename T, size_t MaxStackSize>
struct LuaConverter<SmallList<T, MaxStackSize>> : LuaContainerConverter<SmallList<T, MaxStackSize>> {};

template <>
struct LuaConverter<StringList> : LuaContainerConverter<StringList> {};

template <typename T, typename BaseSet>
struct LuaConverter<Set<T, BaseSet>> : LuaContainerConverter<Set<T, BaseSet>> {};

template <typename T, typename BaseSet>
struct LuaConverter<HashSet<T, BaseSet>> : LuaContainerConverter<HashSet<T, BaseSet>> {};

template <typename Key, typename Value, typename Compare, typename Allocator>
struct LuaConverter<Map<Key, Value, Compare, Allocator>> : LuaMapConverter<Map<Key, Value, Compare, Allocator>> {};

template <typename Key, typename Value, typename Hash, typename Equals, typename Allocator>
struct LuaConverter<HashMap<Key, Value, Hash, Equals, Allocator>> : LuaMapConverter<HashMap<Key, Value, Hash, Equals, Allocator>> {};

template <>
struct LuaConverter<Json> {
  static LuaValue from(LuaEngine& engine, Json const& v);
  static Maybe<Json> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<JsonObject> {
  static LuaValue from(LuaEngine& engine, JsonObject v);
  static Maybe<JsonObject> to(LuaEngine& engine, LuaValue v);
};

template <>
struct LuaConverter<JsonArray> {
  static LuaValue from(LuaEngine& engine, JsonArray v);
  static Maybe<JsonArray> to(LuaEngine& engine, LuaValue v);
};

namespace LuaDetail {
  inline LuaHandle::LuaHandle(LuaEnginePtr engine, int handleIndex)
    : engine(std::move(engine)), handleIndex(handleIndex) {}

  inline LuaHandle::~LuaHandle() {
    if (engine)
      engine->destroyHandle(handleIndex);
  }

  inline LuaHandle::LuaHandle(LuaHandle const& other) {
    engine = other.engine;
    if (engine)
      handleIndex = engine->copyHandle(other.handleIndex);
  }

  inline LuaHandle::LuaHandle(LuaHandle&& other) {
    engine = take(other.engine);
    handleIndex = take(other.handleIndex);
  }

  inline LuaHandle& LuaHandle::operator=(LuaHandle const& other) {
    if (engine)
      engine->destroyHandle(handleIndex);

    engine = other.engine;
    if (engine)
      handleIndex = engine->copyHandle(other.handleIndex);

    return *this;
  }

  inline LuaHandle& LuaHandle::operator=(LuaHandle&& other) {
    if (engine)
      engine->destroyHandle(handleIndex);

    engine = take(other.engine);
    handleIndex = take(other.handleIndex);

    return *this;
  }

  template <typename T>
  struct FromFunctionReturn {
    static T convert(LuaEngine& engine, LuaFunctionReturn const& ret) {
      if (auto l = ret.ptr<LuaValue>()) {
        return engine.luaTo<T>(*l);
      } else if (auto vec = ret.ptr<LuaVariadic<LuaValue>>()) {
        return engine.luaTo<T>(vec->at(0));
      } else {
        return engine.luaTo<T>(LuaNil);
      }
    }
  };

  template <typename T>
  struct FromFunctionReturn<LuaVariadic<T>> {
    static LuaVariadic<T> convert(LuaEngine& engine, LuaFunctionReturn const& ret) {
      if (auto l = ret.ptr<LuaValue>()) {
        return {engine.luaTo<T>(*l)};
      } else if (auto vec = ret.ptr<LuaVariadic<LuaValue>>()) {
        LuaVariadic<T> ret(vec->size());
        for (size_t i = 0; i < vec->size(); ++i)
          ret[i] = engine.luaTo<T>((*vec)[i]);
        return ret;
      } else {
        return {};
      }
    }
  };

  template <typename ArgFirst, typename... ArgRest>
  struct FromFunctionReturn<LuaTupleReturn<ArgFirst, ArgRest...>> {
    static LuaTupleReturn<ArgFirst, ArgRest...> convert(LuaEngine& engine, LuaFunctionReturn const& ret) {
      if (auto l = ret.ptr<LuaValue>()) {
        return doConvertSingle(engine, *l, typename GenIndexSequence<0, sizeof...(ArgRest)>::type());
      } else if (auto vec = ret.ptr<LuaVariadic<LuaValue>>()) {
        return doConvertMulti(engine, *vec, typename GenIndexSequence<0, sizeof...(ArgRest)>::type());
      } else {
        return doConvertNone(engine, typename GenIndexSequence<0, sizeof...(ArgRest)>::type());
      }
    }

    template <size_t... Indexes>
    static LuaTupleReturn<ArgFirst, ArgRest...> doConvertSingle(
        LuaEngine& engine, LuaValue const& single, IndexSequence<Indexes...> const&) {
      return LuaTupleReturn<ArgFirst, ArgRest...>(engine.luaTo<ArgFirst>(single), engine.luaTo<ArgRest>(LuaNil)...);
    }

    template <size_t... Indexes>
    static LuaTupleReturn<ArgFirst, ArgRest...> doConvertMulti(
        LuaEngine& engine, LuaVariadic<LuaValue> const& multi, IndexSequence<Indexes...> const&) {
      return LuaTupleReturn<ArgFirst, ArgRest...>(
          engine.luaTo<ArgFirst>(multi.at(0)), engine.luaTo<ArgRest>(multi.get(Indexes + 1))...);
    }

    template <size_t... Indexes>
    static LuaTupleReturn<ArgFirst, ArgRest...> doConvertNone(LuaEngine& engine, IndexSequence<Indexes...> const&) {
      return LuaTupleReturn<ArgFirst, ArgRest...>(engine.luaTo<ArgFirst>(LuaNil), engine.luaTo<ArgRest>(LuaNil)...);
    }
  };

  template <typename... Args, size_t... Indexes>
  LuaVariadic<LuaValue> toVariadicReturn(
      LuaEngine& engine, LuaTupleReturn<Args...> const& vals, IndexSequence<Indexes...> const&) {
    return LuaVariadic<LuaValue>{engine.luaFrom(get<Indexes>(vals))...};
  }

  template <typename... Args>
  LuaVariadic<LuaValue> toWrappedReturn(LuaEngine& engine, LuaTupleReturn<Args...> const& vals) {
    return toVariadicReturn(engine, vals, typename GenIndexSequence<0, sizeof...(Args)>::type());
  }

  template <typename T>
  LuaVariadic<LuaValue> toWrappedReturn(LuaEngine& engine, LuaVariadic<T> const& vals) {
    LuaVariadic<LuaValue> ret(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      ret[i] = engine.luaFrom(vals[i]);
    return ret;
  }

  template <typename T>
  LuaValue toWrappedReturn(LuaEngine& engine, T const& t) {
    return engine.luaFrom(t);
  }

  template <typename T>
  struct ArgGet {
    static T get(LuaEngine& engine, size_t argc, LuaValue* argv, size_t index) {
      if (index < argc)
        return engine.luaTo<T>(std::move(argv[index]));
      return engine.luaTo<T>(LuaNil);
    }
  };

  template <typename T>
  struct ArgGet<LuaVariadic<T>> {
    static LuaVariadic<T> get(LuaEngine& engine, size_t argc, LuaValue* argv, size_t index) {
      if (index >= argc)
        return {};

      LuaVariadic<T> subargs(argc - index);
      for (size_t i = index; i < argc; ++i)
        subargs[i - index] = engine.luaTo<T>(std::move(argv[i]));
      return subargs;
    }
  };

  template <typename Return, typename... Args>
  struct FunctionWrapper {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        return toWrappedReturn(engine, (Return const&)func(ArgGet<Args>::get(engine, argc, argv, Indexes)...));
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename... Args>
  struct FunctionWrapper<void, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        func(ArgGet<Args>::get(engine, argc, argv, Indexes)...);
        return LuaFunctionReturn();
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename Return, typename... Args>
  struct FunctionWrapper<Return, LuaEngine, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        return toWrappedReturn(engine, (Return const&)func(engine, ArgGet<Args>::get(engine, argc, argv, Indexes)...));
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename... Args>
  struct FunctionWrapper<void, LuaEngine, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        func(engine, ArgGet<Args>::get(engine, argc, argv, Indexes)...);
        return LuaFunctionReturn();
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename Return, typename... Args, typename Function>
  LuaWrappedFunction wrapFunctionWithSignature(Function&& func) {
    return FunctionWrapper<Return, typename std::decay<Args>::type...>::wrap(std::forward<Function>(func));
  }

  template <typename Return, typename Function, typename... Args>
  LuaWrappedFunction wrapFunctionArgs(Function&& func, VariadicTypedef<Args...> const&) {
    return wrapFunctionWithSignature<Return, Args...>(std::forward<Function>(func));
  }

  template <typename Function>
  LuaWrappedFunction wrapFunction(Function&& func) {
    return wrapFunctionArgs<typename FunctionTraits<Function>::Return>(
        std::forward<Function>(func), typename FunctionTraits<Function>::Args());
  }

  template <typename Return, typename T, typename... Args>
  struct MethodWrapper {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) mutable {
        if (argc == 0)
          throw LuaException("No object argument passed to wrapped method");
        return toWrappedReturn(engine,
            (Return const&)func(argv[0].get<LuaUserData>().get<T>(), ArgGet<Args>::get(engine, argc - 1, argv + 1, Indexes)...));
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function&& func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename T, typename... Args>
  struct MethodWrapper<void, T, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        if (argc == 0)
          throw LuaException("No object argument passed to wrapped method");
        func(argv[0].get<LuaUserData>().get<T>(), ArgGet<Args>::get(engine, argc - 1, argv + 1, Indexes)...);
        return LuaFunctionReturn();
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename Return, typename T, typename... Args>
  struct MethodWrapper<Return, T, LuaEngine, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        if (argc == 0)
          throw LuaException("No object argument passed to wrapped method");
        return toWrappedReturn(
            engine,
            (Return const&)func(argv[0].get<LuaUserData>().get<T>(), engine, ArgGet<Args>::get(engine, argc - 1, argv + 1, Indexes)...));
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename T, typename... Args>
  struct MethodWrapper<void, T, LuaEngine, Args...> {
    template <typename Function, size_t... Indexes>
    static LuaWrappedFunction wrapIndexes(Function func, IndexSequence<Indexes...> const&) {
      return [func = std::move(func)](LuaEngine& engine, size_t argc, LuaValue* argv) {
        if (argc == 0)
          throw LuaException("No object argument passed to wrapped method");
        func(argv[0].get<LuaUserData>().get<T>(), engine, ArgGet<Args>::get(engine, argc - 1, argv + 1, Indexes)...);
        return LuaValue();
      };
    }

    template <typename Function>
    static LuaWrappedFunction wrap(Function func) {
      return wrapIndexes(std::forward<Function>(func), typename GenIndexSequence<0, sizeof...(Args)>::type());
    }
  };

  template <typename Return, typename... Args, typename Function>
  LuaWrappedFunction wrapMethodWithSignature(Function&& func) {
    return MethodWrapper<Return, typename std::decay<Args>::type...>::wrap(std::forward<Function>(func));
  }

  template <typename Return, typename Function, typename... Args>
  LuaWrappedFunction wrapMethodArgs(Function&& func, VariadicTypedef<Args...> const&) {
    return wrapMethodWithSignature<Return, Args...>(std::forward<Function>(func));
  }

  template <typename Function>
  LuaWrappedFunction wrapMethod(Function&& func) {
    return wrapMethodArgs<typename FunctionTraits<Function>::Return>(
        std::forward<Function>(func), typename FunctionTraits<Function>::Args());
  }

  template <typename Ret, typename... Args>
  struct TableIteratorWrapper;

  template <typename Key, typename Value>
  struct TableIteratorWrapper<bool, LuaEngine&, Key, Value> {
    template <typename Function>
    static function<bool(LuaValue, LuaValue)> wrap(LuaEngine& engine, Function&& func) {
      return [&engine, func = std::move(func)](LuaValue key, LuaValue value) -> bool {
        return func(engine, engine.luaTo<Key>(std::move(key)), engine.luaTo<Value>(std::move(value)));
      };
    }
  };

  template <typename Key, typename Value>
  struct TableIteratorWrapper<void, LuaEngine&, Key, Value> {
    template <typename Function>
    static function<bool(LuaValue, LuaValue)> wrap(LuaEngine& engine, Function&& func) {
      return [&engine, func = std::move(func)](LuaValue key, LuaValue value) -> bool {
        func(engine, engine.luaTo<Key>(std::move(key)), engine.luaTo<Value>(std::move(value)));
        return true;
      };
    }
  };

  template <typename Key, typename Value>
  struct TableIteratorWrapper<bool, Key, Value> {
    template <typename Function>
    static function<bool(LuaValue, LuaValue)> wrap(LuaEngine& engine, Function&& func) {
      return [&engine, func = std::move(func)](LuaValue key, LuaValue value) -> bool {
        return func(engine.luaTo<Key>(std::move(key)), engine.luaTo<Value>(std::move(value)));
      };
    }
  };

  template <typename Key, typename Value>
  struct TableIteratorWrapper<void, Key, Value> {
    template <typename Function>
    static function<bool(LuaValue, LuaValue)> wrap(LuaEngine& engine, Function&& func) {
      return [&engine, func = std::move(func)](LuaValue key, LuaValue value) -> bool {
        func(engine.luaTo<Key>(std::move(key)), engine.luaTo<Value>(std::move(value)));
        return true;
      };
    }
  };

  template <typename Return, typename... Args, typename Function>
  function<bool(LuaValue, LuaValue)> wrapTableIteratorWithSignature(LuaEngine& engine, Function&& func) {
    return TableIteratorWrapper<Return, typename std::decay<Args>::type...>::wrap(engine, std::forward<Function>(func));
  }

  template <typename Return, typename Function, typename... Args>
  function<bool(LuaValue, LuaValue)> wrapTableIteratorArgs(
      LuaEngine& engine, Function&& func, VariadicTypedef<Args...> const&) {
    return wrapTableIteratorWithSignature<Return, Args...>(engine, std::forward<Function>(func));
  }

  template <typename Function>
  function<bool(LuaValue, LuaValue)> wrapTableIterator(LuaEngine& engine, Function&& func) {
    return wrapTableIteratorArgs<typename FunctionTraits<Function>::Return>(
        engine, std::forward<Function>(func), typename FunctionTraits<Function>::Args());
  }

  // Like lua_setfield / lua_getfield but raw.
  void rawSetField(lua_State* state, int index, char const* key);
  void rawGetField(lua_State* state, int index, char const* key);

  // Shallow copies a lua table at the given index into the table at the target
  // index.
  void shallowCopy(lua_State* state, int sourceIndex, int targetIndex);

  LuaTable insertJsonMetatable(LuaEngine& engine, LuaTable const& table, Json::Type type);

  // Creates a custom lua table from a JsonArray or JsonObject that has
  // slightly different behavior than a standard lua table.  The table
  // remembers nil entries, as well as whether it was initially constructed
  // from a JsonArray or JsonObject as a hint on how to convert it back into a
  // Json.  The custom containers are meant to act nearly identical to standard
  // lua tables, so iterating over the table with pairs or ipairs works exactly
  // like a standard lua table, so will skip over nil entries and in the case
  // of ipairs, stop at the first nil entry.
  LuaTable jsonContainerToTable(LuaEngine& engine, Json const& container);

  // popJsonContainer must be called with a lua table on the top of the stack.
  // Uses the table contents, as well as any hint entries if the table was
  // created originally from a Json, to determine whether a JsonArray or
  // JsonObject is more appropriate.
  Maybe<Json> tableToJsonContainer(LuaTable const& t);

  // Special lua functions to operate on our custom jarray / jobject container
  // types.  Should always do some "sensible" action if given a regular lua
  // table instead of a custom json container one.

  // Create a JsonList container table
  Json jarrayCreate();
  // Create a JsonMap container table
  Json jobjectCreate();

  // Adds the Json array metatable to a Lua table or creates one.
  LuaTable jarray(LuaEngine& engine, Maybe<LuaTable> table);
  // Adds the Json object metatable to a Lua table or creates one.
  LuaTable jobject(LuaEngine& engine, Maybe<LuaTable> table);

  // *Really* remove an entry from a JsonList or JsonMap container table,
  // including removing it from the __nils table.  If the given table is not a
  // special container table, is equivalent to setting the key entry to nil.
  void jcontRemove(LuaTable const& t, LuaValue const& key);

  // Returns the element count of the lua table argument, or, in the case of a
  // special JsonList container table, returns the "true" element count
  // including any nil entries.
  size_t jcontSize(LuaTable const& t);

  // Resize the given lua table by removing any indexed entries greater than the
  // target size, and in the case of a special JsonList container table, pads
  // to the end of the new size with nil entries.
  void jcontResize(LuaTable const& t, size_t size);

  // Coerces a values (strings, floats, ints) into an integer, but fails if the
  // number looks fractional (does not parse as int, float is not an exact
  // integer)
  Maybe<LuaInt> asInteger(LuaValue const& v);
}

template <typename Container>
LuaVariadic<typename std::decay<Container>::type::value_type> luaUnpack(Container&& c) {
  LuaVariadic<typename std::decay<Container>::type::value_type> ret;
  if (std::is_rvalue_reference<Container&&>::value) {
    for (auto& e : c)
      ret.append(std::move(e));
  } else {
    for (auto const& e : c)
      ret.append(e);
  }
  return ret;
}

template <typename... Types>
LuaTupleReturn<Types...>::LuaTupleReturn(Types const&... args)
  : Base(args...) {}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>::LuaTupleReturn(UTypes&&... args)
  : Base(std::move(args)...) {}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>::LuaTupleReturn(UTypes const&... args)
  : Base(args...) {}

template <typename... Types>
LuaTupleReturn<Types...>::LuaTupleReturn(LuaTupleReturn const& rhs)
  : Base(rhs) {}

template <typename... Types>
LuaTupleReturn<Types...>::LuaTupleReturn(LuaTupleReturn&& rhs)
  : Base(std::move(rhs)) {}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>::LuaTupleReturn(LuaTupleReturn<UTypes...> const& rhs)
  : Base(rhs) {}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>::LuaTupleReturn(LuaTupleReturn<UTypes...>&& rhs)
  : Base(std::move(rhs)) {}

template <typename... Types>
LuaTupleReturn<Types...>& LuaTupleReturn<Types...>::operator=(LuaTupleReturn const& rhs) {
  Base::operator=(rhs);
  return *this;
}

template <typename... Types>
LuaTupleReturn<Types...>& LuaTupleReturn<Types...>::operator=(LuaTupleReturn&& rhs) {
  Base::operator=(std::move(rhs));
  return *this;
}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>& LuaTupleReturn<Types...>::operator=(LuaTupleReturn<UTypes...> const& rhs) {
  Base::operator=((tuple<UTypes...> const&)rhs);
  return *this;
}

template <typename... Types>
template <typename... UTypes>
LuaTupleReturn<Types...>& LuaTupleReturn<Types...>::operator=(LuaTupleReturn<UTypes...>&& rhs) {
  Base::operator=((tuple<UTypes...> && )std::move(rhs));
  return *this;
}

template <typename... Types>
LuaTupleReturn<Types&...> luaTie(Types&... args) {
  return LuaTupleReturn<Types&...>(args...);
}

template <typename... Types>
LuaTupleReturn<typename std::decay<Types>::type...> luaTupleReturn(Types&&... args) {
  return LuaTupleReturn<typename std::decay<Types>::type...>(std::forward<Types>(args)...);
}

inline LuaReference::LuaReference(LuaDetail::LuaHandle handle) : m_handle(std::move(handle)) {}

inline bool LuaReference::operator==(LuaReference const& rhs) const {
  return tie(m_handle.engine, m_handle.handleIndex) == tie(rhs.m_handle.engine, rhs.m_handle.handleIndex);
}

inline bool LuaReference::operator!=(LuaReference const& rhs) const {
  return tie(m_handle.engine, m_handle.handleIndex) != tie(rhs.m_handle.engine, rhs.m_handle.handleIndex);
}

inline LuaEngine& LuaReference::engine() const {
  return *m_handle.engine;
}

inline int LuaReference::handleIndex() const {
  return m_handle.handleIndex;
}

inline char const* LuaString::ptr() const {
  return engine().stringPtr(handleIndex());
}

inline size_t LuaString::length() const {
  return engine().stringLength(handleIndex());
}

inline String LuaString::toString() const {
  return engine().string(handleIndex());
}

inline StringView LuaString::view() const {
  return engine().stringView(handleIndex());
}

inline bool operator==(LuaString const& s1, LuaString const& s2) {
  return s1.view() == s2.view();
}

inline bool operator==(LuaString const& s1, char const* s2) {
  return s1.view() == s2;
}

inline bool operator==(LuaString const& s1, std::string const& s2) {
  return s1.view() == s2;
}

inline bool operator==(LuaString const& s1, String const& s2) {
  return s1.view() == s2;
}

inline bool operator==(char const* s1, LuaString const& s2) {
  return s2.view() == s1;
}

inline bool operator==(std::string const& s1, LuaString const& s2) {
  return s2.view() == s1;
}

inline bool operator==(String const& s1, LuaString const& s2) {
  return s2.view() == s1;
}

inline bool operator!=(LuaString const& s1, LuaString const& s2) {
  return !(s1 == s2);
}

inline bool operator!=(LuaString const& s1, char const* s2) {
  return !(s1 == s2);
}

inline bool operator!=(LuaString const& s1, std::string const& s2) {
  return !(s1 == s2);
}

inline bool operator!=(LuaString const& s1, String const& s2) {
  return !(s1 == s2);
}

inline bool operator!=(char const* s1, LuaString const& s2) {
  return !(s1 == s2);
}

inline bool operator!=(std::string const& s1, LuaString const& s2) {
  return !(s1 == s2);
}

inline bool operator!=(String const& s1, LuaString const& s2) {
  return !(s1 == s2);
}

template <typename T, typename K>
T LuaTable::get(K key) const {
  return engine().luaTo<T>(engine().tableGet(false, handleIndex(), engine().luaFrom(std::move(key))));
}

template <typename T>
T LuaTable::get(char const* key) const {
  return engine().luaTo<T>(engine().tableGet(false, handleIndex(), key));
}

template <typename T, typename K>
void LuaTable::set(K key, T value) const {
  engine().tableSet(false, handleIndex(), engine().luaFrom(std::move(key)), engine().luaFrom(std::move(value)));
}

template <typename T>
void LuaTable::set(char const* key, T value) const {
  engine().tableSet(false, handleIndex(), key, engine().luaFrom(std::move(value)));
}

template <typename K>
bool LuaTable::contains(K key) const {
  return engine().tableGet(false, handleIndex(), engine().luaFrom(std::move(key))) != LuaNil;
}

template <typename K>
void LuaTable::remove(K key) const {
  engine().tableSet(false, handleIndex(), engine().luaFrom(key), LuaValue());
}

template <typename Function>
void LuaTable::iterate(Function&& function) const {
  return engine().tableIterate(handleIndex(), LuaDetail::wrapTableIterator(engine(), std::forward<Function>(function)));
}

template <typename Return, typename... Args, typename Function>
void LuaTable::iterateWithSignature(Function&& func) const {
  return engine().tableIterate(handleIndex(), LuaDetail::wrapTableIteratorWithSignature<Return, Args...>(engine(), std::forward<Function>(func)));
}

template <typename T, typename K>
T LuaTable::rawGet(K key) const {
  return engine().luaTo<T>(engine().tableGet(true, handleIndex(), engine().luaFrom(key)));
}

template <typename T>
T LuaTable::rawGet(char const* key) const {
  return engine().luaTo<T>(engine().tableGet(true, handleIndex(), key));
}

template <typename T, typename K>
void LuaTable::rawSet(K key, T value) const {
  engine().tableSet(true, handleIndex(), engine().luaFrom(key), engine().luaFrom(value));
}

template <typename T>
void LuaTable::rawSet(char const* key, T value) const {
  engine().tableSet(true, handleIndex(), engine().luaFrom(key), engine().luaFrom(value));
}

template <typename Ret, typename... Args>
Ret LuaFunction::invoke(Args const&... args) const {
  return LuaDetail::FromFunctionReturn<Ret>::convert(engine(), engine().callFunction(handleIndex(), args...));
}

template <typename Ret, typename... Args>
Maybe<Ret> LuaThread::resume(Args const&... args) const {
  auto res = engine().resumeThread(handleIndex(), args...);
  if (!res)
    return {};
  return LuaDetail::FromFunctionReturn<Ret>::convert(engine(), res.take());
}

inline void LuaThread::pushFunction(LuaFunction const& func) const {
  engine().threadPushFunction(handleIndex(), func.handleIndex());
}

inline LuaThread::Status LuaThread::status() const {
  return engine().threadStatus(handleIndex());
}

template <typename T>
bool LuaUserData::is() const {
  return engine().userDataIsType<T>(handleIndex());
}

template <typename T>
T& LuaUserData::get() const {
  return *engine().getUserData<T>(handleIndex());
}

template <typename Function>
void LuaCallbacks::registerCallback(String name, Function&& func) {
  if (!m_callbacks.insert(name, LuaDetail::wrapFunction(std::forward<Function>(func))).second)
    throw LuaException::format("Lua callback '{}' was registered twice", name);
}

template <typename Return, typename... Args, typename Function>
void LuaCallbacks::registerCallbackWithSignature(String name, Function&& func) {
  if (!m_callbacks.insert(name, LuaDetail::wrapFunctionWithSignature<Return, Args...>(std::forward<Function>(func))).second)
    throw LuaException::format("Lua callback '{}' was registered twice", name);
}

template <typename T>
template <typename Function>
void LuaMethods<T>::registerMethod(String name, Function&& func) {
  if (!m_methods.insert(name, LuaDetail::wrapMethod(std::forward<Function>(std::move(func)))).second)
    throw LuaException::format("Lua method '{}' was registered twice", name);
}

template <typename T>
template <typename Return, typename... Args, typename Function>
void LuaMethods<T>::registerMethodWithSignature(String name, Function&& func) {
  if (!m_methods.insert(name, LuaDetail::wrapMethodWithSignature<Return, Args...>(std::forward<Function>(std::move(func))))
           .second)
    throw LuaException::format("Lua method '{}' was registered twice", name);
}

template <typename T>
StringMap<LuaDetail::LuaWrappedFunction> const& LuaMethods<T>::methods() const {
  return m_methods;
}

template <typename T>
T LuaContext::getPath(String path) const {
  return engine().luaTo<T>(engine().contextGetPath(handleIndex(), std::move(path)));
}

template <typename T>
void LuaContext::setPath(String key, T value) {
  engine().contextSetPath(handleIndex(), std::move(key), engine().luaFrom<T>(std::move(value)));
}

template <typename Ret>
Ret LuaContext::eval(String const& lua) {
  return LuaDetail::FromFunctionReturn<Ret>::convert(engine(), engine().contextEval(handleIndex(), lua));
}

template <typename Ret, typename... Args>
Ret LuaContext::invokePath(String const& key, Args const&... args) const {
  auto p = getPath(key);
  if (auto f = p.ptr<LuaFunction>())
    return f->invoke<Ret>(args...);
  throw LuaException::format("invokePath called on path '{}' which is not function type", key);
}

template <typename T>
LuaValue LuaContext::luaFrom(T&& t) {
  return engine().luaFrom(std::forward<T>(t));
}

template <typename T>
LuaValue LuaContext::luaFrom(T const& t) {
  return engine().luaFrom(t);
}

template <typename T>
Maybe<T> LuaContext::luaMaybeTo(LuaValue&& v) {
  return engine().luaFrom(std::move(v));
}

template <typename T>
Maybe<T> LuaContext::luaMaybeTo(LuaValue const& v) {
  return engine().luaFrom(v);
}

template <typename T>
T LuaContext::luaTo(LuaValue&& v) {
  return engine().luaTo<T>(std::move(v));
}

template <typename T>
T LuaContext::luaTo(LuaValue const& v) {
  return engine().luaTo<T>(v);
}

template <typename Container>
LuaTable LuaContext::createTable(Container const& map) {
  return engine().createTable(map);
}

template <typename Container>
LuaTable LuaContext::createArrayTable(Container const& array) {
  return engine().createArrayTable(array);
}

template <typename Function>
LuaFunction LuaContext::createFunction(Function&& func) {
  return engine().createFunction(std::forward<Function>(func));
}

template <typename Return, typename... Args, typename Function>
LuaFunction LuaContext::createFunctionWithSignature(Function&& func) {
  return engine().createFunctionWithSignature<Return, Args...>(std::forward<Function>(func));
}

template <typename T>
LuaUserData LuaContext::createUserData(T t) {
  return engine().createUserData(std::move(t));
}

template <typename T>
LuaMethods<T> LuaUserDataMethods<T>::make() {
  return LuaMethods<T>();
}

template <typename T>
LuaValue LuaUserDataConverter<T>::from(LuaEngine& engine, T t) {
  return engine.createUserData(std::move(t));
}

template <typename T>
Maybe<T> LuaUserDataConverter<T>::to(LuaEngine&, LuaValue const& v) {
  if (auto ud = v.ptr<LuaUserData>()) {
    if (ud->is<T>())
      return ud->get<T>();
  }
  return {};
}

template <typename T>
LuaValue LuaEngine::luaFrom(T&& t) {
  return LuaConverter<typename std::decay<T>::type>::from(*this, std::forward<T>(t));
}

template <typename T>
LuaValue LuaEngine::luaFrom(T const& t) {
  return LuaConverter<T>::from(*this, t);
}

template <typename T>
Maybe<T> LuaEngine::luaMaybeTo(LuaValue&& v) {
  return LuaConverter<T>::to(*this, std::move(v));
}

template <typename T>
Maybe<T> LuaEngine::luaMaybeTo(LuaValue const& v) {
  return LuaConverter<T>::to(*this, v);
}

template <typename T>
T LuaEngine::luaTo(LuaValue&& v) {
  if (auto res = luaMaybeTo<T>(std::move(v)))
    return res.take();
  throw LuaConversionException::format("Error converting LuaValue to type '{}'", typeid(T).name());
}

template <typename T>
T LuaEngine::luaTo(LuaValue const& v) {
  if (auto res = luaMaybeTo<T>(v))
    return res.take();
  throw LuaConversionException::format("Error converting LuaValue to type '{}'", typeid(T).name());
}

template <typename Container>
LuaTable LuaEngine::createTable(Container const& map) {
  auto table = createTable(0, map.size());
  for (auto const& p : map)
    table.set(p.first, p.second);
  return table;
}

template <typename Container>
LuaTable LuaEngine::createArrayTable(Container const& array) {
  auto table = createTable(array.size(), 0);
  int i = 1;
  for (auto const& elem : array) {
    table.set(LuaInt(i), elem);
    ++i;
  }
  return table;
}

template <typename Function>
LuaFunction LuaEngine::createFunction(Function&& func) {
  return createWrappedFunction(LuaDetail::wrapFunction(std::forward<Function>(func)));
}

template <typename Return, typename... Args, typename Function>
LuaFunction LuaEngine::createFunctionWithSignature(Function&& func) {
  return createWrappedFunction(LuaDetail::wrapFunctionWithSignature<Return, Args...>(std::forward<Function>(func)));
}

template <typename... Args>
LuaDetail::LuaFunctionReturn LuaEngine::callFunction(int handleIndex, Args const&... args) {
  lua_checkstack(m_state, 1);

  int stackSize = lua_gettop(m_state);
  pushHandle(m_state, handleIndex);

  size_t argSize = pushArguments(m_state, args...);

  incrementRecursionLevel();
  int res = pcallWithTraceback(m_state, argSize, LUA_MULTRET);
  decrementRecursionLevel();
  handleError(m_state, res);

  int returnValues = lua_gettop(m_state) - stackSize;
  if (returnValues == 0) {
    return LuaDetail::LuaFunctionReturn();
  } else if (returnValues == 1) {
    return popLuaValue(m_state);
  } else {
    LuaVariadic<LuaValue> ret(returnValues);
    for (int i = returnValues - 1; i >= 0; --i)
      ret[i] = popLuaValue(m_state);
    return ret;
  }
}

template <typename... Args>
Maybe<LuaDetail::LuaFunctionReturn> LuaEngine::resumeThread(int handleIndex, Args const&... args) {
  lua_checkstack(m_state, 1);

  pushHandle(m_state, handleIndex);
  lua_State* threadState = lua_tothread(m_state, -1);
  lua_pop(m_state, 1);

  if (lua_status(threadState) != LUA_YIELD && lua_gettop(threadState) == 0) {
    throw LuaException("cannot resume a dead or errored thread");
  }

  size_t argSize = pushArguments(threadState, args...);
  incrementRecursionLevel();
  int res = lua_resume(threadState, nullptr, argSize);
  decrementRecursionLevel();
  if (res != LUA_OK && res != LUA_YIELD) {
    propagateErrorWithTraceback(threadState, m_state);
    handleError(m_state, res);
  }

  int returnValues = lua_gettop(threadState);
  if (returnValues == 0) {
    return LuaDetail::LuaFunctionReturn();
  } else if (returnValues == 1) {
    return LuaDetail::LuaFunctionReturn(popLuaValue(threadState));
  } else {
    LuaVariadic<LuaValue> ret(returnValues);
    for (int i = returnValues - 1; i >= 0; --i)
      ret[i] = popLuaValue(threadState);
    return LuaDetail::LuaFunctionReturn(std::move(ret));
  }
}

template <typename T>
void LuaEngine::registerUserDataType() {
  if (m_registeredUserDataTypes.contains(typeid(T)))
    return;

  lua_checkstack(m_state, 2);

  lua_newtable(m_state);

  // Set the __index on the metatable to itself
  lua_pushvalue(m_state, -1);
  LuaDetail::rawSetField(m_state, -2, "__index");
  lua_pushboolean(m_state, 0);
  LuaDetail::rawSetField(m_state, -2, "__metatable"); // protect metatable

  // Set the __gc function to the userdata destructor
  auto gcFunction = [](lua_State* state) {
    T& t = *(T*)(lua_touserdata(state, 1));
    t.~T();
    return 0;
  };
  lua_pushcfunction(m_state, gcFunction);
  LuaDetail::rawSetField(m_state, -2, "__gc");

  auto methods = LuaUserDataMethods<T>::make();
  for (auto& p : methods.methods()) {
    pushLuaValue(m_state, createWrappedFunction(p.second));
    LuaDetail::rawSetField(m_state, -2, p.first.utf8Ptr());
  }

  m_registeredUserDataTypes.add(typeid(T), luaL_ref(m_state, LUA_REGISTRYINDEX));
}

template <typename T>
LuaUserData LuaEngine::createUserData(T t) {
  registerUserDataType<T>();

  int typeMetatable = m_registeredUserDataTypes.get(typeid(T));

  lua_checkstack(m_state, 2);

  new (lua_newuserdata(m_state, sizeof(T))) T(std::move(t));

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, typeMetatable);
  lua_setmetatable(m_state, -2);

  return LuaUserData(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

template <typename T, typename K>
T LuaEngine::getGlobal(K key) {
  lua_checkstack(m_state, 1);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_scriptDefaultEnvRegistryId);
  pushLuaValue(m_state, luaFrom(std::move(key)));
  lua_rawget(m_state, -2);

  LuaValue v = popLuaValue(m_state);
  lua_pop(m_state, 1);

  return luaTo<T>(v);
}

template <typename T>
T LuaEngine::getGlobal(char const* key) {
  lua_checkstack(m_state, 1);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_scriptDefaultEnvRegistryId);
  lua_getfield(m_state, -1, key);

  LuaValue v = popLuaValue(m_state);
  lua_pop(m_state, 1);

  return luaTo<T>(v);
}

template <typename T, typename K>
void LuaEngine::setGlobal(K key, T value) {
  lua_checkstack(m_state, 1);

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_scriptDefaultEnvRegistryId);
  pushLuaValue(m_state, luaFrom(std::move(key)));
  pushLuaValue(m_state, luaFrom(std::move(value)));

  lua_rawset(m_state, -3);
  lua_pop(m_state, 1);
}

template <typename T>
void LuaEngine::setGlobal(char const* key, T value) {
  lua_checkstack(m_state, 1);

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_scriptDefaultEnvRegistryId);
  pushLuaValue(m_state, value);

  lua_setfield(m_state, -2, key);
  lua_pop(m_state, 1);
}

template <typename T>
bool LuaEngine::userDataIsType(int handleIndex) {
  int typeRef = m_registeredUserDataTypes.value(typeid(T), LUA_NOREF);
  if (typeRef == LUA_NOREF)
    return false;

  lua_checkstack(m_state, 3);

  pushHandle(m_state, handleIndex);
  if (lua_getmetatable(m_state, -1) == 0) {
    lua_pop(m_state, 1);
    throw LuaException("Userdata missing metatable in userDataIsType");
  }

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, typeRef);
  bool typesEqual = lua_rawequal(m_state, -1, -2);
  lua_pop(m_state, 3);

  return typesEqual;
}

template <typename T>
T* LuaEngine::getUserData(int handleIndex) {
  int typeRef = m_registeredUserDataTypes.value(typeid(T), LUA_NOREF);
  if (typeRef == LUA_NOREF)
    throw LuaException::format("Cannot convert userdata type of {}, not registered", typeid(T).name());

  lua_checkstack(m_state, 3);

  pushHandle(m_state, handleIndex);
  T* userdata = (T*)lua_touserdata(m_state, -1);
  if (lua_getmetatable(m_state, -1) == 0) {
    lua_pop(m_state, 1);
    throw LuaException("Cannot get userdata from lua type, no metatable found");
  }

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, typeRef);
  if (!lua_rawequal(m_state, -1, -2)) {
    lua_pop(m_state, 3);
    throw LuaException::format("Improper conversion from userdata to type {}", typeid(T).name());
  }

  lua_pop(m_state, 3);

  return userdata;
}

inline void LuaEngine::destroyHandle(int handleIndex) {
  // We don't bother setting the entry in the handle stack to nil, we just wait
  // for it to be reused and overwritten.  We could provide a way to
  // periodically ensure that the entire free list is niled out if this becomes
  // a memory issue?
  m_handleFree.append(handleIndex);
}

template <typename T>
size_t LuaEngine::pushArgument(lua_State* state, T const& arg) {
  pushLuaValue(state, luaFrom(arg));
  return 1;
}

template <typename T>
size_t LuaEngine::pushArgument(lua_State* state, LuaVariadic<T> const& args) {
  // If the argument list was empty then we've checked one extra space on the
  // stack, oh well.
  if (args.empty())
    return 0;

  // top-level pushArguments does a stack check of the total size of the
  // argument list, for variadic arguments, it could be more than one
  // argument so check the stack for the arguments in the variadic list minus
  // one.
  lua_checkstack(state, args.size() - 1);
  for (size_t i = 0; i < args.size(); ++i)
    pushLuaValue(state, luaFrom(args[i]));
  return args.size();
}

inline size_t LuaEngine::doPushArguments(lua_State*) {
  return 0;
}

template <typename First, typename... Rest>
size_t LuaEngine::doPushArguments(lua_State* state, First const& first, Rest const&... rest) {
  size_t s = pushArgument(state, first);
  return s + doPushArguments(state, rest...);
}

template <typename... Args>
size_t LuaEngine::pushArguments(lua_State* state, Args const&... args) {
  lua_checkstack(state, sizeof...(args));
  return doPushArguments(state, args...);
}

}

template <> struct fmt::formatter<Star::LuaValue> : ostream_formatter {};
