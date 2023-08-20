#ifndef STAR_TOOLS_HPP
#define STAR_TOOLS_HPP

#include "StarItem.hpp"
#include "StarBeamItem.hpp"
#include "StarSwingableItem.hpp"
#include "StarDurabilityItem.hpp"
#include "StarPointableItem.hpp"
#include "StarFireableItem.hpp"
#include "StarEntityRendering.hpp"
#include "StarPreviewTileTool.hpp"

namespace Star {

STAR_CLASS(World);
STAR_CLASS(WireConnector);
STAR_CLASS(ToolUserEntity);

STAR_CLASS(MiningTool);
STAR_CLASS(HarvestingTool);
STAR_CLASS(WireTool);
STAR_CLASS(Flashlight);
STAR_CLASS(BeamMiningTool);
STAR_CLASS(TillingTool);
STAR_CLASS(PaintingBeamTool);

class MiningTool : public Item, public SwingableItem, public DurabilityItem {
public:
  MiningTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;
  // In pixels, offset from image center
  Vec2F handPosition() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  float durabilityStatus() override;

  float getAngle(float aimAngle) override;

private:
  void changeDurability(float amount);

  String m_image;
  int m_frames;
  float m_frameCycle;
  float m_frameTiming;
  List<String> m_animationFrame;
  String m_idleFrame;

  Vec2F m_handPosition;
  float m_blockRadius;
  float m_altBlockRadius;

  StringList m_strikeSounds;
  String m_breakSound;
  float m_toolVolume;
  float m_blockVolume;

  bool m_pointable;
};

class HarvestingTool : public Item, public SwingableItem {
public:
  HarvestingTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;
  // In pixels, offset from image center
  Vec2F handPosition() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;
  float getAngle(float aimAngle) override;

private:
  String m_image;
  int m_frames;
  float m_frameCycle;
  float m_frameTiming;
  List<String> m_animationFrame;
  String m_idleFrame;

  Vec2F m_handPosition;

  String m_idleSound;
  StringList m_strikeSounds;
  float m_toolVolume;
  float m_harvestPower;
};

class Flashlight : public Item, public PointableItem, public ToolUserItem {
public:
  Flashlight(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;

  List<LightSource> lightSources() const;

private:
  String m_image;
  Vec2F m_handPosition;
  Vec2F m_lightPosition;
  Color m_lightColor;
  float m_beamWidth;
  float m_ambientFactor;
};

class WireTool : public Item, public FireableItem, public PointableItem, public BeamItem {
public:
  WireTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  void init(ToolUserEntity* owner, ToolHand hand) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  List<Drawable> drawables() const override;
  List<Drawable> nonRotatedDrawables() const override;

  void setEnd(EndType type) override;

  // In pixels, offset from image center
  Vec2F handPosition() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  float getAngle(float aimAngle) override;

  void setConnector(WireConnector* connector);

private:
  String m_image;
  Vec2F m_handPosition;

  StringList m_strikeSounds;
  float m_toolVolume;

  WireConnector* m_wireConnector;
};

class BeamMiningTool : public Item, public FireableItem, public PreviewTileTool, public PointableItem, public BeamItem {
public:
  BeamMiningTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;

  virtual void setEnd(EndType type) override;
  virtual List<PreviewTile> previewTiles(bool shifting) const override;
  virtual List<Drawable> nonRotatedDrawables() const override;
  virtual void fire(FireMode mode, bool shifting, bool edgeTriggered) override;

  float getAngle(float angle) override;

  void init(ToolUserEntity* owner, ToolHand hand) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  List<PersistentStatusEffect> statusEffects() const override;

private:
  float m_blockRadius;
  float m_altBlockRadius;

  float m_tileDamage;
  unsigned m_harvestLevel;
  bool m_canCollectLiquid;

  StringList m_strikeSounds;
  float m_toolVolume;
  float m_blockVolume;

  List<PersistentStatusEffect> m_inhandStatusEffects;
};

class TillingTool : public Item, public SwingableItem {
public:
  TillingTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;
  // In pixels, offset from image center
  Vec2F handPosition() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;
  float getAngle(float aimAngle) override;

private:
  String m_image;
  int m_frames;
  float m_frameCycle;
  float m_frameTiming;
  List<String> m_animationFrame;
  String m_idleFrame;

  Vec2F m_handPosition;

  String m_idleSound;
  StringList m_strikeSounds;
  float m_toolVolume;
};

class PaintingBeamTool
  : public Item,
    public FireableItem,
    public PreviewTileTool,
    public PointableItem,
    public BeamItem {
public:
  PaintingBeamTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  List<Drawable> drawables() const override;

  void setEnd(EndType type) override;
  void update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;
  List<PreviewTile> previewTiles(bool shifting) const override;
  void init(ToolUserEntity* owner, ToolHand hand) override;
  List<Drawable> nonRotatedDrawables() const override;
  void fire(FireMode mode, bool shifting, bool edgeTriggered) override;

  float getAngle(float angle) override;

private:
  List<Color> m_colors;
  List<String> m_colorKeys;
  int m_colorIndex;

  float m_blockRadius;
  float m_altBlockRadius;

  StringList m_strikeSounds;
  float m_toolVolume;
  float m_blockVolume;
};

}

#endif
