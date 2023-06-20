#include "StarDamageBarEntity.hpp"

namespace Star {

EnumMap<DamageBarType> const DamageBarTypeNames{
  {DamageBarType::Default, "Default"},
  {DamageBarType::None, "None"},
  {DamageBarType::Special, "Special"}
};

}
