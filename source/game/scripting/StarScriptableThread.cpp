#include "StarScriptableThread.hpp"
#include "StarLuaRoot.hpp"
#include "StarLuaComponents.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarTickRateMonitor.hpp"
#include "StarNpc.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarAssets.hpp"

namespace Star {

ScriptableThread::ScriptableThread(Json parameters)
  : Thread("ScriptableThread: " + parameters.getString("name")), // TODO
    m_stop(false),
    m_parameters(std::move(parameters)),
    m_errorOccurred(false),
    m_shouldExpire(true) {
      m_luaRoot = make_shared<LuaRoot>();
      m_name = m_parameters.getString("name");
      
      m_timestep = 1.0f / m_parameters.getFloat("tickRate",60.0f);
      
      // since thread's not blocking anything important, allow modifying the instruction limit
      if (auto instructionLimit = m_parameters.optUInt("instructionLimit"))
        m_luaRoot->luaEngine().setInstructionLimit(instructionLimit.value());
        
      m_luaRoot->addCallbacks("thread", makeThreadCallbacks());
      m_luaRoot->addCallbacks(
          "config", LuaBindings::makeConfigCallbacks(bind(&ScriptableThread::configValue, this, _1, _2)));
      
      for (auto& p : m_parameters.getObject("scripts")) {
        auto scriptComponent = make_shared<ScriptComponent>();
        scriptComponent->setLuaRoot(m_luaRoot);
        scriptComponent->setScripts(jsonToStringList(p.second.toArray()));

        m_scriptContexts.set(p.first, scriptComponent);
        scriptComponent->init();
      }
}

ScriptableThread::~ScriptableThread() {
  m_stop = true;

  m_scriptContexts.clear();
  
  join();
}

void ScriptableThread::start() {
  m_stop = false;
  m_errorOccurred = false;
  Thread::start();
}

void ScriptableThread::stop() {
  m_stop = true;
  Thread::join();
}

void ScriptableThread::setPause(bool pause) {
  m_pause = pause;
}

bool ScriptableThread::errorOccurred() {
  return m_errorOccurred;
}

bool ScriptableThread::shouldExpire() {
  return m_shouldExpire;
}

void ScriptableThread::passMessage(Message&& message) {
  RecursiveMutexLocker locker(m_messageMutex);
  m_messages.append(std::move(message));
}

void ScriptableThread::run() {
  try {
    double updateMeasureWindow = m_parameters.getDouble("updateMeasureWindow",0.5);
    TickRateApproacher tickApproacher(1.0f / m_timestep, updateMeasureWindow);

    while (!m_stop && !m_errorOccurred) {
      LogMap::set(strf("lua_{}_update", m_name), strf("{:4.2f}Hz", tickApproacher.rate()));

      update();
      tickApproacher.setTargetTickRate(1.0f / m_timestep);
      tickApproacher.tick();

      double spareTime = tickApproacher.spareTime();

      int64_t spareMilliseconds = floor(spareTime * 1000);
      if (spareMilliseconds > 0)
        Thread::sleepPrecise(spareMilliseconds);
    }
  } catch (std::exception const& e) {
    Logger::error("ScriptableThread exception caught: {}", outputException(e, true));
    m_errorOccurred = true;
  }
  for (auto& p : m_scriptContexts)
    p.second->uninit();
}

Maybe<Json> ScriptableThread::receiveMessage(String const& message, JsonArray const& args) {
  Maybe<Json> result;
  for (auto& p : m_scriptContexts) {
    result = p.second->handleMessage(message, true, args);
    if (result)
      break;
  }
  return result;
}

void ScriptableThread::update() {
  float dt = m_timestep;
  
  if (dt > 0.0f && !m_pause) {
    for (auto& p : m_scriptContexts) {
      p.second->update(p.second->updateDt(dt));
    }
  }
  
  List<Message> messages;
  {
    RecursiveMutexLocker locker(m_messageMutex);
    messages = std::move(m_messages);
  }
  for (auto& message : messages) {
    if (auto resp = receiveMessage(message.message, message.args))
      message.promise.fulfill(*resp);
    else
      message.promise.fail("Message not handled by thread");
  }
}

LuaCallbacks ScriptableThread::makeThreadCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("stop", [this]() {
      m_stop = true;
    });

  return callbacks;
}

Json ScriptableThread::configValue(String const& name, Json def) const {
  return m_parameters.get(name, std::move(def));
}
}
