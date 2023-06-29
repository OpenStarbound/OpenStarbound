#ifndef STAR_PLAYER_DEPLOYMENT_HPP
#define STAR_PLAYER_DEPLOYMENT_HPP

#include "StarLuaComponents.hpp"
#include "StarLuaAnimationComponent.hpp"
#include "StarWorld.hpp"

namespace Star {

STAR_CLASS(RenderCallback);

STAR_CLASS(PlayerDeployment);

class PlayerDeployment {
public:
  PlayerDeployment(Json const& config);

  void diskLoad(Json const& diskStore);
  Json diskStore() const;

  bool canDeploy();
  void setDeploying(bool deploying);
  bool isDeploying() const;
  bool isDeployed() const;

  void init(Entity* player, World* world);
  void uninit();

  void teleportOut();
  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});
  void update();

  void render(RenderCallback* renderCallback, Vec2F const& position);

  void renderLightSources(RenderCallback* renderCallback);
private:
  World* m_world;
  Json m_config;

  bool m_deploying;
  bool m_deployed;
  LuaAnimationComponent<LuaMessageHandlingComponent<LuaStorableComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>> m_scriptComponent;
};

}

#endif
