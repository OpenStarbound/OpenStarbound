#ifndef STAR_AI_TYPES_HPP
#define STAR_AI_TYPES_HPP

#include "StarOrderedSet.hpp"
#include "StarItemDescriptor.hpp"
#include "StarAnimation.hpp"
#include "StarQuestDescriptor.hpp"

namespace Star {

STAR_EXCEPTION(AiException, StarException);

struct AiSpeech {
  String animation;
  String text;
  float speedModifier;
};

struct AiState {
  AiState();
  AiState(Json const& v);

  Json toJson() const;

  OrderedHashSet<String> availableMissions;
  OrderedHashSet<String> completedMissions;
};

struct AiSpeciesMissionText {
  String buttonText;
  String repeatButtonText;
  AiSpeech selectSpeech;
};

struct AiMission {
  String missionName;
  String missionUniqueWorld;
  Maybe<String> warpAnimation;
  Maybe<bool> warpDeploy;
  String icon;
  StringMap<AiSpeciesMissionText> speciesText;
};

}

#endif
