#ifndef STAR_PLAYER_COMPANIONS_HPP
#define STAR_PLAYER_COMPANIONS_HPP

#include "StarAiTypes.hpp"
#include "StarUuid.hpp"
#include "StarDrawable.hpp"
#include "StarLuaComponents.hpp"
#include "StarWorld.hpp"

namespace Star {

STAR_CLASS(Companion);
STAR_CLASS(PlayerCompanions);

class Companion {
public:
  Companion(Json const& json);
  Json toJson() const;

  Uuid podUuid() const;
  Maybe<String> name() const;
  Maybe<String> description() const;

  List<Drawable> portrait() const;

  Maybe<float> resource(String const& resourceName) const;
  Maybe<float> resourceMax(String const& resourceName) const;

  Maybe<float> stat(String const& statName) const;

private:
  Json m_json;
  List<Drawable> m_portrait;
};

class PlayerCompanions {
public:
  PlayerCompanions(Json const& config);

  void diskLoad(Json const& diskStore);
  Json diskStore() const;

  List<CompanionPtr> getCompanions(String const& category) const;

  void init(Entity* player, World* world);
  void uninit();

  void dismissCompanion(String const& category, Uuid const& podUuid);

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args = {});
  void update(float dt);

private:
  LuaCallbacks makeCompanionsCallbacks();

  World* m_world;
  Json m_config;
  StringMap<List<CompanionPtr>> m_companions;

  LuaMessageHandlingComponent<LuaStorableComponent<LuaUpdatableComponent<LuaWorldComponent<LuaBaseComponent>>>>
      m_scriptComponent;
};

}

#endif
