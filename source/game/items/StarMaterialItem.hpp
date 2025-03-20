#pragma once

#include "StarItem.hpp"
#include "StarFireableItem.hpp"
#include "StarBeamItem.hpp"
#include "StarEntityRendering.hpp"
#include "StarPreviewTileTool.hpp"
#include "StarRenderableItem.hpp"
#include "StarPreviewableItem.hpp"
#include "StarCollisionBlock.hpp"

namespace Star {

STAR_CLASS(MaterialItem);
STAR_CLASS(Player);

class MaterialItem : public Item, public FireableItem, public PreviewTileTool, public RenderableItem, public PreviewableItem, public BeamItem {
public:
  MaterialItem(Json const& config, String const& directory, Json const& settings);
  virtual ~MaterialItem() {}

  ItemPtr clone() const override;

  void init(ToolUserEntity* owner, ToolHand hand) override;
  void uninit() override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;
  void render(RenderCallback* renderCallback, EntityRenderLayer renderLayer) override;

  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const override;
  virtual List<Drawable> dropDrawables() const override;
  List<Drawable> nonRotatedDrawables() const override;

  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void endFire(FireMode mode, bool shifting) override;

  MaterialId materialId() const;
  MaterialHue materialHueShift() const;

  bool canPlace(bool shifting) const;
  bool multiplaceEnabled() const;

  float& blockRadius();
  float& altBlockRadius();
  TileCollisionOverride& collisionOverride();

  List<PreviewTile> previewTiles(bool shifting) const override;
  List<Drawable> const& generatedPreview(Vec2I position = {}) const;
private:
  void blockSwap(float radius, TileLayer layer);
  void updatePropertiesFromPlayer(Player* player);
  float calcRadius(bool shifting) const;
  List<Vec2I>& tileArea(float radius, Vec2F const& position) const;
  MaterialHue placementHueShift(Vec2I const& position) const;

  MaterialId m_material;
  MaterialHue m_materialHueShift;

  float m_blockRadius;
  float m_altBlockRadius;
  bool m_blockSwap;
  bool m_shifting;
  bool m_multiplace;
  StringList m_placeSounds;
  Maybe<Vec2F> m_lastAimPosition;
  TileCollisionOverride m_collisionOverride;

  mutable Vec2F m_lastTileAreaOriginCache;
  mutable float m_lastTileAreaRadiusCache;
  mutable List<Vec2I> m_tileAreasCache;

  mutable Maybe<List<Drawable>> m_generatedPreviewCache;
};

}
