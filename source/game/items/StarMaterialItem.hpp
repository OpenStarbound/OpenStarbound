#ifndef STAR_MATERIAL_ITEM_HPP
#define STAR_MATERIAL_ITEM_HPP

#include "StarItem.hpp"
#include "StarFireableItem.hpp"
#include "StarBeamItem.hpp"
#include "StarEntityRendering.hpp"

namespace Star {

STAR_CLASS(MaterialItem);

class MaterialItem : public Item, public FireableItem, public BeamItem {
public:
  MaterialItem(Json const& config, String const& directory, Json const& settings);
  virtual ~MaterialItem() {}

  ItemPtr clone() const override;

  void init(ToolUserEntity* owner, ToolHand hand) override;
  void uninit() override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  List<Drawable> nonRotatedDrawables() const override;

  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void endFire(FireMode mode, bool shifting) override;

  MaterialId materialId() const;
  MaterialHue materialHueShift() const;

  bool canPlace(bool shifting) const;
  bool multiplaceEnabled() const;

  // FIXME: Why isn't this a PreviewTileTool then??
  List<PreviewTile> preview(bool shifting) const;

private:
  MaterialHue placementHueShift(Vec2I const& position) const;

  MaterialId m_material;
  MaterialHue m_materialHueShift;

  float m_blockRadius;
  float m_altBlockRadius;
  bool m_shifting;
  bool m_multiplace;
  Maybe<Vec2F> m_lastAimPosition;
};

}

#endif
