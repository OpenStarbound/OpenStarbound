#include "StarGuiTypes.hpp"

namespace Star {

bool ImageStretchSet::fullyPopulated() const {
  return inner.size() && begin.size() && end.size();
}

GuiDirection otherDirection(GuiDirection direction) {
  switch (direction) {
    case GuiDirection::Horizontal:
      return GuiDirection::Vertical;
    case GuiDirection::Vertical:
      return GuiDirection::Horizontal;
    default:
      starAssert(false);
      return (GuiDirection)-1;
  }
}

EnumMap<GuiDirection> const GuiDirectionNames{
    {GuiDirection::Horizontal, "horizontal"}, {GuiDirection::Vertical, "vertical"},
};

String rarityBorder(Rarity rarity) {
  return strf("/interface/inventory/itemborder{}.png", RarityNames.getRight(rarity).toLower());
}

}
