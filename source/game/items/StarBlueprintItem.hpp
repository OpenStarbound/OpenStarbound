#ifndef STAR_BLUEPRINT_ITEM_HPP
#define STAR_BLUEPRINT_ITEM_HPP

#include "StarItem.hpp"
#include "StarWorld.hpp"
#include "StarSwingableItem.hpp"

namespace Star {

STAR_CLASS(BlueprintItem);

class BlueprintItem : public Item, public SwingableItem {
public:
  BlueprintItem(Json const& config, String const& directory, Json const& data);
  virtual ItemPtr clone() const override;

  virtual List<Drawable> drawables() const override;

  virtual void fireTriggered() override;

  virtual List<Drawable> iconDrawables() const override;
  virtual List<Drawable> dropDrawables() const override;

private:
  ItemDescriptor m_recipe;
  Drawable m_recipeIconUnderlay;
  List<Drawable> m_inHandDrawable;
};

}

#endif
