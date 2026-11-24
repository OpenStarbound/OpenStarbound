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

// Returns a StatSet with a resource "Energy" that is exactly full at the
// given maximum.
static StatSet fullEnergy(float max) {
  StatSet stats;
  stats.addStat("MaxEnergy", max);
  stats.addResource("Energy", String("MaxEnergy"), 0.0f);
  stats.setResourceValue("Energy", max);
  return stats;
}

TEST(StatTest, FullResourceStaysFullWhenMaxChanges) {
  for (int i = 0; i < 20000; ++i) {
    float oldMax = 50.0f + i * 0.37f;
    for (float newMax : {oldMax + 10.0f, oldMax - 10.0f, oldMax * 1.1f, oldMax / 3.0f}) {
      StatSet stats = fullEnergy(oldMax);
      ASSERT_EQ(stats.resourceValue("Energy"), oldMax);

      stats.setStatBaseValue("MaxEnergy", newMax);
      ASSERT_EQ(*stats.resourceMaxValue("Energy"), newMax);
      ASSERT_EQ(stats.resourceValue("Energy"), newMax) << "old max " << oldMax << ", new max " << newMax;
      ASSERT_EQ(*stats.resourcePercentage("Energy"), 1.0f) << "old max " << oldMax << ", new max " << newMax;
    }
  }
}

TEST(StatTest, FullResourceStaysFullWhenMaxIsUnchanged) {
  for (int i = 0; i < 20000; ++i) {
    float max = 50.0f + i * 0.37f;
    StatSet stats = fullEnergy(max);
    stats.update(0.0f);
    stats.update(1.0f / 60.0f);
    ASSERT_EQ(stats.resourceValue("Energy"), max);
    ASSERT_EQ(*stats.resourcePercentage("Energy"), 1.0f);
  }
}

TEST(StatTest, PartialResourceScalesWithMax) {
  StatSet stats = fullEnergy(100.0f);
  stats.setResourceValue("Energy", 40.0f);
  stats.setStatBaseValue("MaxEnergy", 150.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Energy"), 60.0f, 0.0001f));
  EXPECT_TRUE(withinAmount(*stats.resourcePercentage("Energy"), 0.4f, 0.000001f));
  stats.setStatBaseValue("MaxEnergy", 50.0f);
  EXPECT_TRUE(withinAmount(stats.resourceValue("Energy"), 20.0f, 0.0001f));
  EXPECT_LT(*stats.resourcePercentage("Energy"), 1.0f);

  for (int i = 0; i < 2000; ++i) {
    float oldMax = 50.0f + i * 0.37f;
    StatSet sweep = fullEnergy(oldMax);
    sweep.setResourceValue("Energy", oldMax * 0.25f);
    sweep.setStatBaseValue("MaxEnergy", oldMax + 10.0f);
    ASSERT_TRUE(withinAmount(*sweep.resourcePercentage("Energy"), 0.25f, 0.00001f)) << "old max " << oldMax;
  }
}

TEST(StatTest, EmptyResourceStaysEmptyWhenMaxChanges) {
  StatSet stats = fullEnergy(100.0f);
  stats.setResourceValue("Energy", 0.0f);
  stats.setStatBaseValue("MaxEnergy", 110.0f);
  EXPECT_EQ(stats.resourceValue("Energy"), 0.0f);
  stats.setStatBaseValue("MaxEnergy", 60.0f);
  EXPECT_EQ(stats.resourceValue("Energy"), 0.0f);
}

TEST(StatTest, ResourceWithFixedMaxStaysFull) {
  StatSet stats;
  stats.addResource("Fixed", 123.456f, 0.0f);
  stats.setResourceValue("Fixed", 1000.0f);
  stats.update(0.0f);
  EXPECT_EQ(stats.resourceValue("Fixed"), 123.456f);
  EXPECT_EQ(*stats.resourcePercentage("Fixed"), 1.0f);
}
