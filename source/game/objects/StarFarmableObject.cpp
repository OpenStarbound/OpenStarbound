#include "StarFarmableObject.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarRandom.hpp"
#include "StarPlantDatabase.hpp"
#include "StarPlant.hpp"
#include "StarWorldServer.hpp"
#include "StarTreasure.hpp"
#include "StarItemDrop.hpp"
#include "StarLogging.hpp"
#include "StarObjectDatabase.hpp"
#include "StarMaterialDatabase.hpp"

namespace Star {

FarmableObject::FarmableObject(ObjectConfigConstPtr config, Json const& parameters) : Object(config, parameters) {
  m_stages = configValue("stages", JsonArray({JsonObject()})).toArray();
  m_stage = configValue("startingStage", 0).toInt();

  m_stageAlt = -1;
  m_stageEnterTime = 0.0;
  m_nextStageTime = 0.0;
  m_finalStage = false;

  auto assets = Root::singleton().assets();
  m_minImmersion = configValue("minImmersion", 0).toFloat();
  m_maxImmersion = configValue("maxImmersion", 2).toFloat();
  m_immersion = SlidingWindow(assets->json("/farming.config:immersionWindow").toFloat(),
      assets->json("/farming.config:immersionResolution").toUInt(), (m_minImmersion + m_maxImmersion) / 2);

  m_consumeSoilMoisture = configValue("consumeSoilMoisture", true).toBool();
}

void FarmableObject::update(uint64_t currentStep) {
  Object::update(currentStep);

  if (isMaster()) {
    if (m_nextStageTime == 0) {
      m_nextStageTime = world()->epochTime();
      enterStage(m_stage);
    }

    while (!m_finalStage && world()->epochTime() >= m_nextStageTime)
      enterStage(m_stage + 1);

    // update immersion and check whether farmable should break
    m_immersion.update(bind(&Object::liquidFillLevel, this));
    if (m_immersion.average() > m_maxImmersion || m_immersion.average() < m_minImmersion)
      breakObject(false);
  }
}

bool FarmableObject::damageTiles(List<Vec2I> const& position, Vec2F const& sourcePosition, TileDamage const& tileDamage) {
  if ((tileDamage.type != TileDamageType::Beamish && tileDamage.type != TileDamageType::Blockish && tileDamage.type != TileDamageType::Plantish) || !harvest())
    return Object::damageTiles(position, sourcePosition, tileDamage);

  return false;
}

InteractAction FarmableObject::interact(InteractRequest const&) {
  harvest();
  return {};
}

bool FarmableObject::harvest() {
  if (isMaster() && m_stages.get(m_stage).contains("harvestPool")) {
    for (auto const& treasureItem : Root::singleton().treasureDatabase()->createTreasure(m_stages.get(m_stage).getString("harvestPool"), world()->threatLevel()))
      world()->addEntity(ItemDrop::createRandomizedDrop(treasureItem, position()));

    if (m_stages.get(m_stage).contains("resetToStage")) {
      m_nextStageTime = world()->epochTime();
      enterStage(m_stages.get(m_stage).getInt("resetToStage"));
    } else
      breakObject(true);

    return true;
  }
  return false;
}

int FarmableObject::stage() const {
  return m_stage;
}

void FarmableObject::enterStage(int newStage) {
  newStage = clamp<int>(newStage, 0, m_stages.size() - 1);

  // attempt to consume water from the soil if needed
  if (m_consumeSoilMoisture && newStage > m_stage) {
    if (auto orientation = currentOrientation()) {
      auto assets = Root::singleton().assets();
      auto materialDatabase = Root::singleton().materialDatabase();
      auto wetToDryMods = assets->json("/farming.config:wetToDryMods");

      // try to transform all anchor spaces, back out and reset stage time if
      // they're not wet
      for (auto anchor : orientation->anchors) {
        auto pos = tilePosition() + anchor.position;
        if (auto newMod = wetToDryMods.optString(materialDatabase->modName(world()->mod(pos, anchor.layer)))) {
          world()->modifyTile(pos, PlaceMod{anchor.layer, materialDatabase->modId(*newMod), MaterialHue()}, true);
        } else {
          Vec2F durationRange = jsonToVec2F(m_stages.get(m_stage).get("duration", JsonArray({0, 0})));
          m_nextStageTime = world()->epochTime() + Random::randf(durationRange[0], durationRange[1]);

          return;
        }
      }
    }
  }

  // TODO: remove this hacky tree stuff and make plants handle it
  if (m_stages.get(newStage).getBool("tree", false)) {
    String stemName = configValue("stemName").toString();
    float stemHueShift = configValue("stemHueShift", 0).toFloat();
    String foliageName = configValue("foliageName", "").toString();
    float foliageHueShift = configValue("foliageHueShift", 0).toFloat();
    Vec2I position = tilePosition();

    TreeVariant tv;
    auto plantDatabase = Root::singleton().plantDatabase();
    if (!foliageName.empty())
      tv = plantDatabase->buildTreeVariant(stemName, stemHueShift, foliageName, foliageHueShift);
    else
      tv = plantDatabase->buildTreeVariant(stemName, stemHueShift);

    auto plant = plantDatabase->createPlant(tv, Random::randi64());
    plant->setTilePosition(position);

    if (anySpacesOccupied(plant->spaces()) || !allSpacesOccupied(plant->roots())) {
      newStage = 0;
    } else {
      world()->timer(2, [plant](World* world) {
        world->addEntity(plant);
      });

      m_finalStage = true;
      breakObject(true);
      return;
    }
  }

  if (newStage == (int)m_stages.size() - 1) {
    m_finalStage = true;
  } else {
    m_finalStage = false;
    m_stageEnterTime = m_nextStageTime;
    Vec2F durationRange = jsonToVec2F(m_stages.get(newStage).get("duration", JsonArray({0, 0})));
    m_nextStageTime += Random::randf(durationRange[0], durationRange[1]);
  }

  m_interactive.set(m_stages.get(newStage).contains("harvestPool"));

  // keep the same variant if stages have same number of alts
  if (m_stageAlt == -1 || m_stages.get(newStage).getInt("alts", 1) != m_stages.get(m_stage).getInt("alts", 1))
    m_stageAlt = Random::randInt(m_stages.get(newStage).getInt("alts", 1) - 1);

  m_stage = newStage;

  setImageKey("stage", toString(m_stage));
  setImageKey("alt", toString(m_stageAlt));
}

void FarmableObject::readStoredData(Json const& diskStore) {
  Object::readStoredData(diskStore);

  m_stage = diskStore.getInt("stage");
  m_stageAlt = diskStore.getInt("stageAlt");
  m_stageEnterTime = diskStore.getDouble("stageEnterTime");
  m_nextStageTime = diskStore.getDouble("nextStageTime");

  m_finalStage = (m_stage == (int)m_stages.size() - 1);
  setImageKey("stage", toString(m_stage));
  setImageKey("alt", toString(m_stageAlt));
}

Json FarmableObject::writeStoredData() const {
  return Object::writeStoredData().setAll({
      {"stage", m_stage},
      {"stageAlt", m_stageAlt},
      {"stageEnterTime", m_stageEnterTime},
      {"nextStageTime", m_nextStageTime}
    });
}

}
