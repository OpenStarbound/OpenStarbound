#ifndef STAR_STAT_COLLECTION_HPP
#define STAR_STAT_COLLECTION_HPP

#include "StarEither.hpp"
#include "StarNetElementSystem.hpp"
#include "StarStatSet.hpp"

namespace Star {

// Extension of StatSet that can easily be set up from config, and is network
// capable.
class StatCollection : public NetElementSyncGroup {
public:
  explicit StatCollection(Json const& config);

  StringList statNames() const;
  float stat(String const& statName) const;
  // Returns true if the stat is strictly greater than zero
  bool statPositive(String const& statName) const;

  StringList resourceNames() const;
  bool isResource(String const& resourceName) const;
  float resource(String const& resourceName) const;
  // Returns true if the resource is strictly greater than zero
  bool resourcePositive(String const& resourceName) const;

  void setResource(String const& resourceName, float value);
  void modifyResource(String const& resourceName, float amount);

  float giveResource(String const& resourceName, float amount);

  bool consumeResource(String const& resourceName, float amount);
  bool overConsumeResource(String const& resourceName, float amount);

  bool resourceLocked(String const& resourceName) const;
  void setResourceLocked(String const& resourceName, bool locked);

  // Resetting a resource also clears any locked states
  void resetResource(String const& resourceName);
  void resetAllResources();

  Maybe<float> resourceMax(String const& resourceName) const;
  Maybe<float> resourcePercentage(String const& resourceName) const;
  float setResourcePercentage(String const& resourceName, float resourcePercentage);
  float modifyResourcePercentage(String const& resourceName, float resourcePercentage);

  StatModifierGroupId addStatModifierGroup(List<StatModifier> modifiers = {});
  void setStatModifierGroup(StatModifierGroupId modifierGroupId, List<StatModifier> modifiers);
  void removeStatModifierGroup(StatModifierGroupId modifierGroupId);
  void clearStatModifiers();

  void tickMaster();
  void tickSlave();

private:
  void netElementsNeedLoad(bool full) override;
  void netElementsNeedStore() override;

  StatSet m_stats;
  // Left value is a raw value, right value is a percentage.
  StringMap<Either<float, float>> m_defaultResourceValues;

  NetElementMap<StatModifierGroupId, List<StatModifier>> m_statModifiersNetState;
  StableStringMap<NetElementFloat> m_resourceValuesNetStates;
  StableStringMap<NetElementBool> m_resourceLockedNetStates;
};

}

#endif
