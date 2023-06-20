#ifndef STAR_DAMAGE_DATABASE_HPP
#define STAR_DAMAGE_DATABASE_HPP

#include "StarJson.hpp"
#include "StarThread.hpp"
#include "StarDamageTypes.hpp"

namespace Star {

STAR_STRUCT(DamageKind);
STAR_CLASS(DamageDatabase);
STAR_STRUCT(ElementalType);

typedef String TargetMaterial;

struct ElementalType {
  String resistanceStat;
  HashMap<HitType, String> damageNumberParticles;
};

struct DamageEffect {
  Json sounds;
  Json particles;
};

struct DamageKind {
  String name;
  HashMap<TargetMaterial, HashMap<HitType, DamageEffect>> effects;
  String elementalType;
};

class DamageDatabase {
public:
  DamageDatabase();

  DamageKind const& damageKind(String name) const;
  ElementalType const& elementalType(String const& name) const;

private:
  StringMap<DamageKind> m_damageKinds;
  StringMap<ElementalType> m_elementalTypes;
};

}

#endif
