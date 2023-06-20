#include "StarDamageTypes.hpp"

namespace Star {

EnumMap<DamageType> const DamageTypeNames{{DamageType::NoDamage, "NoDamage"},
    {DamageType::Damage, "Damage"},
    {DamageType::IgnoresDef, "IgnoresDef"},
    {DamageType::Knockback, "Knockback"},
    {DamageType::Environment, "Environment"},
		{DamageType::Status, "Status"}};

EnumMap<HitType> const HitTypeNames{
    {HitType::Hit, "Hit"},
    {HitType::StrongHit, "StrongHit"},
    {HitType::WeakHit, "WeakHit"},
    {HitType::ShieldHit, "ShieldHit"},
    {HitType::Kill, "Kill"}};

EnumMap<TeamType> const TeamTypeNames{{TeamType::Null, "null"},
    {TeamType::Friendly, "friendly"},
    {TeamType::Enemy, "enemy"},
    {TeamType::PVP, "pvp"},
    {TeamType::Passive, "passive"},
    {TeamType::Ghostly, "ghostly"},
    {TeamType::Environment, "environment"},
    {TeamType::Indiscriminate, "indiscriminate"},
    {TeamType::Assistant, "assistant"}};

EntityDamageTeam::EntityDamageTeam() : type(TeamType::Null), team(0) {}

EntityDamageTeam::EntityDamageTeam(TeamType type, TeamNumber team) : type(type), team(team) {}

EntityDamageTeam::EntityDamageTeam(Json const& json) {
  type = TeamTypeNames.getLeft(json.getString("type"));
  team = json.getUInt("team", 0);
}

Json EntityDamageTeam::toJson() const {
  return JsonObject{{"type", TeamTypeNames.getRight(type)}, {"team", team}};
}

bool EntityDamageTeam::canDamage(EntityDamageTeam victim, bool victimIsSelf) const {
  if (victimIsSelf) {
    if (type == TeamType::Indiscriminate)
      return true;
  } else if (type == TeamType::Friendly) {
    if (victim.type == TeamType::Enemy || victim.type == TeamType::Passive || victim.type == TeamType::Environment || victim.type == TeamType::Indiscriminate)
      return true;
  } else if (type == TeamType::Enemy) {
    if (victim.type == TeamType::Friendly || victim.type == TeamType::PVP || victim.type == TeamType::Indiscriminate)
      return true;
    else if (victim.type == TeamType::Enemy && team != victim.team)
      return true;
  } else if (type == TeamType::PVP) {
    if (victim.type == TeamType::Enemy || victim.type == TeamType::Passive || victim.type == TeamType::Environment || victim.type == TeamType::Indiscriminate)
      return true;
    else if (victim.type == TeamType::PVP && (team == 0 || team != victim.team))
      return true;
  } else if (type == TeamType::Passive) {
    // never deal damage
  } else if (type == TeamType::Ghostly) {
    // never deal damage
  } else if (type == TeamType::Environment) {
    if (victim.type == TeamType::Friendly || victim.type == TeamType::PVP || victim.type == TeamType::Indiscriminate)
      return true;
  } else if (type == TeamType::Indiscriminate) {
    if (victim.type == TeamType::Friendly || victim.type == TeamType::Enemy || victim.type == TeamType::PVP
        || victim.type == TeamType::Passive
        || victim.type == TeamType::Environment
        || victim.type == TeamType::Indiscriminate)
      return true;
  } else if (type == TeamType::Assistant) {
    if (victim.type == TeamType::Enemy || victim.type == TeamType::Passive || victim.type == TeamType::Environment || victim.type == TeamType::Indiscriminate)
      return true;
  }

  return false;
}

bool EntityDamageTeam::operator==(EntityDamageTeam const& rhs) const {
  return tie(type, team) == tie(rhs.type, rhs.team);
}

DataStream& operator<<(DataStream& ds, EntityDamageTeam const& team) {
  ds.write(team.type);
  ds.write(team.team);
  return ds;
}

DataStream& operator>>(DataStream& ds, EntityDamageTeam& team) {
  ds.read(team.type);
  ds.read(team.team);
  return ds;
}

TeamNumber soloPvpTeam(ConnectionId clientId) {
  return (TeamNumber)clientId;
}

}
