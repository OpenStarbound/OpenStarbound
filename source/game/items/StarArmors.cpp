#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarImageProcessing.hpp"
#include "StarHumanoid.hpp"
#include "StarRoot.hpp"
#include "StarStoredFunctions.hpp"
#include "StarPlayer.hpp"
#include "StarDirectives.hpp"

namespace Star {

ArmorItem::ArmorItem(Json const& config, String const& directory, Json const& data) : Item(config, directory, data), SwingableItem(config) {
  refreshStatusEffects();
  m_effectSources = jsonToStringSet(instanceValue("effectSources", JsonArray()));
  m_techModule = instanceValue("techModule", "").toString();
  if (m_techModule->empty())
    m_techModule = {};
  else
    m_techModule = AssetPath::relativeTo(directory, *m_techModule);

  m_directives = instanceValue("directives", "").toString();
  m_colorOptions = colorDirectivesFromConfig(config.getArray("colorOptions", JsonArray{""}));
  if (!m_directives)
    m_directives = "?" + m_colorOptions.wrap(instanceValue("colorIndex", 0).toUInt());
  refreshIconDrawables();

  m_hideBody = config.getBool("hideBody", false);
}

List<PersistentStatusEffect> ArmorItem::statusEffects() const {
  return m_statusEffects;
}

StringSet ArmorItem::effectSources() const {
  return m_effectSources;
}

List<Drawable> ArmorItem::drawables() const {
  auto drawables = iconDrawables();
  Drawable::scaleAll(drawables, 1.0f / TilePixels);
  Drawable::translateAll(drawables, -handPosition() / TilePixels);
  return drawables;
}

float ArmorItem::getAngle(float aimAngle) {
  return -25.0f * Constants::deg2rad;
}

void ArmorItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {}
void ArmorItem::fireTriggered() {}

List<String> const& ArmorItem::colorOptions() {
  return m_colorOptions;
}

Directives const& ArmorItem::directives() const {
  return m_directives;
}

bool ArmorItem::hideBody() const {
  return m_hideBody;
}

Maybe<String> const& ArmorItem::techModule() const {
  return m_techModule;
}

void ArmorItem::refreshIconDrawables() {
  auto drawables = iconDrawables();
  for (auto& drawable : drawables) {
    if (drawable.isImage()) {
      drawable.imagePart().removeDirectives(true);
      drawable.imagePart().addDirectives(m_directives, true);
    }
  }
  setIconDrawables(std::move(drawables));
}

void ArmorItem::refreshStatusEffects() {
  m_statusEffects = instanceValue("statusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  if (auto leveledStatusEffects = instanceValue("leveledStatusEffects", Json())) {
    auto functionDatabase = Root::singleton().functionDatabase();
    float level = instanceValue("level", 1).toFloat();
    for (auto effectConfig : leveledStatusEffects.iterateArray()) {
      float levelFunctionFactor = functionDatabase->function(effectConfig.getString("levelFunction"))->evaluate(level);
      auto statModifier = jsonToStatModifier(effectConfig);
      if (auto p = statModifier.ptr<StatBaseMultiplier>())
        p->baseMultiplier = 1 + (p->baseMultiplier - 1) * levelFunctionFactor;
      else if (auto p = statModifier.ptr<StatValueModifier>())
        p->value *= levelFunctionFactor;
      else if (auto p = statModifier.ptr<StatEffectiveMultiplier>())
        p->effectiveMultiplier = 1 + (p->effectiveMultiplier - 1) * levelFunctionFactor;
      m_statusEffects.append(statModifier);
    }
  }
  if (auto augmentConfig = instanceValue("currentAugment", Json()))
    m_statusEffects.appendAll(augmentConfig.getArray("effects", JsonArray()).transformed(jsonToPersistentStatusEffect));
}

HeadArmor::HeadArmor(Json const& config, String const& directory, Json const& data)
  : ArmorItem(config, directory, data) {
  m_maleImage = AssetPath::relativeTo(directory, config.getString("maleFrames"));
  m_femaleImage = AssetPath::relativeTo(directory, config.getString("femaleFrames"));

  String maskDirectivesStr = instanceValue("mask").toString();
  if (!maskDirectivesStr.empty() && !maskDirectivesStr.contains("?"))
    m_maskDirectives = "?addmask=" + AssetPath::relativeTo(directory, maskDirectivesStr) + ";0;0";
  else
    m_maskDirectives = maskDirectivesStr;
}

ItemPtr HeadArmor::clone() const {
  return make_shared<HeadArmor>(*this);
}

String const& HeadArmor::frameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleImage;
  else
    return m_femaleImage;
}

Directives const& HeadArmor::maskDirectives() const {
  return m_maskDirectives;
}

List<Drawable> HeadArmor::preview(PlayerPtr const& viewer) const {
  Gender gender = viewer ? viewer->gender() : Gender::Male;
  //Humanoid humanoid = Humanoid::makeDummy(gender);
  Humanoid humanoid = viewer ? *viewer->humanoid() : Humanoid::makeDummy(gender);
  return humanoid.renderDummy(gender, this, nullptr, nullptr, nullptr);
}

ChestArmor::ChestArmor(Json const& config, String const& directory, Json const& data)
  : ArmorItem(config, directory, data) {
  Json maleImages = config.get("maleFrames");
  m_maleBodyImage = AssetPath::relativeTo(directory, maleImages.getString("body"));
  m_maleFrontSleeveImage = AssetPath::relativeTo(directory, maleImages.getString("frontSleeve"));
  m_maleBackSleeveImage = AssetPath::relativeTo(directory, maleImages.getString("backSleeve"));

  Json femaleImages = config.get("femaleFrames");
  m_femaleBodyImage = AssetPath::relativeTo(directory, femaleImages.getString("body"));
  m_femaleFrontSleeveImage = AssetPath::relativeTo(directory, femaleImages.getString("frontSleeve"));
  m_femaleBackSleeveImage = AssetPath::relativeTo(directory, femaleImages.getString("backSleeve"));
}

ItemPtr ChestArmor::clone() const {
  return make_shared<ChestArmor>(*this);
}

String const& ChestArmor::bodyFrameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleBodyImage;
  else
    return m_femaleBodyImage;
}

String const& ChestArmor::frontSleeveFrameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleFrontSleeveImage;
  else
    return m_femaleFrontSleeveImage;
}

String const& ChestArmor::backSleeveFrameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleBackSleeveImage;
  else
    return m_femaleBackSleeveImage;
}

List<Drawable> ChestArmor::preview(PlayerPtr const& viewer) const {
  Gender gender = viewer ? viewer->gender() : Gender::Male;
  //Humanoid humanoid = Humanoid::makeDummy(gender);
  Humanoid humanoid = viewer ? *viewer->humanoid() : Humanoid::makeDummy(gender);
  return humanoid.renderDummy(gender, nullptr, this, nullptr, nullptr);
}

LegsArmor::LegsArmor(Json const& config, String const& directory, Json const& data)
  : ArmorItem(config, directory, data) {
  m_maleImage = AssetPath::relativeTo(directory, config.getString("maleFrames"));
  m_femaleImage = AssetPath::relativeTo(directory, config.getString("femaleFrames"));
}

ItemPtr LegsArmor::clone() const {
  return make_shared<LegsArmor>(*this);
}

String const& LegsArmor::frameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleImage;
  else
    return m_femaleImage;
}

List<Drawable> LegsArmor::preview(PlayerPtr const& viewer) const {
  Gender gender = viewer ? viewer->gender() : Gender::Male;
  //Humanoid humanoid = Humanoid::makeDummy(gender);
  Humanoid humanoid = viewer ? *viewer->humanoid() : Humanoid::makeDummy(gender);
  return humanoid.renderDummy(gender, nullptr, nullptr, this, nullptr);
}

BackArmor::BackArmor(Json const& config, String const& directory, Json const& data)
  : ArmorItem(config, directory, data) {
  m_maleImage = AssetPath::relativeTo(directory, config.getString("maleFrames"));
  m_femaleImage = AssetPath::relativeTo(directory, config.getString("femaleFrames"));
}

ItemPtr BackArmor::clone() const {
  return make_shared<BackArmor>(*this);
}

String const& BackArmor::frameset(Gender gender) const {
  if (gender == Gender::Male)
    return m_maleImage;
  else
    return m_femaleImage;
}

List<Drawable> BackArmor::preview(PlayerPtr const& viewer) const {
  Gender gender = viewer ? viewer->gender() : Gender::Male;
  //Humanoid humanoid = Humanoid::makeDummy(gender);
  Humanoid humanoid = viewer ? *viewer->humanoid() : Humanoid::makeDummy(gender);
  return humanoid.renderDummy(gender, nullptr, nullptr, nullptr, this);
}

}
