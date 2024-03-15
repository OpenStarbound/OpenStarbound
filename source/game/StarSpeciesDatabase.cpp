#include "StarSpeciesDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarItemDatabase.hpp"
#include "StarNameGenerator.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarImageProcessing.hpp"

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

SpeciesDatabase::SpeciesDatabase() {
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

SpeciesDefinition::SpeciesDefinition(Json const& config) {
  m_config = config;
  m_kind = config.getString("kind");
  m_humanoidConfig = config.getString("humanoidConfig", "/humanoid.config");
  m_humanoidOverrides = config.getObject("humanoidOverrides", JsonObject());

  Json tooltip = config.get("charCreationTooltip");

  m_tooltip.title = tooltip.getString("title", "");
  m_tooltip.subTitle = tooltip.getString("subTitle", "");
  m_tooltip.description = tooltip.getString("description", "");

  m_nameGen = jsonToStringList(config.get("nameGen"));
  m_charGenTextLabels = jsonToStringList(config.getArray("charGenTextLabels", {}));
  m_skull = config.getString("skull", "/humanoid/any/dead.png");

  m_ouchNoises = jsonToStringList(config.get("ouchNoises"));

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
  species.bodyColorDirectives = colorDirectivesFromConfig(config.getArray("bodyColor"));
  species.undyColorDirectives = colorDirectivesFromConfig(config.getArray("undyColor"));
  species.hairColorDirectives = colorDirectivesFromConfig(config.getArray("hairColor"));
  for (auto genderData : config.getArray("genders", JsonArray())) {
    SpeciesGenderOption gender;
    gender.name = genderData.getString("name", "");
    gender.gender = species.genderOptions.size() == 0 ? Gender::Male : Gender::Female;
    gender.image = genderData.getString("image", "");
    gender.characterImage = genderData.getString("characterImage", "");

    gender.hairOptions = jsonToStringList(genderData.get("hair"));
    gender.hairGroup = genderData.getString("hairGroup", "hair");
    gender.shirtOptions = jsonToStringList(genderData.get("shirt"));
    gender.pantsOptions = jsonToStringList(genderData.get("pants"));
    gender.facialHairGroup = genderData.getString("facialHairGroup");
    gender.facialHairOptions = jsonToStringList(genderData.get("facialHair"));
    gender.facialMaskGroup = genderData.getString("facialMaskGroup");
    gender.facialMaskOptions = jsonToStringList(genderData.get("facialMask"));

    species.genderOptions.append(gender);
  }
  m_options = species;
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

void SpeciesDefinition::generateHumanoid(HumanoidIdentity& identity, int64_t seed) {
  RandomSource randSource(seed);

  identity.species = m_kind;
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
