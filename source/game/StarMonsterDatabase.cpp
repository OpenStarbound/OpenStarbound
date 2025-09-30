#include "StarMonsterDatabase.hpp"
#include "StarMonster.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarLexicalCast.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarRebuilder.hpp"

namespace Star {

MonsterDatabase::MonsterDatabase() : m_rebuilder(make_shared<Rebuilder>("monster")) {
  auto assets = Root::singleton().assets();

  auto& monsterTypes = assets->scanExtension("monstertype");
  auto& monsterParts = assets->scanExtension("monsterpart");
  auto& monsterSkills = assets->scanExtension("monsterskill");
  auto& monsterColors = assets->scanExtension("monstercolors");

  assets->queueJsons(monsterTypes);
  assets->queueJsons(monsterParts);
  assets->queueJsons(monsterSkills);
  assets->queueJsons(monsterColors);

  for (auto const& file : monsterTypes) {
    try {
      auto config = assets->json(file);
      String typeName = config.getString("type");

      if (m_monsterTypes.contains(typeName))
        throw MonsterException(strf("Repeat monster type name '{}'", typeName));

      MonsterType& monsterType = m_monsterTypes[typeName];

      monsterType.typeName = typeName;
      monsterType.shortDescription = config.optString("shortdescription");
      monsterType.description = config.optString("description");

      monsterType.categories = jsonToStringList(config.get("categories"));
      monsterType.partTypes = jsonToStringList(config.get("parts"));

      monsterType.animationConfigPath = AssetPath::relativeTo(file, config.getString("animation"));
      monsterType.colors = config.getString("colors", "default");

      monsterType.reversed = config.getBool("reversed", false);

      if (config.contains("dropPools"))
        monsterType.dropPools = config.getArray("dropPools");

      monsterType.baseParameters = config.get("baseParameters", {});

      // for updated monsters, use the partParameterDescription from the
      // .partparams file
      if (config.contains("partParameters")) {
        Json partParameterSource = assets->json(AssetPath::relativeTo(file, config.getString("partParameters")));
        monsterType.partParameterDescription = partParameterSource.getObject("partParameterDescription");
        monsterType.partParameterOverrides = partParameterSource.getObject("partParameters");
      } else {
        // outdated monsters still have partParameterDescription defined
        // directly in the
        // .monstertype file
        monsterType.partParameterDescription = config.getObject("partParameterDescription", {});
      }

    } catch (StarException const& e) {
      throw MonsterException(strf("Error loading monster type '{}'", file), e);
    }
  }

  for (auto const& file : monsterParts) {
    try {
      auto config = assets->json(file);
      if (config.isNull())
        continue;

      MonsterPart part;
      part.name = config.getString("name");
      part.category = config.getString("category");
      part.type = config.getString("type");
      part.path = AssetPath::directory(file);
      part.frames = config.getObject("frames");
      part.partParameters = config.get("parameters", JsonObject());

      auto& partMap = m_partDirectory[part.category][part.type];

      if (partMap.contains(part.name))
        throw MonsterException(strf("Repeat monster part name '{}' for category '{}'", part.name, part.category));
      else
        partMap[part.name] = part;
    } catch (StarException const& e) {
      throw MonsterException(strf("Error loading monster part '{}'", file), e);
    }
  }

  for (auto const& file : monsterSkills) {
    try {
      auto config = assets->json(file);
      if (config.isNull())
        continue;

      MonsterSkill skill;
      skill.name = config.getString("name");
      skill.label = config.getString("label");
      skill.image = config.getString("image");

      skill.config = config.get("config", JsonObject());
      skill.parameters = config.get("parameters", JsonObject());
      skill.animationParameters = config.get("animationParameters", JsonObject());

      if (m_skills.contains(skill.name))
        throw MonsterException(strf("Repeat monster skill name '{}'", skill.name));
      else
        m_skills[skill.name] = skill;
    } catch (StarException const& e) {
      throw MonsterException(strf("Error loading monster skill '{}'", file), e);
    }
  }

  for (auto const& file : monsterColors) {
    try {
      auto config = assets->json(file);
      if (config.isNull())
        continue;

      auto paletteName = config.getString("name");
      if (m_colorSwaps.contains(paletteName))
        throw MonsterException(strf("Duplicate monster colors name '{}'", paletteName));

      ColorReplaceMap colorSwaps;
      for (auto const& swapSet : config.getArray("swaps")) {
        ColorReplaceMap colorSwaps;
        for (auto const& swap : swapSet.iterateObject()) {
          colorSwaps[Color::fromHex(swap.first).toRgba()] = Color::fromHex(swap.second.toString()).toRgba();
        }
        m_colorSwaps[paletteName].append(colorSwaps);
      }
    } catch (StarException const& e) {
      throw MonsterException(strf("Error loading monster colors '{}'", file), e);
    }
  }
}

void MonsterDatabase::cleanup() {
  MutexLocker locker(m_cacheMutex);
  m_monsterCache.cleanup();
}

StringList MonsterDatabase::monsterTypes() const {
  return m_monsterTypes.keys();
}

MonsterVariant MonsterDatabase::randomMonster(String const& typeName, Json const& uniqueParameters) const {
  size_t seed;
  if (auto seedConfig = uniqueParameters.opt("seed")) {
    if (seedConfig->type() == Json::Type::String) {
      seed = lexicalCast<uint64_t>(seedConfig->toString());
    } else {
      seed = seedConfig->toUInt();
    }
  } else {
    seed = Random::randu64();
  }

  return monsterVariant(typeName, seed, uniqueParameters);
}

MonsterVariant MonsterDatabase::monsterVariant(String const& typeName, uint64_t seed, Json const& uniqueParameters) const {
  MutexLocker locker(m_cacheMutex);
  return m_monsterCache.get(make_tuple(typeName, seed, uniqueParameters), [this](tuple<String, uint64_t, Json> const& key) {
      return produceMonster(get<0>(key), get<1>(key), get<2>(key));
    });
}

ByteArray MonsterDatabase::writeMonsterVariant(MonsterVariant const& variant, NetCompatibilityRules rules) const {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);

  ds.write(variant.type);
  ds.write(variant.seed);
  ds.write(variant.uniqueParameters);

  return ds.data();
}

MonsterVariant MonsterDatabase::readMonsterVariant(ByteArray const& data, NetCompatibilityRules rules) const {
  DataStreamBuffer ds(data);
  ds.setStreamCompatibilityVersion(rules);

  String type = ds.read<String>();
  uint64_t seed = ds.read<uint64_t>();
  Json uniqueParameters = ds.read<Json>();

  return monsterVariant(type, seed, uniqueParameters);
}

Json MonsterDatabase::writeMonsterVariantToJson(MonsterVariant const& mVar) const {
  return JsonObject{
      {"type", mVar.type},
      {"seed", mVar.seed},
      {"uniqueParameters", mVar.uniqueParameters},
  };
}

MonsterVariant MonsterDatabase::readMonsterVariantFromJson(Json const& variant) const {
  return monsterVariant(variant.getString("type"), variant.getUInt("seed"), variant.getObject("uniqueParameters"));
}

MonsterPtr MonsterDatabase::createMonster(
    MonsterVariant monsterVariant, Maybe<float> level, Json uniqueParameters) const {
  if (uniqueParameters) {
    monsterVariant.uniqueParameters = jsonMerge(monsterVariant.uniqueParameters, uniqueParameters);
    monsterVariant.parameters = jsonMerge(monsterVariant.parameters, monsterVariant.uniqueParameters);
    readCommonParameters(monsterVariant);
  }
  return make_shared<Monster>(monsterVariant, level);
}

MonsterPtr MonsterDatabase::diskLoadMonster(Json const& diskStore) const {
  MonsterPtr monster;
  try {
    monster = make_shared<Monster>(diskStore);
  } catch (std::exception const& e) {
    auto exception = std::current_exception();
    bool success = m_rebuilder->rebuild(diskStore, strf("{}", outputException(e, false)), [&](Json const& store) -> String {
      try {
        monster = make_shared<Monster>(store);
      } catch (std::exception const& e) {
        exception = std::current_exception();
        return strf("{}", outputException(e, false));
      }
      return {};
    });

    if (!success)
      std::rethrow_exception(exception);
  }
  return monster;
}

MonsterPtr MonsterDatabase::netLoadMonster(ByteArray const& netStore, NetCompatibilityRules rules) const {
  return make_shared<Monster>(readMonsterVariant(netStore, rules));
}

List<Drawable> MonsterDatabase::monsterPortrait(MonsterVariant const& variant) const {
  NetworkedAnimator animator(variant.animatorConfig);
  for (auto const& pair : variant.animatorPartTags)
    animator.setPartTag(pair.first, "partImage", pair.second);
  animator.setZoom(variant.animatorZoom);
  auto colorSwap = variant.colorSwap.value(this->colorSwap(variant.parameters.getString("colors", "default"), variant.seed));
  if (!colorSwap.empty())
    animator.setProcessingDirectives(imageOperationToString(ColorReplaceImageOperation{colorSwap}));
  auto drawables = animator.drawables();
  Drawable::scaleAll(drawables, TilePixels);
  return drawables;
}

std::pair<String, String> MonsterDatabase::skillInfo(String const& skillName) const {
  if (m_skills.contains(skillName)) {
    auto& skill = m_skills.get(skillName);
    return std::make_pair(skill.label, skill.image);
  } else {
    return std::make_pair("", "");
  }
}

Json MonsterDatabase::skillConfigParameter(String const& skillName, String const& configParameterName) const {
  if (m_skills.contains(skillName)) {
    auto& skill = m_skills.get(skillName);
    return skill.config.get(configParameterName, Json());
  } else {
    return Json();
  }
}

ColorReplaceMap MonsterDatabase::colorSwap(String const& setName, uint64_t seed) const {
  if (m_colorSwaps.contains(setName))
    return staticRandomFrom(m_colorSwaps.get(setName), seed);
  else {
    Logger::error("Monster colors '{}' not found!", setName);
    return staticRandomFrom(m_colorSwaps.get("default"), seed);
  }
}

Json MonsterDatabase::mergePartParameters(Json const& partParameterDescription, JsonArray const& parameters) {
  JsonObject mergedParameters;

  // First assign all the defaults.
  for (auto const& pair : partParameterDescription.iterateObject())
    mergedParameters[pair.first] = pair.second.get(1);

  // Then go through parameter list and merge based on the merge rules.
  for (auto const& applyParams : parameters) {
    for (auto const& pair : applyParams.iterateObject()) {
      String mergeMethod = partParameterDescription.get(pair.first).getString(0);
      Json value = mergedParameters.get(pair.first);

      if (mergeMethod.equalsIgnoreCase("add")) {
        value = value.toDouble() + pair.second.toDouble();
      } else if (mergeMethod.equalsIgnoreCase("multiply")) {
        value = value.toDouble() * pair.second.toDouble();
      } else if (mergeMethod.equalsIgnoreCase("merge")) {
        // "merge" means to either merge maps, or *append* lists together
        if (!pair.second.isNull()) {
          if (value.isNull()) {
            value = pair.second;
          } else if (value.type() != pair.second.type()) {
            value = pair.second;
          } else {
            if (pair.second.type() == Json::Type::Array) {
              auto array = value.toArray();
              array.appendAll(pair.second.toArray());
              value = std::move(array);
            } else if (pair.second.type() == Json::Type::Object) {
              auto obj = value.toObject();
              obj.merge(pair.second.toObject(), true);
              value = std::move(obj);
            }
          }
        }
      } else if (mergeMethod.equalsIgnoreCase("override") && !pair.second.isNull()) {
        value = pair.second;
      }

      mergedParameters[pair.first] = value;
    }
  }

  return mergedParameters;
}

Json MonsterDatabase::mergeFinalParameters(JsonArray const& parameters) {
  JsonObject mergedParameters;

  for (auto const& applyParams : parameters) {
    for (auto const& pair : applyParams.iterateObject()) {
      Json value = mergedParameters.value(pair.first);

      // Hard-coded merge for scripts and skills parameters, otherwise merge.
      if (pair.first == "scripts" || pair.first == "skills" || pair.first == "specialSkills"
          || pair.first == "baseSkills") {
        auto array = value.optArray().value();
        array.appendAll(pair.second.optArray().value());
        value = std::move(array);
      } else {
        value = jsonMerge(value, pair.second);
      }

      mergedParameters[pair.first] = value;
    }
  }

  return mergedParameters;
}

void MonsterDatabase::readCommonParameters(MonsterVariant& variant) {
  variant.shortDescription = variant.parameters.optString("shortdescription").orMaybe(variant.shortDescription);
  variant.dropPoolConfig = variant.parameters.get("dropPools", variant.dropPoolConfig);
  variant.scripts = jsonToStringList(variant.parameters.get("scripts"));
  variant.animationScripts = jsonToStringList(variant.parameters.getArray("animationScripts", {}));
  variant.animatorConfig = jsonMerge(variant.animatorConfig, variant.parameters.get("animationCustom", JsonObject()));
  variant.initialScriptDelta = variant.parameters.getUInt("initialScriptDelta", 5);
  variant.metaBoundBox = jsonToRectF(variant.parameters.get("metaBoundBox"));
  variant.renderLayer = variant.parameters.optString("renderLayer").apply(parseRenderLayer).value(RenderLayerMonster);
  variant.scale = variant.parameters.getFloat("scale");
  variant.movementSettings = ActorMovementParameters(variant.parameters.get("movementSettings", {}));
  variant.walkMultiplier = variant.parameters.getFloat("walkMultiplier", 1.0f);
  variant.runMultiplier = variant.parameters.getFloat("runMultiplier", 1.0f);
  variant.jumpMultiplier = variant.parameters.getFloat("jumpMultiplier", 1.0f);
  variant.weightMultiplier = variant.parameters.getFloat("weightMultiplier", 1.0f);
  variant.healthMultiplier = variant.parameters.getFloat("healthMultiplier", 1.0f);
  variant.touchDamageMultiplier = variant.parameters.getFloat("touchDamageMultiplier", 1.0f);
  variant.touchDamageConfig = variant.parameters.get("touchDamage", {});
  variant.animationDamageParts = variant.parameters.getObject("animationDamageParts", {});
  variant.statusSettings = variant.parameters.get("statusSettings");
  variant.mouthOffset = jsonToVec2F(variant.parameters.get("mouthOffset")) / TilePixels;
  variant.feetOffset = jsonToVec2F(variant.parameters.get("feetOffset")) / TilePixels;
  variant.powerLevelFunction = variant.parameters.getString("powerLevelFunction", "monsterLevelPowerMultiplier");
  variant.healthLevelFunction = variant.parameters.getString("healthLevelFunction", "monsterLevelHealthMultiplier");
  variant.clientEntityMode = ClientEntityModeNames.getLeft(variant.parameters.getString("clientEntityMode", "ClientSlaveOnly"));
  variant.persistent = variant.parameters.getBool("persistent", false);
  variant.damageTeamType = TeamTypeNames.getLeft(variant.parameters.getString("damageTeamType", "enemy"));
  variant.damageTeam = variant.parameters.getUInt("damageTeam", 2);

  if (auto sdp = variant.parameters.get("selfDamagePoly", {}))
    variant.selfDamagePoly = jsonToPolyF(sdp);
  else
    variant.selfDamagePoly = *variant.movementSettings.standingPoly;

  variant.portraitIcon = variant.parameters.optString("portraitIcon");
  variant.damageReceivedAggressiveDuration = variant.parameters.getFloat("damageReceivedAggressiveDuration", 1.0f);
  variant.onDamagedOthersAggressiveDuration = variant.parameters.getFloat("onDamagedOthersAggressiveDuration", 5.0f);
  variant.onFireAggressiveDuration = variant.parameters.getFloat("onFireAggressiveDuration", 5.0f);

  variant.nametagColor = jsonToVec3B(variant.parameters.get("nametagColor", JsonArray{255, 255, 255}));

  variant.colorSwap = variant.parameters.optObject("colorSwap").apply([](JsonObject const& json) -> ColorReplaceMap {
      ColorReplaceMap swaps;
      for (auto pair : json) {
        swaps.insert(Color::fromHex(pair.first).toRgba(), Color::fromHex(pair.second.toString()).toRgba());
      }
      return swaps;
    });
}

MonsterVariant MonsterDatabase::produceMonster(String const& typeName, uint64_t seed, Json const& uniqueParameters) const {
  RandomSource rand = RandomSource(seed);

  auto const& monsterType = m_monsterTypes.get(typeName);

  MonsterVariant monsterVariant;
  monsterVariant.type = typeName;
  monsterVariant.shortDescription = monsterType.shortDescription;
  monsterVariant.description = monsterType.description;
  monsterVariant.seed = seed;
  monsterVariant.uniqueParameters = uniqueParameters;

  monsterVariant.animatorConfig = Root::singleton().assets()->fetchJson(monsterType.animationConfigPath);
  monsterVariant.reversed = monsterType.reversed;

  // select a list of monster parts
  List<MonsterPart> monsterParts;
  auto categoryName = rand.randFrom(monsterType.categories);

  for (auto const& partTypeName : monsterType.partTypes) {
    auto randPart = rand.randFrom(m_partDirectory.get(categoryName).get(partTypeName)).second;
    auto selectedPart = uniqueParameters.get("selectedParts", JsonObject()).optString(partTypeName);
    if (selectedPart)
      monsterParts.append(m_partDirectory.get(categoryName).get(partTypeName).get(*selectedPart));
    else
      monsterParts.append(randPart);
  }

  for (auto const& partConfig : monsterParts) {
    for (auto const& pair : partConfig.frames)
      monsterVariant.animatorPartTags[pair.first] = AssetPath::relativeTo(partConfig.path, pair.second.toString());
  }
  JsonArray partParameterList;
  for (auto const& partConfig : monsterParts) {
    partParameterList.append(partConfig.partParameters);
    // Include part parameter overrides
    if (!monsterType.partParameterOverrides.isNull() && monsterType.partParameterOverrides.contains(partConfig.name))
      partParameterList.append(monsterType.partParameterOverrides.getObject(partConfig.name));
  }

  // merge part parameters and unique parameters into base parameters
  Json baseParameters = monsterType.baseParameters;
  Json mergedPartParameters = mergePartParameters(monsterType.partParameterDescription, partParameterList);
  monsterVariant.parameters = mergeFinalParameters({baseParameters, mergedPartParameters});
  monsterVariant.parameters = jsonMerge(monsterVariant.parameters, uniqueParameters);

  tie(monsterVariant.parameters, monsterVariant.animatorConfig) = chooseSkills(monsterVariant.parameters, monsterVariant.animatorConfig, rand);
  monsterVariant.animatorZoom = 1.0f;
  monsterVariant.dropPoolConfig = monsterType.dropPools;

  readCommonParameters(monsterVariant);
  monsterVariant.animatorZoom = monsterVariant.scale;
  if (monsterVariant.dropPoolConfig.isType(Json::Type::Array))
    monsterVariant.dropPoolConfig = rand.randValueFrom(monsterVariant.dropPoolConfig.toArray());

  return monsterVariant;
}

pair<Json, Json> MonsterDatabase::chooseSkills(
    Json const& parameters, Json const& animatorConfig, RandomSource& rand) const {
  // Pick a subset of skills, then merge in any params from those skills
  if (parameters.contains("baseSkills") || parameters.contains("specialSkills")) {
    auto skillCount = parameters.getUInt("skillCount", 2);

    auto baseSkillNames = jsonToStringList(parameters.get("baseSkills"));
    auto specialSkillNames = jsonToStringList(parameters.get("specialSkills"));

    List<String> skillNames;

    // First, pick from base skills
    while (!baseSkillNames.empty() && skillNames.size() < skillCount)
      skillNames.append(baseSkillNames.takeAt(rand.randInt(baseSkillNames.size() - 1)));

    // ...then fill in from special skills as needed
    while (!specialSkillNames.empty() && skillNames.size() < skillCount)
      skillNames.append(specialSkillNames.takeAt(rand.randInt(specialSkillNames.size() - 1)));

    Json finalAnimatorConfig = animatorConfig;
    JsonArray allParameters({parameters});
    for (auto& skillName : skillNames) {
      if (m_skills.contains(skillName)) {
        auto const& skill = m_skills.get(skillName);
        allParameters.append(skill.parameters);
        finalAnimatorConfig = jsonMerge(finalAnimatorConfig, skill.animationParameters);
      }
    }

    // Need to override the final list of skills, instead of merging the lists
    Json finalParameters = mergeFinalParameters(allParameters).set("skills", jsonFromStringList(skillNames));

    return {finalParameters, finalAnimatorConfig};
  } else if (parameters.contains("skills")) {
    auto availableSkillNames = jsonToStringList(parameters.get("skills"));
    auto skillCount = min<size_t>(parameters.getUInt("skillCount", 2), availableSkillNames.size());

    List<String> skillNames;
    for (size_t i = 0; i < skillCount; ++i)
      skillNames.append(availableSkillNames.takeAt(rand.randInt(availableSkillNames.size() - 1)));

    Json finalAnimatorConfig = animatorConfig;
    JsonArray allParameters({parameters});
    for (auto& skillName : skillNames) {
      if (m_skills.contains(skillName)) {
        auto const& skill = m_skills.get(skillName);
        allParameters.append(skill.parameters);
        finalAnimatorConfig = jsonMerge(finalAnimatorConfig, skill.animationParameters);
      }
    }

    // Need to override the final list of skills, instead of merging the lists
    Json finalParameters = mergeFinalParameters(allParameters).set("skills", jsonFromStringList(skillNames));

    return {finalParameters, finalAnimatorConfig};
  } else {
    return {parameters, animatorConfig};
  }
}

}
