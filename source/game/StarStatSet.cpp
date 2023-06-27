#include "StarStatSet.hpp"
#include "StarMathCommon.hpp"

namespace Star {

void StatSet::addStat(String statName, float baseValue) {
  if (!m_baseStats.insert(move(statName), baseValue).second)
    throw StatusException::format("Added duplicate stat named '{}' in StatSet", statName);
  update(0.0f);
}

void StatSet::removeStat(String const& statName) {
  if (!m_baseStats.remove(statName))
    throw StatusException::format("No such base stat '{}' in StatSet", statName);
  update(0.0f);
}

StringList StatSet::baseStatNames() const {
  return m_baseStats.keys();
}

bool StatSet::isBaseStat(String const& statName) const {
  return m_baseStats.contains(statName);
}

float StatSet::statBaseValue(String const& statName) const {
  if (auto s = m_baseStats.ptr(statName))
    return *s;
  throw StatusException::format("No such base stat '{}' in StatSet", statName);
}

void StatSet::setStatBaseValue(String const& statName, float value) {
  if (auto s = m_baseStats.ptr(statName)) {
    if (*s != value) {
      *s = value;
      update(0.0f);
    }
  } else {
    throw StatusException::format("No such base stat '{}' in StatSet", statName);
  }
}

StatModifierGroupId StatSet::addStatModifierGroup(List<StatModifier> modifiers) {
  bool empty = modifiers.empty();
  auto id = m_statModifierGroups.add(move(modifiers));
  if (!empty)
    update(0.0f);
  return id;
}

List<StatModifierGroupId> StatSet::statModifierGroupIds() const {
  return m_statModifierGroups.keys();
}

List<StatModifier> StatSet::statModifierGroup(StatModifierGroupId modifierGroupId) const {
  return m_statModifierGroups.get(modifierGroupId);
}

void StatSet::addStatModifierGroup(StatModifierGroupId groupId, List<StatModifier> modifiers) {
  bool empty = modifiers.empty();
  m_statModifierGroups.add(groupId, move(modifiers));
  if (!empty)
    update(0.0f);
}

bool StatSet::setStatModifierGroup(StatModifierGroupId groupId, List<StatModifier> modifiers) {
  auto& list = m_statModifierGroups.get(groupId);
  if (list != modifiers) {
    list = move(modifiers);
    update(0.0f);
    return true;
  }

  return false;
}

bool StatSet::removeStatModifierGroup(StatModifierGroupId modifierSetId) {
  if (m_statModifierGroups.remove(modifierSetId)) {
    update(0.0f);
    return true;
  }
  return false;
}

void StatSet::clearStatModifiers() {
  if (!m_statModifierGroups.empty()) {
    m_statModifierGroups.clear();
    update(0.0f);
  }
}

StatModifierGroupMap const& StatSet::allStatModifierGroups() const {
  return m_statModifierGroups;
}

void StatSet::setAllStatModifierGroups(StatModifierGroupMap map) {
  if (m_statModifierGroups != map) {
    m_statModifierGroups = move(map);
    update(0.0f);
  }
}

StringList StatSet::effectiveStatNames() const {
  return m_effectiveStats.keys();
}

bool StatSet::isEffectiveStat(String const& statName) const {
  return m_effectiveStats.contains(statName);
}

float StatSet::statEffectiveValue(String const& statName) const {
  // All stat values will be added to m_effectiveStats regardless of whether a
  // modifier is applied for it.
  if (auto modified = m_effectiveStats.ptr(statName))
    return modified->effectiveModifiedValue;
  else
    return 0.0f;
}

void StatSet::addResource(String resourceName, MVariant<String, float> max, MVariant<String, float> delta) {
  auto pair = m_resources.insert({move(resourceName), Resource{move(max), move(delta), false, 0.0f, {}}});
  if (!pair.second)
    throw StatusException::format("Added duplicate resource named '{}' in StatSet", resourceName);
  update(0.0f);
}

void StatSet::removeResource(String const& resourceName) {
  if (!m_resources.remove(resourceName))
    throw StatusException::format("No such resource named '{}' in StatSet", resourceName);
}

StringList StatSet::resourceNames() const {
  return m_resources.keys();
}

MVariant<String, float> StatSet::resourceMax(String const& resourceName) const {
  return getResource(resourceName).max;
}

MVariant<String, float> StatSet::resourceDelta(String const& resourceName) const {
  return getResource(resourceName).delta;
}

bool StatSet::isResource(String const& resourceName) const {
  return m_resources.contains(resourceName);
}

float StatSet::resourceValue(String const& resourceName) const {
  if (auto r = m_resources.ptr(resourceName))
    return r->value;
  return 0.0f;
}

float StatSet::setResourceValue(String const& resourceName, float value) {
  return getResource(resourceName).setValue(value);
}

float StatSet::modifyResourceValue(String const& resourceName, float amount) {
  auto& resource = getResource(resourceName);
  return resource.setValue(resource.value + amount);
}

float StatSet::giveResourceValue(String const& resourceName, float amount) {
  if (auto r = m_resources.ptr(resourceName)) {
    float previousValue = r->value;
    r->setValue(r->value + amount);
    return r->value - previousValue;
  }
  return 0;
}

bool StatSet::consumeResourceValue(String const& resourceName, float amount) {
  return consumeResourceValue(resourceName, amount, false);
}

bool StatSet::overConsumeResourceValue(String const& resourceName, float amount) {
  return consumeResourceValue(resourceName, amount, true);
}

bool StatSet::resourceLocked(String const& resourceName) const {
  return getResource(resourceName).locked;
}

void StatSet::setResourceLocked(String const& resourceName, bool locked) {
  getResource(resourceName).locked = locked;
}

Maybe<float> StatSet::resourceMaxValue(String const& resourceName) const {
  return getResource(resourceName).maxValue;
}

Maybe<float> StatSet::resourcePercentage(String const& resourceName) const {
  auto const& resource = getResource(resourceName);
  if (!resource.maxValue)
    return {};
  return resource.value / *resource.maxValue;
}

float StatSet::setResourcePercentage(String const& resourceName, float resourcePercentage) {
  auto& resource = getResource(resourceName);
  if (!resource.maxValue)
    throw StatusException::format("setResourcePersentage called on resource '{}' which has no maximum", resourceName);
  return resource.setValue(resourcePercentage * *resource.maxValue);
}

float StatSet::modifyResourcePercentage(String const& resourceName, float resourcePercentage) {
  auto& resource = getResource(resourceName);
  if (!resource.maxValue)
    throw StatusException::format(
        "modifyResourcePercentage called on resource '{}' which has no maximum", resourceName);
  return resource.setValue(resource.value + resourcePercentage * *resource.maxValue);
}

void StatSet::update(float dt) {
  // We use two intermediate values for calculating the effective stat value.
  // The baseModifiedValue represents the application of the base percentage
  // modifiers and the value modifiers, which only depend on the baseValue.
  // The effectiveModifiedValue is the application of all effective percentage
  // modifiers successively on the baseModifiedValue, causing them to stack with
  // each other in addition to base multipliers and value modifiers

  // First, clear the modified values to get rid of temporary stats applied
  // from modifiers that may no longer be there
  m_effectiveStats.clear();

  // Then we do all the StatValueModifiers and StatBaseMultipliers and
  // compute the baseModifiedValue

  for (auto& p : m_baseStats) {
    auto& stat = m_effectiveStats[p.first];
    stat.baseValue = p.second;
    stat.baseModifiedValue = stat.baseValue;
  }

  for (auto const& p : m_statModifierGroups) {
    for (auto const& modifier : p.second) {
      if (auto baseMultiplier = modifier.ptr<StatBaseMultiplier>()) {
        auto& stat = m_effectiveStats[baseMultiplier->statName];
        stat.baseModifiedValue += (baseMultiplier->baseMultiplier - 1.0f) * stat.baseValue;
      } else if (auto valueModifier = modifier.ptr<StatValueModifier>()) {
        auto& stat = m_effectiveStats[valueModifier->statName];
        stat.baseModifiedValue += valueModifier->value;
      }
    }
  }

  // Then we do all the StatEffectiveMultipliers and compute the
  // final effectiveModifiedValue

  for (auto& p : m_effectiveStats)
    p.second.effectiveModifiedValue = p.second.baseModifiedValue;

  for (auto const& p : m_statModifierGroups) {
    for (auto const& modifier : p.second) {
      if (auto effectiveMultiplier = modifier.ptr<StatEffectiveMultiplier>()) {
        auto& stat = m_effectiveStats[effectiveMultiplier->statName];
        stat.effectiveModifiedValue *= effectiveMultiplier->effectiveMultiplier;
      }
    }
  }

  // Then update all the resources due to charging and percentage tracking,
  // after updating the stats.

  for (auto& p : m_resources) {
    Maybe<float> newMaxValue;
    if (p.second.max.is<String>())
      newMaxValue = statEffectiveValue(p.second.max.get<String>());
    else if (p.second.max.is<float>())
      newMaxValue = p.second.max.get<float>();

    // If the resource has a maximum value, rather than keeping the absolute
    // value of the resource the same between updates, the resource value
    // should instead track the percentage.
    if (p.second.maxValue && newMaxValue && *p.second.maxValue > 0.0f)
      p.second.value *= *newMaxValue / *p.second.maxValue;

    p.second.maxValue = newMaxValue;
    if (p.second.maxValue)
      p.second.value = clamp(p.second.value, 0.0f, *p.second.maxValue);

    if (dt != 0.0f) {
      float delta = 0.0f;
      if (p.second.delta.is<String>())
        delta = statEffectiveValue(p.second.delta.get<String>());
      else if (p.second.delta.is<float>())
        delta = p.second.delta.get<float>();
      p.second.setValue(p.second.value + delta * dt);
    }
  }
}

float StatSet::Resource::setValue(float v) {
  if (maxValue)
    value = clamp(v, 0.0f, *maxValue);
  else
    value = Star::max(v, 0.0f);
  return value;
}

StatSet::Resource const& StatSet::getResource(String const& resourceName) const {
  if (auto r = m_resources.ptr(resourceName))
    return *r;
  throw StatusException::format("No such resource '{}' in StatSet", resourceName);
}

StatSet::Resource& StatSet::getResource(String const& resourceName) {
  if (auto r = m_resources.ptr(resourceName))
    return *r;
  throw StatusException::format("No such resource '{}' in StatSet", resourceName);
}

bool StatSet::consumeResourceValue(String const& resourceName, float amount, bool allowOverConsume) {
  if (amount < 0.0f)
    throw StatusException::format("StatSet, consumeResource called with negative amount '{}' {}", resourceName, amount);

  if (auto r = m_resources.ptr(resourceName)) {
    if (r->locked)
      return false;

    if (r->value >= amount) {
      r->setValue(r->value - amount);
      return true;
    } else if (r->value > 0.0f && allowOverConsume) {
      r->setValue(0.0f);
      return true;
    }
  }
  return false;
}

}
