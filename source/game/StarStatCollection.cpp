#include "StarStatCollection.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarGameTypes.hpp"
#include "StarLogging.hpp"

namespace Star {

StatCollection::StatCollection(Json const& config) {
  for (auto const& stat : config.getObject("stats", {}))
    m_stats.addStat(stat.first, stat.second.getFloat("baseValue", 0.0));

  for (auto const& resource : config.getObject("resources", {})) {
    auto statOrValue = [&resource](String const& statName, String const& valueName, MVariant<String, float> def = {}) -> MVariant<String, float> {
      if (auto maxStat = resource.second.optString(statName))
        return *maxStat;
      else if (auto maxValue = resource.second.optFloat(valueName))
        return *maxValue;
      else
        return def;
    };

    MVariant<String, float> resourceMax = statOrValue("maxStat", "maxValue");
    MVariant<String, float> resourceDelta = statOrValue("deltaStat", "deltaValue");
    m_stats.addResource(resource.first, resourceMax, resourceDelta);

    if (auto initialValue = resource.second.optFloat("initialValue")) {
      m_stats.setResourceValue(resource.first, *initialValue);
      m_defaultResourceValues[resource.first] = makeLeft(*initialValue);
    } else if (auto percentage = resource.second.optFloat("initialPercentage")) {
      m_stats.setResourcePercentage(resource.first, *percentage);
      m_defaultResourceValues[resource.first] = makeRight(*percentage);
    } else {
      if (m_stats.resourceMax(resource.first)) {
        m_stats.setResourcePercentage(resource.first, 1.0f);
        m_defaultResourceValues[resource.first] = makeRight(1.0f);
      } else {
        m_stats.setResourceValue(resource.first, 0.0f);
        m_defaultResourceValues[resource.first] = makeLeft(0.0f);
      }
    }
  }

  addNetElement(&m_statModifiersNetState);

  // Sort resource names alphabetically to ensure the same order on master and
  // slaves.
  for (auto const& resource : m_stats.resourceNames().sorted()) {
    auto& resourceNetState = m_resourceValuesNetStates[resource];
    addNetElement(&resourceNetState);
    addNetElement(&m_resourceLockedNetStates[resource]);
  }
}

StringList StatCollection::statNames() const {
  return m_stats.effectiveStatNames();
}

float StatCollection::stat(String const& statName) const {
  return m_stats.statEffectiveValue(statName);
}

bool StatCollection::statPositive(String const& statName) const {
  return stat(statName) > 0.0f;
}

StringList StatCollection::resourceNames() const {
  return m_stats.resourceNames();
}

bool StatCollection::isResource(String const& resourceName) const {
  return m_stats.isResource(resourceName);
}

float StatCollection::resource(String const& resourceName) const {
  return m_stats.resourceValue(resourceName);
}

bool StatCollection::resourcePositive(String const& resourceName) const {
  return resource(resourceName) > 0.0f;
}

void StatCollection::setResource(String const& resourceName, float value) {
  m_stats.setResourceValue(resourceName, value);
}

void StatCollection::modifyResource(String const& resourceName, float amount) {
  m_stats.modifyResourceValue(resourceName, amount);
}

float StatCollection::giveResource(String const& resourceName, float amount) {
  return m_stats.giveResourceValue(resourceName, amount);
}

bool StatCollection::consumeResource(String const& resourceName, float amount) {
  return m_stats.consumeResourceValue(resourceName, amount);
}

bool StatCollection::overConsumeResource(String const& resourceName, float amount) {
  return m_stats.overConsumeResourceValue(resourceName, amount);
}

bool StatCollection::resourceLocked(String const& resourceName) const {
  return m_stats.resourceLocked(resourceName);
}

void StatCollection::setResourceLocked(String const& resourceName, bool locked) {
  m_stats.setResourceLocked(resourceName, locked);
}

void StatCollection::resetResource(String const& resourceName) {
  m_stats.setResourceLocked(resourceName, false);
  auto def = m_defaultResourceValues.get(resourceName);
  if (def.isLeft())
    m_stats.setResourceValue(resourceName, def.left());
  else
    m_stats.setResourcePercentage(resourceName, def.right());
}

void StatCollection::resetAllResources() {
  for (auto const& resourceName : m_stats.resourceNames())
    resetResource(resourceName);
}

Maybe<float> StatCollection::resourceMax(String const& resourceName) const {
  return m_stats.resourceMaxValue(resourceName);
}

Maybe<float> StatCollection::resourcePercentage(String const& resourceName) const {
  return m_stats.resourcePercentage(resourceName);
}

float StatCollection::setResourcePercentage(String const& resourceName, float resourcePercentage) {
  return m_stats.setResourcePercentage(resourceName, resourcePercentage);
}

float StatCollection::modifyResourcePercentage(String const& resourceName, float resourcePercentage) {
  return m_stats.modifyResourcePercentage(resourceName, resourcePercentage);
}

StatModifierGroupId StatCollection::addStatModifierGroup(List<StatModifier> modifiers) {
  return m_stats.addStatModifierGroup(modifiers);
}

void StatCollection::setStatModifierGroup(StatModifierGroupId modifierGroupId, List<StatModifier> modifiers) {
  m_stats.setStatModifierGroup(modifierGroupId, modifiers);
}

void StatCollection::removeStatModifierGroup(StatModifierGroupId modifierGroupId) {
  m_stats.removeStatModifierGroup(modifierGroupId);
}

void StatCollection::clearStatModifiers() {
  m_stats.clearStatModifiers();
}

void StatCollection::tickMaster(float dt) {
  m_stats.update(dt);
}

void StatCollection::tickSlave(float dt) {
  m_stats.update(0.0f);
}

void StatCollection::netElementsNeedLoad(bool) {
  if (m_statModifiersNetState.pullUpdated()) {
    StatModifierGroupMap allModifiers;
    for (auto const& p : m_statModifiersNetState)
      allModifiers.add(p.first, p.second);
    m_stats.setAllStatModifierGroups(std::move(allModifiers));
  }

  for (auto const& pair : m_resourceValuesNetStates)
    m_stats.setResourceValue(pair.first, pair.second.get());

  for (auto& pair : m_resourceLockedNetStates)
    m_stats.setResourceLocked(pair.first, pair.second.get());
}

void StatCollection::netElementsNeedStore() {
  m_statModifiersNetState.setContents(m_stats.allStatModifierGroups());

  for (auto& pair : m_resourceValuesNetStates)
    pair.second.set(m_stats.resourceValue(pair.first));

  for (auto& pair : m_resourceLockedNetStates)
    pair.second.set(m_stats.resourceLocked(pair.first));
}

}
