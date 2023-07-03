#include "StarLua.hpp"
#include "StarArray.hpp"
#include "StarTime.hpp"

namespace Star {

std::ostream& operator<<(std::ostream& os, LuaValue const& value) {
  if (value.is<LuaBoolean>()) {
    os << (value.get<LuaBoolean>() ? "true" : "false");
  } else if (value.is<LuaInt>()) {
    os << value.get<LuaInt>();
  } else if (value.is<LuaFloat>()) {
    os << value.get<LuaFloat>();
  } else if (value.is<LuaString>()) {
    os << value.get<LuaString>().ptr();
  } else if (value.is<LuaTable>()) {
    os << "{";
    bool first = true;
    value.get<LuaTable>().iterate([&os, &first](LuaValue const& key, LuaValue const& value) {
      if (first)
        first = false;
      else
        os << ", ";
      os << key << ": " << value;
    });
    os << "}";
  } else if (value.is<LuaFunction>()) {
    os << "<function reg:" << value.get<LuaFunction>().handleIndex() << ">";
  } else if (value.is<LuaThread>()) {
    os << "<thread reg:" << value.get<LuaThread>().handleIndex() << ">";
  } else if (value.is<LuaUserData>()) {
    os << "<userdata reg:" << value.get<LuaUserData>().handleIndex() << ">";
  } else {
    os << "nil";
  }
  return os;
}

bool LuaTable::contains(char const* key) const {
  return engine().tableGet(false, handleIndex(), key) != LuaNil;
}

void LuaTable::remove(char const* key) const {
  engine().tableSet(false, handleIndex(), key, LuaNil);
}

LuaInt LuaTable::length() const {
  return engine().tableLength(false, handleIndex());
}

Maybe<LuaTable> LuaTable::getMetatable() const {
  return engine().tableGetMetatable(handleIndex());
}

void LuaTable::setMetatable(LuaTable const& table) const {
  return engine().tableSetMetatable(handleIndex(), table);
}

LuaInt LuaTable::rawLength() const {
  return engine().tableLength(true, handleIndex());
}

LuaCallbacks& LuaCallbacks::merge(LuaCallbacks const& callbacks) {
  try {
    for (auto const& pair : callbacks.m_callbacks)
      m_callbacks.add(pair.first, pair.second);
  } catch (MapException const& e) {
    throw LuaException(strf("Failed to merge LuaCallbacks: {}", outputException(e, true)));
  }

  return *this;
}

StringMap<LuaDetail::LuaWrappedFunction> const& LuaCallbacks::callbacks() const {
  return m_callbacks;
}

bool LuaContext::containsPath(String path) const {
  return engine().contextGetPath(handleIndex(), move(path)) != LuaNil;
}

void LuaContext::load(char const* contents, size_t size, char const* name) {
  engine().contextLoad(handleIndex(), contents, size, name);
}

void LuaContext::load(String const& contents, String const& name) {
  load(contents.utf8Ptr(), contents.utf8Size(), name.utf8Ptr());
}

void LuaContext::load(ByteArray const& contents, String const& name) {
  load(contents.ptr(), contents.size(), name.utf8Ptr());
}

void LuaContext::setRequireFunction(RequireFunction requireFunction) {
  engine().setContextRequire(handleIndex(), move(requireFunction));
}

void LuaContext::setCallbacks(String const& tableName, LuaCallbacks const& callbacks) const {
  auto& eng = engine();
  auto callbackTable = eng.createTable();
  for (auto const& p : callbacks.callbacks())
    callbackTable.set(p.first, eng.createWrappedFunction(p.second));
  LuaContext::set(tableName, callbackTable);
}

LuaString LuaContext::createString(String const& str) {
  return engine().createString(str);
}

LuaString LuaContext::createString(char const* str) {
  return engine().createString(str);
}

LuaTable LuaContext::createTable() {
  return engine().createTable();
}

LuaValue LuaConverter<Json>::from(LuaEngine& engine, Json const& v) {
  if (v.isType(Json::Type::Null)) {
    return LuaNil;
  } else if (v.isType(Json::Type::Float)) {
    return LuaFloat(v.toDouble());
  } else if (v.isType(Json::Type::Bool)) {
    return v.toBool();
  } else if (v.isType(Json::Type::Int)) {
    return LuaInt(v.toInt());
  } else if (v.isType(Json::Type::String)) {
    return engine.createString(*v.stringPtr());
  } else {
    return LuaDetail::jsonContainerToTable(engine, v);
  }
}

Maybe<Json> LuaConverter<Json>::to(LuaEngine&, LuaValue const& v) {
  if (v == LuaNil)
    return Json();

  if (auto b = v.ptr<LuaBoolean>())
    return Json(*b);

  if (auto i = v.ptr<LuaInt>())
    return Json(*i);

  if (auto f = v.ptr<LuaFloat>())
    return Json(*f);

  if (auto s = v.ptr<LuaString>())
    return Json(s->ptr());

  if (v.is<LuaTable>())
    return LuaDetail::tableToJsonContainer(v.get<LuaTable>());

  return {};
}

LuaValue LuaConverter<JsonObject>::from(LuaEngine& engine, JsonObject v) {
  return engine.luaFrom<Json>(Json(move(v)));
}

Maybe<JsonObject> LuaConverter<JsonObject>::to(LuaEngine& engine, LuaValue v) {
  auto j = engine.luaTo<Json>(move(v));
  if (j.type() == Json::Type::Object) {
    return j.toObject();
  } else if (j.type() == Json::Type::Array) {
    auto list = j.arrayPtr();
    if (list->empty())
      return JsonObject();
  }

  return {};
}

LuaValue LuaConverter<JsonArray>::from(LuaEngine& engine, JsonArray v) {
  return engine.luaFrom<Json>(Json(move(v)));
}

Maybe<JsonArray> LuaConverter<JsonArray>::to(LuaEngine& engine, LuaValue v) {
  auto j = engine.luaTo<Json>(move(v));
  if (j.type() == Json::Type::Array) {
    return j.toArray();
  } else if (j.type() == Json::Type::Object) {
    auto map = j.objectPtr();
    if (map->empty())
      return JsonArray();
  }

  return {};
}

LuaEnginePtr LuaEngine::create(bool safe) {
  LuaEnginePtr self(new LuaEngine);

  self->m_state = lua_newstate(allocate, nullptr);

  self->m_scriptDefaultEnvRegistryId = LUA_NOREF;
  self->m_wrappedFunctionMetatableRegistryId = LUA_NOREF;
  self->m_requireFunctionMetatableRegistryId = LUA_NOREF;

  self->m_instructionLimit = 0;
  self->m_profilingEnabled = false;
  self->m_instructionMeasureInterval = 1000;
  self->m_instructionCount = 0;
  self->m_recursionLevel = 0;
  self->m_recursionLimit = 0;

  if (!self->m_state)
    throw LuaException("Failed to initialize Lua");

  lua_checkstack(self->m_state, 5);

  // Create handle stack thread and place it in the registry to prevent it from being garbage
  // collected.

  self->m_handleThread = lua_newthread(self->m_state);
  luaL_ref(self->m_state, LUA_REGISTRYINDEX);

  // We need 1 extra stack space to move values in and out of the handle stack.
  self->m_handleStackSize = LUA_MINSTACK - 1;
  self->m_handleStackMax = 0;

  // Set the extra space in the lua main state to the pointer to the main
  // LuaEngine
  *reinterpret_cast<LuaEngine**>(lua_getextraspace(self->m_state)) = self.get();

  // Create the common message handler function for pcall to print a better
  // message with a traceback
  lua_pushcfunction(self->m_state, [](lua_State* state) {
      // Don't modify the error if it is one of the special limit errrors
      if (lua_islightuserdata(state, 1)) {
        void* error = lua_touserdata(state, -1);
        if (error == &s_luaInstructionLimitExceptionKey || error == &s_luaRecursionLimitExceptionKey)
          return 1;
      }

      luaL_traceback(state, state, lua_tostring(state, 1), 0);
      lua_remove(state, 1);
      return 1;
    });
  self->m_pcallTracebackMessageHandlerRegistryId = luaL_ref(self->m_state, LUA_REGISTRYINDEX);

  // Create the common metatable for wrapped functions
  lua_newtable(self->m_state);
  lua_pushcfunction(self->m_state, [](lua_State* state) {
      auto func = (LuaDetail::LuaWrappedFunction*)lua_touserdata(state, 1);
      func->~function();
      return 0;
    });
  LuaDetail::rawSetField(self->m_state, -2, "__gc");
  lua_pushboolean(self->m_state, 0);
  LuaDetail::rawSetField(self->m_state, -2, "__metatable");
  self->m_wrappedFunctionMetatableRegistryId = luaL_ref(self->m_state, LUA_REGISTRYINDEX);

  // Create the common metatable for require functions
  lua_newtable(self->m_state);
  lua_pushcfunction(self->m_state, [](lua_State* state) {
      auto func = (LuaContext::RequireFunction*)lua_touserdata(state, 1);
      func->~function();
      return 0;
    });
  LuaDetail::rawSetField(self->m_state, -2, "__gc");
  lua_pushboolean(self->m_state, 0);
  LuaDetail::rawSetField(self->m_state, -2, "__metatable");
  self->m_requireFunctionMetatableRegistryId = luaL_ref(self->m_state, LUA_REGISTRYINDEX);

  // Load all base libraries and prune them of unsafe functions

  luaL_requiref(self->m_state, "_ENV", luaopen_base, true);
  if (safe) {
    StringSet baseWhitelist = {
        "assert",
        "error",
        "getmetatable",
        "ipairs",
        "next",
        "pairs",
        "pcall",
        "print",
        "rawequal",
        "rawget",
        "rawlen",
        "rawset",
        "select",
        "setmetatable",
        "tonumber",
        "tostring",
        "type",
        "unpack",
        "_VERSION",
        "xpcall"};

    lua_pushnil(self->m_state);
    while (lua_next(self->m_state, -2) != 0) {
      lua_pop(self->m_state, 1);
      String key(lua_tostring(self->m_state, -1));

      if (!baseWhitelist.contains(key)) {
        lua_pushvalue(self->m_state, -1);
        lua_pushnil(self->m_state);
        lua_rawset(self->m_state, -4);
      }
    }
  }
  lua_pop(self->m_state, 1);

  luaL_requiref(self->m_state, "os", luaopen_os, true);
  if (safe) {
    StringSet osWhitelist = {"clock", "difftime", "time"};

    lua_pushnil(self->m_state);
    while (lua_next(self->m_state, -2) != 0) {
      lua_pop(self->m_state, 1);
      String key(lua_tostring(self->m_state, -1));

      if (!osWhitelist.contains(key)) {
        lua_pushvalue(self->m_state, -1);
        lua_pushnil(self->m_state);
        lua_rawset(self->m_state, -4);
      }
    }
  }
  lua_pop(self->m_state, 1);
  
  // loads a lua base library, leaves it at the top of the stack
  auto loadBaseLibrary = [](lua_State* state, char const* modname, lua_CFunction openf) {
    luaL_requiref(state, modname, openf, true);
    
    // set __metatable metamethod to false
    // otherwise scripts can access and mutate the metatable, allowing passing values
    // between script contexts, breaking the sandbox
    lua_newtable(state);
    lua_pushliteral(state, "__metatable");
    lua_pushboolean(state, 0);
    lua_rawset(state, -3);
    lua_setmetatable(state, -2);
  };

  loadBaseLibrary(self->m_state, "coroutine", luaopen_coroutine);
  // replace coroutine resume with one that appends tracebacks
  lua_pushliteral(self->m_state, "resume");
  lua_pushcfunction(self->m_state, &LuaEngine::coresumeWithTraceback);
  lua_rawset(self->m_state, -3);

  loadBaseLibrary(self->m_state, "math", luaopen_math);
  loadBaseLibrary(self->m_state, "string", luaopen_string);
  loadBaseLibrary(self->m_state, "table", luaopen_table);
  loadBaseLibrary(self->m_state, "utf8", luaopen_utf8);
  lua_pop(self->m_state, 5);

  if (!safe) {
    loadBaseLibrary(self->m_state, "io", luaopen_io);
    loadBaseLibrary(self->m_state, "package", luaopen_package);
    loadBaseLibrary(self->m_state, "debug", luaopen_debug);
    lua_pop(self->m_state, 3);
  }

  // Make a shallow copy of the default script environment and save it for
  // resetting the global state.
  lua_rawgeti(self->m_state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  lua_newtable(self->m_state);
  LuaDetail::shallowCopy(self->m_state, -2, -1);
  self->m_scriptDefaultEnvRegistryId = luaL_ref(self->m_state, LUA_REGISTRYINDEX);
  lua_pop(self->m_state, 1);

  self->setGlobal("jarray", self->createFunction(&LuaDetail::jarrayCreate));
  self->setGlobal("jobject", self->createFunction(&LuaDetail::jobjectCreate));
  self->setGlobal("jremove", self->createFunction(&LuaDetail::jcontRemove));
  self->setGlobal("jsize", self->createFunction(&LuaDetail::jcontSize));
  self->setGlobal("jresize", self->createFunction(&LuaDetail::jcontResize));

  self->setGlobal("shared", self->createTable());
  return self;
}

LuaEngine::~LuaEngine() {
  // If we've had a stack space leak, this will not be zero
  starAssert(lua_gettop(m_state) == 0);
  lua_close(m_state);
}

void LuaEngine::setInstructionLimit(uint64_t instructionLimit) {
  if (instructionLimit != m_instructionLimit) {
    m_instructionLimit = instructionLimit;
    updateCountHook();
  }
}

uint64_t LuaEngine::instructionLimit() const {
  return m_instructionLimit;
}

void LuaEngine::setProfilingEnabled(bool profilingEnabled) {
  if (profilingEnabled != m_profilingEnabled) {
    m_profilingEnabled = profilingEnabled;
    m_profileEntries.clear();
    updateCountHook();
  }
}

bool LuaEngine::profilingEnabled() const {
  return m_profilingEnabled;
}

List<LuaProfileEntry> LuaEngine::getProfile() {
  List<LuaProfileEntry> profileEntries;
  for (auto const& p : m_profileEntries) {
    profileEntries.append(*p.second);
  }

  return profileEntries;
}

void LuaEngine::setInstructionMeasureInterval(unsigned measureInterval) {
  if (measureInterval != m_instructionMeasureInterval) {
    m_instructionMeasureInterval = measureInterval;
    updateCountHook();
  }
}

unsigned LuaEngine::instructionMeasureInterval() const {
  return m_instructionMeasureInterval;
}

void LuaEngine::setRecursionLimit(unsigned recursionLimit) {
  m_recursionLimit = recursionLimit;
}

unsigned LuaEngine::recursionLimit() const {
  return m_recursionLimit;
}

ByteArray LuaEngine::compile(char const* contents, size_t size, char const* name) {
  lua_checkstack(m_state, 1);

  handleError(m_state, luaL_loadbuffer(m_state, contents, size, name));

  ByteArray compiledScript;
  lua_Writer writer = [](lua_State*, void const* data, size_t size, void* byteArrayPtr) -> int {
    ((ByteArray*)byteArrayPtr)->append((char const*)data, size);
    return 0;
  };
  lua_dump(m_state, writer, &compiledScript, false);
  lua_pop(m_state, 1);

  return compiledScript;
}

ByteArray LuaEngine::compile(String const& contents, String const& name) {
  return compile(contents.utf8Ptr(), contents.utf8Size(), name.empty() ? nullptr : name.utf8Ptr());
}

ByteArray LuaEngine::compile(ByteArray const& contents, String const& name) {
  return compile(contents.ptr(), contents.size(), name.empty() ? nullptr : name.utf8Ptr());
}

LuaString LuaEngine::createString(String const& str) {
  lua_checkstack(m_state, 1);

  lua_pushlstring(m_state, str.utf8Ptr(), str.utf8Size());
  return LuaString(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

LuaString LuaEngine::createString(char const* str) {
  lua_checkstack(m_state, 1);

  lua_pushstring(m_state, str);
  return LuaString(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

LuaTable LuaEngine::createTable(int narr, int nrec) {
  lua_checkstack(m_state, 1);

  lua_createtable(m_state, narr, nrec);
  return LuaTable(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

LuaThread LuaEngine::createThread() {
  lua_checkstack(m_state, 1);

  lua_newthread(m_state);
  return LuaThread(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

void LuaEngine::threadPushFunction(int threadIndex, int functionIndex) {
  lua_State* thread = lua_tothread(m_handleThread, threadIndex);

  int status = lua_status(thread);
  lua_Debug ar;
  if (status != LUA_OK || lua_getstack(thread, 0, &ar) > 0 || lua_gettop(thread) > 0)
    throw LuaException(strf("Cannot push function to active or errored thread with status {}", status));

  pushHandle(thread, functionIndex);
}

LuaThread::Status LuaEngine::threadStatus(int handleIndex) {
  lua_State* thread = lua_tothread(m_handleThread, handleIndex);

  int status = lua_status(thread);
  if (status != LUA_OK && status != LUA_YIELD)
    return LuaThread::Status::Error;

  lua_Debug ar;
  if (status == LUA_YIELD || lua_getstack(thread, 0, &ar) > 0 || lua_gettop(thread) > 0)
    return LuaThread::Status::Active;

  return LuaThread::Status::Dead;
}

LuaContext LuaEngine::createContext() {
  lua_checkstack(m_state, 2);

  // Create a new blank environment and copy the default environment to it.
  lua_newtable(m_state);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_scriptDefaultEnvRegistryId);
  LuaDetail::shallowCopy(m_state, -1, -2);
  lua_pop(m_state, 1);

  // Then set that environment as the new context environment in the registry.
  return LuaContext(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

void LuaEngine::collectGarbage(Maybe<unsigned> steps) {
  for (auto handleIndex : take(m_handleFree)) {
    lua_pushnil(m_handleThread);
    lua_replace(m_handleThread, handleIndex);
  }

  if (steps)
    lua_gc(m_state, LUA_GCSTEP, *steps);
  else
    lua_gc(m_state, LUA_GCCOLLECT, 0);
}

void LuaEngine::setAutoGarbageCollection(bool autoGarbageColleciton) {
  lua_gc(m_state, LUA_GCSTOP, autoGarbageColleciton ? 1 : 0);
}

void LuaEngine::tuneAutoGarbageCollection(float pause, float stepMultiplier) {
  lua_gc(m_state, LUA_GCSETPAUSE, round(pause * 100));
  lua_gc(m_state, LUA_GCSETSTEPMUL, round(stepMultiplier * 100));
}

size_t LuaEngine::memoryUsage() const {
  return (size_t)lua_gc(m_state, LUA_GCCOUNT, 0) * 1024 + lua_gc(m_state, LUA_GCCOUNTB, 0);
}

LuaEngine* LuaEngine::luaEnginePtr(lua_State* state) {
  return (*reinterpret_cast<LuaEngine**>(lua_getextraspace(state)));
}

void LuaEngine::countHook(lua_State* state, lua_Debug* ar) {
  starAssert(ar->event == LUA_HOOKCOUNT);
  lua_checkstack(state, 4);

  auto self = luaEnginePtr(state);

  // If the instruction count is 0, that means in this sequence of calls,
  // we have not hit a debug hook yet.  Since we don't know the state of
  // the internal lua instruction counter at the start, we don't know how
  // many instructions have been executed, only that it is >= 1 and <=
  // m_instructionMeasureInterval, so we pick the low estimate.
  if (self->m_instructionCount == 0)
    self->m_instructionCount = 1;
  else
    self->m_instructionCount += self->m_instructionMeasureInterval;

  if (self->m_instructionLimit != 0 && self->m_instructionCount > self->m_instructionLimit) {
    lua_pushlightuserdata(state, &s_luaInstructionLimitExceptionKey);
    lua_error(state);
  }

  if (self->m_profilingEnabled) {
    // find bottom of the stack
    // ar will contain the stack info from the last call that returns 1
    int stackLevel = -1;
    while (lua_getstack(state, stackLevel + 1, ar) == 1)
      stackLevel++;

    shared_ptr<LuaProfileEntry> parentEntry = nullptr;
    while (true) {
      // Get the 'n' name info and 'S' source info
      if (lua_getinfo(state, "nS", ar) == 0)
        break;

      auto key = make_tuple(String(ar->short_src), (unsigned)ar->linedefined);
      auto& entryMap = parentEntry ? parentEntry->calls : self->m_profileEntries;
      if (!entryMap.contains(key)) {
        auto e = make_shared<LuaProfileEntry>();
        e->source = ar->short_src;
        e->sourceLine = ar->linedefined;
        entryMap.set(key, e);
      }
      auto entry = entryMap.get(key);

      // metadata
      if (!entry->name && ar->name)
        entry->name = String(ar->name);
      if (!entry->nameScope && String() != ar->namewhat)
        entry->nameScope = String(ar->namewhat);

      // Only add timeTaken to the self time for the function we are actually
      // in, not parent functions
      if (stackLevel == 0) {
        entry->totalTime += 1;
        entry->selfTime += 1;
      } else {
        entry->totalTime += 1;
      }

      parentEntry = entry;
      if (lua_getstack(state, --stackLevel, ar) == 0)
        break;
    }
  }
}

void* LuaEngine::allocate(void*, void* ptr, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    Star::free(ptr, oldSize);
    return nullptr;
  } else {
    return Star::realloc(ptr, newSize);
  }
}

void LuaEngine::handleError(lua_State* state, int res) {
  if (res != LUA_OK) {
    if (lua_islightuserdata(state, -1)) {
      void* error = lua_touserdata(state, -1);
      if (error == &s_luaInstructionLimitExceptionKey) {
        lua_pop(state, 1);
        throw LuaInstructionLimitReached();
      }
      if (error == &s_luaRecursionLimitExceptionKey) {
        lua_pop(state, 1);
        throw LuaRecursionLimitReached();
      }
    }

    String error;
    if (lua_isstring(state, -1))
      error = strf("Error code {}, {}", res, lua_tostring(state, -1));
    else
      error = strf("Error code {}, <unknown error>", res);

    lua_pop(state, 1);

    // This seems terrible, but as far as I can tell, this is exactly what the
    // stock lua repl does.
    if (error.endsWith("<eof>"))
      throw LuaIncompleteStatementException(error.takeUtf8());
    else
      throw LuaException(error.takeUtf8());
  }
}

int LuaEngine::pcallWithTraceback(lua_State* state, int nargs, int nresults) {
  int msghPosition = lua_gettop(state) - nargs;
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_pcallTracebackMessageHandlerRegistryId);
  lua_insert(state, msghPosition);
  int ret = lua_pcall(state, nargs, nresults, msghPosition);
  lua_remove(state, msghPosition);
  return ret;
}

int LuaEngine::coresumeWithTraceback(lua_State* state) {
  lua_State* co = lua_tothread(state, 1);
  if (!co) {
    lua_checkstack(state, 2);
    lua_pushboolean(state, 0);
    lua_pushliteral(state, "bad argument #1 to 'resume' (thread expected)");
    return 2;
  }

  int args = lua_gettop(state) - 1;
  lua_checkstack(co, args);
  if (lua_status(co) == LUA_OK && lua_gettop(co) == 0) {
    lua_checkstack(state, 2);
    lua_pushboolean(state, 0);
    lua_pushliteral(state, "cannot resume dead coroutine");
    return 2;
  }

  lua_xmove(state, co, args);
  int status = lua_resume(co, state, args);
  if (status == LUA_OK || status == LUA_YIELD) {
    int res = lua_gettop(co);
    lua_checkstack(state, res + 1);
    lua_pushboolean(state, 1);
    lua_xmove(co, state, res);
    return res + 1;
  } else {
    lua_checkstack(state, 2);
    lua_pushboolean(state, 0);
    propagateErrorWithTraceback(co, state);
    return 2;
  }
}

void LuaEngine::propagateErrorWithTraceback(lua_State* from, lua_State* to) {
  if (const char* error = lua_tostring(from, -1)) {
    luaL_traceback(to, from, error, 0); // error + traceback
    lua_pop(from, 1);
  } else {
    lua_xmove(from, to, 1); // just error, no traceback
  }
}

char const* LuaEngine::stringPtr(int handleIndex) {
  return lua_tostring(m_handleThread, handleIndex);
}

size_t LuaEngine::stringLength(int handleIndex) {
  size_t len = 0;
  lua_tolstring(m_handleThread, handleIndex, &len);
  return len;
}

LuaValue LuaEngine::tableGet(bool raw, int handleIndex, LuaValue const& key) {
  lua_checkstack(m_state, 1);

  pushHandle(m_state, handleIndex);
  pushLuaValue(m_state, key);
  if (raw)
    lua_rawget(m_state, -2);
  else
    lua_gettable(m_state, -2);

  LuaValue v = popLuaValue(m_state);
  lua_pop(m_state, 1);
  return v;
}

LuaValue LuaEngine::tableGet(bool raw, int handleIndex, char const* key) {
  lua_checkstack(m_state, 1);

  pushHandle(m_state, handleIndex);
  if (raw)
    LuaDetail::rawGetField(m_state, -1, key);
  else
    lua_getfield(m_state, -1, key);
  lua_remove(m_state, -2);
  return popLuaValue(m_state);
}

void LuaEngine::tableSet(bool raw, int handleIndex, LuaValue const& key, LuaValue const& value) {
  lua_checkstack(m_state, 1);

  pushHandle(m_state, handleIndex);
  pushLuaValue(m_state, key);
  pushLuaValue(m_state, value);

  if (raw)
    lua_rawset(m_state, -3);
  else
    lua_settable(m_state, -3);

  lua_pop(m_state, 1);
}

void LuaEngine::tableSet(bool raw, int handleIndex, char const* key, LuaValue const& value) {
  lua_checkstack(m_state, 1);
  pushHandle(m_state, handleIndex);
  pushLuaValue(m_state, value);

  if (raw)
    LuaDetail::rawSetField(m_state, -2, key);
  else
    lua_setfield(m_state, -2, key);

  lua_pop(m_state, 1);
}

LuaInt LuaEngine::tableLength(bool raw, int handleIndex) {
  if (raw) {
    return lua_rawlen(m_handleThread, handleIndex);

  } else {
    lua_checkstack(m_state, 1);
    pushHandle(m_state, handleIndex);
    lua_len(m_state, -1);
    LuaInt len = lua_tointeger(m_state, -1);
    lua_pop(m_state, 2);
    return len;
  }
}

void LuaEngine::tableIterate(int handleIndex, function<bool(LuaValue key, LuaValue value)> iterator) {
  lua_checkstack(m_state, 4);

  pushHandle(m_state, handleIndex);
  lua_pushnil(m_state);
  while (lua_next(m_state, -2) != 0) {
    lua_pushvalue(m_state, -2);
    LuaValue key = popLuaValue(m_state);
    LuaValue value = popLuaValue(m_state);
    bool cont = false;
    try {
      cont = iterator(move(key), move(value));
    } catch (...) {
      lua_pop(m_state, 2);
      throw;
    }
    if (!cont) {
      lua_pop(m_state, 1);
      break;
    }
  }

  lua_pop(m_state, 1);
}

Maybe<LuaTable> LuaEngine::tableGetMetatable(int handleIndex) {
  lua_checkstack(m_state, 2);

  pushHandle(m_state, handleIndex);
  if (lua_getmetatable(m_state, -1) == 0) {
    lua_pop(m_state, 1);
    return {};
  }
  LuaTable table = popLuaValue(m_state).get<LuaTable>();
  lua_pop(m_state, 1);
  return table;
}

void LuaEngine::tableSetMetatable(int handleIndex, LuaTable const& table) {
  lua_checkstack(m_state, 2);

  pushHandle(m_state, handleIndex);
  pushHandle(m_state, table.handleIndex());
  lua_setmetatable(m_state, -2);
  lua_pop(m_state, 1);
}

void LuaEngine::setContextRequire(int handleIndex, LuaContext::RequireFunction requireFunction) {
  lua_checkstack(m_state, 4);

  pushHandle(m_state, handleIndex);

  auto funcUserdata = (LuaContext::RequireFunction*)lua_newuserdata(m_state, sizeof(LuaContext::RequireFunction));
  new (funcUserdata) LuaContext::RequireFunction(move(requireFunction));
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_requireFunctionMetatableRegistryId);
  lua_setmetatable(m_state, -2);

  lua_pushvalue(m_state, -2);

  auto invokeRequire = [](lua_State* state) {
    try {
      lua_checkstack(state, 2);

      auto require = (LuaContext::RequireFunction*)lua_touserdata(state, lua_upvalueindex(1));
      auto self = luaEnginePtr(state);

      auto moduleName = self->luaTo<LuaString>(self->popLuaValue(state));

      lua_pushvalue(state, lua_upvalueindex(2));
      LuaContext context(LuaDetail::LuaHandle(RefPtr<LuaEngine>(self), self->popHandle(state)));

      (*require)(context, moduleName);
      return 0;
    } catch (LuaInstructionLimitReached const&) {
      lua_pushlightuserdata(state, &s_luaInstructionLimitExceptionKey);
      return lua_error(state);
    } catch (LuaRecursionLimitReached const&) {
      lua_pushlightuserdata(state, &s_luaRecursionLimitExceptionKey);
      return lua_error(state);
    } catch (std::exception const& e) {
      luaL_where(state, 1);
      lua_pushstring(state, printException(e, true).c_str());
      lua_concat(state, 2);
      return lua_error(state);
    }
  };

  lua_pushcclosure(m_state, invokeRequire, 2);

  LuaDetail::rawSetField(m_state, -2, "require");

  lua_pop(m_state, 1);
}

void LuaEngine::contextLoad(int handleIndex, char const* contents, size_t size, char const* name) {
  lua_checkstack(m_state, 2);

  // First load the script...
  handleError(m_state, luaL_loadbuffer(m_state, contents, size, name));

  // Then set the _ENV upvalue for the newly loaded chunk to our context env so
  // we load the scripts into the right environment.
  pushHandle(m_state, handleIndex);
  lua_setupvalue(m_state, -2, 1);

  incrementRecursionLevel();
  int res = pcallWithTraceback(m_state, 0, 0);
  decrementRecursionLevel();
  handleError(m_state, res);
}

LuaDetail::LuaFunctionReturn LuaEngine::contextEval(int handleIndex, String const& lua) {
  int stackSize = lua_gettop(m_state);
  lua_checkstack(m_state, 2);

  // First, try interpreting the lua as an expression by adding "return", then
  // as a statement.  This is the same thing the actual lua repl does.
  int loadRes = luaL_loadstring(m_state, ("return " + lua).utf8Ptr());
  if (loadRes == LUA_ERRSYNTAX) {
    lua_pop(m_state, 1);
    loadRes = luaL_loadstring(m_state, lua.utf8Ptr());
  }
  handleError(m_state, loadRes);

  pushHandle(m_state, handleIndex);
  lua_setupvalue(m_state, -2, 1);

  incrementRecursionLevel();
  int callRes = pcallWithTraceback(m_state, 0, LUA_MULTRET);
  decrementRecursionLevel();
  handleError(m_state, callRes);

  int returnValues = lua_gettop(m_state) - stackSize;
  if (returnValues == 0) {
    return LuaDetail::LuaFunctionReturn();
  } else if (returnValues == 1) {
    return LuaDetail::LuaFunctionReturn(popLuaValue(m_state));
  } else {
    LuaVariadic<LuaValue> ret(returnValues);
    for (int i = returnValues - 1; i >= 0; --i)
      ret[i] = popLuaValue(m_state);
    return LuaDetail::LuaFunctionReturn(ret);
  }
}

LuaValue LuaEngine::contextGetPath(int handleIndex, String path) {
  lua_checkstack(m_state, 2);
  pushHandle(m_state, handleIndex);

  std::string utf8Path = path.takeUtf8();
  char* utf8Ptr = &utf8Path[0];
  size_t utf8Size = utf8Path.size();

  size_t subPathStart = 0;
  for (size_t i = 0; i < utf8Size; ++i) {
    if (utf8Path[i] == '.') {
      utf8Path[i] = '\0';

      lua_getfield(m_state, -1, utf8Ptr + subPathStart);
      lua_remove(m_state, -2);

      if (lua_type(m_state, -1) != LUA_TTABLE) {
        lua_pop(m_state, 1);
        return LuaNil;
      }

      subPathStart = i + 1;
    }
  }

  lua_getfield(m_state, -1, utf8Ptr + subPathStart);
  lua_remove(m_state, -2);

  return popLuaValue(m_state);
}

void LuaEngine::contextSetPath(int handleIndex, String path, LuaValue const& value) {
  lua_checkstack(m_state, 3);
  pushHandle(m_state, handleIndex);

  std::string utf8Path = path.takeUtf8();
  char* utf8Ptr = &utf8Path[0];
  size_t utf8Size = utf8Path.size();

  size_t subPathStart = 0;
  for (size_t i = 0; i < utf8Size; ++i) {
    if (utf8Path[i] == '.') {
      utf8Path[i] = '\0';

      int type = lua_getfield(m_state, -1, utf8Ptr + subPathStart);
      if (type == LUA_TNIL) {
        lua_pop(m_state, 1);
        lua_newtable(m_state);
        lua_pushvalue(m_state, -1);
        lua_setfield(m_state, -3, utf8Ptr + subPathStart);
        lua_remove(m_state, -2);
      } else if (type == LUA_TTABLE) {
        lua_remove(m_state, -2);
      } else {
        lua_pop(m_state, 2);
        throw LuaException("Sub-path in setPath is not nil and is not a table");
      }

      subPathStart = i + 1;
    }
  }

  pushLuaValue(m_state, value);
  lua_setfield(m_state, -2, utf8Ptr + subPathStart);
  lua_pop(m_state, 1);
}

int LuaEngine::popHandle(lua_State* state) {
  lua_xmove(state, m_handleThread, 1);
  return placeHandle();
}

void LuaEngine::pushHandle(lua_State* state, int handleIndex) {
  lua_pushvalue(m_handleThread, handleIndex);
  lua_xmove(m_handleThread, state, 1);
}

int LuaEngine::copyHandle(int handleIndex) {
  lua_pushvalue(m_handleThread, handleIndex);
  return placeHandle();
}

int LuaEngine::placeHandle() {
  if (auto free = m_handleFree.maybeTakeLast()) {
    lua_replace(m_handleThread, *free);
    return *free;

  } else {
    if (m_handleStackMax >= m_handleStackSize) {
      if (!lua_checkstack(m_handleThread, m_handleStackSize)) {
        throw LuaException("Exhausted the size of the handle thread stack");
      }
      m_handleStackSize *= 2;
    }
    m_handleStackMax += 1;
    return m_handleStackMax;
  }
}

LuaFunction LuaEngine::createWrappedFunction(LuaDetail::LuaWrappedFunction function) {
  lua_checkstack(m_state, 2);

  auto funcUserdata = (LuaDetail::LuaWrappedFunction*)lua_newuserdata(m_state, sizeof(LuaDetail::LuaWrappedFunction));
  new (funcUserdata) LuaDetail::LuaWrappedFunction(move(function));

  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_wrappedFunctionMetatableRegistryId);
  lua_setmetatable(m_state, -2);

  auto invokeFunction = [](lua_State* state) {
    auto func = (LuaDetail::LuaWrappedFunction*)lua_touserdata(state, lua_upvalueindex(1));
    auto self = luaEnginePtr(state);

    int argumentCount = lua_gettop(state);
    try {
      // For speed, if the argument count is less than some pre-defined
      // value, use a stack array.
      int const MaxArrayArgs = 8;
      LuaDetail::LuaFunctionReturn res;
      if (argumentCount <= MaxArrayArgs) {
        Array<LuaValue, MaxArrayArgs> args;
        for (int i = argumentCount - 1; i >= 0; --i)
          args[i] = self->popLuaValue(state);
        res = (*func)(*self, argumentCount, args.ptr());
      } else {
        List<LuaValue> args(argumentCount);
        for (int i = argumentCount - 1; i >= 0; --i)
          args[i] = self->popLuaValue(state);
        res = (*func)(*self, argumentCount, args.ptr());
      }

      if (auto val = res.ptr<LuaValue>()) {
        self->pushLuaValue(state, *val);
        return 1;
      } else if (auto vec = res.ptr<LuaVariadic<LuaValue>>()) {
        for (auto const& r : *vec)
          self->pushLuaValue(state, r);
        return (int)vec->size();
      } else {
        return 0;
      }
    } catch (LuaInstructionLimitReached const&) {
      lua_pushlightuserdata(state, &s_luaInstructionLimitExceptionKey);
      return lua_error(state);
    } catch (LuaRecursionLimitReached const&) {
      lua_pushlightuserdata(state, &s_luaRecursionLimitExceptionKey);
      return lua_error(state);
    } catch (std::exception const& e) {
      luaL_where(state, 1);
      lua_pushstring(state, printException(e, true).c_str());
      lua_concat(state, 2);
      return lua_error(state);
    }
  };

  lua_pushcclosure(m_state, invokeFunction, 1);

  return LuaFunction(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

LuaFunction LuaEngine::createRawFunction(lua_CFunction function) {
  lua_checkstack(m_state, 2);

  lua_pushcfunction(m_state, function);
  return LuaFunction(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(m_state)));
}

void LuaEngine::pushLuaValue(lua_State* state, LuaValue const& luaValue) {
  lua_checkstack(state, 1);

  struct Pusher {
    LuaEngine* engine;
    lua_State* state;

    void operator()(LuaNilType const&) {
      lua_pushnil(state);
    }

    void operator()(LuaBoolean const& b) {
      lua_pushboolean(state, b);
    }

    void operator()(LuaInt const& i) {
      lua_pushinteger(state, i);
    }

    void operator()(LuaFloat const& f) {
      lua_pushnumber(state, f);
    }

    void operator()(LuaReference const& ref) {
      if (&ref.engine() != engine)
        throw LuaException("lua reference values cannot be shared between engines");
      engine->pushHandle(state, ref.handleIndex());
    }
  };

  luaValue.call(Pusher{this, state});
}

LuaValue LuaEngine::popLuaValue(lua_State* state) {
  lua_checkstack(state, 1);

  LuaValue result;
  starAssert(!lua_isnone(state, -1));
  switch (lua_type(state, -1)) {
    case LUA_TNIL: {
      lua_pop(state, 1);
      break;
    }
    case LUA_TBOOLEAN: {
      result = lua_toboolean(state, -1) != 0;
      lua_pop(state, 1);
      break;
    }
    case LUA_TNUMBER: {
      if (lua_isinteger(state, -1)) {
        result = lua_tointeger(state, -1);
        lua_pop(state, 1);
      } else {
        result = lua_tonumber(state, -1);
        lua_pop(state, 1);
      }
      break;
    }
    case LUA_TSTRING: {
      result = LuaString(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(state)));
      break;
    }
    case LUA_TTABLE: {
      result = LuaTable(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(state)));
      break;
    }
    case LUA_TFUNCTION: {
      result = LuaFunction(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(state)));
      break;
    }
    case LUA_TTHREAD: {
      result = LuaThread(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(state)));
      break;
    }
    case LUA_TUSERDATA: {
      if (lua_getmetatable(state, -1) == 0) {
        lua_pop(state, 1);
        throw LuaException("Userdata in popLuaValue missing metatable");
      }
      lua_pop(state, 1);
      result = LuaUserData(LuaDetail::LuaHandle(RefPtr<LuaEngine>(this), popHandle(state)));
      break;
    }
    default: {
      lua_pop(state, 1);
      throw LuaException("Unsupported type in popLuaValue");
    }
  }

  return result;
}

void LuaEngine::incrementRecursionLevel() {
  // We reset the instruction count and profiling timing only on the *top
  // level* function entrance, not on recursive entrances.
  if (m_recursionLevel == 0) {
    m_instructionCount = 0;
  }

  if (m_recursionLimit != 0 && m_recursionLevel == m_recursionLimit)
    throw LuaRecursionLimitReached();

  ++m_recursionLevel;
}

void LuaEngine::decrementRecursionLevel() {
  starAssert(m_recursionLevel != 0);
  --m_recursionLevel;
}

void LuaEngine::updateCountHook() {
  if (m_instructionLimit || m_profilingEnabled)
    lua_sethook(m_state, &LuaEngine::countHook, LUA_MASKCOUNT, m_instructionMeasureInterval);
  else
    lua_sethook(m_state, &LuaEngine::countHook, 0, 0);
}

int LuaEngine::s_luaInstructionLimitExceptionKey = 0;
int LuaEngine::s_luaRecursionLimitExceptionKey = 0;

void LuaDetail::rawSetField(lua_State* state, int index, char const* key) {
  lua_checkstack(state, 1);

  int absTableIndex = lua_absindex(state, index);
  lua_pushstring(state, key);

  // Move the newly pushed key to the secont to top spot, leaving the value in
  // the top spot.
  lua_insert(state, -2);

  // Pops the value and the key
  lua_rawset(state, absTableIndex);
}

void LuaDetail::rawGetField(lua_State* state, int index, char const* key) {
  lua_checkstack(state, 2);

  int absTableIndex = lua_absindex(state, index);
  lua_pushstring(state, key);

  // Pops the key
  lua_rawget(state, absTableIndex);
}

void LuaDetail::shallowCopy(lua_State* state, int sourceIndex, int targetIndex) {
  lua_checkstack(state, 3);

  int absSourceIndex = lua_absindex(state, sourceIndex);
  int absTargetIndex = lua_absindex(state, targetIndex);

  lua_pushnil(state);
  while (lua_next(state, absSourceIndex) != 0) {
    lua_pushvalue(state, -2);
    lua_insert(state, -2);
    lua_rawset(state, absTargetIndex);
  }
}

LuaTable LuaDetail::jsonContainerToTable(LuaEngine& engine, Json const& container) {
  if (!container.isType(Json::Type::Array) && !container.isType(Json::Type::Object))
    throw LuaException("jsonContainerToTable called on improper json type");

  auto newIndexMetaMethod = [](LuaTable const& table, LuaValue const& key, LuaValue const& value) {
    auto mt = table.getMetatable();
    auto nils = mt->rawGet<LuaTable>("__nils");

    // If we are setting an entry to nil, need to add a bogus integer entry
    // to the __nils table, otherwise need to set the entry *in* the __nils
    // table to nil and remove it.
    if (value == LuaNil)
      nils.rawSet(key, 0);
    else
      nils.rawSet(key, LuaNil);
    table.rawSet(key, value);
  };

  auto mt = engine.createTable();
  auto nils = engine.createTable();
  mt.rawSet("__nils", nils);
  mt.rawSet("__newindex", engine.createFunction(newIndexMetaMethod));
  if (container.isType(Json::Type::Array))
    mt.rawSet("__typehint", 1);
  else
    mt.rawSet("__typehint", 2);

  auto table = engine.createTable();
  table.setMetatable(mt);

  if (container.isType(Json::Type::Array)) {
    auto vlist = container.arrayPtr();
    for (size_t i = 0; i < vlist->size(); ++i) {
      auto const& val = (*vlist)[i];
      if (val)
        table.rawSet(i + 1, val);
      else
        nils.rawSet(i + 1, 0);
    }
  } else {
    for (auto const& pair : *container.objectPtr()) {
      if (pair.second)
        table.rawSet(pair.first, pair.second);
      else
        nils.rawSet(pair.first, 0);
    }
  }

  return table;
}

Maybe<Json> LuaDetail::tableToJsonContainer(LuaTable const& table) {
  JsonObject stringEntries;
  Map<unsigned, Json> intEntries;
  int typeHint = 0;

  if (auto mt = table.getMetatable()) {
    if (auto th = mt->get<Maybe<int>>("__typehint"))
      typeHint = *th;

    if (auto nils = mt->get<Maybe<LuaTable>>("__nils")) {
      bool failedConversion = false;
      // Nil entries just have a garbage integer as their value
      nils->iterate([&](LuaValue const& key, LuaValue const&) {
        if (auto i = asInteger(key)) {
          intEntries[*i] = Json();
        } else {
          if (auto str = table.engine().luaMaybeTo<String>(key)) {
            stringEntries[str.take()] = Json();
          } else {
            failedConversion = true;
            return false;
          }
        }
        return true;
      });
      if (failedConversion)
        return {};
    }
  }

  bool failedConversion = false;
  table.iterate([&](LuaValue key, LuaValue value) {
      auto jsonValue = table.engine().luaMaybeTo<Json>(value);
      if (!jsonValue) {
        failedConversion = true;
        return false;
      }

      if (auto i = asInteger(key)) {
        intEntries[*i] = jsonValue.take();
      } else {
        auto stringKey = table.engine().luaMaybeTo<String>(move(key));
        if (!stringKey) {
          failedConversion = true;
          return false;
        }

        stringEntries[stringKey.take()] = jsonValue.take();
      }

      return true;
    });

  if (failedConversion)
    return {};

  bool interpretAsList = stringEntries.empty()
      && (typeHint == 1 || (typeHint != 2 && !intEntries.empty() && prev(intEntries.end())->first == intEntries.size()));
  if (interpretAsList) {
    JsonArray list;
    for (auto& p : intEntries)
      list.set(p.first - 1, move(p.second));
    return Json(move(list));
  } else {
    for (auto& p : intEntries)
      stringEntries[toString(p.first)] = move(p.second);
    return Json(move(stringEntries));
  }
}

Json LuaDetail::jarrayCreate() {
  return JsonArray();
}

Json LuaDetail::jobjectCreate() {
  return JsonObject();
}

void LuaDetail::jcontRemove(LuaTable const& table, LuaValue const& key) {
  if (auto mt = table.getMetatable()) {
    if (auto nils = mt->rawGet<Maybe<LuaTable>>("__nils"))
      nils->rawSet(key, LuaNil);
  }

  table.rawSet(key, LuaNil);
}

size_t LuaDetail::jcontSize(LuaTable const& table) {
  size_t elemCount = 0;
  size_t highestIndex = 0;
  bool hintList = false;

  if (auto mt = table.getMetatable()) {
    if (mt->rawGet<Maybe<int>>("__typehint") == 1)
      hintList = true;

    if (auto nils = mt->rawGet<Maybe<LuaTable>>("__nils")) {
      nils->iterate([&](LuaValue const& key, LuaValue const&) {
        auto i = asInteger(key);
        if (i && *i >= 0)
          highestIndex = max<int>(*i, highestIndex);
        else
          hintList = false;
        ++elemCount;
      });
    }
  }

  table.iterate([&](LuaValue const& key, LuaValue const&) {
    auto i = asInteger(key);
    if (i && *i >= 0)
      highestIndex = max<int>(*i, highestIndex);
    else
      hintList = false;
    ++elemCount;
  });

  if (hintList)
    return highestIndex;
  else
    return elemCount;
}

void LuaDetail::jcontResize(LuaTable const& table, size_t targetSize) {
  if (auto mt = table.getMetatable()) {
    if (auto nils = mt->rawGet<Maybe<LuaTable>>("__nils")) {
      nils->iterate([&](LuaValue const& key, LuaValue const&) {
        auto i = asInteger(key);
        if (i && *i > 0 && (size_t)*i > targetSize)
          nils->rawSet(key, LuaNil);
      });
    }
  }

  table.iterate([&](LuaValue const& key, LuaValue const&) {
    auto i = asInteger(key);
    if (i && *i > 0 && (size_t)*i > targetSize)
      table.rawSet(key, LuaNil);
  });

  table.set(targetSize, table.get(targetSize));
}

Maybe<LuaInt> LuaDetail::asInteger(LuaValue const& v) {
  if (v.is<LuaInt>())
    return v.get<LuaInt>();
  if (v.is<LuaFloat>()) {
    auto f = v.get<LuaFloat>();
    if ((LuaFloat)(LuaInt)f == f)
      return (LuaInt)f;
    return {};
  }
  /*// Kae: This prevents 1-1 conversion between Lua and Star::Json.
  if (v.is<LuaString>())
    return maybeLexicalCast<LuaInt>(v.get<LuaString>().ptr());
  //*/
  return {};
}

}
