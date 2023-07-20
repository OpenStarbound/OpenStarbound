#ifndef STAR_EFFECT_EMITTER_HPP
#define STAR_EFFECT_EMITTER_HPP

#include "StarNetElementSystem.hpp"
#include "StarEffectSourceDatabase.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_CLASS(RenderCallback);
STAR_CLASS(EffectEmitter);

class EffectEmitter : public NetElementGroup {
public:
  EffectEmitter();

  void addEffectSources(String const& position, StringSet effectSources);
  void setSourcePosition(String name, Vec2F const& position);
  void setDirection(Direction direction);
  void setBaseVelocity(Vec2F const& velocity);

  void tick(float dt, EntityMode mode);
  void reset();

  void render(RenderCallback* renderCallback);

  Json toJson() const;
  void fromJson(Json const& diskStore);

private:
  Set<pair<String, String>> m_newSources;
  List<EffectSourcePtr> m_sources;
  NetElementData<Set<pair<String, String>>> m_activeSources;

  StringMap<Vec2F> m_positions;
  Direction m_direction;
  Vec2F m_baseVelocity;

  bool m_renders;
};

}

#endif
