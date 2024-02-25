#pragma once

#include "StarItem.hpp"
#include "StarDrawable.hpp"
#include "StarSwingableItem.hpp"
#include "StarPreviewableItem.hpp"

namespace Star {

class ThrownItem : public Item, public SwingableItem, public PreviewableItem {
public:
  ThrownItem(Json const& config, String const& directory, Json const& itemParameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;
  List<Drawable> preview(PlayerPtr const& viewer = {}) const override;

protected:
  void fireTriggered() override;

private:
  String m_projectileType;
  Json m_projectileConfig;
  size_t m_ammoUsage;
  List<Drawable> m_drawables;
};

}
