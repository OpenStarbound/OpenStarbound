#pragma once

#include "StarJson.hpp"

namespace Star {

STAR_CLASS(AudioInstance);
STAR_STRUCT(AmbientTrackGroup);
STAR_STRUCT(AmbientNoisesDescription);
STAR_CLASS(AmbientManager);

struct AmbientTrackGroup {
  AmbientTrackGroup();
  AmbientTrackGroup(StringList tracks);
  AmbientTrackGroup(Json const& config, String const& directory = "");

  Json toJson() const;

  StringList tracks;
};

// represents the ambient sounds data for a biome
struct AmbientNoisesDescription {
  AmbientNoisesDescription();
  AmbientNoisesDescription(AmbientTrackGroup day, AmbientTrackGroup night, int loops = -1);
  AmbientNoisesDescription(Json const& config, String const& directory = "");

  Json toJson() const;

  AmbientTrackGroup daySounds;
  AmbientTrackGroup nightSounds;
  int trackLoops = -1;
  bool sequential = false; //flag for sequential playback of tracks
};

typedef AmbientTrackGroup WeatherNoisesDescription;
typedef shared_ptr<WeatherNoisesDescription> WeatherNoisesDescriptionPtr;

// manages the running ambient sounds
class AmbientManager {
public:
  // Automatically calls cancelAll();
  ~AmbientManager();

  void setTrackSwitchGrace(float grace);
  void setTrackFadeInTime(float fadeInTime);

  // Returns a new AudioInstance if a new ambient sound is to be started.
  AudioInstancePtr updateAmbient(AmbientNoisesDescriptionPtr current, bool dayTime = false);
  AudioInstancePtr updateWeather(WeatherNoisesDescriptionPtr current);
  void cancelAll();

  void setVolume(float volume, float delay, float duration);

private:
  AudioInstancePtr m_currentTrack;
  AudioInstancePtr m_weatherTrack;
  String m_currentTrackName;
  String m_weatherTrackName;
  float m_trackFadeInTime = 0.0f;
  float m_trackSwitchGrace = 0.0f;
  double m_trackGraceTimestamp = 0;
  Deque<String> m_recentTracks;
  float m_volume = 1.0f;
  float m_delay = 0.0f;
  float m_duration = 0.0f;
  bool m_volumeChanged = false;
  String m_lastSequentialTrack; // last track played in sequential mode, to pick up next one
};

}
