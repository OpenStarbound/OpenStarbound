#include "StarSpeciesDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarItemDatabase.hpp"
#include "StarNameGenerator.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarImageProcessing.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"

namespace Star {

SpeciesOption::SpeciesOption()
  : species(),
    headOptionAsHairColor(),
    headOptionAsFacialhair(),
    altOptionAsUndyColor(),
    altOptionAsHairColor(),
    altOptionAsFacialMask(),
    hairColorAsBodySubColor(),
    bodyColorAsFacialMaskSubColor(),
    altColorAsFacialMaskSubColor(),
    genderOptions(),
    bodyColorDirectives(),
    undyColorDirectives(),
    hairColorDirectives() {}

SpeciesDatabase::SpeciesDatabase() : m_luaRoot(make_shared<LuaRoot>()) {
  auto assets = Root::singleton().assets();

  auto& files = assets->scanExtension("species");
  assets->queueJsons(files);
  for (auto& file : files) {
    auto speciesDefinition = make_shared<SpeciesDefinition>(assets->json(file));
    if (m_species.contains(speciesDefinition->kind()))
      throw StarException(strf("Duplicate species asset with kind {}. configfile {}", speciesDefinition->kind(), file));
    auto k = speciesDefinition->kind().toLower();
    m_species[k] = speciesDefinition;
  }
}

SpeciesDefinitionPtr SpeciesDatabase::species(String const& kind) const {
  auto k = kind.toLower();
  if (!m_species.contains(k))
    throw StarException(strf("Unknown species kind '{}'.", kind));
  return m_species.get(k);
}

StringMap<SpeciesDefinitionPtr> SpeciesDatabase::allSpecies() const {
  return m_species;
}

Json SpeciesDatabase::humanoidConfig(HumanoidIdentity identity, JsonObject parameters, Json config) const {
  auto speciesDef = species(identity.species);
  if (speciesDef->m_buildScripts.size() > 0) {
    RecursiveMutexLocker locker(m_luaMutex);
    auto context = m_luaRoot->createContext(speciesDef->m_buildScripts);
    context.setCallbacks("root", LuaBindings::makeRootCallbacks());
    context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());

    // NPCs can have their own custom humanoidConfig that don't align with their species
    // however we need to make sure it only gets passed into this if its different from base
    // so in script we know when we have a unique case we should probably ignore or not
    return context.invokePath<Json>("build", identity.toJson(), parameters, speciesDef->humanoidConfig(), config );
  } else {
    if (config.isType(Json::Type::Object))
      return config;
    // assuming most people would just use it to merge over default humanoid config params
    return jsonMerge(speciesDef->humanoidConfig(), parameters);
  }
}

CharacterCreationResult SpeciesDatabase::createHumanoid(
  String name,
  String speciesChoice,
  size_t genderChoice,
  size_t bodyColorChoice,
  size_t alty,
  size_t hairChoice,
  size_t heady,
  size_t shirtChoice,
  size_t shirtColor,
  size_t pantsChoice,
  size_t pantsColor,
  size_t personality,
  LuaVariadic<LuaValue> ext
) const {
  CharacterCreationResult result;

  auto speciesDefinition = species(speciesChoice);
  if (speciesDefinition->m_creationScripts.size() > 0) {
    RecursiveMutexLocker locker(m_luaMutex);
    auto context = m_luaRoot->createContext(speciesDefinition->m_creationScripts);
    context.setCallbacks("root", LuaBindings::makeRootCallbacks());
    context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
    Json identityReturn;
    luaTie(identityReturn, result.humanoidParameters, result.armor) = context.invokePath<LuaTupleReturn<Json, JsonObject, JsonObject>>(
      "create",
      name,
      speciesChoice,
      genderChoice,
      bodyColorChoice,
      alty,
      hairChoice,
      heady,
      shirtChoice,
      shirtColor,
      pantsChoice,
      pantsColor,
      personality,
      ext);
    result.identity = HumanoidIdentity(identityReturn);
  } else {
    auto species = speciesDefinition->options();
    auto gender = species.genderOptions.wrap(genderChoice);

    auto bodyColor = species.bodyColorDirectives.wrap(bodyColorChoice);

    String altColor;

    if (species.altOptionAsUndyColor) {
      // undyColor
      altColor = species.undyColorDirectives.wrap(alty);
    }

    auto hair = gender.hairOptions.wrap(hairChoice);
    String hairColor = bodyColor;
    if (species.headOptionAsHairColor && species.altOptionAsHairColor) {
      hairColor = species.hairColorDirectives.wrap(heady);
      hairColor += species.undyColorDirectives.wrap(alty);
    } else if (species.headOptionAsHairColor) {
      hairColor = species.hairColorDirectives.wrap(heady);
    }

    if (species.hairColorAsBodySubColor)
      bodyColor += hairColor;

    String facialHair;
    String facialHairGroup;
    String facialHairDirective;
    if (species.headOptionAsFacialhair) {
      facialHair = gender.facialHairOptions.wrap(heady);
      facialHairGroup = gender.facialHairGroup;
      facialHairDirective = hairColor;
    }

    String facialMask;
    String facialMaskGroup;
    String facialMaskDirective;
    if (species.altOptionAsFacialMask) {
      facialMask = gender.facialMaskOptions.wrap(alty);
      facialMaskGroup = gender.facialMaskGroup;
      facialMaskDirective = "";
    }
    if (species.bodyColorAsFacialMaskSubColor)
      facialMaskDirective += bodyColor;
    if (species.altColorAsFacialMaskSubColor)
      facialMaskDirective += altColor;

    auto shirt = gender.shirtOptions.wrap(shirtChoice);
    auto pants = gender.pantsOptions.wrap(pantsChoice);

    auto personalityResult = speciesDefinition->personalities().wrap(personality);

    result.identity.name = name;
    result.identity.species = species.species;
    result.identity.bodyDirectives = bodyColor + altColor;
    result.identity.gender = GenderNames.getLeft(gender.name);
    result.identity.hairGroup = gender.hairGroup;
    result.identity.hairType = hair;
    result.identity.hairDirectives = hairColor;
    result.identity.emoteDirectives = bodyColor + altColor;
    result.identity.facialHairGroup = facialHairGroup;
    result.identity.facialHairType = facialHair;
    result.identity.facialHairDirectives = facialHairDirective;
    result.identity.facialMaskGroup = facialMaskGroup;
    result.identity.facialMaskType = facialMask;
    result.identity.facialMaskDirectives = facialMaskDirective;
    result.identity.personality = personalityResult;

    result.armor.set(EquipmentSlotNames.getRight(EquipmentSlot::Chest), ItemDescriptor(shirt, 1, JsonObject{{"colorIndex", shirtColor}}).toJson());
    result.armor.set(EquipmentSlotNames.getRight(EquipmentSlot::Legs), ItemDescriptor(pants, 1, JsonObject{{"colorIndex", pantsColor}}).toJson());

    result.humanoidParameters.set("choices", JsonArray({genderChoice, bodyColorChoice, alty, hairChoice, heady, shirtChoice, shirtColor, pantsChoice, pantsColor, personality}));
  }

  return result;
}

CharacterCreationResult SpeciesDatabase::generateHumanoid(String speciesChoice, int64_t seed, Maybe<Gender> gender) const {
  RandomSource randSource(seed);
  auto speciesDefinition = species(speciesChoice);
  auto chosenGender = gender.value(randSource.randb() ? Gender::Male : Gender::Female);
  return createHumanoid(
    Root::singleton().nameGenerator()->generateName(speciesDefinition->nameGen(chosenGender), randSource),
    speciesChoice,
    (unsigned)chosenGender,
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32(),
    randSource.randu32()
  );
}

SpeciesDefinition::SpeciesDefinition(Json const& config) {
  m_config = config;
  m_kind = config.getString("kind");
  m_humanoidConfig = config.getString("humanoidConfig", "/humanoid.config");
  m_humanoidOverrides = config.getObject("humanoidOverrides", JsonObject());
  m_buildScripts = jsonToStringList(config.getArray("buildScripts", JsonArray()));
  m_creationScripts = jsonToStringList(config.getArray("createScripts", JsonArray()));

  Json tooltip = config.get("charCreationTooltip");

  m_tooltip.title = tooltip.getString("title", "");
  m_tooltip.subTitle = tooltip.getString("subTitle", "");
  m_tooltip.description = tooltip.getString("description", "");

  m_nameGen = jsonToStringList(config.get("nameGen"));
  m_charGenTextLabels = jsonToStringList(config.getArray("charGenTextLabels", {}));
  m_skull = config.getString("skull", "/humanoid/any/dead.png");

  m_ouchNoises = jsonToStringList(config.get("ouchNoises"));

  for (Json v : config.getArray("defaultItems", JsonArray()))
    m_defaultItems.append(ItemDescriptor(v));

  for (Json v : config.get("defaultBlueprints", JsonObject()).getArray("tier1", JsonArray()))
    m_defaultBlueprints.append(ItemDescriptor(v));

  auto personalities = humanoidConfig().getArray("personalities");
  for (auto personality : personalities) {
    m_personalities.push_back(parsePersonalityArray(personality));
  }

  m_statusEffects = config.getArray("statusEffects", {}).transformed(jsonToPersistentStatusEffect);

  m_effectDirectives = config.getString("effectDirectives", "");

  SpeciesOption species;
  species.species = m_kind;
  species.headOptionAsHairColor = config.getBool("headOptionAsHairColor", false);
  species.headOptionAsFacialhair = config.getBool("headOptionAsFacialhair", false);
  species.altOptionAsUndyColor = config.getBool("altOptionAsUndyColor", false);
  species.altOptionAsHairColor = config.getBool("altOptionAsHairColor", false);
  species.altOptionAsFacialMask = config.getBool("altOptionAsFacialMask", false);
  species.hairColorAsBodySubColor = config.getBool("hairColorAsBodySubColor", false);
  species.bodyColorAsFacialMaskSubColor = config.getBool("bodyColorAsFacialMaskSubColor", false);
  species.altColorAsFacialMaskSubColor = config.getBool("altColorAsFacialMaskSubColor", false);
  species.bodyColorDirectives = colorDirectivesFromConfig(config.getArray("bodyColor", JsonArray({""})));
  species.undyColorDirectives = colorDirectivesFromConfig(config.getArray("undyColor", JsonArray({""})));
  species.hairColorDirectives = colorDirectivesFromConfig(config.getArray("hairColor", JsonArray({""})));
  for (auto genderData : config.getArray("genders", JsonArray())) {
    SpeciesGenderOption gender;
    gender.name = genderData.getString("name", "");
    gender.gender = species.genderOptions.size() == 0 ? Gender::Male : Gender::Female;
    gender.image = genderData.getString("image", "");
    gender.characterImage = genderData.getString("characterImage", "");

    gender.hairOptions = jsonToStringList(genderData.get("hair", config.get("hair", JsonArray({""}))));
    gender.hairGroup = genderData.getString("hairGroup", config.getString("HairGroup", "hair"));
    gender.shirtOptions = jsonToStringList(genderData.get("shirt", config.get("shirt", JsonArray({""}))));
    gender.pantsOptions = jsonToStringList(genderData.get("pants", config.get("pants", JsonArray({""}))));
    gender.facialHairGroup = genderData.getString("facialHairGroup", config.getString("facialHairGroup", ""));
    gender.facialHairOptions = jsonToStringList(genderData.get("facialHair", config.get("facialHair", JsonArray({""}))));
    gender.facialMaskGroup = genderData.getString("facialMaskGroup", config.getString("facialMaskGroup",""));
    gender.facialMaskOptions = jsonToStringList(genderData.get("facialMask", config.get("facialMask", JsonArray({""}))));

    species.genderOptions.append(gender);
  }
  m_options = species;
}

Json SpeciesDefinition::config() const {
  return m_config;
}

String SpeciesDefinition::kind() const {
  return m_kind;
}

String SpeciesDefinition::nameGen(Gender gender) const {
  return m_nameGen[(unsigned)gender];
}

String SpeciesDefinition::ouchNoise(Gender gender) const {
  return m_ouchNoises[(unsigned)gender];
}

SpeciesOption const& SpeciesDefinition::options() const {
  return m_options;
}

Json SpeciesDefinition::humanoidConfig() const {
  auto config = Root::singleton().assets()->json(m_humanoidConfig);
  return jsonMerge(config, m_humanoidOverrides);
}

List<Personality> const& SpeciesDefinition::personalities() const {
  return m_personalities;
}

List<ItemDescriptor> SpeciesDefinition::defaultItems() const {
  return m_defaultItems;
}

List<ItemDescriptor> SpeciesDefinition::defaultBlueprints() const {
  return m_defaultBlueprints;
}

StringList SpeciesDefinition::charGenTextLabels() const {
  return m_charGenTextLabels;
}

SpeciesCharCreationTooltip const& SpeciesDefinition::tooltip() const {
  return m_tooltip;
}

String SpeciesDefinition::skull() const {
  return m_skull;
}

List<PersistentStatusEffect> SpeciesDefinition::statusEffects() const {
  return m_statusEffects;
}

String SpeciesDefinition::effectDirectives() const {
  return m_effectDirectives;
}

}
