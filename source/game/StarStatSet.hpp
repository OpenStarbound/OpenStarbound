#ifndef STAR_STAT_SET_HPP
#define STAR_STAT_SET_HPP

#include "StarStatusTypes.hpp"

namespace Star {

STAR_CLASS(StatSet);

// Manages a collection of Stats and Resources.
//
// Stats are named floating point values of any base value, with an arbitrary
// number of "stat modifiers" attached to them.  Stat modifiers can be added
// and removed in groups, and they can either raise or lower stats by a
// constant value or a percentage of the stat value without any other
// percentage modifications applied.  The effective stat value is always the
// value with all mods applied.  If a modifier is created for a stat that does
// not exist, there will be an effective stat value for the modified stat, but
// NO base stat.  If the modifier is a base percentage modifier, it will have
// no effect because it is assumed that base stats that do not exist are zero.
//
// Resources are also named floating point values, but are in a different
// namespaced and are intended to be used as values that change regularly.
// They are always >= 0.0f, and optionally have a maximum value based on a
// given value or stat.  In addition to a max value, they can also have a
// "delta" value or stat, which automatically adds or removes that delta to the
// resource every second.
//
// If a resource has a maximum value, then rather than trying to keep the
// *value* of the resource constant, this class will instead attempt to keep
// the *percentage* of the resource constant across stat changes.  For example,
// if "health" is a stat with a max of 100, and the current health value is 50,
// and the max health stat is changed to 200 through any means, the health
// value will automatically update to 100.
class StatSet {
public:
  void addStat(String statName, float baseValue = 0.0f);
  void removeStat(String const& statName);

  // Only lists base stats added with addStat, not stats that come only from
  // modifiers
  StringList baseStatNames() const;
  bool isBaseStat(String const& statName) const;

  // Throws when the stat is not a base stat that is added via addStat.
  float statBaseValue(String const& statName) const;
  void setStatBaseValue(String const& statName, float value);

  List<StatModifierGroupId> statModifierGroupIds() const;
  List<StatModifier> statModifierGroup(StatModifierGroupId modifierGroupId) const;

  StatModifierGroupId addStatModifierGroup(List<StatModifier> modifiers = {});
  void addStatModifierGroup(StatModifierGroupId groupId, List<StatModifier> modifiers);
  bool setStatModifierGroup(StatModifierGroupId groupId, List<StatModifier> modifiers);
  bool removeStatModifierGroup(StatModifierGroupId modifierGroupId);
  void clearStatModifiers();

  StatModifierGroupMap const& allStatModifierGroups() const;
  void setAllStatModifierGroups(StatModifierGroupMap map);

  StringList effectiveStatNames() const;

  // Does this stat exist either from the base stats or the modifiers
  bool isEffectiveStat(String const& statName) const;

  // Will never throw, returns either the base stat value, or the modified
  // stat value if a modifier is applied, or 0.0.  This is to support stats that
  // may come only from modifiers and have no base value.
  float statEffectiveValue(String const& statName) const;

  void addResource(String resourceName, MVariant<String, float> max = {}, MVariant<String, float> delta = {});
  void removeResource(String const& resourceName);

  MVariant<String, float> resourceMax(String const& resourceName) const;
  MVariant<String, float> resourceDelta(String const& resourceName) const;

  StringList resourceNames() const;
  bool isResource(String const& resourceName) const;

  // Will never throw, returns either the resource value, or 0.0 for a missing
  // resource
  float resourceValue(String const& resourceName) const;

  float setResourceValue(String const& resourceName, float value);
  float modifyResourceValue(String const& resourceName, float amount);

  // Similar to consumeResource, will add the given amount to a resource if
  // it exists. Returns the amount by which the resource was actually increased.
  float giveResourceValue(String const& resourceName, float amount);

  // If a resource exists and has more than the given amount available, and the
  // resource is not locked, then subtracts this amount from the resource and
  // returns true.  Otherwise, does nothing and returns false.  Will only throw
  // if 'amount' is less than zero, will simply return false on missing
  // resource.
  bool consumeResourceValue(String const& resourceName, float amount);

  // Like consumeResource, but always succeeds if the resource is unlocked and
  // the amount is nonzero.  If the amount is greater than the available
  // resource, then the resource will be consumed to zero.
  bool overConsumeResourceValue(String const& resourceName, float amount);

  // A locked resource cannot be consumed in any way.
  bool resourceLocked(String const& resourceName) const;
  void setResourceLocked(String const& resourceName, bool locked);

  // If a resource has a maximum value, this will return it.
  Maybe<float> resourceMaxValue(String const& resourceName) const;
  // Returns the resource percentage if the resource has a max value.
  Maybe<float> resourcePercentage(String const& resourceName) const;
  // If the resource has a max value, then modifies the value percentage,
  // otherwise this is nonsense so throws.
  float setResourcePercentage(String const& resourceName, float resourcePercentage);
  float modifyResourcePercentage(String const& resourceName, float resourcePercentage);

  void update(float dt);

private:
  struct EffectiveStat {
    float baseValue;
    // Value with just the base percent modifiers applied and the value
    // modifiers
    float baseModifiedValue;
    // Final modified value that includes the effective modifiers.
    float effectiveModifiedValue;
  };

  struct Resource {
    MVariant<String, float> max;
    MVariant<String, float> delta;
    bool locked;
    float value;
    Maybe<float> maxValue;

    // Sets value and clamps between [0.0, maxStatValue] or just >= 0.0 if
    // maxStatValue is not given.
    float setValue(float v);
  };

  Resource const& getResource(String const& resourceName) const;
  Resource& getResource(String const& resourceName);

  bool consumeResourceValue(String const& resourceName, float amount, bool allowOverConsume);

  StringMap<float> m_baseStats;
  StringMap<EffectiveStat> m_effectiveStats;
  StatModifierGroupMap m_statModifierGroups;
  StringMap<Resource> m_resources;
};

}

#endif
