#ifndef STAR_INSTRUMENT_ITEM_HPP
#define STAR_INSTRUMENT_ITEM_HPP

#include "StarItem.hpp"
#include "StarInstrumentItem.hpp"
#include "StarStatusEffectItem.hpp"
#include "StarEffectSourceItem.hpp"
#include "StarToolUserItem.hpp"
#include "StarActivatableItem.hpp"
#include "StarPointableItem.hpp"

namespace Star {

STAR_CLASS(World);
STAR_CLASS(ToolUserEntity);
STAR_CLASS(InstrumentItem);

class InstrumentItem : public Item,
                       public StatusEffectItem,
                       public EffectSourceItem,
                       public ToolUserItem,
                       public ActivatableItem,
                       public PointableItem {
public:
  InstrumentItem(Json const& config, String const& directory, Json const& data);

  ItemPtr clone() const override;

  List<PersistentStatusEffect> statusEffects() const override;
  StringSet effectSources() const override;

  void update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  bool active() const override;
  void setActive(bool active) override;
  bool usable() const override;
  void activate() override;

  List<Drawable> drawables() const override;
  float getAngle(float angle) override;

private:
  List<PersistentStatusEffect> m_activeStatusEffects;
  List<PersistentStatusEffect> m_inactiveStatusEffects;
  StringSet m_activeEffectSources;
  StringSet m_inactiveEffectSources;
  List<Drawable> m_drawables;
  List<Drawable> m_activeDrawables;
  int m_activeCooldown;

  float m_activeAngle;
  String m_kind;
};

}

#endif
