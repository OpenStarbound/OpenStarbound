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

SpeciesDefinition::SpeciesDefinition(Json const& config) {
  m_config = config;
  m_kind = config.getString("kind");
  m_humanoidConfig = config.getString("humanoidConfig", "/humanoid.config");
  m_humanoidOverrides = config.getObject("humanoidOverrides", JsonObject());
  m_buildScripts = jsonToStringList(config.getArray("buildScripts", JsonArray()));

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


void SpeciesDefinition::generateHumanoid(HumanoidIdentity& identity, int64_t seed, Maybe<Gender> genderOverride) {
  RandomSource randSource(seed);

  identity.species = m_kind;
  if (genderOverride.isValid())
    identity.gender = genderOverride.value();
  else
    identity.gender = randSource.randb() ? Gender::Male : Gender::Female;

  identity.name = Root::singleton().nameGenerator()->generateName(nameGen(identity.gender), randSource);

  auto gender = m_options.genderOptions[(unsigned)identity.gender];
  auto bodyColor = randSource.randFrom(m_options.bodyColorDirectives);

  String altColor;

  size_t altOpt = randSource.randu32();
  size_t headOpt = randSource.randu32();
  size_t hairOpt = randSource.randu32();

  if (m_options.altOptionAsUndyColor) {
    // undyColor
    altColor = m_options.undyColorDirectives.wrap(altOpt);
  }

  auto hair = gender.hairOptions.wrap(hairOpt);
  String hairColor = bodyColor;
  auto hairGroup = gender.hairGroup;
  if (m_options.headOptionAsHairColor && m_options.altOptionAsHairColor) {
    hairColor = m_options.hairColorDirectives.wrap(headOpt);
    hairColor += m_options.undyColorDirectives.wrap(altOpt);
  } else if (m_options.headOptionAsHairColor) {
    hairColor = m_options.hairColorDirectives.wrap(headOpt);
  }

  if (m_options.hairColorAsBodySubColor)
    bodyColor += hairColor;

  String facialHair;
  String facialHairGroup;
  String facialHairDirective;
  if (m_options.headOptionAsFacialhair) {
    facialHair = gender.facialHairOptions.wrap(headOpt);
    facialHairGroup = gender.facialHairGroup;
    facialHairDirective = hairColor;
  }

  String facialMask;
  String facialMaskGroup;
  String facialMaskDirective;
  if (m_options.altOptionAsFacialMask) {
    facialMask = gender.facialMaskOptions.wrap(altOpt);
    facialMaskGroup = gender.facialMaskGroup;
    facialMaskDirective = "";
  }
  if (m_options.bodyColorAsFacialMaskSubColor)
    facialMaskDirective += bodyColor;
  if (m_options.altColorAsFacialMaskSubColor)
    facialMaskDirective += altColor;

  identity.hairGroup = hairGroup;
  identity.hairType = hair;
  identity.hairDirectives = hairColor;
  identity.bodyDirectives = bodyColor + altColor;
  identity.emoteDirectives = bodyColor + altColor;
  identity.facialHairGroup = facialHairGroup;
  identity.facialHairType = facialHair;
  identity.facialHairDirectives = facialHairDirective;
  identity.facialMaskGroup = facialMaskGroup;
  identity.facialMaskType = facialMask;
  identity.facialMaskDirectives = facialMaskDirective;
}

}
