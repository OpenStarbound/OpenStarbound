#include "StarRoot.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(RootTest, All) {
  auto root = Root::singletonPtr();
  EXPECT_TRUE(root);

  EXPECT_TRUE((bool)root->assets());
  EXPECT_TRUE((bool)root->objectDatabase());
  EXPECT_TRUE((bool)root->plantDatabase());
  EXPECT_TRUE((bool)root->projectileDatabase());
  EXPECT_TRUE((bool)root->monsterDatabase());
  EXPECT_TRUE((bool)root->npcDatabase());
  EXPECT_TRUE((bool)root->playerFactory());
  EXPECT_TRUE((bool)root->entityFactory());
  EXPECT_TRUE((bool)root->nameGenerator());
  EXPECT_TRUE((bool)root->itemDatabase());
  EXPECT_TRUE((bool)root->materialDatabase());
  EXPECT_TRUE((bool)root->terrainDatabase());
  EXPECT_TRUE((bool)root->biomeDatabase());
  EXPECT_TRUE((bool)root->liquidsDatabase());
  EXPECT_TRUE((bool)root->statusEffectDatabase());
  EXPECT_TRUE((bool)root->damageDatabase());
  EXPECT_TRUE((bool)root->particleDatabase());
  EXPECT_TRUE((bool)root->effectSourceDatabase());
  EXPECT_TRUE((bool)root->functionDatabase());
  EXPECT_TRUE((bool)root->treasureDatabase());
  EXPECT_TRUE((bool)root->dungeonDefinitions());
  EXPECT_TRUE((bool)root->emoteProcessor());
  EXPECT_TRUE((bool)root->speciesDatabase());
  EXPECT_TRUE((bool)root->imageMetadataDatabase());
  EXPECT_TRUE((bool)root->versioningDatabase());
  EXPECT_TRUE((bool)root->questTemplateDatabase());
  EXPECT_TRUE((bool)root->aiDatabase());
  EXPECT_TRUE((bool)root->techDatabase());
  EXPECT_TRUE((bool)root->codexDatabase());
  EXPECT_TRUE((bool)root->stagehandDatabase());
  EXPECT_TRUE((bool)root->behaviorDatabase());
  EXPECT_TRUE((bool)root->tenantDatabase());
  EXPECT_TRUE((bool)root->danceDatabase());
  EXPECT_TRUE((bool)root->spawnTypeDatabase());

  root->reload();

  EXPECT_TRUE((bool)root->assets());
  EXPECT_TRUE((bool)root->objectDatabase());
  EXPECT_TRUE((bool)root->plantDatabase());
  EXPECT_TRUE((bool)root->projectileDatabase());
  EXPECT_TRUE((bool)root->monsterDatabase());
  EXPECT_TRUE((bool)root->npcDatabase());
  EXPECT_TRUE((bool)root->playerFactory());
  EXPECT_TRUE((bool)root->entityFactory());
  EXPECT_TRUE((bool)root->nameGenerator());
  EXPECT_TRUE((bool)root->itemDatabase());
  EXPECT_TRUE((bool)root->materialDatabase());
  EXPECT_TRUE((bool)root->terrainDatabase());
  EXPECT_TRUE((bool)root->biomeDatabase());
  EXPECT_TRUE((bool)root->liquidsDatabase());
  EXPECT_TRUE((bool)root->statusEffectDatabase());
  EXPECT_TRUE((bool)root->damageDatabase());
  EXPECT_TRUE((bool)root->particleDatabase());
  EXPECT_TRUE((bool)root->effectSourceDatabase());
  EXPECT_TRUE((bool)root->functionDatabase());
  EXPECT_TRUE((bool)root->treasureDatabase());
  EXPECT_TRUE((bool)root->dungeonDefinitions());
  EXPECT_TRUE((bool)root->emoteProcessor());
  EXPECT_TRUE((bool)root->speciesDatabase());
  EXPECT_TRUE((bool)root->imageMetadataDatabase());
  EXPECT_TRUE((bool)root->versioningDatabase());
  EXPECT_TRUE((bool)root->questTemplateDatabase());
  EXPECT_TRUE((bool)root->aiDatabase());
  EXPECT_TRUE((bool)root->techDatabase());
  EXPECT_TRUE((bool)root->codexDatabase());
  EXPECT_TRUE((bool)root->stagehandDatabase());
  EXPECT_TRUE((bool)root->behaviorDatabase());
  EXPECT_TRUE((bool)root->tenantDatabase());
  EXPECT_TRUE((bool)root->danceDatabase());
  EXPECT_TRUE((bool)root->spawnTypeDatabase());
}
