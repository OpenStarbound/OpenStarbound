#include "StarEffectSourceDatabase.hpp"
#include "StarGameTypes.hpp"
#include "StarParticleDatabase.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarMixer.hpp"
#include "StarAssets.hpp"

namespace Star {

EffectSourceDatabase::EffectSourceDatabase() {
  auto assets = Root::singleton().assets();

  auto files = assets->scanExtension("effectsource");
  assets->queueJsons(files);
  for (auto const& file : files) {
    auto sourceConfig = make_shared<EffectSourceConfig>(assets->json(file));
    if (m_sourceConfigs.contains(sourceConfig->kind()))
      throw StarException(
          strf("Duplicate effect source asset kind Name %s. configfile %s", sourceConfig->kind(), file));
    auto k = sourceConfig->kind().toLower();
    m_sourceConfigs[k] = sourceConfig;
  }
}

EffectSourceConfigPtr EffectSourceDatabase::effectSourceConfig(String const& kind) const {
  auto k = kind.toLower();
  if (!m_sourceConfigs.contains(k))
    throw StarException(strf("Unknown effect source definition with kind '%s'.", kind));
  return m_sourceConfigs.get(k);
}

EffectSourceConfig::EffectSourceConfig(Json const& config) {
  m_kind = config.getString("kind");
  m_config = config;
}

String const& EffectSourceConfig::kind() {
  return m_kind;
}

EffectSourcePtr EffectSourceConfig::instance(String const& suggestedSpawnLocation) {
  return make_shared<EffectSource>(kind(), suggestedSpawnLocation, m_config.getObject("definition"));
}

EffectSource::EffectSource(String const& kind, String suggestedSpawnLocation, Json const& definition) {
  m_kind = kind;
  m_config = definition;
  m_expired = false;
  m_loopDuration = m_config.getFloat("duration", 0);
  m_durationVariance = m_config.getFloat("durationVariance", 0);
  m_loops = m_config.getBool("loops", m_loopDuration != 0);
  m_timer = Random::randf() * (m_loopDuration + 0.5 * m_durationVariance);
  m_stop = false;
  m_initialTick = true;
  m_loopTick = false;
  m_finalTick = false;
  m_effectSpawnLocation = m_config.getString("location", "normal");
  m_suggestedSpawnLocation = suggestedSpawnLocation;
}

String const& EffectSource::kind() const {
  return m_kind;
}

bool EffectSource::expired() const {
  return m_expired;
}

void EffectSource::stop() {
  m_stop = true;
}

void EffectSource::tick() {
  m_timer -= WorldTimestep;
  if ((m_timer <= 0) && m_loops) {
    m_timer = m_loopDuration + m_durationVariance * Random::randf(-0.5f, 0.5f);
    m_loopTick = true;
  }
  if (m_stop || (m_timer <= 0))
    if (!m_expired)
      m_finalTick = true;
}

List<String> EffectSource::particles() {
  auto pickParticleSources = [](Json const& config, List<String>& particles) {
    particles.appendAll(jsonToStringList(Random::randValueFrom(config.toArray(), JsonArray())));
  };
  List<String> result;
  if (m_initialTick)
    pickParticleSources(m_config.get("start", JsonObject()).get("particles", JsonArray()), result);
  if (m_loopTick)
    pickParticleSources(m_config.get("particles", JsonArray()), result);
  if (m_finalTick)
    pickParticleSources(m_config.get("stop", JsonObject()).get("particles", JsonArray()), result);
  return result;
}

List<AudioInstancePtr> EffectSource::sounds(Vec2F offset) {
  List<AudioInstancePtr> result;
  if (m_initialTick) {
    result.appendAll(soundsFromDefinition(m_config.get("start", JsonObject()).get("sounds", Json()), offset));

    m_mainSounds = soundsFromDefinition(m_config.get("sounds", Json()), offset);
    result.appendAll(m_mainSounds);
  }
  if (m_finalTick) {
    for (auto& s : m_mainSounds)
      s->stop();
    result.appendAll(soundsFromDefinition(m_config.get("stop", JsonObject()).get("sounds", Json()), offset));
  }
  return result;
}

void EffectSource::postRender() {
  m_initialTick = false;
  m_loopTick = false;
  if (m_finalTick) {
    m_finalTick = false;
    m_expired = true;
  }
}

String EffectSource::effectSpawnLocation() const {
  if ((m_effectSpawnLocation == "normal") && (!m_suggestedSpawnLocation.empty()))
    return m_suggestedSpawnLocation;
  return m_effectSpawnLocation;
}

String EffectSource::suggestedSpawnLocation() const {
  return m_suggestedSpawnLocation;
}

List<Particle> particlesFromDefinition(Json const& config, Vec2F const& position) {
  Json particles;
  if (config.type() == Json::Type::Array)
    particles = Random::randValueFrom(config.toArray(), Json());
  else
    particles = config;
  if (!particles.isNull()) {
    if (particles.type() != Json::Type::Array)
      particles = JsonArray{particles};
    List<Particle> result;
    for (auto entry : particles.iterateArray()) {
      if (entry.type() != Json::Type::Object) {
        result.append(Root::singleton().particleDatabase()->particle(entry.toString()));
      } else {
        Particle particle(entry.toObject());
        Particle variance(entry.getObject("variance", {}));
        particle.applyVariance(variance);
        particle.position += position;
        result.append(particle);
      }
    }
    return result;
  }
  return {};
}

List<AudioInstancePtr> soundsFromDefinition(Json const& config, Vec2F const& position) {
  Json sound;
  if (config.type() == Json::Type::Array)
    sound = Random::randValueFrom(config.toArray(), Json());
  else
    sound = config;
  if (!sound.isNull()) {
    if (sound.type() != Json::Type::Array)
      sound = JsonArray{sound};
    List<AudioInstancePtr> result;
    auto assets = Root::singleton().assets();
    for (auto entry : sound.iterateArray()) {
      if (entry.type() != Json::Type::Object) {
        JsonObject t;
        t["resource"] = entry.toString();
        entry = t;
      }

      auto sample = make_shared<AudioInstance>(*assets->audio(entry.getString("resource")));
      sample->setLoops(entry.getInt("loops", 0));
      sample->setVolume(entry.getFloat("volume", 1.0f));
      sample->setPitchMultiplier(entry.getFloat("pitch", 1.0f) + Random::randf(-1, 1) * entry.getFloat("pitchVariability", 0.0f));
      sample->setRangeMultiplier(entry.getFloat("audioRangeMultiplier", 1.0f));
      sample->setPosition(position);

      result.append(move(sample));
    }
    return result;
  }
  return {};
}

}
