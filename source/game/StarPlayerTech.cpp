#include "StarPlayerTech.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"

namespace Star {

PlayerTech::PlayerTech() {}

PlayerTech::PlayerTech(Json const& json) {
  m_availableTechs = jsonToStringSet(json.get("availableTechs"));
  m_enabledTechs = jsonToStringSet(json.get("enabledTechs"));
  auto techDatabase = Root::singleton().techDatabase();
  for (auto& p : json.getObject("equippedTechs")) {
    String techName = p.second.toString();
    if (techDatabase->contains(techName))
      m_equippedTechs.set(TechTypeNames.getLeft(p.first), techName);
    else
      Logger::warn("Unequipping unknown tech '{}' from slot '{}'", techName, p.first);
  }
}

Json PlayerTech::toJson() const {
  return JsonObject{
    {"availableTechs", jsonFromStringSet(m_availableTechs)},
    {"enabledTechs", jsonFromStringSet(m_enabledTechs)},
    {"equippedTechs", jsonFromMapK(m_equippedTechs, [](TechType t) {
        return TechTypeNames.getRight(t);
      })},
  };
}

bool PlayerTech::isAvailable(String const& techModule) const {
  return m_availableTechs.contains(techModule);
}

void PlayerTech::makeAvailable(String const& techModule) {
  m_availableTechs.add(techModule);
}

void PlayerTech::makeUnavailable(String const& techModule) {
  disable(techModule);
  m_availableTechs.remove(techModule);
}

bool PlayerTech::isEnabled(String const& techModule) const {
  return m_enabledTechs.contains(techModule);
}

void PlayerTech::enable(String const& techModule) {
  if (!m_availableTechs.contains(techModule))
    throw PlayerTechException::format("Enabling tech module '{}' when not available", techModule);
  m_enabledTechs.add(techModule);
}

void PlayerTech::disable(String const& techModule) {
  unequip(techModule);
  m_enabledTechs.remove(techModule);
}

bool PlayerTech::isEquipped(String const& techModule) const {
  for (auto t : m_equippedTechs.keys()) {
    if (m_equippedTechs.get(t) == techModule)
      return true;
  }
  return false;
}

void PlayerTech::equip(String const& techModule) {
  if (!m_enabledTechs.contains(techModule))
    throw PlayerTechException::format("Equipping tech module '{}' when not enabled", techModule);

  auto techDatabase = Root::singleton().techDatabase();
  m_equippedTechs[techDatabase->tech(techModule).type] = techModule;
}

void PlayerTech::unequip(String const& techModule) {
  for (auto t : m_equippedTechs.keys()) {
    if (m_equippedTechs.get(t) == techModule)
      m_equippedTechs.remove(t);
  }
}

StringSet const& PlayerTech::availableTechs() const {
  return m_availableTechs;
}

StringSet const& PlayerTech::enabledTechs() const {
  return m_enabledTechs;
}

HashMap<TechType, String> const& PlayerTech::equippedTechs() const {
  return m_equippedTechs;
}

StringList PlayerTech::techModules() const {
  return m_equippedTechs.values();
}

}
