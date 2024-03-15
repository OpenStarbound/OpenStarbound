#include "StarAiDatabase.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

AiDatabase::AiDatabase() {
  auto assets = Root::singleton().assets();
  auto config = assets->json("/ai/ai.config");

  auto& missions = assets->scanExtension("aimission");
  assets->queueJsons(missions);

  for (auto const& file : missions) {
    if (auto config = assets->json(file)) {
      auto mission = parseMission(config);
      m_missions[mission.missionName] = mission;
    }
  }

  for (auto const& speciesPair : config.get("species").iterateObject())
    m_speciesParameters[speciesPair.first] = parseSpeciesParameters(speciesPair.second);

  for (auto const& p : config.get("shipStatus").iterateObject())
    m_shipStatus[lexicalCast<unsigned>(p.first)] = parseSpeech(p.second);

  m_noMissionsSpeech = parseSpeech(config.get("noMissionsSpeech"));
  m_noCrewSpeech = parseSpeech(config.get("noCrewSpeech"));

  m_animationConfig.charactersPerSecond = config.getFloat("charactersPerSecond");
  m_animationConfig.defaultAnimation = config.getString("defaultAnimation");
  m_animationConfig.staticAnimation = Animation("/ai/ai.config:staticAnimation");
  m_animationConfig.staticOpacity = config.getFloat("staticOpacity");
  m_animationConfig.scanlineAnimation = Animation("/ai/ai.config:scanlineAnimation");
  m_animationConfig.scanlineOpacity = config.getFloat("scanlineOpacity");

  for (auto const& pair : config.get("aiAnimations").iterateObject())
    m_animationConfig.aiAnimations[pair.first] = Animation(pair.second, "/ai/");
}

AiMission AiDatabase::mission(String const& missionName) const {
  return m_missions.get(missionName);
}

AiSpeech AiDatabase::shipStatus(unsigned shipLevel) const {
  // Find the first open speech set at this ship level or below.
  auto i = m_shipStatus.upper_bound(shipLevel);
  if (i != m_shipStatus.begin() && (--i)->first <= shipLevel)
    return i->second;

  return {};
}

AiSpeech AiDatabase::noCrewSpeech() const {
  return m_noCrewSpeech;
}

AiSpeech AiDatabase::noMissionsSpeech() const {
  return m_noMissionsSpeech;
}

String AiDatabase::portraitImage(String const& species, String const& frame) const {
  return strf("/ai/{}:{}", m_speciesParameters.get(species).portraitFrames, frame);
}

Animation AiDatabase::animation(String const& species, String const& animationName) const {
  auto faceAnimation = m_animationConfig.aiAnimations.get(animationName);
  faceAnimation.setTag("image", m_speciesParameters.get(species).aiFrames);
  return faceAnimation;
}

Animation AiDatabase::staticAnimation(String const& species) const {
  auto staticAnimation = m_animationConfig.staticAnimation;
  staticAnimation.setTag("image", m_speciesParameters.get(species).staticFrames);
  staticAnimation.setColor(Color::rgbaf(1.0f, 1.0f, 1.0f, m_animationConfig.staticOpacity));
  return staticAnimation;
}

Animation AiDatabase::scanlineAnimation() const {
  auto animation = m_animationConfig.scanlineAnimation;
  animation.setColor(Color::rgbaf(1.0f, 1.0f, 1.0f, m_animationConfig.scanlineOpacity));
  return animation;
}

float AiDatabase::charactersPerSecond() const {
  return m_animationConfig.charactersPerSecond;
}

String AiDatabase::defaultAnimation() const {
  return m_animationConfig.defaultAnimation;
}

AiSpeech AiDatabase::parseSpeech(Json const& v) {
  return AiSpeech{v.getString("animation"), v.getString("text"), v.getFloat("speedModifier", 1.0f)};
}

AiDatabase::AiSpeciesParameters AiDatabase::parseSpeciesParameters(Json const& v) {
  AiSpeciesParameters species;
  species.aiFrames = v.getString("aiFrames");
  species.portraitFrames = v.getString("portraitFrames");
  species.staticFrames = v.getString("staticFrames");
  return species;
}

AiSpeciesMissionText AiDatabase::parseSpeciesMissionText(Json const& vm) {
  return AiSpeciesMissionText{
      vm.getString("buttonText"), vm.getString("repeatButtonText"), parseSpeech(vm.get("selectSpeech", {}))};
}

AiMission AiDatabase::parseMission(Json const& vm) {
  AiMission mission;
  mission.missionName = vm.getString("missionName");
  mission.missionUniqueWorld = vm.getString("missionWorld");
  mission.warpAnimation = vm.optString("warpAnimation");
  mission.warpDeploy = vm.optBool("warpDeploy");
  mission.icon = AssetPath::relativeTo("/ai/", vm.getString("icon"));
  for (auto const& textPair : vm.get("speciesText").iterateObject())
    mission.speciesText[textPair.first] = parseSpeciesMissionText(textPair.second);
  return mission;
}

}
