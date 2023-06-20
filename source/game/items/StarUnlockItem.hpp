#ifndef STAR_CELESTIAL_ITEM_HPP
#define STAR_CELESTIAL_ITEM_HPP

#include "StarItem.hpp"
#include "StarWorld.hpp"
#include "StarSwingableItem.hpp"
#include "StarPreviewableItem.hpp"

namespace Star {

STAR_CLASS(UnlockItem);

class UnlockItem : public Item, public SwingableItem, public PreviewableItem {
public:
  UnlockItem(Json const& config, String const& directory, Json const& itemParameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;
  List<Drawable> preview(PlayerPtr const& viewer = {}) const override;

protected:
  void fireTriggered() override;

private:
  Maybe<String> m_sectorUnlock;
  Maybe<String> m_tierRecipesUnlock;
  Maybe<unsigned> m_shipUpgrade;
  String m_unlockMessage;
  List<Drawable> m_drawables;
};

}

#endif
