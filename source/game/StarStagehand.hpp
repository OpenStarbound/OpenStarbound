#ifndef STAR_STAGEHAND_HPP
#define STAR_STAGEHAND_HPP

#include "StarEntity.hpp"
#include "StarLuaComponents.hpp"
#include "StarScriptedEntity.hpp"
#include "StarBehaviorState.hpp"
#include "StarNetElementSystem.hpp"
#include "StarStagehandDatabase.hpp"
#include "StarRoot.hpp"

namespace Star {

STAR_CLASS(Stagehand);

class Stagehand : public virtual ScriptedEntity {
public:
  Stagehand(Json const& config);
  Stagehand(ByteArray const& netStore);

  Json diskStore() const;
  ByteArray netStore();

  void init(World* world, EntityId entityId, EntityMode mode) override;
  void uninit() override;

  EntityType entityType() const override;

  void setPosition(Vec2F const& position);

  Vec2F position() const override;

  RectF metaBoundBox() const override;

  pair<ByteArray, uint64_t> writeNetState(uint64_t fromVersion = 0) override;
  void readNetState(ByteArray data, float interpolationTime = 0.0f) override;

  void update(float dt, uint64_t currentStep) override;

  bool shouldDestroy() const override;

  Maybe<LuaValue> callScript(String const& func, LuaVariadic<LuaValue> const& args) override;
  Maybe<LuaValue> evalScript(String const& code) override;

  String typeName() const;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) override;

  using Entity::setUniqueId;

private:
  Stagehand();

  void readConfig(Json config);

  LuaCallbacks makeStagehandCallbacks();

  Json m_config;

  RectF m_boundBox;

  bool m_dead = false;

  NetElementTopGroup m_netGroup;

  NetElementFloat m_xPosition;
  NetElementFloat m_yPosition;

  NetElementData<Maybe<String>> m_uniqueIdNetState;

  bool m_scripted = false;
  List<BehaviorStatePtr> m_behaviors;
  LuaMessageHandlingComponent<LuaStorableComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>
      m_scriptComponent;
};

}

#endif
