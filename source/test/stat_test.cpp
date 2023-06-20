#include "StarStatCollection.hpp"

#include "gtest/gtest.h"

using namespace Star;

bool withinAmount(float value, float target, float amount) {
  return fabs(value - target) <= amount;
}

TEST(StatTest, Set) {
  StatSet stats;

  stats.addStat("MaxHealth", 100.0f);
  stats.addStat("HealthRegen", 0.0f);
  stats.addStat("MaxEnergy", 100.0f);
  stats.addStat("EnergyRegen", 0.0f);

  EXPECT_EQ(stats.statBaseValue("MaxHealth"), 100.0f);
  EXPECT_EQ(stats.statEffectiveValue("MaxHealth"), 100.0f);
  EXPECT_EQ(stats.statBaseValue("MaxEnergy"), 100.0f);
  EXPECT_EQ(stats.statEffectiveValue("MaxEnergy"), 100.0f);

  stats.addResource("Health", String("MaxHealth"), String("HealthRegen"));
  stats.setResourceValue("Health", 110.0f);
  stats.addResource("Energy", String("MaxEnergy"), 0.0f);
  stats.setResourceValue("Energy", 112.0f);
  stats.addResource("Experience");

  EXPECT_EQ(stats.resourceValue("Health"), 100.0f);
  EXPECT_EQ(stats.resourceValue("Energy"), 100.0f);
  EXPECT_EQ(stats.resourceValue("Experience"), 0.0f);

  stats.setStatBaseValue("MaxHealth", 200.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 200.0f, 0.0001f));

  stats.modifyResourceValue("Health", -100.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 100.0f, 0.0001f));

  stats.modifyResourceValue("Experience", -100.0f);
  EXPECT_EQ(stats.resourceValue("Experience"), 0.0f);
  stats.modifyResourceValue("Experience", 1000.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Experience"), 1000.0f, 0.0001f));

  stats.setStatBaseValue("HealthRegen", 100.0f);
  stats.update(0.01f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 101.0f, 0.0001f));
  stats.update(2.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 200.0f, 0.0001f));
  stats.setStatBaseValue("HealthRegen", 0.0f);

  auto id = stats.addStatModifierGroup({StatValueModifier{"HealthRegen", -50.0f}});
  stats.update(1.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 150.0f, 0.0001f));
  stats.removeStatModifierGroup(id);
  stats.update(1.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Health"), 150.0f, 0.0001f));

  id = stats.addStatModifierGroup({StatBaseMultiplier{"MaxHealth", 1.1f},
      StatEffectiveMultiplier{"MaxHealth", 1.1f},
      StatEffectiveMultiplier{"MaxHealth", 1.2f},
      StatValueModifier{"MaxHealth", 50.0}});
  // 200 (base) + 20 (base perc mod) + 50 (value mod) = 270 ...
  // * 1.1 (eff perc mod) * 1.2 (eff perc mod) = 356.4
  EXPECT_TRUE(withinAmount(stats.statEffectiveValue("MaxHealth"), 356.4f, 0.0001f));
  stats.removeStatModifierGroup(id);

  id = stats.addStatModifierGroup({StatBaseMultiplier{"MaxHealth", 1.5f}, StatBaseMultiplier{"MaxHealth", 1.5f}});
  // 200 (base) + 100 (base perc mod) + 100 (base perc mod) -- make sure base
  // perc mods do NOT stack
  // with each other
  EXPECT_TRUE(withinAmount(stats.statEffectiveValue("MaxHealth"), 400.0f, 0.0001f));
  stats.removeStatModifierGroup(id);

  EXPECT_FALSE(stats.isEffectiveStat("TempStat"));
  EXPECT_TRUE(withinAmount(stats.statEffectiveValue("TempStat"), 0.0f, 0.0001f));
  auto nextId = stats.addStatModifierGroup({StatValueModifier{"TempStat", 20}});
  EXPECT_TRUE(withinAmount(stats.statEffectiveValue("TempStat"), 20.0f, 0.0001f));
  EXPECT_TRUE(stats.isEffectiveStat("TempStat"));
  stats.removeStatModifierGroup(nextId);
  EXPECT_TRUE(withinAmount(stats.statEffectiveValue("TempStat"), 0.0f, 0.0001f));
  EXPECT_FALSE(stats.isEffectiveStat("TempStat"));
}
