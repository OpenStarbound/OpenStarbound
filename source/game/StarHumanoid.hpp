#pragma once

#include "StarDataStream.hpp"
#include "StarGameTypes.hpp"
#include "StarDrawable.hpp"
#include "StarParticle.hpp"

namespace Star {

// Required for renderDummy
STAR_CLASS(ArmorItem);
STAR_CLASS(HeadArmor);
STAR_CLASS(ChestArmor);
STAR_CLASS(LegsArmor);
STAR_CLASS(BackArmor);

STAR_CLASS(Humanoid);

STAR_STRUCT(Dance);

enum class HumanoidEmote {
  Idle,
  Blabbering,
  Shouting,
  Happy,
  Sad,
  NEUTRAL,
  Laugh,
  Annoyed,
  Oh,
  OOOH,
  Blink,
  Wink,
  Eat,
  Sleep
};
extern EnumMap<HumanoidEmote> const HumanoidEmoteNames;
size_t const EmoteSize = 14;

enum class HumanoidHand {
  Idle,
  Blabbering,
  Shouting,
  Happy,
  Sad,
  NEUTRAL,
  Laugh,
  Annoyed,
  Oh,
  OOOH,
  Blink,
  Wink,
  Eat,
  Sleep
};
extern EnumMap<HumanoidEmote> const HumanoidEmoteNames;

struct Personality {
  String idle = "idle.1";
  String armIdle = "idle.1";
  Vec2F headOffset = Vec2F();
  Vec2F armOffset = Vec2F();
};

Personality parsePersonalityArray(Json const& config);

Personality& parsePersonality(Personality& personality, Json const& config);
Personality parsePersonality(Json const& config);

Json jsonFromPersonality(Personality const& personality);

struct HumanoidIdentity {
  explicit HumanoidIdentity(Json config = Json());

  Json toJson() const;

  String name;
  // Must have :idle[1-5], :sit, :duck, :walk[1-8], :run[1-8], :jump[1-4], and
  // :fall[1-4]
  String species;
  Gender gender;

  String hairGroup;
  // Must have :normal and :climb
  String hairType;
  Directives hairDirectives;
  Directives bodyDirectives;
  Directives emoteDirectives;
  String facialHairGroup;
  String facialHairType;
  Directives facialHairDirectives;
  String facialMaskGroup;
  String facialMaskType;
  Directives facialMaskDirectives;

  Personality personality;
  Vec4B color;

  Maybe<String> imagePath;
};

DataStream& operator>>(DataStream& ds, HumanoidIdentity& identity);
DataStream& operator<<(DataStream& ds, HumanoidIdentity const& identity);

class Humanoid {
public:
  enum State {
    Idle, // 1 idle frame
    Walk, // 8 walking frames
    Run, // 8 run frames
    Jump, // 4 jump frames
    Fall, // 4 fall frames
    Swim, // 7 swim frames
    SwimIdle, // 2 swim idle frame
    Duck, // 1 ducking frame
    Sit, // 1 sitting frame
    Lay, // 1 laying frame
    STATESIZE
  };
  static EnumMap<State> const StateNames;

  static bool& globalHeadRotation();

  Humanoid(Json const& config);
  Humanoid(HumanoidIdentity const& identity);
  Humanoid(Humanoid const&) = default;

  struct HumanoidTiming {
    explicit HumanoidTiming(Json config = Json());

    static bool cyclicState(State state);
    static bool cyclicEmoteState(HumanoidEmote state);

    int stateSeq(float timer, State state) const;
    int emoteStateSeq(float timer, HumanoidEmote state) const;
    int danceSeq(float timer, DancePtr dance) const;
    int genericSeq(float timer, float cycle, unsigned frames, bool cyclic) const;

    Array<float, STATESIZE> stateCycle;
    Array<unsigned, STATESIZE> stateFrames;

    Array<float, EmoteSize> emoteCycle;
    Array<unsigned, EmoteSize> emoteFrames;
  };

  void setIdentity(HumanoidIdentity const& identity);
  HumanoidIdentity const& identity() const;

  void loadConfig(Json merger = JsonObject());

  // All of the image identifiers here are meant to be image *base* names, with
  // a collection of frames specific to each piece.  If an image is set to
  // empty string, it is disabled.
  struct WornAny {
    Directives directives;
    String frameset;
    bool rotateWithHead = false;
  };

  // Must have :normal, climb
  struct WornHead : WornAny {
    Directives maskDirectives;
  };
  // Will have :run, :normal, and :duck
  struct WornChest : WornAny {
    String frontSleeveFrameset;
    String backSleeveFrameset;
  };
  // Must have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  struct WornLegs : WornAny {};
  // Must have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  struct WornBack : WornAny {};

  typedef MVariant<WornHead, WornChest, WornLegs, WornBack> Wearable;
  
  struct Fashion {
    // 4 vanilla + 12 extra slots
    Array<Wearable, 16> wearables;
    // below 3 are recalculated when rendering updated wearables, null-terminated
    Array<uint8_t, 16> wornHeads;
    // chests and leg layering is interchangeable
    // if index > 16 then it's WornLegs at [i - 16]
    Array<uint8_t, 16> wornChestsLegs;
    Array<uint8_t, 16> wornBacks;
    bool wornHeadsChanged = true;
    bool wornChestsLegsChanged = true;
    bool wornBacksChanged = true;
    DirectivesGroup helmetMaskDirectivesGroup;
    bool helmetMasksChanged;
  };

  template <typename T>
  inline T const* getLastWearableOfType() const;

  void removeWearable(uint8_t slot);
  void setWearable(uint8_t slot, Wearable&& wearable);
  void setWearableFromHead(uint8_t slot, HeadArmor const& head);
  void setWearableFromChest(uint8_t slot, ChestArmor const& chest);
  void setWearableFromLegs(uint8_t slot, LegsArmor const& legs);
  void setWearableFromBack(uint8_t slot, BackArmor const& back);
  void refreshWearables(Fashion& fashion);

  // Legacy getters for all of the above, returns last found
  Directives const& headArmorDirectives() const;
  String const& headArmorFrameset() const;
  Directives const& chestArmorDirectives() const;
  String const& chestArmorFrameset() const;
  String const& backSleeveFrameset() const;
  String const& frontSleeveFrameset() const;
  Directives const& legsArmorDirectives() const;
  String const& legsArmorFrameset() const;
  Directives const& backArmorDirectives() const;
  String const& backArmorFrameset() const;

  void setBodyHidden(bool hidden);

  void setState(State state);
  void setEmoteState(HumanoidEmote state);
  void setDance(Maybe<String> const& dance);
  void setFacingDirection(Direction facingDirection);
  void setMovingBackwards(bool movingBackwards);
  void setHeadRotation(float headRotation);
  void setBackRotatesWithHead(bool backRotatesWithHead);
  void setRotation(float rotation);
  void setScale(Vec2F scale);

  void setVaporTrail(bool enabled);

  State state() const;
  HumanoidEmote emoteState() const;
  Maybe<String> dance() const;
  bool danceCyclicOrEnded() const;
  Direction facingDirection() const;
  bool movingBackwards() const;

  // If not rotating, then the arms follow normal movement animation.  The
  // angle parameter should be in the range [-pi/2, pi/2] (the facing direction
  // should not be included in the angle).
  void setHandParameters(ToolHand hand, bool holdingItem, float angle, float itemAngle, bool twoHanded,
      bool recoil, bool outsideOfHand);
  void setHandFrameOverrides(ToolHand hand, StringView back, StringView front);
  void setHandDrawables(ToolHand hand, List<Drawable> drawables);
  void setHandNonRotatedDrawables(ToolHand hand, List<Drawable> drawables);
  bool handHoldingItem(ToolHand hand) const;

  // Updates the animation based on whatever the current animation state is,
  // wrapping or clamping animation time as appropriate.
  void animate(float dt);

  // Reset animation time to 0.0f
  void resetAnimation();

  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in world space, not pixels)
  List<Drawable> render(bool withItems = true, bool withRotationAndScale = true);

  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in pixels, not world space)
  List<Drawable> renderPortrait(PortraitMode mode) const;

  List<Drawable> renderSkull() const;

  static HumanoidPtr makeDummy(Gender gender);
  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in pixels, not world space)
  List<Drawable> renderDummy(Gender gender, HeadArmor const* head = {}, ChestArmor const* chest = {},
      LegsArmor const* legs = {}, BackArmor const* back = {});

  Vec2F primaryHandPosition(Vec2F const& offset) const;
  Vec2F altHandPosition(Vec2F const& offset) const;

  // Finds the arm position in world space if the humanoid was facing the given
  // direction and applying the given arm angle.  The offset given is from the
  // rotation center of the arm.
  Vec2F primaryArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const;
  Vec2F altArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const;

  // Gives the offset of the hand from the arm rotation center
  Vec2F primaryHandOffset(Direction facingDirection) const;
  Vec2F altHandOffset(Direction facingDirection) const;

  Vec2F armAdjustment() const;

  Vec2F mouthOffset(bool ignoreAdjustments = false) const;
  float getBobYOffset() const;
  Vec2F feetOffset() const;

  Vec2F headArmorOffset() const;
  Vec2F chestArmorOffset() const;
  Vec2F legsArmorOffset() const;
  Vec2F backArmorOffset() const;

  String defaultDeathParticles() const;
  List<Particle> particles(String const& name) const;

  Json const& defaultMovementParameters() const;
  
  String getHeadFromIdentity() const;
  String getBodyFromIdentity() const;
  String getBodyMaskFromIdentity() const;
  String getBodyHeadMaskFromIdentity() const;
  String getFacialEmotesFromIdentity() const;
  String getHairFromIdentity() const;
  String getFacialHairFromIdentity() const;
  String getFacialMaskFromIdentity() const;
  String getBackArmFromIdentity() const;
  String getFrontArmFromIdentity() const;
  String getVaporTrailFrameset() const;

  // Extracts scalenearest from directives and returns the combined scale and
  // a new Directives without those scalenearest directives.
  static pair<Vec2F, Directives> extractScaleFromDirectives(Directives const& directives);

private:
  struct HandDrawingInfo {
    List<Drawable> itemDrawables;
    List<Drawable> nonRotatedDrawables;
    bool holdingItem = false;
    float angle = 0.0f;
    float itemAngle = 0.0f;
    String backFrame;
    String frontFrame;
    Directives backDirectives;
    Directives frontDirectives;
    float frameAngleAdjust = 0.0f;
    bool recoil = false;
    bool outsideOfHand = false;
  };

  HandDrawingInfo const& getHand(ToolHand hand) const;
  
  void wearableRemoved(Wearable const& wearable);

  String frameBase(State state) const;
  String emoteFrameBase(HumanoidEmote state) const;

  Directives const& getBodyDirectives() const;
  Directives const& getHairDirectives() const;
  Directives const& getEmoteDirectives() const;
  Directives const& getFacialHairDirectives() const;
  Directives const& getFacialMaskDirectives() const;
  DirectivesGroup const& getHelmetMaskDirectivesGroup() const;

  int getEmoteStateSequence() const;
  int getArmStateSequence() const;
  int getBodyStateSequence() const;

  Maybe<DancePtr> getDance() const;

  Json m_baseConfig;
  Json m_mergeConfig;

  Vec2F m_globalOffset;
  Vec2F m_headRunOffset;
  Vec2F m_headSwimOffset;
  Vec2F m_headDuckOffset;
  Vec2F m_headSitOffset;
  Vec2F m_headLayOffset;
  float m_runFallOffset;
  float m_duckOffset;
  float m_sitOffset;
  float m_layOffset;
  Vec2F m_recoilOffset;
  Vec2F m_mouthOffset;
  Vec2F m_feetOffset;

  Vec2F m_headArmorOffset;
  Vec2F m_chestArmorOffset;
  Vec2F m_legsArmorOffset;
  Vec2F m_backArmorOffset;
  
  bool m_useBodyMask;
  bool m_useBodyHeadMask;

  bool m_bodyHidden;

  List<int> m_armWalkSeq;
  List<int> m_armRunSeq;
  List<float> m_walkBob;
  List<float> m_runBob;
  List<float> m_swimBob;
  float m_jumpBob;
  Vec2F m_frontArmRotationCenter;
  Vec2F m_backArmRotationCenter;
  Vec2F m_frontHandPosition;
  Vec2F m_backArmOffset;

  String m_headFrameset;
  String m_bodyFrameset;
  String m_bodyMaskFrameset;
  String m_bodyHeadMaskFrameset;
  String m_backArmFrameset;
  String m_frontArmFrameset;
  String m_emoteFrameset;
  String m_hairFrameset;
  String m_facialHairFrameset;
  String m_facialMaskFrameset;

  bool m_bodyFullbright;

  String m_vaporTrailFrameset;
  unsigned m_vaporTrailFrames;
  float m_vaporTrailCycle;

  std::shared_ptr<Fashion> m_fashion;

  State m_state;
  HumanoidEmote m_emoteState;
  Maybe<String> m_dance;
  Direction m_facingDirection;
  bool m_movingBackwards;
  bool m_backRotatesWithHead;
  float m_headRotation;
  float m_headRotationTarget;
  float m_rotation;
  Vec2F m_scale;
  bool m_drawVaporTrail;

  HandDrawingInfo m_primaryHand;
  HandDrawingInfo m_altHand;

  bool m_twoHanded;

  HumanoidIdentity m_identity;
  HumanoidTiming m_timing;

  float m_animationTimer;
  float m_emoteAnimationTimer;
  float m_danceTimer;

  Json m_particleEmitters;
  String m_defaultDeathParticles;

  Json m_defaultMovementParameters;
};

template <typename T>
inline T const* Humanoid::getLastWearableOfType() const {
  for (size_t i = m_fashion->wearables.size(); i != 0; --i) {
    if (auto ptr = m_fashion->wearables[i - 1].ptr<T>())
      return ptr;
  }
  return nullptr;
}

}
