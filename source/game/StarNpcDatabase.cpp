#include "StarNpcDatabase.hpp"
#include "StarEncode.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarNpc.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarNameGenerator.hpp"
#include "StarStoredFunctions.hpp"
#include "StarAssets.hpp"
#include "StarEncode.hpp"
#include "StarArmors.hpp"

namespace Star {

NpcDatabase::NpcDatabase() {
  auto assets = Root::singleton().assets();

  auto& files = assets->scanExtension("npctype");
  assets->queueJsons(files);
  for (auto& file : files) {
    try {
      auto config = assets->json(file);
      String typeName = config.getString("type");

      if (m_npcTypes.contains(typeName))
        throw NpcException(strf("Repeat npc type name '{}'", typeName));

      m_npcTypes[typeName] = config;

    } catch (StarException const& e) {
      throw NpcException(strf("Error loading npc type '{}'", file), e);
    }
  }
}

NpcVariant NpcDatabase::generateNpcVariant(String const& species, String const& typeName, float level) const {
  return generateNpcVariant(species, typeName, level, Random::randu64(), {});
}

NpcVariant NpcDatabase::generateNpcVariant(
    String const& species, String const& typeName, float level, uint64_t seed, Json const& overrides) const {
  NpcVariant variant;
  variant.species = species;
  variant.typeName = typeName;
  variant.seed = seed;
  variant.overrides = overrides;

  RandomSource randSource(seed);

  auto config = buildConfig(typeName, overrides);

  variant.description = config.getString("description", "Some funny looking person");

  auto levelVariance = jsonToVec2F(config.getArray("levelVariance", {0, 0}));
  variant.level = max(randSource.randf(level + levelVariance[0], level + levelVariance[1]), 0.0f);

  variant.scripts = jsonToStringList(config.get("scripts"));
  variant.initialScriptDelta = config.getUInt("initialScriptDelta", 5);
  variant.scriptConfig = config.get("scriptConfig");

  HumanoidIdentity identity;
  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDefinition = speciesDatabase->species(species);

  variant.uniqueHumanoidConfig = config.contains("humanoidConfig");
  if (variant.uniqueHumanoidConfig)
    variant.humanoidConfig = Root::singleton().assets()->json(config.getString("humanoidConfig"));
  else
    variant.humanoidConfig = speciesDefinition->humanoidConfig();

  speciesDefinition->generateHumanoid(identity, seed);

  if (config.contains("npcname"))
    identity.name = config.getString("npcname");
  else if (config.contains("nameGen")) {
    identity.name = Root::singleton().nameGenerator()->generateName(
        jsonToStringList(config.get("nameGen"))[(int)identity.gender], randSource);
  }

  identity.personality = parsePersonalityArray(randSource.randFrom(variant.humanoidConfig.getArray("personalities")));

  if (config.contains("identity"))
    identity = HumanoidIdentity(jsonMerge(identity.toJson(), config.get("identity")));

  variant.humanoidIdentity = identity;

  variant.movementParameters = config.get("movementParameters", {});
  variant.statusControllerSettings = config.get("statusControllerSettings");

  auto functionDatabase = Root::singleton().functionDatabase();
  float powerMultiplierModifier = functionDatabase->function("npcLevelPowerMultiplierModifier")->evaluate(variant.level);
  float protectionMultiplier = functionDatabase->function("npcLevelProtectionMultiplier")->evaluate(variant.level);
  float maxHealthMultiplier = functionDatabase->function("npcLevelHealthMultiplier")->evaluate(variant.level);
  float maxEnergyMultiplier = functionDatabase->function("npcLevelEnergyMultiplier")->evaluate(variant.level);

  variant.innateStatusEffects = config.get("innateStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  variant.innateStatusEffects.append(StatModifier(StatValueModifier{"powerMultiplier", powerMultiplierModifier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"protection", protectionMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxHealth", maxHealthMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxEnergy", maxEnergyMultiplier}));

  variant.touchDamageConfig = config.get("touchDamage", {});

  auto itemsConfig = config.get("items", {});
  if (!itemsConfig.isNull()) {
    auto speciesItemsConfig = itemsConfig.get("override", {});
    if (speciesItemsConfig.isNull())
      speciesItemsConfig = itemsConfig.get(species, {});
    if (speciesItemsConfig.isNull())
      speciesItemsConfig = itemsConfig.get("default", {});

    if (!speciesItemsConfig.isNull()) {
      Json highestLevelItemsConfig;
      for (auto levelItemsConfig : speciesItemsConfig.toArray()) {
        if (variant.level >= levelItemsConfig.getFloat(0)) {
          highestLevelItemsConfig = levelItemsConfig.get(1);
        }
      }

      if (!highestLevelItemsConfig.isNull()) {
        int randomColorIndex = -1;
        bool matchColorIndices = config.getBool("matchColorIndices", false);

        for (auto itemSlotConfig : randSource.randFrom(highestLevelItemsConfig.toArray()).toObject()) {
          ItemDescriptor item = ItemDescriptor(randSource.randFrom(itemSlotConfig.second.toArray()));

          // Randomize color index if colorIndex is an array
          if (item.parameters().contains("colorIndex")) {
            auto colorIndex = item.parameters().get("colorIndex");
            if (colorIndex.isType(Json::Type::Array)) {
              if (!matchColorIndices || randomColorIndex == -1)
                randomColorIndex = randSource.randFrom(colorIndex.toArray()).toInt();

              item = item.applyParameters({{"colorIndex", randomColorIndex}});
            }
          }

          variant.items[itemSlotConfig.first] = std::move(item);
        }
      }
    }
  }

  variant.disableWornArmor = config.getBool("disableWornArmor", true);

  variant.dropPools = jsonToStringList(config.get("dropPools", JsonArray{}));

  variant.persistent = config.getBool("persistent", false);
  variant.keepAlive = config.getBool("keepAlive", false);

  variant.damageTeam = config.getUInt("damageTeam", 0);
  variant.damageTeamType = TeamTypeNames.getLeft(config.getString("damageTeamType", "enemy"));

  variant.nametagColor = jsonToVec3B(config.get("nametagColor", JsonArray{255, 255, 255}));

  variant.splashConfig = EntitySplashConfig(config.get("splashConfig"));

  return variant;
}

ByteArray NpcDatabase::writeNpcVariant(NpcVariant const& variant, NetCompatibilityRules rules) const {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);

  ds.write(variant.species);
  ds.write(variant.typeName);
  ds.write(variant.level);
  ds.write(variant.seed);
  ds.write(variant.overrides);

  ds.write(variant.initialScriptDelta);
  ds.write(variant.humanoidIdentity);

  ds.writeMapContainer(variant.items);

  ds.write(variant.persistent);
  ds.write(variant.keepAlive);
  ds.write(variant.damageTeam);
  ds.write(variant.damageTeamType);

  return ds.data();
}

NpcVariant NpcDatabase::readNpcVariant(ByteArray const& data, NetCompatibilityRules rules) const {
  DataStreamBuffer ds(data);
  ds.setStreamCompatibilityVersion(rules);

  NpcVariant variant;

  ds.read(variant.species);
  ds.read(variant.typeName);
  ds.read(variant.level);
  ds.read(variant.seed);
  ds.read(variant.overrides);

  auto config = buildConfig(variant.typeName, variant.overrides);

  variant.description = config.getString("description", "Some funny looking person");

  variant.scripts = jsonToStringList(config.get("scripts"));
  variant.scriptConfig = config.get("scriptConfig");

  ds.read(variant.initialScriptDelta);
  ds.read(variant.humanoidIdentity);

  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDefinition = speciesDatabase->species(variant.species);
  variant.uniqueHumanoidConfig = config.contains("humanoidConfig");
  if (variant.uniqueHumanoidConfig)
    variant.humanoidConfig = Root::singleton().assets()->json(config.getString("humanoidConfig"));
  else
    variant.humanoidConfig = speciesDefinition->humanoidConfig();

  variant.movementParameters = config.get("movementParameters", {});
  variant.statusControllerSettings = config.get("statusControllerSettings");

  auto functionDatabase = Root::singleton().functionDatabase();
  float powerMultiplierModifier =
      functionDatabase->function("npcLevelPowerMultiplierModifier")->evaluate(variant.level);
  float protectionMultiplier = functionDatabase->function("npcLevelProtectionMultiplier")->evaluate(variant.level);
  float maxHealthMultiplier = functionDatabase->function("npcLevelHealthMultiplier")->evaluate(variant.level);
  float maxEnergyMultiplier = functionDatabase->function("npcLevelEnergyMultiplier")->evaluate(variant.level);

  variant.innateStatusEffects =
      config.get("innateStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  variant.innateStatusEffects.append(StatModifier(StatValueModifier{"powerMultiplier", powerMultiplierModifier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"protection", protectionMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxHealth", maxHealthMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxEnergy", maxEnergyMultiplier}));

  variant.touchDamageConfig = config.get("touchDamage", {});

  ds.readMapContainer(variant.items);

  variant.disableWornArmor = config.getBool("disableWornArmor", true);
  variant.dropPools = jsonToStringList(config.get("dropPools", JsonArray{}));

  ds.read(variant.persistent);
  ds.read(variant.keepAlive);
  ds.read(variant.damageTeam);
  ds.read(variant.damageTeamType);

  variant.nametagColor = jsonToVec3B(config.get("nametagColor", JsonArray{255, 255, 255}));

  variant.splashConfig = EntitySplashConfig(config.get("splashConfig"));

  return variant;
}

Json NpcDatabase::writeNpcVariantToJson(NpcVariant const& variant) const {
  return JsonObject{{"species", variant.species},
      {"typeName", variant.typeName},
      {"level", variant.level},
      {"seed", variant.seed},
      {"overrides", variant.overrides},
      {"initialScriptDelta", variant.initialScriptDelta},
      {"humanoidIdentity", variant.humanoidIdentity.toJson()},
      {"items", jsonFromMapV<StringMap<ItemDescriptor>>(variant.items, mem_fn(&ItemDescriptor::diskStore))},
      {"persistent", variant.persistent},
      {"keepAlive", variant.keepAlive},
      {"damageTeam", variant.damageTeam},
      {"damageTeamType", TeamTypeNames.getRight(variant.damageTeamType)}};
}

NpcVariant NpcDatabase::readNpcVariantFromJson(Json const& data) const {
  NpcVariant variant;

  variant.species = data.getString("species");
  variant.typeName = data.getString("typeName");
  variant.level = data.getFloat("level");
  variant.seed = data.getUInt("seed");
  variant.overrides = data.get("overrides");

  auto config = buildConfig(variant.typeName, variant.overrides);

  variant.description = config.getString("description", "Some funny looking person");

  variant.scripts = jsonToStringList(config.get("scripts"));
  variant.scriptConfig = config.get("scriptConfig");

  variant.initialScriptDelta = data.getInt("initialScriptDelta");
  variant.humanoidIdentity = HumanoidIdentity(data.get("humanoidIdentity"));

  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDefinition = speciesDatabase->species(variant.species);
  variant.uniqueHumanoidConfig = config.contains("humanoidConfig");
  if (variant.uniqueHumanoidConfig)
    variant.humanoidConfig = Root::singleton().assets()->json(config.getString("humanoidConfig"));
  else
    variant.humanoidConfig = speciesDefinition->humanoidConfig();

  variant.movementParameters = config.get("movementParameters", {});
  variant.statusControllerSettings = config.get("statusControllerSettings", {});

  auto functionDatabase = Root::singleton().functionDatabase();
  float powerMultiplierModifier =
      functionDatabase->function("npcLevelPowerMultiplierModifier")->evaluate(variant.level);
  float protectionMultiplier = functionDatabase->function("npcLevelProtectionMultiplier")->evaluate(variant.level);
  float maxHealthMultiplier = functionDatabase->function("npcLevelHealthMultiplier")->evaluate(variant.level);
  float maxEnergyMultiplier = functionDatabase->function("npcLevelEnergyMultiplier")->evaluate(variant.level);

  variant.innateStatusEffects =
      config.get("innateStatusEffects", JsonArray()).toArray().transformed(jsonToPersistentStatusEffect);
  variant.innateStatusEffects.append(StatModifier(StatValueModifier{"powerMultiplier", powerMultiplierModifier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"protection", protectionMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxHealth", maxHealthMultiplier}));
  variant.innateStatusEffects.append(StatModifier(StatBaseMultiplier{"maxEnergy", maxEnergyMultiplier}));

  variant.touchDamageConfig = config.get("touchDamage", {});

  variant.items = jsonToMapV<StringMap<ItemDescriptor>>(data.get("items"), ItemDescriptor::loadStore);

  variant.disableWornArmor = config.getBool("disableWornArmor", true);
  variant.dropPools = jsonToStringList(config.get("dropPools", JsonArray{}));

  variant.persistent = data.getBool("persistent");
  variant.keepAlive = data.getBool("keepAlive");
  variant.damageTeam = data.getUInt("damageTeam");
  variant.damageTeamType = TeamTypeNames.getLeft(data.getString("damageTeamType"));

  variant.nametagColor = jsonToVec3B(config.get("nametagColor", JsonArray{255, 255, 255}));

  variant.splashConfig = EntitySplashConfig(config.get("splashConfig"));

  return variant;
}

NpcPtr NpcDatabase::createNpc(NpcVariant const& npcVariant) const {
  return make_shared<Npc>(npcVariant);
}

NpcPtr NpcDatabase::diskLoadNpc(Json const& diskStore) const {
  auto npcVariant = readNpcVariantFromJson(diskStore.get("npcVariant"));
  return make_shared<Npc>(npcVariant, diskStore);
}

NpcPtr NpcDatabase::netLoadNpc(ByteArray const& netStore, NetCompatibilityRules rules) const {
  return make_shared<Npc>(readNpcVariant(netStore, rules));
}

List<Drawable> NpcDatabase::npcPortrait(NpcVariant const& npcVariant, PortraitMode mode) const {
  Humanoid humanoid(npcVariant.humanoidConfig);
  humanoid.setIdentity(npcVariant.humanoidIdentity);

  auto itemDatabase = Root::singleton().itemDatabase();
  auto items = StringMap<ItemDescriptor, CaseInsensitiveStringHash, CaseInsensitiveStringCompare>::from(npcVariant.items);

  auto makeItem = [&npcVariant, &itemDatabase](ItemDescriptor itemDescriptor) -> ItemPtr {
    return itemDatabase->item(itemDescriptor, npcVariant.level, npcVariant.seed);
  };

  ArmorWearer armor;
  if (items.contains("head"))
    armor.setHeadItem(as<HeadArmor>(makeItem(items["head"])));
  if (items.contains("headCosmetic"))
    armor.setHeadItem(as<HeadArmor>(makeItem(items["headCosmetic"])));
  if (items.contains("chest"))
    armor.setChestItem(as<ChestArmor>(makeItem(items["chest"])));
  if (items.contains("chestCosmetic"))
    armor.setChestCosmeticItem(as<ChestArmor>(makeItem(items["chestCosmetic"])));
  if (items.contains("legs"))
    armor.setLegsItem(as<LegsArmor>(makeItem(items["legs"])));
  if (items.contains("legsCosmetic"))
    armor.setLegsCosmeticItem(as<LegsArmor>(makeItem(items["legsCosmetic"])));
  if (items.contains("back"))
    armor.setBackItem(as<BackArmor>(makeItem(items["back"])));
  if (items.contains("backCosmetic"))
    armor.setBackCosmeticItem(as<BackArmor>(makeItem(items["backCosmetic"])));

  armor.setupHumanoid(humanoid, false);

  return humanoid.renderPortrait(mode);
}

Json NpcDatabase::buildConfig(String const& typeName, Json const& overrides) const {
  auto const& baseConfig = m_npcTypes.get(typeName);
  auto config = mergeConfigValues(baseConfig, overrides);

  String baseTypeName = baseConfig.getString("baseType", "");
  if (baseTypeName.empty()) {
    return config;
  } else {
    return buildConfig(baseTypeName, config);
  }
}

Json NpcDatabase::mergeConfigValues(Json const& base, Json const& merger) const {
  if (base.type() == Json::Type::Object && merger.type() == Json::Type::Object) {
    auto map = base.toObject();
    for (auto const& entry : merger.iterateObject()) {
      if (!map.insert(entry.first, entry.second).second) {
        map[entry.first] = mergeConfigValues(map[entry.first], entry.second);
      }
    }
    return map;
  } else if (merger.isNull()) {
    return base;
  } else {
    return merger;
  }
}

}
