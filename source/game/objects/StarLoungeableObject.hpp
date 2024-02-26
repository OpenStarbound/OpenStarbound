#pragma once

#include "StarObject.hpp"
#include "StarLoungingEntities.hpp"

namespace Star {

class LoungeableObject : public Object, public virtual LoungeableEntity {
public:
  LoungeableObject(ObjectConfigConstPtr config, Json const& parameters = Json());

  void render(RenderCallback* renderCallback) override;

  InteractAction interact(InteractRequest const& request) override;

  size_t anchorCount() const override;
  LoungeAnchorConstPtr loungeAnchor(size_t positionIndex) const override;

protected:
  void setOrientationIndex(size_t orientationIndex) override;

private:
  List<Vec2F> m_sitPositions;
  bool m_sitFlipDirection;
  LoungeOrientation m_sitOrientation;
  float m_sitAngle;
  String m_sitCoverImage;
  bool m_flipImages;
  List<PersistentStatusEffect> m_sitStatusEffects;
  StringSet m_sitEffectEmitters;
  Maybe<String> m_sitEmote;
  Maybe<String> m_sitDance;
  JsonObject m_sitArmorCosmeticOverrides;
  Maybe<String> m_sitCursorOverride;
};

}
