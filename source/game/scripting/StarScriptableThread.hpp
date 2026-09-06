#pragma once

#include "StarThread.hpp"
#include "StarLuaRoot.hpp"
#include "StarLuaComponents.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(ScriptableThread);

// Runs a Lua in a separate thread and guards exceptions that occur in
// it.  All methods are designed to not throw exceptions, but will instead log
// the error and trigger the ScriptableThread error state.
class ScriptableThread : public Thread {
public:
  struct Message {
    String message;
    JsonArray args;
    RpcPromiseKeeper<Json> promise;
  };

  typedef LuaMessageHandlingComponent<LuaUpdatableComponent<LuaBaseComponent>> ScriptComponent;
  typedef shared_ptr<ScriptComponent> ScriptComponentPtr;

  ScriptableThread(Json parameters, LuaBaseComponent* parent);
  ~ScriptableThread();

  void start();
  // Signals the ScriptableThread to stop and then joins it
  void stop();
  void setPause(bool pause);

  // An exception occurred and the
  // ScriptableThread has stopped running.
  bool errorOccurred();
  bool shouldExpire();

  // 
  void passMessage(Message&& message);
  
  // expected to be run before the thread is started or after it is stopped!
  void addCallbacks(String const& groupName, LuaCallbacks const& callbacks);
  void removeCallbacks(String const& groupName);

protected:
  virtual void run();

private:
  void update();
  Maybe<ChainableJsonMessageResponse> receiveMessage(String const& message, JsonArray const& args);

  mutable RecursiveMutex m_mutex;
  
  LuaRootPtr m_luaRoot;
  StringMap<ScriptComponentPtr> m_scriptContexts;
  
  Json m_parameters;
  String m_name;
  // log mapping can be disabled on scriptable threads to reduce debug clutter
  bool m_logMapped;
  
  float m_timestep;

  mutable RecursiveMutex m_messageMutex;
  List<Message> m_messages;

  atomic<bool> m_stop;
  atomic<bool> m_pause;
  mutable atomic<bool> m_errorOccurred;
  mutable atomic<bool> m_shouldExpire;
  
  LuaBaseComponent* m_parent;
  
  StringMap<LuaCallbacks> m_threadCallbacks;
  
  LuaCallbacks makeThreadCallbacks();
  Json configValue(String const& name, Json def) const;
};

}
