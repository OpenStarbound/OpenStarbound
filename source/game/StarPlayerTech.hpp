#ifndef STAR_PLAYER_TECH_HPP
#define STAR_PLAYER_TECH_HPP

#include "StarTechDatabase.hpp"

namespace Star {

STAR_EXCEPTION(PlayerTechException, StarException);

STAR_CLASS(PlayerTech);

// Set of player techs, techs can be either unavailable, available but not
// enabled, enabled but not equipped, or equipped.
class PlayerTech {
public:
  PlayerTech();
  PlayerTech(Json const& json);

  Json toJson() const;

  bool isAvailable(String const& techModule) const;
  void makeAvailable(String const& techModule);
  void makeUnavailable(String const& techModule);

  bool isEnabled(String const& techModule) const;
  void enable(String const& techModule);
  void disable(String const& techModule);

  bool isEquipped(String const& techModule) const;
  void equip(String const& techModule);
  void unequip(String const& techModule);

  StringSet const& availableTechs() const;
  StringSet const& enabledTechs() const;
  HashMap<TechType, String> const& equippedTechs() const;
  StringList techModules() const;

private:
  StringSet m_availableTechs;
  StringSet m_enabledTechs;
  HashMap<TechType, String> m_equippedTechs;
};

}

#endif
