#include "StarEffectEmitter.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarParticleDatabase.hpp"
#include "StarEntityRendering.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

EffectEmitter::EffectEmitter() {
  m_renders = false;
  m_direction = Direction::Right;
  addNetElement(&m_activeSources);
}

void EffectEmitter::addEffectSources(String const& position, StringSet effectSources) {
  for (auto& e : effectSources)
    m_newSources.add({position, move(e)});
}

void EffectEmitter::setSourcePosition(String name, Vec2F const& position) {
  m_positions[move(name)] = position;
}

void EffectEmitter::setDirection(Direction direction) {
  m_direction = direction;
}

void EffectEmitter::setBaseVelocity(Vec2F const& velocity) {
  m_baseVelocity = velocity;
}

void EffectEmitter::tick(float dt, EntityMode mode) {
  if (mode == EntityMode::Master) {
    m_activeSources.set(move(m_newSources));
    m_newSources.clear();
  } else {
    if (!m_newSources.empty())
      throw StarException("EffectEmitters can only be added to the master entity.");
  }
  if (m_renders) {
    eraseWhere(m_sources, [](EffectSourcePtr const& source) { return source->expired(); });

    for (auto& ps : m_sources)
      ps->tick(dt);

    Set<pair<String, String>> current;
    for (auto& ps : m_sources) {
      pair<String, String> entry = {ps->suggestedSpawnLocation(), ps->kind()};
      current.add(entry);
      if (!m_activeSources.get().contains(entry)) {
        ps->stop();
      }
    }
    for (auto& c : m_activeSources.get()) {
      if (!current.contains(c)) {
        m_sources.append(Root::singleton().effectSourceDatabase()->effectSourceConfig(c.second)->instance(c.first));
      }
    }
  }
}

void EffectEmitter::reset() {
  m_sources.clear();
  m_newSources.clear();
  m_activeSources.set({});
}

void EffectEmitter::render(RenderCallback* renderCallback) {
  m_renders = true;
  if (m_sources.empty())
    return;
  for (auto& ps : m_sources) {
    Vec2F position = m_positions.get(ps->effectSpawnLocation());
    for (auto& p : ps->particles()) {
      Particle particle = Root::singleton().particleDatabase()->particle(p);
      if (m_direction == Direction::Left) {
        particle.flip = true;
        particle.position[0] = -particle.position[0];
        particle.velocity[0] = -particle.velocity[0];
        particle.finalVelocity[0] = -particle.finalVelocity[0];
      }
      particle.velocity += m_baseVelocity;
      particle.finalVelocity += m_baseVelocity;
      particle.position += position;
      renderCallback->addParticle(particle);
    }
    for (auto& s : ps->sounds(position))
      renderCallback->addAudio(s);
  }
  for (auto& ps : m_sources)
    ps->postRender();
}

Json EffectEmitter::toJson() const {
  return JsonObject{{"activeSources",
      jsonFromSet<pair<String, String>>(m_activeSources.get(),
                         [](pair<String, String> const& entry) {
                           return JsonObject{{"position", entry.first}, {"source", entry.second}};
                         })}};
}

void EffectEmitter::fromJson(Json const& diskStore) {
  m_activeSources.set(jsonToSet<pair<String, String>>(diskStore.get("activeSources"),
      [](Json const& v) {
        return pair<String, String>{v.getString("position"), v.getString("source")};
      }));
}

}
