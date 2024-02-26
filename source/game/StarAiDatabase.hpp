#pragma once

#include "StarAiTypes.hpp"

namespace Star {

class AiDatabase {
public:
  AiDatabase();

  AiMission mission(String const& missionName) const;

  AiSpeech shipStatus(unsigned shipLevel) const;
  AiSpeech noMissionsSpeech() const;
  AiSpeech noCrewSpeech() const;

  String portraitImage(String const& species, String const& frame = "idle.0") const;
  Animation animation(String const& species, String const& animationName) const;
  Animation staticAnimation(String const& species) const;
  Animation scanlineAnimation() const;

  float charactersPerSecond() const;
  String defaultAnimation() const;

private:
  struct AiAnimationConfig {
    StringMap<Animation> aiAnimations;
    String defaultAnimation;
    float charactersPerSecond;

    Animation staticAnimation;
    float staticOpacity;

    Animation scanlineAnimation;
    float scanlineOpacity;
  };

  struct AiSpeciesParameters {
    String aiFrames;
    String portraitFrames;
    String staticFrames;
  };

  static AiSpeech parseSpeech(Json const& v);
  static AiSpeciesParameters parseSpeciesParameters(Json const& vm);

  static AiSpeciesMissionText parseSpeciesMissionText(Json const& vm);
  static AiMission parseMission(Json const& vm);

  StringMap<AiMission> m_missions;
  StringMap<AiSpeciesParameters> m_speciesParameters;
  Map<unsigned, AiSpeech> m_shipStatus;
  AiSpeech m_noMissionsSpeech;
  AiSpeech m_noCrewSpeech;

  AiAnimationConfig m_animationConfig;
};

}
