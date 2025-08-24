#include "StarAmbient.hpp"
#include "StarJsonExtra.hpp"
#include "StarTime.hpp"
#include "StarMixer.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarRandom.hpp"
#include "StarGameTypes.hpp"

namespace Star {

AmbientTrackGroup::AmbientTrackGroup() {
  tracks = {};
}

AmbientTrackGroup::AmbientTrackGroup(StringList tracks) : tracks(std::move(tracks)) {}

AmbientTrackGroup::AmbientTrackGroup(Json const& config, String const& directory) {
  for (auto track : jsonToStringList(config.get("tracks", JsonArray())))
    tracks.append(AssetPath::relativeTo(directory, track));
}

Json AmbientTrackGroup::toJson() const {
  return JsonObject{{"tracks", jsonFromStringList(tracks)}};
}

AmbientNoisesDescription::AmbientNoisesDescription() : trackLoops(-1) {}

AmbientNoisesDescription::AmbientNoisesDescription(AmbientTrackGroup day, AmbientTrackGroup night, int loops)
  : daySounds(std::move(day)), nightSounds(std::move(night)), trackLoops(loops) {
  if ((daySounds.tracks.size() > 1 || nightSounds.tracks.size() > 1) && trackLoops == -1)
    sequential = true, trackLoops = 0;
}

AmbientNoisesDescription::AmbientNoisesDescription(Json const& config, String const& directory) {
  if (auto day = config.opt("day"))
    daySounds = AmbientTrackGroup(*day, directory);
  if (auto night = config.opt("night"))
    nightSounds = AmbientTrackGroup(*night, directory);
  if (auto loopsOpt = config.optInt("loops"))
    trackLoops = *loopsOpt;
  bool seqSpecified = false;
  if (auto seqOpt = config.optBool("sequential")) {
    sequential = *seqOpt;
    seqSpecified = true;
  }
  if (!seqSpecified && (daySounds.tracks.size() > 1 || nightSounds.tracks.size() > 1) && trackLoops == -1) {
    sequential = true;
    trackLoops = 0;
  }
}

Json AmbientNoisesDescription::toJson() const {
  return JsonObject{{"day", daySounds.toJson()}, {"night", nightSounds.toJson()}, {"loops", trackLoops}, {"sequential", sequential}};
}

AmbientManager::~AmbientManager() {
  cancelAll();
}

void AmbientManager::setTrackSwitchGrace(float grace) {
  m_trackSwitchGrace = grace;
}

void AmbientManager::setTrackFadeInTime(float fadeInTime) {
  m_trackFadeInTime = fadeInTime;
}

AudioInstancePtr AmbientManager::updateAmbient(AmbientNoisesDescriptionPtr current, bool dayTime) {
  auto assets = Root::singleton().assets();

  if (m_currentTrack) {
    if (m_currentTrack->finished())
      m_currentTrack = {};
  }
  StringList tracks;
  if (current)
    tracks = dayTime ? current->daySounds.tracks : current->nightSounds.tracks;

  if (m_currentTrack) {
    if (m_currentTrack->finished() || !tracks.contains(m_currentTrackName)) {
      if (m_trackSwitchGrace <= Time::monotonicTime() - m_trackGraceTimestamp) {
        m_currentTrack->stop(m_trackFadeInTime);
        m_currentTrack = {};
      }
    } else {
      m_trackGraceTimestamp = Time::monotonicTime();
    }
  }
  if (!m_currentTrack) {
    m_currentTrackName = "";
    if (!tracks.empty()) {
      if (current && current->sequential) {
        // Sequential selection
        int nextIndex = 0;
        if (!m_lastSequentialTrack.empty()) {
          int lastIndex = tracks.indexOf(m_lastSequentialTrack);
          if (lastIndex >= 0)
            nextIndex = (lastIndex + 1) % tracks.size();
        }
        m_currentTrackName = tracks.at(nextIndex);
      } else {
        // Random selection with history
        while ((m_recentTracks.size() / 2) >= tracks.size())
          m_recentTracks.removeFirst();
        while (true) {
          m_currentTrackName = Random::randValueFrom(tracks);
          if (m_currentTrackName.empty() || !m_recentTracks.contains(m_currentTrackName))
            break;
          m_recentTracks.removeFirst();
        }
      }
    }
    if (!m_currentTrackName.empty()) {
      if (auto audio = assets->tryAudio(m_currentTrackName)) {
        if (current && current->sequential)
          m_lastSequentialTrack = m_currentTrackName;
        if (!current || !current->sequential)
          m_recentTracks.append(m_currentTrackName);
        m_currentTrack = make_shared<AudioInstance>(*audio);
        m_currentTrack->setLoops(current ? current->trackLoops : -1);
        m_currentTrack->setVolume(0.0f);
        m_currentTrack->setVolume(m_volume, m_trackFadeInTime);
        m_delay = 0;
        m_duration = 0;
        m_volumeChanged = false;
        return m_currentTrack;
      }
      // If we failed to load the track, clear the current track name so we try again next update
      if (current && current->sequential && !tracks.empty()) {
        int failIndex = tracks.indexOf(m_currentTrackName);
        if (failIndex >= 0 && tracks.size() > 1) {
          m_lastSequentialTrack = tracks.at(failIndex);
        } else {
          m_lastSequentialTrack.clear();
        }
      }
      m_currentTrackName.clear();
    } else if (current && current->sequential) {
      m_lastSequentialTrack.clear();
    }
  }
  if (m_volumeChanged) {
    if (m_delay > 0)
      m_delay -= GlobalTimestep;
    else {
      m_volumeChanged = false;
      if (m_currentTrack) {
        m_currentTrack->setVolume(m_volume, m_duration);
      }
    }
  }

  return {};
}

AudioInstancePtr AmbientManager::updateWeather(WeatherNoisesDescriptionPtr current) {
  auto assets = Root::singleton().assets();

  if (m_weatherTrack) {
    if (m_weatherTrack->finished())
      m_weatherTrack = {};
  }

  StringList tracks;
  if (current)
    tracks = current->tracks;

  if (m_weatherTrack) {
    if (!tracks.contains(m_weatherTrackName)) {
      m_weatherTrack->stop(10.0f);
      m_weatherTrack = {};
    }
  }

  if (!m_weatherTrack) {
    m_weatherTrackName = Random::randValueFrom(tracks);
    if (!m_weatherTrackName.empty()) {
      if (auto audio = assets->tryAudio(m_weatherTrackName)) {
        m_weatherTrack = make_shared<AudioInstance>(*audio);
        m_weatherTrack->setLoops(-1);
        m_weatherTrack->setVolume(0.0f);
        m_weatherTrack->setVolume(1, m_trackFadeInTime);
        return m_weatherTrack;
      }
    }
  }

  return {};
}

void AmbientManager::cancelAll() {
  if (m_weatherTrack) {
    m_weatherTrack->stop();
    m_weatherTrack = {};
  }
  if (m_currentTrack) {
    m_currentTrack->stop();
    m_currentTrack = {};
  }
}

void AmbientManager::setVolume(float volume, float delay, float duration) {
  if (m_volume == volume)
    return;
  m_volume = volume;
  m_delay = delay;
  m_duration = duration;
  m_volumeChanged = true;
}

}
