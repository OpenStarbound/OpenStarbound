#include "StarHumanoid.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarArmors.hpp"
#include "StarParticleDatabase.hpp"
#include "StarAssets.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarDanceDatabase.hpp"

namespace Star {

extern EnumMap<HumanoidEmote> const HumanoidEmoteNames{
  {HumanoidEmote::Idle, "Idle"},
  {HumanoidEmote::Blabbering, "Blabbering"},
  {HumanoidEmote::Shouting, "Shouting"},
  {HumanoidEmote::Happy, "Happy"},
  {HumanoidEmote::Sad, "Sad"},
  {HumanoidEmote::NEUTRAL, "NEUTRAL"},
  {HumanoidEmote::Laugh, "Laugh"},
  {HumanoidEmote::Annoyed, "Annoyed"},
  {HumanoidEmote::Oh, "Oh"},
  {HumanoidEmote::OOOH, "OOOH"},
  {HumanoidEmote::Blink, "Blink"},
  {HumanoidEmote::Wink, "Wink"},
  {HumanoidEmote::Eat, "Eat"},
  {HumanoidEmote::Sleep, "Sleep"}
};

Personality parsePersonalityArray(Json const& config) {
  return Personality{config.getString(0), config.getString(1), jsonToVec2F(config.get(2)), jsonToVec2F(config.get(3))};
}

Personality& parsePersonality(Personality& personality, Json const& config) {
  if (auto idle = config.opt("idle"))
    personality.idle = idle->toString();
  if (auto armIdle = config.opt("armIdle"))
    personality.armIdle = armIdle->toString();
  if (auto headOffset = config.opt("headOffset"))
    personality.headOffset = jsonToVec2F(*headOffset);
  if (auto armOffset = config.opt("armOffset"))
    personality.armOffset = jsonToVec2F(*armOffset);

  return personality;
}

Personality parsePersonality(Json const& config) {
  Personality personality;
  return parsePersonality(personality, config);
}

Json jsonFromPersonality(Personality const& personality) {
  return JsonObject{
    { "idle", personality.idle },
    { "armIdle", personality.armIdle },
    { "headOffset", jsonFromVec2F(personality.headOffset) },
    { "armOffset", jsonFromVec2F(personality.armOffset)   }
  };
}

HumanoidIdentity::HumanoidIdentity(Json config) {
  if (config.isNull())
    config = JsonObject();

  name = config.getString("name", "Humanoid");
  species = config.getString("species", "human");
  gender = GenderNames.getLeft(config.getString("gender", "male"));
  hairGroup = config.getString("hairGroup", "hair");
  hairType = config.getString("hairType", "male1");
  hairDirectives = config.getString("hairDirectives", "");
  bodyDirectives = config.getString("bodyDirectives", "");
  if (auto jEmoteDirectives = config.optString("emoteDirectives")) // Passing Directives as a default arg would be inefficient
    emoteDirectives = jEmoteDirectives.take();
  else
    emoteDirectives = bodyDirectives;

  facialHairGroup = config.getString("facialHairGroup", "");
  facialHairType = config.getString("facialHairType", "");
  facialHairDirectives = config.getString("facialHairDirectives", "");
  facialMaskGroup = config.getString("facialMaskGroup", "");
  facialMaskType = config.getString("facialMaskType", "");
  facialMaskDirectives = config.getString("facialMaskDirectives", "");

  personality.idle = config.getString("personalityIdle", "idle.1");
  personality.armIdle = config.getString("personalityArmIdle", "idle.1");
  personality.headOffset = jsonToVec2F(config.get("personalityHeadOffset", JsonArray{0, 0}));
  personality.armOffset = jsonToVec2F(config.get("personalityArmOffset", JsonArray{0, 0}));

  color = jsonToColor(config.get("color", "white")).toRgba();

  imagePath = config.optString("imagePath");
}

Json HumanoidIdentity::toJson() const {
  auto result = JsonObject{
    {"name", name},
    {"species", species},
    {"gender", GenderNames.getRight(gender)},
    {"hairGroup", hairGroup},
    {"hairType", hairType},
    {"hairDirectives", hairDirectives.string()},
    {"bodyDirectives", bodyDirectives.string()},
    {"emoteDirectives", emoteDirectives.string()},
    {"facialHairGroup", facialHairGroup},
    {"facialHairType", facialHairType},
    {"facialHairDirectives", facialHairDirectives.string()},
    {"facialMaskGroup", facialMaskGroup},
    {"facialMaskType", facialMaskType},
    {"facialMaskDirectives", facialMaskDirectives.string()},
    {"personalityIdle", personality.idle},
    {"personalityArmIdle", personality.armIdle},
    {"personalityHeadOffset", jsonFromVec2F(personality.headOffset)},
    {"personalityArmOffset", jsonFromVec2F(personality.armOffset)},
    {"color", jsonFromColor(Color::rgba(color))}
  };
  if (imagePath)
    result["imagePath"] = *imagePath;
  return result;
}

DataStream& operator>>(DataStream& ds, HumanoidIdentity& identity) {
  ds.read(identity.name);
  ds.read(identity.species);
  ds.read(identity.gender);
  ds.read(identity.hairGroup);
  ds.read(identity.hairType);
  ds.read(identity.hairDirectives);
  ds.read(identity.bodyDirectives);
  ds.read(identity.emoteDirectives);
  ds.read(identity.facialHairGroup);
  ds.read(identity.facialHairType);
  ds.read(identity.facialHairDirectives);
  ds.read(identity.facialMaskGroup);
  ds.read(identity.facialMaskType);
  ds.read(identity.facialMaskDirectives);
  ds.read(identity.personality.idle);
  ds.read(identity.personality.armIdle);
  ds.read(identity.personality.headOffset);
  ds.read(identity.personality.armOffset);
  ds.read(identity.color);
  ds.read(identity.imagePath);

  return ds;
}

DataStream& operator<<(DataStream& ds, HumanoidIdentity const& identity) {
  ds.write(identity.name);
  ds.write(identity.species);
  ds.write(identity.gender);
  ds.write(identity.hairGroup);
  ds.write(identity.hairType);
  ds.write(identity.hairDirectives);
  ds.write(identity.bodyDirectives);
  ds.write(identity.emoteDirectives);
  ds.write(identity.facialHairGroup);
  ds.write(identity.facialHairType);
  ds.write(identity.facialHairDirectives);
  ds.write(identity.facialMaskGroup);
  ds.write(identity.facialMaskType);
  ds.write(identity.facialMaskDirectives);
  ds.write(identity.personality.idle);
  ds.write(identity.personality.armIdle);
  ds.write(identity.personality.headOffset);
  ds.write(identity.personality.armOffset);
  ds.write(identity.color);
  ds.write(identity.imagePath);

  return ds;
}

Humanoid::HumanoidTiming::HumanoidTiming(Json config) {
  if (config.type() != Json::Type::Object) {
    auto assets = Root::singleton().assets();
    config = assets->json("/humanoid.config:humanoidTiming");
  }

  if (config.contains("stateCycle"))
    stateCycle = jsonToArrayF<STATESIZE>(config.get("stateCycle"));
  if (config.contains("stateFrames"))
    stateFrames = jsonToArrayU<STATESIZE>(config.get("stateFrames"));

  if (config.contains("emoteCycle"))
    emoteCycle = jsonToArrayF<EmoteSize>(config.get("emoteCycle"));
  if (config.contains("emoteFrames"))
    emoteFrames = jsonToArrayU<EmoteSize>(config.get("emoteFrames"));
}

bool Humanoid::HumanoidTiming::cyclicState(State state) {
  switch (state) {
    case State::Walk:
    case State::Run:
    case State::Swim:
      return true;
    default:
      return false;
  }
}

bool Humanoid::HumanoidTiming::cyclicEmoteState(HumanoidEmote state) {
  switch (state) {
    case HumanoidEmote::Blabbering:
    case HumanoidEmote::Shouting:
    case HumanoidEmote::Sad:
    case HumanoidEmote::Laugh:
    case HumanoidEmote::Eat:
    case HumanoidEmote::Sleep:
      return true;
    default:
      return false;
  }
}

int Humanoid::HumanoidTiming::stateSeq(float timer, State state) const {
  return genericSeq(timer, stateCycle[state], stateFrames[state], cyclicState(state));
}

int Humanoid::HumanoidTiming::emoteStateSeq(float timer, HumanoidEmote state) const {
  return genericSeq(timer, emoteCycle[(size_t)state], emoteFrames[(size_t)state], cyclicEmoteState(state));
}

int Humanoid::HumanoidTiming::danceSeq(float timer, DancePtr dance) const {
  return genericSeq(timer, dance->cycle, dance->steps.size(), dance->cyclic) - 1;
}

int Humanoid::HumanoidTiming::genericSeq(float timer, float cycle, unsigned frames, bool cyclic) const {
  if (cyclic) {
    timer = fmod(timer, cycle);
  }
  return clamp<int>(timer * frames / cycle, 0, frames - 1) + 1;
}

EnumMap<Humanoid::State> const Humanoid::StateNames{
    {Humanoid::State::Idle, "idle"},
    {Humanoid::State::Walk, "walk"},
    {Humanoid::State::Run, "run"},
    {Humanoid::State::Jump, "jump"},
    {Humanoid::State::Fall, "fall"},
    {Humanoid::State::Swim, "swim"},
    {Humanoid::State::SwimIdle, "swimIdle"},
    {Humanoid::State::Duck, "duck"},
    {Humanoid::State::Sit, "sit"},
    {Humanoid::State::Lay, "lay"},
};

// gross, but I don't want to make config calls more than I need to
bool& Humanoid::globalHeadRotation() {
  static Maybe<bool> s_headRotation;
  if (!s_headRotation)
    s_headRotation = Root::singleton().configuration()->get("humanoidHeadRotation").optBool().value(true);
  return *s_headRotation;
};

Humanoid::Humanoid(Json const& config) {
  m_fashion = std::make_shared<Fashion>();
  m_baseConfig = config;
  loadConfig(JsonObject());

  m_twoHanded = false;
  m_primaryHand.holdingItem = false;
  m_altHand.holdingItem = false;

  m_movingBackwards = false;
  m_altHand.angle = 0;
  m_facingDirection = Direction::Left;
  m_headRotationTarget = m_headRotation = m_rotation = 0;
  m_scale = Vec2F::filled(1.f);
  m_drawVaporTrail = false;
  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;
  m_dance = {};

  m_primaryHand.angle = 0;
  m_animationTimer = m_emoteAnimationTimer = m_danceTimer = 0.0f;
}

Humanoid::Humanoid(HumanoidIdentity const& identity)
  : Humanoid(Root::singleton().speciesDatabase()->species(identity.species)->humanoidConfig()) {
  setIdentity(identity);
}

void Humanoid::setIdentity(HumanoidIdentity const& identity) {
  String lastSpecies = std::move(m_identity.species);
  m_identity = identity;
  if (m_identity.species != lastSpecies) {
    m_baseConfig = Root::singleton().speciesDatabase()->species(identity.species)->humanoidConfig();
    loadConfig(take(m_mergeConfig), true);
  }
  m_headFrameset = getHeadFromIdentity();
  m_bodyFrameset = getBodyFromIdentity();
  m_emoteFrameset = getFacialEmotesFromIdentity();
  m_hairFrameset = getHairFromIdentity();
  m_facialHairFrameset = getFacialHairFromIdentity();
  m_facialMaskFrameset = getFacialMaskFromIdentity();
  m_backArmFrameset = getBackArmFromIdentity();
  m_frontArmFrameset = getFrontArmFromIdentity();
  m_vaporTrailFrameset = getVaporTrailFrameset();
  if (m_useBodyMask) {
    m_bodyMaskFrameset = getBodyMaskFromIdentity();
  }
  if (m_useBodyHeadMask) {
    m_bodyHeadMaskFrameset = getBodyHeadMaskFromIdentity();
  }
}

HumanoidIdentity const& Humanoid::identity() const {
  return m_identity;
}

bool Humanoid::loadConfig(Json merger, bool forceRefresh) {
  if (m_mergeConfig == merger && !forceRefresh)
    return false;

  auto config = jsonMerge(m_baseConfig, merger);
  m_timing = HumanoidTiming(config.getObject("humanoidTiming"));

  m_globalOffset = jsonToVec2F(config.get("globalOffset")) / TilePixels;
  m_headRunOffset = jsonToVec2F(config.get("headRunOffset")) / TilePixels;
  m_headSwimOffset = jsonToVec2F(config.get("headSwimOffset")) / TilePixels;
  m_runFallOffset = config.get("runFallOffset").toDouble() / TilePixels;
  m_duckOffset = config.get("duckOffset").toDouble() / TilePixels;
  m_headDuckOffset = jsonToVec2F(config.get("headDuckOffset")) / TilePixels;
  m_sitOffset = config.get("sitOffset").toDouble() / TilePixels;
  m_layOffset = config.get("layOffset").toDouble() / TilePixels;
  m_headSitOffset = jsonToVec2F(config.get("headSitOffset")) / TilePixels;
  m_headLayOffset = jsonToVec2F(config.get("headLayOffset")) / TilePixels;
  m_recoilOffset = jsonToVec2F(config.get("recoilOffset")) / TilePixels;
  m_mouthOffset = jsonToVec2F(config.get("mouthOffset")) / TilePixels;
  m_feetOffset = jsonToVec2F(config.get("feetOffset")) / TilePixels;

  m_bodyFullbright = config.getBool("bodyFullbright", false);
  m_useBodyMask = config.getBool("useBodyMask", false);
  m_useBodyHeadMask = config.getBool("useBodyHeadMask", false);

  m_headArmorOffset = jsonToVec2F(config.get("headArmorOffset")) / TilePixels;
  m_chestArmorOffset = jsonToVec2F(config.get("chestArmorOffset")) / TilePixels;
  m_legsArmorOffset = jsonToVec2F(config.get("legsArmorOffset")) / TilePixels;
  m_backArmorOffset = jsonToVec2F(config.get("backArmorOffset")) / TilePixels;

  m_bodyHidden = config.getBool("bodyHidden", false);

  m_armWalkSeq = jsonToIntList(config.get("armWalkSeq"));
  m_armRunSeq = jsonToIntList(config.get("armRunSeq"));

  m_walkBob.clear();
  for (auto const& v : config.get("walkBob").toArray())
    m_walkBob.append(v.toDouble() / TilePixels);
  m_runBob.clear();
  for (auto const& v : config.get("runBob").toArray())
    m_runBob.append(v.toDouble() / TilePixels);
  m_swimBob.clear();
  for (auto const& v : config.get("swimBob").toArray())
    m_swimBob.append(v.toDouble() / TilePixels);

  m_jumpBob = config.get("jumpBob").toDouble() / TilePixels;
  m_frontArmRotationCenter = jsonToVec2F(config.get("frontArmRotationCenter")) / TilePixels;
  m_backArmRotationCenter = jsonToVec2F(config.get("backArmRotationCenter")) / TilePixels;
  m_frontHandPosition = jsonToVec2F(config.get("frontHandPosition")) / TilePixels;
  m_backArmOffset = jsonToVec2F(config.get("backArmOffset")) / TilePixels;
  m_vaporTrailFrames = config.get("vaporTrailFrames").toInt();
  m_vaporTrailCycle = config.get("vaporTrailCycle").toFloat();

  m_defaultDeathParticles = config.getString("deathParticles");
  m_particleEmitters = config.get("particleEmitters");

  Json newMovementParameters = config.get("movementParameters");
  bool movementParametersChanged = m_defaultMovementParameters != newMovementParameters;
  m_defaultMovementParameters = newMovementParameters;

  m_mergeConfig = merger;

  return movementParametersChanged;
}

void Humanoid::wearableRemoved(Wearable const& wearable) {
  auto& fashion = *m_fashion;
  if (auto head = wearable.ptr<WornHead>()) {
    fashion.wornHeadsChanged = true;
    if (!head->maskDirectives.empty())
      fashion.helmetMasksChanged = true;
  } else if (wearable.is<WornChest>() || wearable.is<WornLegs>())
    fashion.wornChestsLegsChanged = true;
  else if (wearable.is<WornBack>())
    fashion.wornBacksChanged = true;
}

void Humanoid::removeWearable(uint8_t slot) {
  auto& fashion = *m_fashion;
  Wearable& current = fashion.wearables.at(slot);
  wearableRemoved(current);
  current.reset();
}

void Humanoid::setWearableFromHead(uint8_t slot, HeadArmor const& head, Gender gender) {
  auto& fashion = *m_fashion;
  Wearable& current = fashion.wearables.at(slot);
  if (auto currentHead = current.ptr<WornHead>())
    fashion.helmetMasksChanged |= currentHead->maskDirectives != head.maskDirectives();
  else {
    wearableRemoved(current);
    fashion.wornHeadsChanged = true;
    fashion.helmetMasksChanged |= !head.maskDirectives().empty();
  }
  current.makeType(current.typeIndexOf<WornHead>());
  auto& wornHead = current.get<WornHead>();
  wornHead.directives = head.directives(m_facingDirection == Direction::Left);
  wornHead.frameset = head.frameset(gender);
  wornHead.maskDirectives = head.maskDirectives();
}

void Humanoid::setWearableFromChest(uint8_t slot, ChestArmor const& chest, Gender gender) {
  auto& fashion = *m_fashion;
  Wearable& current = fashion.wearables.at(slot);
  if (!current.is<WornChest>() && !current.is<WornLegs>()) {
    wearableRemoved(current);
    fashion.wornChestsLegsChanged = true;
  }

  current.makeType(current.typeIndexOf<WornChest>());
  auto& wornChest = current.get<WornChest>();
  wornChest.directives = chest.directives(m_facingDirection == Direction::Left);
  wornChest.frameset = chest.bodyFrameset(gender);
  wornChest.backSleeveFrameset = chest.backSleeveFrameset(gender);
  wornChest.frontSleeveFrameset = chest.frontSleeveFrameset(gender);
}

void Humanoid::setWearableFromLegs(uint8_t slot, LegsArmor const& legs, Gender gender) {
  auto& fashion = *m_fashion;
  Wearable& current = fashion.wearables.at(slot);
  if (!current.is<WornChest>() && !current.is<WornLegs>()) {
    wearableRemoved(current);
    fashion.wornChestsLegsChanged = true;
  }

  current.makeType(current.typeIndexOf<WornLegs>());
  auto& wornLegs = current.get<WornLegs>();
  wornLegs.directives = legs.directives(m_facingDirection == Direction::Left);
  wornLegs.frameset = legs.frameset(gender);
}

void Humanoid::setWearableFromBack(uint8_t slot, BackArmor const& back, Gender gender) {
  auto& fashion = *m_fashion;
  Wearable& current = fashion.wearables.at(slot);
  if (!current.is<WornBack>()) {
    wearableRemoved(current);
    fashion.wornBacksChanged = true;
  }

  current.makeType(current.typeIndexOf<WornBack>());
  auto& wornBack = current.get<WornBack>();
  wornBack.directives = back.directives(m_facingDirection == Direction::Left);
  wornBack.frameset = back.frameset(gender);
  wornBack.rotateWithHead = back.instanceValue("rotateWithHead", false).optBool().value();
}

const uint8_t ChestLegsSortOrder[21] = {255, 14, 8, 2, 0, 15, 9, 3, 1, 4, 5, 6, 7, 10, 11, 12, 13, 16, 17, 18, 19};

void Humanoid::refreshWearables(Fashion& fashion) {
  bool wornHeadsChanged = fashion.wornHeadsChanged;
  bool wornChestsLegsChanged = fashion.wornChestsLegsChanged;
  bool wornBacksChanged = fashion.wornBacksChanged;
  bool helmetMasksChanged = fashion.helmetMasksChanged;
  if (!wornHeadsChanged && !wornChestsLegsChanged && !wornBacksChanged && !helmetMasksChanged)
    return;

  if (wornHeadsChanged)
    fashion.wornHeads.fill(0);
  if (wornChestsLegsChanged)
    fashion.wornChestsLegs.fill(0);
  if (wornBacksChanged)
    fashion.wornBacks.fill(0);
  if (helmetMasksChanged)
    fashion.helmetMaskDirectivesGroup.clear();
  uint8_t headI = 0, chestsLegsI = 0, backsI = 0;
  for (uint8_t i = 0; i != fashion.wearables.size(); ++i) {
    auto& wearable = fashion.wearables[i];
    if (!wearable)
      continue;
    else if (auto head = wearable.ptr<WornHead>()) {
      if (helmetMasksChanged)
        fashion.helmetMaskDirectivesGroup += head->maskDirectives;
      if (wornHeadsChanged)
        fashion.wornHeads[headI++] = i + 1;
    } else if (wearable.is<WornChest>() || wearable.is<WornLegs>()) {
      if (wornChestsLegsChanged)
        fashion.wornChestsLegs[chestsLegsI++] = i + 1;
    } else if (wearable.is<WornBack>()) {
      if (wornBacksChanged)
        fashion.wornBacks[backsI++] = i + 1;
    }
  }
  if (wornChestsLegsChanged) {
    sort(fashion.wornChestsLegs, [](uint8_t a, uint8_t b) {
      return ChestLegsSortOrder[a] < ChestLegsSortOrder[b];
    });
  }
  fashion.wornHeadsChanged = fashion.wornChestsLegsChanged = fashion.wornBacksChanged = fashion.helmetMasksChanged = false;
}

const Directives nullDirectives = {};
const String nullFrameset = "";

Directives const& Humanoid::headArmorDirectives() const {
  if (auto head = getLastWearableOfType<WornHead>())
    return head->directives;
  return nullDirectives;
};

String const& Humanoid::headArmorFrameset() const {
  if (auto head = getLastWearableOfType<WornHead>())
    return head->frameset;
  return nullFrameset;
};

Directives const& Humanoid::chestArmorDirectives() const {
  if (auto chest = getLastWearableOfType<WornChest>())
    return chest->directives;
  return nullDirectives;
};

String const& Humanoid::chestArmorFrameset() const {
  if (auto chest = getLastWearableOfType<WornChest>())
    return chest->frameset;
  return nullFrameset;
};

String const& Humanoid::backSleeveFrameset() const {
  if (auto chest = getLastWearableOfType<WornChest>())
    return chest->backSleeveFrameset;
  return nullFrameset;
};

String const& Humanoid::frontSleeveFrameset() const {
  if (auto chest = getLastWearableOfType<WornChest>())
    return chest->frontSleeveFrameset;
  return nullFrameset;
};

Directives const& Humanoid::legsArmorDirectives() const {
  if (auto legs = getLastWearableOfType<WornLegs>())
    return legs->directives;
  return nullDirectives;
};

String const& Humanoid::legsArmorFrameset() const {
  if (auto legs = getLastWearableOfType<WornLegs>())
    return legs->frameset;
  return nullFrameset;
};

Directives const& Humanoid::backArmorDirectives() const {
  if (auto back = getLastWearableOfType<WornBack>())
    return back->directives;
  return nullDirectives;
};

String const& Humanoid::backArmorFrameset() const {
  if (auto back = getLastWearableOfType<WornBack>())
    return back->frameset;
  return nullFrameset;
};

void Humanoid::setBodyHidden(bool hidden) {
  m_bodyHidden = hidden;
}

void Humanoid::setState(State state) {
  if (m_state != state)
    m_animationTimer = 0.0f;
  m_state = state;
}

void Humanoid::setEmoteState(HumanoidEmote state) {
  if (m_emoteState != state)
    m_emoteAnimationTimer = 0.0f;
  m_emoteState = state;
}

void Humanoid::setDance(Maybe<String> const& dance) {
  if (m_dance != dance)
    m_danceTimer = 0.0f;
  m_dance = dance;
}

void Humanoid::setFacingDirection(Direction facingDirection) {
  m_facingDirection = facingDirection;
}

void Humanoid::setMovingBackwards(bool movingBackwards) {
  m_movingBackwards = movingBackwards;
}

void Humanoid::setHeadRotation(float headRotation) {
  m_headRotationTarget = headRotation;
}

void Humanoid::setRotation(float rotation) {
  m_rotation = rotation;
}

void Humanoid::setScale(Vec2F scale) {
  m_scale = scale;
}

void Humanoid::setVaporTrail(bool enabled) {
  m_drawVaporTrail = enabled;
}

Humanoid::State Humanoid::state() const {
  return m_state;
}

HumanoidEmote Humanoid::emoteState() const {
  return m_emoteState;
}

Maybe<String> Humanoid::dance() const {
  return m_dance;
}

bool Humanoid::danceCyclicOrEnded() const {
  if (!m_dance)
    return false;

  auto danceDatabase = Root::singleton().danceDatabase();
  auto dance = danceDatabase->getDance(*m_dance);
  return dance->cyclic || m_danceTimer > dance->duration;
}

Direction Humanoid::facingDirection() const {
  return m_facingDirection;
}

bool Humanoid::movingBackwards() const {
  return m_movingBackwards;
}

void Humanoid::setHandParameters(ToolHand hand, bool holdingItem, float angle, float itemAngle, bool twoHanded,
    bool recoil, bool outsideOfHand) {
  auto& handInfo = const_cast<HandDrawingInfo&>(getHand(hand));
  handInfo.holdingItem = holdingItem;
  handInfo.angle = angle;
  handInfo.itemAngle = itemAngle;
  handInfo.recoil = recoil;
  handInfo.outsideOfHand = outsideOfHand;
  if (hand == ToolHand::Primary)
    m_twoHanded = twoHanded;
}

void Humanoid::setHandFrameOverrides(ToolHand hand, StringView back, StringView front) {
  auto& handInfo = const_cast<HandDrawingInfo&>(getHand(hand));
  // some users stick directives in these?? better make sure they don't break with custom clothing
  size_t  backEnd =  back.utf8().find('?');
  size_t frontEnd = front.utf8().find('?');
  Directives  backDirectives =  backEnd == NPos ? Directives() : Directives( back.utf8().substr( backEnd));
  Directives frontDirectives = frontEnd == NPos ? Directives() : Directives(front.utf8().substr(frontEnd));
  if ( backEnd != NPos)  back =  back.utf8().substr(0,  backEnd);
  if (frontEnd != NPos) front = front.utf8().substr(0, frontEnd);
  handInfo. backFrame = ! back.empty() ? std::move( back) : "rotation";
  handInfo.frontFrame = !front.empty() ? std::move(front) : "rotation";
  handInfo. backDirectives = ! backDirectives.empty() ? std::move( backDirectives) : Directives();
  handInfo.frontDirectives = !frontDirectives.empty() ? std::move(frontDirectives) : Directives();
}

void Humanoid::setHandDrawables(ToolHand hand, List<Drawable> drawables) {
  const_cast<HandDrawingInfo&>(getHand(hand)).itemDrawables = std::move(drawables);
}

void Humanoid::setHandNonRotatedDrawables(ToolHand hand, List<Drawable> drawables) {
  const_cast<HandDrawingInfo&>(getHand(hand)).nonRotatedDrawables = std::move(drawables);
}

bool Humanoid::handHoldingItem(ToolHand hand) const {
  return getHand(hand).holdingItem;
}

void Humanoid::animate(float dt) {
  m_animationTimer += dt;
  m_emoteAnimationTimer += dt;
  m_danceTimer += dt;
  float headRotationTarget = globalHeadRotation() ? m_headRotationTarget : 0.f;
  m_headRotation = (headRotationTarget - (headRotationTarget - m_headRotation) * powf(.333333f, dt * 60.f));
}

void Humanoid::resetAnimation() {
  m_animationTimer = 0.0f;
  m_emoteAnimationTimer = 0.0f;
  m_danceTimer = 0.0f;
  m_headRotation = globalHeadRotation() ? 0.f : m_headRotationTarget;
}

List<Drawable> Humanoid::render(bool withItems, bool withRotationAndScale) {
  auto& fashion = *m_fashion;
  refreshWearables(fashion);
  List<Drawable> drawables;

  int armStateSeq = getArmStateSequence();
  int bodyStateSeq = getBodyStateSequence();
  int emoteStateSeq = getEmoteStateSequence();
  float bobYOffset = getBobYOffset();
  Maybe<DancePtr> dance = getDance();
  Maybe<DanceStep> danceStep = {};
  if (dance.isValid()) {
    if (!(*dance)->states.contains(StateNames.getRight(m_state)))
      dance = {};
    else
      danceStep = (*dance)->steps[m_timing.danceSeq(m_danceTimer, *dance)];
  }

  auto frontHand = (m_facingDirection == Direction::Left || m_twoHanded) ? m_primaryHand : m_altHand;
  auto backHand = (m_facingDirection == Direction::Right || m_twoHanded) ? m_primaryHand : m_altHand;

  Vec2F frontArmFrameOffset = Vec2F(0, bobYOffset);
  if (frontHand.recoil)
    frontArmFrameOffset += m_recoilOffset;
  Vec2F backArmFrameOffset = Vec2F(0, bobYOffset);
  if (backHand.recoil)
    backArmFrameOffset += m_recoilOffset;

  auto addDrawable = [&](Drawable drawable, bool forceFullbright = false) -> Drawable& {
    if (m_facingDirection == Direction::Left)
      drawable.scale(Vec2F(-1, 1));
    drawable.fullbright |= forceFullbright;
    drawables.append(std::move(drawable));
    return drawables.back();
  };

  auto backArmDrawable = [&](String const& frameSet, Directives const& directives) -> Drawable {
    String image = strf("{}:{}{}", frameSet, backHand.backFrame, directives.prefix());
    Drawable backArm = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, backArmFrameOffset);
    backArm.imagePart().addDirectives(directives, true);
    backArm.imagePart().addDirectives(backHand.backDirectives, true);
    backArm.rotate(backHand.angle, backArmFrameOffset + m_backArmRotationCenter + m_backArmOffset);
    return backArm;
  };

  Vec2F headPosition(0, bobYOffset);
  if (dance.isValid())
    headPosition += danceStep->headOffset / TilePixels;
  else if (m_state == Idle)
    headPosition += m_identity.personality.headOffset / TilePixels;
  else if (m_state == Run)
    headPosition += m_headRunOffset;
  else if (m_state == Swim || m_state == SwimIdle)
    headPosition += m_headSwimOffset;
  else if (m_state == Duck)
    headPosition += m_headDuckOffset;
  else if (m_state == Sit)
    headPosition += m_headSitOffset;
  else if (m_state == Lay)
    headPosition += m_headLayOffset;

  auto applyHeadRotation = [&](Drawable& drawable) {
    if (m_headRotation != 0.f) {
      float dir = numericalDirection(m_facingDirection);
      Vec2F rotationPoint = headPosition;
      rotationPoint[0] *= dir;
      rotationPoint[1] -= .25f;
      float headX = (m_headRotation / ((float)Constants::pi * 2.f));
      drawable.rotate(m_headRotation, rotationPoint);
      drawable.position[0] -= state() == State::Run ? (fmaxf(headX * dir, 0.f) * 2.f) * dir : headX;
      drawable.position[1] -= fabsf(m_headRotation / ((float)Constants::pi * 4.f));
    }
  };

  for (uint8_t i : fashion.wornBacks) {
    if (i == 0)
      break;
    auto& back = fashion.wearables[size_t(i) - 1].get<WornBack>();
    if (!back.frameset.empty()) {
      auto frameGroup = frameBase(m_state);
      auto prefix = back.directives.prefix();
      if (m_movingBackwards && (m_state == State::Run))
        frameGroup = "runbackwards";
      String image;
      if (dance.isValid() && danceStep->bodyFrame)
        image = strf("{}:{}{}", back.frameset, *danceStep->bodyFrame, prefix);
      else if (m_state == Idle)
        image = strf("{}:{}{}", back.frameset, m_identity.personality.idle, prefix);
      else
        image = strf("{}:{}.{}{}", back.frameset, frameGroup, bodyStateSeq, prefix);

      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, Vec2F());
      drawable.imagePart().addDirectives(back.directives, true);
      Drawable& applied = addDrawable(std::move(drawable));
      if (back.rotateWithHead)
        applyHeadRotation(applied);
    }
  }
  
  auto drawBackArmAndSleeves = [&](bool holdingItem) {
    auto bodyDirectives = getBodyDirectives();
    if (holdingItem && !m_bodyHidden)
      addDrawable(backArmDrawable(m_backArmFrameset, bodyDirectives), m_bodyFullbright);
    else if (!m_backArmFrameset.empty() && !m_bodyHidden) {
      String image;
      Vec2F position;
      auto prefix = bodyDirectives.prefix();
      if (dance.isValid() && danceStep->backArmFrame) {
        image = strf("{}:{}{}", m_backArmFrameset, *danceStep->backArmFrame, prefix);
        position = danceStep->backArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_backArmFrameset, m_identity.personality.armIdle, prefix);
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_backArmFrameset, frameBase(m_state), armStateSeq, prefix);
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(bodyDirectives, true);
      if (dance.isValid())
        drawable.rotate(danceStep->backArmRotation);
      addDrawable(std::move(drawable), m_bodyFullbright);
    }
    for (uint8_t i : fashion.wornChestsLegs) {
      if (i == 0)
        break;
      auto chest = fashion.wearables[size_t(i) - 1].ptr<WornChest>();
      if (chest && !chest->backSleeveFrameset.empty()) {
        if (holdingItem) {
          addDrawable(backArmDrawable(chest->backSleeveFrameset, chest->directives));
        } else {
          String image;
          Vec2F position;
          auto prefix = chest->directives.prefix();
          if (dance.isValid() && danceStep->backArmFrame) {
            image = strf("{}:{}{}", chest->backSleeveFrameset, *danceStep->backArmFrame, prefix);
            position = danceStep->backArmOffset / TilePixels;
          } else if (m_state == Idle) {
            image = strf("{}:{}{}", chest->backSleeveFrameset, m_identity.personality.armIdle, prefix);
            position = m_identity.personality.armOffset / TilePixels;
          } else
            image = strf("{}:{}.{}{}", chest->backSleeveFrameset, frameBase(m_state), armStateSeq, prefix);
          auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
          drawable.imagePart().addDirectives(chest->directives, true);
          if (dance.isValid())
            drawable.rotate(danceStep->backArmRotation);
          addDrawable(std::move(drawable));
        }
      }
    }
  };

  if (backHand.holdingItem && !dance.isValid() && withItems) {
    auto drawItem = [&]() {
      for (auto& backHandItem : backHand.itemDrawables) {
        backHandItem.translate(m_frontHandPosition + backArmFrameOffset + m_backArmOffset);
        backHandItem.rotate(backHand.itemAngle, backArmFrameOffset + m_backArmRotationCenter + m_backArmOffset);
        addDrawable(std::move(backHandItem));
      }
    };
    if (!m_twoHanded && backHand.outsideOfHand)
      drawItem();

    drawBackArmAndSleeves(true);

    if (!m_twoHanded && !backHand.outsideOfHand)
      drawItem();
  } else {
    drawBackArmAndSleeves(false);
  }

  auto addHeadDrawable = [&](Drawable drawable, bool forceFullbright = false) {
    if (m_facingDirection == Direction::Left)
      drawable.scale(Vec2F(-1, 1));
    drawable.fullbright |= forceFullbright;
    applyHeadRotation(drawable);
    drawables.append(std::move(drawable));
  };

  if (!m_headFrameset.empty() && !m_bodyHidden) {
    String image = strf("{}:normal", m_headFrameset);
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    drawable.imagePart().addDirectives(getBodyDirectives(), true);
    addHeadDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_emoteFrameset.empty() && !m_bodyHidden) {
    auto emoteDirectives = getEmoteDirectives();
    String image = strf("{}:{}.{}{}", m_emoteFrameset, emoteFrameBase(m_emoteState), emoteStateSeq, emoteDirectives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    drawable.imagePart().addDirectives(emoteDirectives, true);
    addHeadDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_hairFrameset.empty() && !m_bodyHidden) {
    String image = strf("{}:normal", m_hairFrameset);
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    drawable.imagePart().addDirectives(getHairDirectives(), true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addHeadDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_bodyFrameset.empty() && !m_bodyHidden) {
    auto bodyDirectives = getBodyDirectives();
    auto prefix = bodyDirectives.prefix();
    String frameName;
    if (dance.isValid() && danceStep->bodyFrame)
      frameName = strf("{}{}", *danceStep->bodyFrame, prefix);
    else if (m_state == Idle)
      frameName = strf("{}{}", m_identity.personality.idle, prefix);
    else
      frameName = strf("{}.{}{}", frameBase(m_state), bodyStateSeq, prefix);
    String image = strf("{}:{}",m_bodyFrameset,frameName);
    auto drawable = Drawable::makeImage(m_useBodyHeadMask ? image : std::move(image), 1.0f / TilePixels, true, {});
    drawable.imagePart().addDirectives(bodyDirectives, true);
    if (m_useBodyMask && !m_bodyMaskFrameset.empty()) {
      String maskImage = strf("{}:{}",m_bodyMaskFrameset,frameName);
      Directives maskDirectives = "?addmask="+maskImage+";0;0";
      drawable.imagePart().addDirectives(maskDirectives, true);
    }
    addDrawable(std::move(drawable), m_bodyFullbright);
    if (m_useBodyHeadMask && !m_bodyHeadMaskFrameset.empty()) {
      String maskImage = strf("{}:{}",m_bodyHeadMaskFrameset,frameName);
      Directives maskDirectives = "?addmask="+maskImage+";0;0";
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
      drawable.imagePart().addDirectives(bodyDirectives, true);
      drawable.imagePart().addDirectives(maskDirectives, true);
      addHeadDrawable(std::move(drawable), m_bodyFullbright);
    }
  }

  for (uint8_t i : fashion.wornChestsLegs) {
    if (i == 0)
      break;
    Wearable& wearable = fashion.wearables[size_t(i) - 1];
    auto* legs = wearable.ptr<WornLegs>();
    if (legs && !legs->frameset.empty()) {
      String image;
      auto prefix = legs->directives.prefix();
      if (dance.isValid() && danceStep->bodyFrame)
        image = strf("{}:{}{}", legs->frameset, *danceStep->bodyFrame, prefix);
      else if (m_state == Idle)
        image = strf("{}:{}{}", legs->frameset, m_identity.personality.idle, prefix);
      else
        image = strf("{}:{}.{}{}", legs->frameset, frameBase(m_state), bodyStateSeq, prefix);
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
      drawable.imagePart().addDirectives(legs->directives, true);
      addDrawable(std::move(drawable));
    } else {
      auto* chest = wearable.ptr<WornChest>();
      if (chest && !chest->frameset.empty()) {
        String image;
        Vec2F position;
        auto prefix = chest->directives.prefix();
        if (dance.isValid() && danceStep->bodyFrame)
          image = strf("{}:{}{}", chest->frameset, *danceStep->bodyFrame, prefix);
        else if (m_state == Run)
          image = strf("{}:run{}", chest->frameset, prefix);
        else if (m_state == Idle)
          image = strf("{}:{}{}", chest->frameset, m_identity.personality.idle, prefix);
        else if (m_state == Duck)
          image = strf("{}:duck{}", chest->frameset, prefix);
        else if ((m_state == Swim) || (m_state == SwimIdle))
          image = strf("{}:swim{}", chest->frameset, prefix);
        else
          image = strf("{}:chest.1{}", chest->frameset, prefix);
        if (m_state != Duck)
          position[1] += bobYOffset;
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(chest->directives, true);
        addDrawable(std::move(drawable));
      }
    }
  }

  if (!m_facialHairFrameset.empty() && !m_bodyHidden) {
    String image = strf("{}:normal", m_facialHairFrameset);
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    drawable.imagePart().addDirectives(getFacialHairDirectives(), true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addHeadDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_facialMaskFrameset.empty() && !m_bodyHidden) {
    String image = strf("{}:normal", m_facialMaskFrameset);
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    drawable.imagePart().addDirectives(getFacialMaskDirectives(), true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addHeadDrawable(std::move(drawable));
  }

  for (uint8_t i : fashion.wornHeads) {
    if (i == 0)
      break;
    auto& head = fashion.wearables[size_t(i) - 1].get<WornHead>();
    if (!head.frameset.empty()) {
      String image = strf("{}:normal{}", head.frameset, head.directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
      drawable.imagePart().addDirectives(head.directives, true);
      addHeadDrawable(std::move(drawable));
    }
  }

  auto frontArmDrawable = [&](String const& frameSet, Directives const& directives) -> Drawable {
    String image = strf("{}:{}{}", frameSet, frontHand.frontFrame, directives.prefix());
    Drawable frontArm = Drawable::makeImage(image, 1.0f / TilePixels, true, frontArmFrameOffset);
    frontArm.imagePart().addDirectives(directives, true);
    frontArm.imagePart().addDirectives(frontHand.frontDirectives, true);
    frontArm.rotate(frontHand.angle, frontArmFrameOffset + m_frontArmRotationCenter);
    return frontArm;
  };

  
  auto drawFrontArmAndSleeves = [&](bool holdingItem) {
    auto& bodyDirectives = getBodyDirectives();
    if (holdingItem && !m_bodyHidden)
      addDrawable(frontArmDrawable(m_frontArmFrameset, bodyDirectives), m_bodyFullbright);
    else if (!m_frontArmFrameset.empty() && !m_bodyHidden) {
      String image;
      Vec2F position;
      auto prefix = bodyDirectives.prefix();
      if (dance.isValid() && danceStep->frontArmFrame) {
        image = strf("{}:{}{}", m_frontArmFrameset, *danceStep->frontArmFrame, prefix);
        position = danceStep->frontArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_frontArmFrameset, m_identity.personality.armIdle, prefix);
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_frontArmFrameset, frameBase(m_state), armStateSeq, prefix);
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(bodyDirectives, true);
      if (dance.isValid())
        drawable.rotate(danceStep->frontArmRotation);
      addDrawable(drawable, m_bodyFullbright);
    }
    for (uint8_t i : fashion.wornChestsLegs) {
      if (i == 0)
        break;
      auto chest = fashion.wearables[size_t(i) - 1].ptr<WornChest>();
      if (chest && !chest->frontSleeveFrameset.empty()) {
        if (holdingItem) {
          addDrawable(frontArmDrawable(chest->frontSleeveFrameset, chest->directives));
        } else {
          String image;
          Vec2F position;
          auto prefix = chest->directives.prefix();
          if (dance.isValid() && danceStep->frontArmFrame) {
            image = strf("{}:{}{}", chest->frontSleeveFrameset, *danceStep->frontArmFrame, prefix);
            position = danceStep->frontArmOffset / TilePixels;
          } else if (m_state == Idle) {
            image = strf("{}:{}{}", chest->frontSleeveFrameset, m_identity.personality.armIdle, prefix);
            position = m_identity.personality.armOffset / TilePixels;
          } else
            image = strf("{}:{}.{}{}", chest->frontSleeveFrameset, frameBase(m_state), armStateSeq, prefix);
          auto drawable = Drawable::makeImage(image, 1.0f / TilePixels, true, position);
          drawable.imagePart().addDirectives(chest->directives, true);
          if (dance.isValid())
            drawable.rotate(danceStep->frontArmRotation);
          addDrawable(drawable);
        }
      }
    }
  };

  if (frontHand.holdingItem && !dance.isValid() && withItems) {
    auto drawItem = [&]() {
      for (auto& frontHandItem : frontHand.itemDrawables) {
        frontHandItem.translate(m_frontHandPosition + frontArmFrameOffset);
        frontHandItem.rotate(frontHand.itemAngle, frontArmFrameOffset + m_frontArmRotationCenter);
        addDrawable(frontHandItem);
      }
    };
    if (!frontHand.outsideOfHand)
      drawItem();

    drawFrontArmAndSleeves(true);

    if (frontHand.outsideOfHand)
      drawItem();
  } else {
    drawFrontArmAndSleeves(false);
  }

  if (m_drawVaporTrail) {
    auto image = strf("{}:{}",
        m_vaporTrailFrameset,
        m_timing.genericSeq(m_animationTimer, m_vaporTrailCycle, m_vaporTrailFrames, true));
    addDrawable(Drawable::makeImage(AssetPath::split(image), 1.0f / TilePixels, true, {}));
  }

  if (withItems) {
    if (m_primaryHand.nonRotatedDrawables.size())
      drawables.insertAllAt(0, m_primaryHand.nonRotatedDrawables);
    if (m_altHand.nonRotatedDrawables.size())
      drawables.insertAllAt(0, m_altHand.nonRotatedDrawables);
  }

  for (auto& drawable : drawables) {
    drawable.translate(m_globalOffset);
    if (withRotationAndScale) {
      if (m_scale.x() != 1.f || m_scale.y() != 1.f)
        drawable.scale(m_scale);
      if (m_rotation != 0.f)
        drawable.rotate(m_rotation);
    }
    drawable.rebase();
  }
  return drawables;
}

List<Drawable> Humanoid::renderPortrait(PortraitMode mode) const {
  auto& fashion = *m_fashion;
  ((Humanoid*)this)->refreshWearables(fashion); // bleh
  List<Drawable> drawables;
  int emoteStateSeq = m_timing.emoteStateSeq(m_emoteAnimationTimer, m_emoteState);

  auto addDrawable = [&](Drawable&& drawable) -> Drawable& {
    if (mode != PortraitMode::Full && mode != PortraitMode::FullNeutral
      && mode != PortraitMode::FullNude && mode != PortraitMode::FullNeutralNude) {
      // TODO: make this configurable
      drawable.imagePart().addDirectives(String("addmask=/humanoid/portraitMask.png;0;0"), false);
    }
    drawables.append(std::move(drawable));
    return drawables.back();
  };

  bool dressed = !(mode == PortraitMode::FullNude || mode == PortraitMode::FullNeutralNude);

  auto personality = m_identity.personality;
  if (mode == PortraitMode::FullNeutral || mode == PortraitMode::FullNeutralNude)
    personality = Root::singleton().speciesDatabase()->species(m_identity.species)->personalities()[0];

  if (mode != PortraitMode::Head) {
    if (!m_backArmFrameset.empty()) {
      auto& bodyDirectives = getBodyDirectives();
      String image = strf("{}:{}{}", m_backArmFrameset, personality.armIdle, bodyDirectives.prefix());
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
      drawable.imagePart().addDirectives(bodyDirectives, true);
      addDrawable(std::move(drawable));
    }
    if (dressed) {
      for (uint8_t i : fashion.wornChestsLegs) {
        if (i == 0)
          break;
        auto chest = fashion.wearables[size_t(i) - 1].ptr<WornChest>();
        if (chest && !chest->backSleeveFrameset.empty()) {
          String image = strf("{}:{}{}", chest->backSleeveFrameset, personality.armIdle, chest->directives.prefix());
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(chest->directives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
    if (mode != PortraitMode::Bust && dressed) {
      for (uint8_t i : fashion.wornBacks) {
        if (i == 0)
          break;
        auto& back = fashion.wearables[size_t(i) - 1].get<WornBack>();
        if (!back.frameset.empty()) {
          auto backDirectives = back.directives;
          String image = strf("{}:{}{}", back.frameset, personality.idle, backDirectives.prefix());
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
          drawable.imagePart().addDirectives(backDirectives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
  }

  if (!m_headFrameset.empty()) {
    auto& bodyDirectives = getBodyDirectives();
    String image = strf("{}:normal{}", m_headFrameset, bodyDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(bodyDirectives, true);
    addDrawable(std::move(drawable));
  }

  if (!m_emoteFrameset.empty()) {
    auto& emoteDirectives = getEmoteDirectives();
    String image = strf("{}:{}.{}{}", m_emoteFrameset, emoteFrameBase(m_emoteState), emoteStateSeq, emoteDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(emoteDirectives, true);
    addDrawable(std::move(drawable));
  }

  if (!m_hairFrameset.empty()) {
    auto& hairDirectives = getHairDirectives();
    String image = strf("{}:normal{}", m_hairFrameset, hairDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(hairDirectives, true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addDrawable(std::move(drawable));
  }

  if (!m_bodyFrameset.empty()) {
    auto& bodyDirectives = getBodyDirectives();
    String image = strf("{}:{}{}", m_bodyFrameset, personality.idle, bodyDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
    drawable.imagePart().addDirectives(bodyDirectives, true);
    addDrawable(std::move(drawable));
  }

  if (mode != PortraitMode::Head && dressed) {
    for (uint8_t i : fashion.wornChestsLegs) {
      if (i == 0)
        break;
      auto& wearable = fashion.wearables[size_t(i) - 1];
      auto* legs = wearable.ptr<WornLegs>();
      if (legs && !legs->frameset.empty()) {
        String image = strf("{}:{}{}", legs->frameset, personality.idle, legs->directives.prefix());
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
        drawable.imagePart().addDirectives(legs->directives, true);
        addDrawable(std::move(drawable));
      } else {
        auto* chest = wearable.ptr<WornChest>();
        if (chest && !chest->frameset.empty()) {
          String image = strf("{}:{}{}", chest->frameset, personality.idle, chest->directives.prefix());
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
          drawable.imagePart().addDirectives(chest->directives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
  }

  if (!m_facialHairFrameset.empty()) {
    auto facialHairDirectives = getFacialHairDirectives();
    String image = strf("{}:normal{}", m_facialHairFrameset, facialHairDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(facialHairDirectives, true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addDrawable(std::move(drawable));
  }

  if (!m_facialMaskFrameset.empty()) {
    auto facialMaskDirectives = getFacialMaskDirectives();
    String image = strf("{}:normal{}", m_facialMaskFrameset, facialMaskDirectives.prefix());
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(facialMaskDirectives, true).addDirectivesGroup(fashion.helmetMaskDirectivesGroup, true);
    addDrawable(std::move(drawable));
  }

  if (dressed) {
    for (uint8_t i : fashion.wornHeads) {
      if (i == 0)
        break;
      auto& head = fashion.wearables[size_t(i) - 1].get<WornHead>();
      if (!head.frameset.empty()) {
        String image = strf("{}:normal{}", head.frameset, head.directives.prefix());
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
        drawable.imagePart().addDirectives(head.directives, true);
        addDrawable(std::move(drawable));
      }
    }
  }

  if (mode != PortraitMode::Head) {
    if (!m_frontArmFrameset.empty()) {
      auto bodyDirectives = getBodyDirectives();
      String image = strf("{}:{}{}", m_frontArmFrameset, personality.armIdle, bodyDirectives.prefix());
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
      drawable.imagePart().addDirectives(bodyDirectives, true);
      addDrawable(std::move(drawable));
    }

    if (dressed) {
      for (uint8_t i : fashion.wornChestsLegs) {
        if (i == 0)
          break;
        auto chest = fashion.wearables[size_t(i) - 1].ptr<WornChest>();
        if (chest && !chest->frontSleeveFrameset.empty()) {
          String image = strf("{}:{}{}", chest->frontSleeveFrameset, personality.armIdle, chest->directives.prefix());
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(chest->directives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
  }

  return drawables;
}

List<Drawable> Humanoid::renderSkull() const {
  return {Drawable::makeImage(
      Root::singleton().speciesDatabase()->species(m_identity.species)->skull(), 1.0f, true, Vec2F())};
}

HumanoidPtr Humanoid::makeDummy(Gender) {
  auto assets = Root::singleton().assets();
  HumanoidPtr humanoid = make_shared<Humanoid>(assets->json("/humanoid.config"));

  humanoid->m_headFrameset = assets->json("/humanoid/any/dummy.config:head").toString();
  humanoid->m_bodyFrameset = assets->json("/humanoid/any/dummy.config:body").toString();
  humanoid->m_frontArmFrameset = assets->json("/humanoid/any/dummy.config:frontArm").toString();
  humanoid->m_backArmFrameset = assets->json("/humanoid/any/dummy.config:backArm").toString();
  humanoid->setFacingDirection(DirectionNames.getLeft(assets->json("/humanoid/any/dummy.config:direction").toString()));

  return humanoid;
}

List<Drawable> Humanoid::renderDummy(Gender gender, HeadArmor const* head, ChestArmor const* chest, LegsArmor const* legs, BackArmor const* back) {
  std::shared_ptr<Fashion> fashion = std::move(m_fashion);
  List<Drawable> drawables;
  try {
    m_fashion = std::make_shared<Fashion>();
    if (head)
      setWearableFromHead(3, *head, gender);
    if (chest)
      setWearableFromChest(2, *chest, gender);
    if (legs)
      setWearableFromLegs(1, *legs, gender);
    if (back)
      setWearableFromBack(0, *back, gender);

    drawables = render(false, false);
    Drawable::scaleAll(drawables, TilePixels);
  }
  catch (std::exception const& e) {
    m_fashion = std::move(fashion);
    throw;
  }
  m_fashion = std::move(fashion);
  return drawables;
}

Vec2F Humanoid::primaryHandPosition(Vec2F const& offset) const {
  return primaryArmPosition(m_facingDirection, m_primaryHand.angle, primaryHandOffset(m_facingDirection) + offset);
}

Vec2F Humanoid::altHandPosition(Vec2F const& offset) const {
  return altArmPosition(m_facingDirection, m_altHand.angle, altHandOffset(m_facingDirection) + offset);
}

Vec2F Humanoid::primaryArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const {
  float bobYOffset = getBobYOffset();

  if (m_primaryHand.holdingItem) {
    Vec2F rotationCenter;
    if (facingDirection == Direction::Left || m_twoHanded)
      rotationCenter = m_frontArmRotationCenter + Vec2F(0, bobYOffset);
    else
      rotationCenter = m_backArmRotationCenter + Vec2F(0, bobYOffset) + m_backArmOffset;

    Vec2F position = offset.rotate(armAngle) + rotationCenter;

    if (facingDirection == Direction::Left)
      position[0] *= -1;

    return position;
  } else {
    // TODO: non-aiming item holding not supported yet
    return Vec2F();
  }
}

Vec2F Humanoid::altArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const {
  float bobYOffset = getBobYOffset();

  if (m_altHand.holdingItem) {
    Vec2F rotationCenter;
    if (facingDirection == Direction::Right)
      rotationCenter = m_frontArmRotationCenter + Vec2F(0, bobYOffset);
    else
      rotationCenter = m_backArmRotationCenter + Vec2F(0, bobYOffset) + m_backArmOffset;

    Vec2F position = Vec2F(offset).rotate(armAngle) + rotationCenter;

    if (facingDirection == Direction::Left)
      position[0] *= -1;

    return position;
  } else {
    // TODO: non-aiming item holding not supported yet
    return Vec2F();
  }
}

Vec2F Humanoid::primaryHandOffset(Direction facingDirection) const {
  if (facingDirection == Direction::Left || m_twoHanded)
    return m_frontHandPosition - m_frontArmRotationCenter;
  else
    return m_frontHandPosition - m_backArmRotationCenter;
}

Vec2F Humanoid::altHandOffset(Direction facingDirection) const {
  if (facingDirection == Direction::Left || m_twoHanded)
    return m_frontHandPosition - m_backArmRotationCenter;
  else
    return m_frontHandPosition - m_frontArmRotationCenter;
}

Humanoid::HandDrawingInfo const& Humanoid::getHand(ToolHand hand) const {
  return hand == ToolHand::Primary ? m_primaryHand : m_altHand;
}

String Humanoid::frameBase(State state) const {
  switch (state) {
    case State::Idle:
      return "idle";
    case State::Walk:
      return "walk";
    case State::Run:
      return "run";
    case State::Jump:
      return "jump";
    case State::Swim:
      return "swim";
    case State::SwimIdle:
      return "swimIdle";
    case State::Duck:
      return "duck";
    case State::Fall:
      return "fall";
    case State::Sit:
      return "sit";
    case State::Lay:
      return "lay";

    default:
      throw StarException(strf("No such state '{}'", StateNames.getRight(state)));
  }
}

String Humanoid::emoteFrameBase(HumanoidEmote state) const {
  switch (state) {
    case HumanoidEmote::Idle:
      return "idle";
    case HumanoidEmote::Blabbering:
      return "blabber";
    case HumanoidEmote::Shouting:
      return "shout";
    case HumanoidEmote::Happy:
      return "happy";
    case HumanoidEmote::Sad:
      return "sad";
    case HumanoidEmote::NEUTRAL:
      return "neutral";
    case HumanoidEmote::Laugh:
      return "laugh";
    case HumanoidEmote::Annoyed:
      return "annoyed";
    case HumanoidEmote::Oh:
      return "oh";
    case HumanoidEmote::OOOH:
      return "oooh";
    case HumanoidEmote::Blink:
      return "blink";
    case HumanoidEmote::Wink:
      return "wink";
    case HumanoidEmote::Eat:
      return "eat";
    case HumanoidEmote::Sleep:
      return "sleep";

    default:
      throw StarException(strf("No such emote state '{}'", HumanoidEmoteNames.getRight(state)));
  }
}

String Humanoid::getHeadFromIdentity() const {
  return strf("/humanoid/{}/{}head.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      GenderNames.getRight(m_identity.gender));
}

String Humanoid::getBodyFromIdentity() const {
  return strf("/humanoid/{}/{}body.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      GenderNames.getRight(m_identity.gender));
}

String Humanoid::getBodyMaskFromIdentity() const {
  return strf("/humanoid/{}/mask/{}body.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      GenderNames.getRight(m_identity.gender));
}

String Humanoid::getBodyHeadMaskFromIdentity() const {
  return strf("/humanoid/{}/headmask/{}body.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      GenderNames.getRight(m_identity.gender));
}

String Humanoid::getFacialEmotesFromIdentity() const {
  return strf("/humanoid/{}/emote.png", m_identity.imagePath ? *m_identity.imagePath : m_identity.species);
}

String Humanoid::getHairFromIdentity() const {
  if (m_identity.hairType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      m_identity.hairGroup,
      m_identity.hairType);
}

String Humanoid::getFacialHairFromIdentity() const {
  if (m_identity.facialHairType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      m_identity.facialHairGroup,
      m_identity.facialHairType);
}

String Humanoid::getFacialMaskFromIdentity() const {
  if (m_identity.facialMaskType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
      m_identity.facialMaskGroup,
      m_identity.facialMaskType);
}

String Humanoid::getBackArmFromIdentity() const {
  return strf("/humanoid/{}/backarm.png", m_identity.imagePath ? *m_identity.imagePath : m_identity.species);
}

String Humanoid::getFrontArmFromIdentity() const {
  return strf("/humanoid/{}/frontarm.png", m_identity.imagePath ? *m_identity.imagePath : m_identity.species);
}

String Humanoid::getVaporTrailFrameset() const {
  return "/humanoid/any/flames.png";
}

Directives const& Humanoid::getBodyDirectives() const {
  return m_identity.bodyDirectives;
}

Directives const& Humanoid::getHairDirectives() const {
  return m_identity.hairDirectives;
}

Directives const& Humanoid::getEmoteDirectives() const {
  return m_identity.emoteDirectives;
}

Directives const& Humanoid::getFacialHairDirectives() const {
  return m_identity.facialHairDirectives;
}

Directives const& Humanoid::getFacialMaskDirectives() const {
  return m_identity.facialMaskDirectives;
}

DirectivesGroup const& Humanoid::getHelmetMaskDirectivesGroup() const {
  return m_fashion->helmetMaskDirectivesGroup;
}

int Humanoid::getEmoteStateSequence() const {
  return m_timing.emoteStateSeq(m_emoteAnimationTimer, m_emoteState);
}

int Humanoid::getArmStateSequence() const {
  int stateSeq = m_timing.stateSeq(m_animationTimer, m_state);

  int armStateSeq;
  if (m_state == Walk) {
    armStateSeq = m_movingBackwards ? m_armWalkSeq.at(m_armWalkSeq.size() - stateSeq) : m_armWalkSeq.at(stateSeq - 1);
  } else if (m_state == Run) {
    armStateSeq = m_movingBackwards ? m_armRunSeq.at(m_armRunSeq.size() - stateSeq) : m_armRunSeq.at(stateSeq - 1);
  } else {
    armStateSeq = stateSeq;
  }

  return armStateSeq;
}

int Humanoid::getBodyStateSequence() const {
  int stateSeq = m_timing.stateSeq(m_animationTimer, m_state);

  int bodyStateSeq;
  if (m_movingBackwards && (m_state == Walk || m_state == Run))
    bodyStateSeq = m_timing.stateFrames[m_state] - stateSeq + 1;
  else
    bodyStateSeq = stateSeq;

  return bodyStateSeq;
}

Maybe<DancePtr> Humanoid::getDance() const {
  if (m_dance.isNothing())
    return {};

  auto danceDatabase = Root::singleton().danceDatabase();
  return danceDatabase->getDance(*m_dance);
}

float Humanoid::getBobYOffset() const {
  int bodyStateSeq = getBodyStateSequence();
  float bobYOffset = 0.0f;
  if (m_state == Run) {
    bobYOffset = m_runFallOffset + m_runBob.at(bodyStateSeq - 1);
  } else if (m_state == Fall) {
    bobYOffset = m_runFallOffset;
  } else if (m_state == Jump) {
    bobYOffset = m_jumpBob;
  } else if (m_state == Walk) {
    bobYOffset = m_walkBob.at(bodyStateSeq - 1);
  } else if (m_state == Swim) {
    bobYOffset = m_swimBob.at(bodyStateSeq - 1);
  } else if (m_state == Duck) {
    bobYOffset = m_duckOffset;
  } else if (m_state == Sit) {
    bobYOffset = m_sitOffset;
  } else if (m_state == Lay) {
    bobYOffset = m_layOffset;
  }
  return bobYOffset;
}

Vec2F Humanoid::armAdjustment() const {
  return Vec2F(0, getBobYOffset());
}

Vec2F Humanoid::mouthOffset(bool ignoreAdjustments) const {
  if (ignoreAdjustments) {
    return (m_mouthOffset).rotate(m_rotation);
  } else {
    Vec2F headPosition(0, getBobYOffset());
    if (m_state == Idle)
      headPosition += m_identity.personality.headOffset / TilePixels;
    else if (m_state == Run)
      headPosition += m_headRunOffset;
    else if (m_state == Swim || m_state == SwimIdle)
      headPosition += m_headSwimOffset;
    else if (m_state == Duck)
      headPosition += m_headDuckOffset;
    else if (m_state == Sit)
      headPosition += m_headSitOffset;
    else if (m_state == Lay)
      headPosition += m_headLayOffset;

    return (m_mouthOffset + headPosition).rotate(m_rotation);
  }
}

Vec2F Humanoid::feetOffset() const {
  return m_feetOffset.rotate(m_rotation);
}

Vec2F Humanoid::headArmorOffset() const {
  Vec2F headPosition(0, getBobYOffset());
  if (m_state == Idle)
    headPosition += m_identity.personality.headOffset / TilePixels;
  else if (m_state == Run)
    headPosition += m_headRunOffset;
  else if (m_state == Swim || m_state == SwimIdle)
    headPosition += m_headSwimOffset;
  else if (m_state == Duck)
    headPosition += m_headDuckOffset;
  else if (m_state == Sit)
    headPosition += m_headSitOffset;
  else if (m_state == Lay)
    headPosition += m_headLayOffset;

  return (m_headArmorOffset + headPosition).rotate(m_rotation);
}

Vec2F Humanoid::chestArmorOffset() const {
  Vec2F position(0, getBobYOffset());
  return (m_chestArmorOffset + position).rotate(m_rotation);
}

Vec2F Humanoid::legsArmorOffset() const {
  return m_legsArmorOffset.rotate(m_rotation);
}

Vec2F Humanoid::backArmorOffset() const {
  Vec2F position(0, getBobYOffset());
  return (m_backArmorOffset + position).rotate(m_rotation);
}

String Humanoid::defaultDeathParticles() const {
  return m_defaultDeathParticles;
}

List<Particle> Humanoid::particles(String const& name) const {
  auto particleDatabase = Root::singleton().particleDatabase();
  List<Particle> res;
  Json particles = m_particleEmitters.get(name).get("particles", {});
  for (auto& particle : particles.toArray()) {
    auto particleSpec = particle.get("particle", {});
    res.push_back(particleDatabase->particle(particleSpec));
  }

  return res;
}

Json const& Humanoid::defaultMovementParameters() const {
  return m_defaultMovementParameters;
}

pair<Vec2F, Directives> Humanoid::extractScaleFromDirectives(Directives const& directives) {
  if (!directives)
    return make_pair(Vec2F::filled(1.f), Directives());

  List<Directives::Entry const*> entries;
  size_t toReserve = 0;
  Maybe<Vec2F> scale;

  for (auto& entry : directives->entries) {
    auto string = entry.string(*directives);
    const ScaleImageOperation* op = nullptr;
    if (string.beginsWith("scalenearest") && string.utf8().find("skip") == NPos)
      op = entry.loadOperation(*directives).ptr<ScaleImageOperation>();

    if (op)
      scale = scale.value(Vec2F::filled(1.f)).piecewiseMultiply(op->scale);
    else {
      entries.emplace_back(&entry);
      toReserve += string.utf8Size() + 1;
    }
  }

  if (!scale)
    return make_pair(Vec2F::filled(1.f), directives);

  String mergedDirectives;
  mergedDirectives.reserve(toReserve);
  for (auto& entry : entries) {
    if (entry->begin > 0)
      mergedDirectives.append('?');
    mergedDirectives.append(entry->string(*directives));
  }

  return make_pair(*scale, Directives(mergedDirectives));
}

}
