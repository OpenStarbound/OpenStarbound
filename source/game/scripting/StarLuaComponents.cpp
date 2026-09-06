#include "StarLuaComponents.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarScriptableThread.hpp"

namespace Star {

LuaBaseComponent::LuaBaseComponent() {
  addCallbacks("sb", LuaBindings::makeUtilityCallbacks());
  addCallbacks("root", LuaBindings::makeRootCallbacks());
  setAutoReInit(true);
}

LuaBaseComponent::~LuaBaseComponent() {
  m_threads.clear();
}

StringList const& LuaBaseComponent::scripts() const {
  return m_scripts;
}

void LuaBaseComponent::setScript(String script) {
  setScripts({std::move(script)});
}

void LuaBaseComponent::setScripts(StringList scripts) {
  if (initialized())
    throw LuaComponentException("Cannot call LuaWorldComponent::setScripts when LuaWorldComponent is initialized");

  m_scripts = std::move(scripts);
}

void LuaBaseComponent::addCallbacks(String groupName, LuaCallbacks callbacks) {
  if (!m_callbacks.insert(groupName, callbacks).second)
    throw LuaComponentException::format("Duplicate callbacks named '{}' in LuaBaseComponent", groupName);

  if (m_context)
    m_context->setCallbacks(groupName, callbacks);
}

bool LuaBaseComponent::removeCallbacks(String const& groupName) {
  if (m_callbacks.remove(groupName)) {
    if (m_context)
      m_context->remove(groupName);
    return true;
  }
  return false;
}

void LuaBaseComponent::addThreadCallbacks(String groupName, LuaCallbacks callbacks) {
  if (!m_threadCallbacks.insert(groupName, callbacks).second)
    throw LuaComponentException::format("Duplicate thread callbacks named '{}' in LuaBaseComponent", groupName);

  for (auto const& p : m_threads) {
    p.second->addCallbacks(groupName,callbacks);
  }
}

bool LuaBaseComponent::removeThreadCallbacks(String const& groupName) {
  if (m_threadCallbacks.remove(groupName)) {
    for (auto const& p : m_threads) {
      p.second->removeCallbacks(groupName);
    }
    return true;
  }
  return false;
}

bool LuaBaseComponent::autoReInit() const {
  return (bool)m_reloadTracker;
}

void LuaBaseComponent::setAutoReInit(bool autoReInit) {
  if (autoReInit) {
    m_reloadTracker = make_shared<TrackerListener>();
    Root::singleton().registerReloadListener(m_reloadTracker);
  } else {
    m_reloadTracker.reset();
  }
}

void LuaBaseComponent::setLuaRoot(LuaRootPtr luaRoot) {
  m_luaRoot = std::move(luaRoot);
}

LuaRootPtr const& LuaBaseComponent::luaRoot() {
  return m_luaRoot;
}

bool LuaBaseComponent::init() {
  uninit();

  if (!m_luaRoot)
    return false;

  m_error.reset();
  try {
    m_context = m_luaRoot->createContext(m_scripts);
  } catch (LuaException const& e) {
    Logger::error("Exception while creating lua context for scripts '{}': {}", m_scripts, outputException(e, true));
    m_error = String(printException(e, false));
    m_context.reset();
    return false;
  }
  contextSetup();

  if (m_context->containsPath("init")) {
    try {
      m_context->invokePath("init");
    } catch (LuaException const& e) {
      Logger::error("Exception while calling script init: {}", outputException(e, true));
      m_error = String(printException(e, false));
      m_context.reset();
      return false;
    }
  }

  return true;
}

void LuaBaseComponent::uninit() {
  if (m_context) {
    if (m_context->containsPath("uninit")) {
      try {
        m_context->invokePath("uninit");
      } catch (LuaException const& e) {
        Logger::error("Exception while calling script uninit: {}", outputException(e, true));
        m_error = String(printException(e, false));
      }
    }
    contextShutdown();
    m_context.reset();
  }
  for (auto p : m_threads) {
    p.second->stop();
  }

  m_error.reset();
}

bool LuaBaseComponent::initialized() const {
  return m_context.isValid();
}

Maybe<String> const& LuaBaseComponent::error() const {
  return m_error;
}

Maybe<LuaContext> const& LuaBaseComponent::context() const {
  return m_context;
}

Maybe<LuaContext>& LuaBaseComponent::context() {
  return m_context;
}

void LuaBaseComponent::contextSetup() {
  m_context->setPath("self", m_context->createTable());

  // set up here instead
  if (!m_callbacks.contains("threads"))
    addCallbacks("threads", makeThreadsCallbacks());
  
  for (auto const& p : m_callbacks)
    m_context->setCallbacks(p.first, p.second);
}

void LuaBaseComponent::contextShutdown() {}

void LuaBaseComponent::setError(String error) {
  m_context.reset();
  m_error = std::move(error);
}

bool LuaBaseComponent::checkInitialization() {
  // We should re-initialize if we are either already initialized or in an
  // error state (which means we WERE initialized until we had an error)
  bool shouldBeInitialized = initialized() || error();
  if (shouldBeInitialized && m_reloadTracker && m_reloadTracker->pullTriggered())
    init();
  return initialized();
}

void LuaBaseComponent::cleanThreads() {
  for (auto const& p : m_threads) {
    if (p.second->shouldExpire()) {
      m_threads.remove(p.first);
    }
  }
}

LuaCallbacks LuaBaseComponent::makeThreadsCallbacks() {
  LuaCallbacks callbacks;
  
  callbacks.registerCallback("create", [this](Json parameters) {
    cleanThreads();
    auto name = parameters.getString("name",Uuid().hex());
    if (m_threads.contains(name)) {
      m_threads.get(name)->stop();
      m_threads.remove(name);
    }
    auto thread = make_shared<ScriptableThread>(parameters.set("name",name), this);
    
    for (auto const& callbackPair : m_threadCallbacks)
      thread->addCallbacks(callbackPair.first, callbackPair.second);
    
    thread->setPause(false);
    thread->start();
    m_threads.set(name,thread);
    return name;
  });
  callbacks.registerCallback("setPause", [this](String const& threadName, bool paused) {
    if (m_threads.contains(threadName))
      m_threads.get(threadName)->setPause(paused);
  });
  callbacks.registerCallback("stop", [this](String const& threadName) {
    if (m_threads.contains(threadName)) {
      m_threads.get(threadName)->stop();
      m_threads.remove(threadName);
    }
    cleanThreads();
  });
  callbacks.registerCallback("sendMessage", [this](String const& threadName, String const& message, LuaVariadic<Json> args) -> RpcPromise<Json> {
    if (!m_threads.contains(threadName))
      return RpcPromise<Json>::createFailed("Thread does not exist");
    
    auto thread = m_threads.get(threadName);
    if (thread->shouldExpire())
      return RpcPromise<Json>::createFailed("Thread is stopped");
    
    auto pair = RpcPromise<Json>::createPair();
    thread->passMessage({ message, JsonArray::from(std::move(args)), pair.second });
    return pair.first;
  });
  
  return callbacks;
}

RpcPromise<Json> LuaBaseComponent::threadPassMessage(String const& thread, String const& message, JsonArray const& args) {
  _unused(thread);
  _unused(message);
  _unused(args);
  return RpcPromise<Json>::createFailed("Lua component cannot handle messages");
}

}
