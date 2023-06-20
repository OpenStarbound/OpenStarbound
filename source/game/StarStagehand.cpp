#include "StarStagehand.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarBehaviorLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

Stagehand::Stagehand(Json const& config)
  : Stagehand() {
  setUniqueId(config.optString("uniqueId"));
  readConfig(config);
}

Stagehand::Stagehand(ByteArray const& netStore)
  : Stagehand() {
  readConfig(DataStreamBuffer::deserialize<Json>(netStore));
}

Json Stagehand::diskStore() const {
  auto saveData = m_config.setAll({
      {"position", jsonFromVec2F({m_xPosition.get(), m_yPosition.get()})},
      {"uniqueId", jsonFromMaybe(uniqueId())}
    });

  if (!m_scripted)
    return saveData;
  else
    return saveData.set("scriptStorage", m_scriptComponent.getScriptStorage());
}

ByteArray Stagehand::netStore() {
  return DataStreamBuffer::serialize(m_config);
}

void Stagehand::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);

  if (isMaster() && m_scripted) {
    m_scriptComponent.addCallbacks("stagehand", makeStagehandCallbacks());
    m_scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
        return m_config.query(name, def);
      }));
    m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptComponent.addCallbacks("behavior", LuaBindings::makeBehaviorLuaCallbacks(&m_behaviors));
    m_scriptComponent.init(world);
  }
}

void Stagehand::uninit() {
  Entity::uninit();

  if (m_scripted) {
    m_scriptComponent.uninit();
    m_scriptComponent.removeCallbacks("stagehand");
    m_scriptComponent.removeCallbacks("config");
    m_scriptComponent.removeCallbacks("entity");
  }
}

EntityType Stagehand::entityType() const {
  return EntityType::Stagehand;
}

void Stagehand::setPosition(Vec2F const& position) {
  m_xPosition.set(position[0]);
  m_yPosition.set(position[1]);
}

Vec2F Stagehand::position() const {
  return {m_xPosition.get(), m_yPosition.get()};
}

RectF Stagehand::metaBoundBox() const {
  return m_boundBox;
}

pair<ByteArray, uint64_t> Stagehand::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Stagehand::readNetState(ByteArray data, float) {
  m_netGroup.readNetState(move(data));
}

void Stagehand::update(uint64_t) {
  if (!inWorld())
    return;

  if (isMaster() && m_scripted)
    m_scriptComponent.update(m_scriptComponent.updateDt());

  SpatialLogger::logPoly("world", PolyF(metaBoundBox().translated(position())), {0, 255, 255, 255});
  SpatialLogger::logPoint("world", position(), {0, 255, 255, 255});
}

bool Stagehand::shouldDestroy() const {
  return m_dead;
}

Maybe<LuaValue> Stagehand::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Stagehand::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

Stagehand::Stagehand() {
  setPersistent(true);

  m_netGroup.addNetElement(&m_xPosition);
  m_netGroup.addNetElement(&m_yPosition);
  m_netGroup.addNetElement(&m_uniqueIdNetState);

  m_netGroup.setNeedsLoadCallback([this](bool) {
      setUniqueId(m_uniqueIdNetState.get());
    });

  m_netGroup.setNeedsStoreCallback([this]() {
      m_uniqueIdNetState.set(uniqueId());
    });
}

void Stagehand::readConfig(Json config) {
  m_config = config;
  m_scripted = m_config.contains("scripts");

  if (m_config.contains("position")) {
    auto pos = jsonToVec2F(m_config.get("position"));
    m_xPosition.set(pos[0]);
    m_yPosition.set(pos[1]);
  }
  
  Maybe<RectF> broadcastArea = jsonToMaybe<RectF>(config.opt("broadcastArea").value(Json()), jsonToRectF);
  if (broadcastArea && (broadcastArea->size()[0] < 0.0 || broadcastArea->size()[1] < 0.0))
    broadcastArea.reset();
  m_boundBox = broadcastArea.value(RectF(-5.0, -5.0, 5.0, 5.0));

  if (m_scripted) {
    m_scriptComponent.setScripts(jsonToStringList(m_config.getArray("scripts", JsonArray())));
    m_scriptComponent.setUpdateDelta(m_config.getInt("scriptDelta", 5));

    if (m_config.contains("scriptStorage"))
      m_scriptComponent.setScriptStorage(m_config.getObject("scriptStorage"));
  }

  setKeepAlive(config.getBool("keepAlive", false));
}

LuaCallbacks Stagehand::makeStagehandCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("id", [this]() {
      return entityId();
    });

  callbacks.registerCallback("position", [this]() {
      return position();
    });

  callbacks.registerCallback("setPosition", [this](Vec2F const& position) {
      setPosition(position);
    });

  callbacks.registerCallback("die", [this]() {
      m_dead = true;
    });

  callbacks.registerCallback("typeName", [this]() {
      return typeName();
    });

  callbacks.registerCallback("setUniqueId", [this](Maybe<String> const& uniqueId) {
      setUniqueId(uniqueId);
    });

  return callbacks;
}

String Stagehand::typeName() const {
  return m_config.getString("type");
}

Maybe<Json> Stagehand::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  return m_scriptComponent.handleMessage(message, sendingConnection == world()->connection(), args);
}

}
