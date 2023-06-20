#ifndef STAR_ITEM_TOOLTIP_HPP
#define STAR_ITEM_TOOLTIP_HPP

#include "StarString.hpp"
#include "StarStatusTypes.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(Widget);
STAR_CLASS(ListWidget);
STAR_CLASS(Augment);
STAR_CLASS(Pane);
STAR_CLASS(Player);

namespace ItemTooltipBuilder {
  PanePtr buildItemTooltip(ItemPtr const& item, PlayerPtr const& viewer = {});

  void buildItemDescription(WidgetPtr const& container, ItemPtr const& item);
  void buildItemDescriptionInner(
      WidgetPtr const& container, ItemPtr const& item, String const& tooltipKind, String& title, String& subtitle, PlayerPtr const& viewer = {});

  void describePersistentEffect(ListWidgetPtr const& container, PersistentStatusEffect const& effect);
};

}

#endif
