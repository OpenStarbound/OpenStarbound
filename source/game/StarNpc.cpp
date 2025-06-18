#include "StarNpc.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarWorld.hpp"
#include "StarRoot.hpp"
#include "StarSongbook.hpp"
#include "StarSongbookLuaBindings.hpp"
#include "StarDamageManager.hpp"
#include "StarDamageDatabase.hpp"
#include "StarLogging.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarBehaviorLuaBindings.hpp"
#include "StarEmoteProcessor.hpp"
#include "StarTreasure.hpp"
#include "StarEncode.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarAssets.hpp"
#include "StarEntityRendering.hpp"
#include "StarTime.hpp"
#include "StarArmors.hpp"
#include "StarFireableItem.hpp"
#include "StarStatusController.hpp"
#include "StarJsonExtra.hpp"
#include "StarDanceDatabase.hpp"
#include "StarSpeciesDatabase.hpp"

namespace Star {

Npc::Npc(NpcVariant const& npcVariant)
  : m_humanoid(npcVariant.humanoidConfig) {
  m_disableWornArmor.set(npcVariant.disableWornArmor);

  m_emoteState = HumanoidEmote::Idle;
  m_chatMessageUpdated = false;

  m_statusText.set({});
  m_displayNametag.set(false);

  auto assets = Root::singleton().assets();

  m_emoteCooldownTimer = GameTimer(assets->json("/npcs/npc.config:emoteCooldown").toFloat());
  m_danceCooldownTimer = GameTimer(0.0f);
  m_blinkInterval = jsonToVec2F(assets->json("/npcs/npc.config:blinkInterval"));

  m_questIndicatorOffset = jsonToVec2F(assets->json("/quests/quests.config:defaultIndicatorOffset"));

  if (npcVariant.overrides)
    m_clientEntityMode = ClientEntityModeNames.getLeft(npcVariant.overrides.getString("clientEntityMode", "ClientSlaveOnly"));

  m_isInteractive.set(false);

  m_shifting.set(false);
  m_damageOnTouch.set(false);

  m_dropPools.set(npcVariant.dropPools);
  m_npcVariant = npcVariant;

  setTeam(EntityDamageTeam(m_npcVariant.damageTeamType, m_npcVariant.damageTeam));

  m_scriptComponent.setScripts(m_npcVariant.scripts);
  m_scriptComponent.setUpdateDelta(m_npcVariant.initialScriptDelta);
  auto movementParameters = ActorMovementParameters(m_npcVariant.movementParameters);
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet({"npc"});
  m_movementController = make_shared<ActorMovementController>(movementParameters);
  m_humanoid.setIdentity(m_npcVariant.humanoidIdentity);
  m_deathParticleBurst.set(m_humanoid.defaultDeathParticles());

  m_statusController = make_shared<StatusController>(m_npcVariant.statusControllerSettings);
  m_statusController->setPersistentEffects("innate", m_npcVariant.innateStatusEffects);
  auto speciesDefinition = Root::singleton().speciesDatabase()->species(species());
  m_statusController->setPersistentEffects("species", speciesDefinition->statusEffects());
  m_statusController->setStatusProperty("species", species());
  if (!m_statusController->statusProperty("effectDirectives"))
    m_statusController->setStatusProperty("effectDirectives", speciesDefinition->effectDirectives());

  m_songbook = make_shared<Songbook>(species());

  m_effectEmitter = make_shared<EffectEmitter>();

  m_hitDamageNotificationLimiter = 0;
  m_hitDamageNotificationLimit = assets->json("/npcs/npc.config:hitDamageNotificationLimit").toInt();

  m_blinkCooldownTimer = GameTimer();

  m_armor = make_shared<ArmorWearer>();
  m_tools = make_shared<ToolUser>();

  m_aggressive.set(false);

  setPersistent(m_npcVariant.persistent);
  setKeepAlive(m_npcVariant.keepAlive);

  setupNetStates();
}

Npc::Npc(NpcVariant const& npcVariant, Json const& diskStore) : Npc(npcVariant) {
  m_movementController->loadState(diskStore.get("movementController"));
  m_statusController->diskLoad(diskStore.get("statusController"));
  auto aimPosition = jsonToVec2F(diskStore.get("aimPosition"));
  m_xAimPosition.set(aimPosition[0]);
  m_yAimPosition.set(aimPosition[1]);
  m_humanoid.setState(Humanoid::StateNames.getLeft(diskStore.getString("humanoidState")));
  m_humanoid.setEmoteState(HumanoidEmoteNames.getLeft(diskStore.getString("humanoidEmoteState")));
  m_isInteractive.set(diskStore.getBool("isInteractive"));
  m_shifting.set(diskStore.getBool("shifting"));
  m_damageOnTouch.set(diskStore.getBool("damageOnTouch", false));

  m_effectEmitter->fromJson(diskStore.get("effectEmitter"));

  m_armor->diskLoad(diskStore.get("armor"));
  m_tools->diskLoad(diskStore.get("tools"));

  m_disableWornArmor.set(diskStore.getBool("disableWornArmor"));

  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage"));

  setUniqueId(diskStore.optString("uniqueId"));
  if (diskStore.contains("team"))
    setTeam(EntityDamageTeam(diskStore.get("team")));

  m_deathParticleBurst.set(diskStore.optString("deathParticleBurst"));

  m_dropPools.set(diskStore.getArray("dropPools").transformed(mem_fn(&Json::toString)));

  m_blinkCooldownTimer = GameTimer();

  m_aggressive.set(diskStore.getBool("aggressive"));
}

Json Npc::diskStore() const {
  return JsonObject{
    {"npcVariant", Root::singleton().npcDatabase()->writeNpcVariantToJson(m_npcVariant)},
    {"movementController", m_movementController->storeState()},
    {"statusController", m_statusController->diskStore()},
    {"armor", m_armor->diskStore()},
    {"tools", m_tools->diskStore()},
    {"aimPosition", jsonFromVec2F({m_xAimPosition.get(), m_yAimPosition.get()})},
    {"humanoidState", Humanoid::StateNames.getRight(m_humanoid.state())},
    {"humanoidEmoteState", HumanoidEmoteNames.getRight(m_humanoid.emoteState())},
    {"isInteractive", m_isInteractive.get()},
    {"shifting", m_shifting.get()},
    {"damageOnTouch", m_damageOnTouch.get()},
    {"effectEmitter", m_effectEmitter->toJson()},
    {"disableWornArmor", m_disableWornArmor.get()},
    {"scriptStorage", m_scriptComponent.getScriptStorage()},
    {"uniqueId", jsonFromMaybe(uniqueId())},
    {"team", getTeam().toJson()},
    {"deathParticleBurst", jsonFromMaybe(m_deathParticleBurst.get())},
    {"dropPools", m_dropPools.get().transformed(construct<Json>())},
    {"aggressive", m_aggressive.get()}
  };
}

ByteArray Npc::netStore(NetCompatibilityRules rules) {
  return Root::singleton().npcDatabase()->writeNpcVariant(m_npcVariant, rules);
}

EntityType Npc::entityType() const {
  return EntityType::Npc;
}

ClientEntityMode Npc::clientEntityMode() const {
  return m_clientEntityMode;
}

void Npc::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  m_movementController->init(world);
  m_movementController->setIgnorePhysicsEntities({entityId});
  m_statusController->init(this, m_movementController.get());
  m_tools->init(this);

  m_armor->setupHumanoidClothingDrawables(m_humanoid, false);

  if (isMaster()) {
    m_movementController->resetAnchorState();

    auto itemDatabase = Root::singleton().itemDatabase();
    for (auto const& item : m_npcVariant.items)
      setItemSlot(item.first, item.second);
    m_scriptComponent.addCallbacks("npc", makeNpcCallbacks());
    m_scriptComponent.addCallbacks("config",
        LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def)
            { return m_npcVariant.scriptConfig.query(name, def); }));
    m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptComponent.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_statusController.get()));
    m_scriptComponent.addCallbacks("behavior", LuaBindings::makeBehaviorCallbacks(&m_behaviors));
    m_scriptComponent.addCallbacks("songbook", LuaBindings::makeSongbookCallbacks(m_songbook.get()));
    m_scriptComponent.addActorMovementCallbacks(m_movementController.get());
    m_scriptComponent.init(world);
  }
}

void Npc::uninit() {
  if (isMaster()) {
    m_movementController->resetAnchorState();
    m_scriptComponent.uninit();
    m_scriptComponent.removeCallbacks("npc");
    m_scriptComponent.removeCallbacks("config");
    m_scriptComponent.removeCallbacks("entity");
    m_scriptComponent.removeCallbacks("status");
    m_scriptComponent.removeCallbacks("behavior");
    m_scriptComponent.removeCallbacks("songbook");
    m_scriptComponent.removeActorMovementCallbacks();
  }
  m_tools->uninit();
  m_statusController->uninit();
  m_movementController->uninit();
  Entity::uninit();
}

void Npc::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Npc::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

Vec2F Npc::position() const {
  return m_movementController->position();
}

RectF Npc::metaBoundBox() const {
  return RectF(-4, -4, 4, 4);
}

Vec2F Npc::mouthOffset(bool ignoreAdjustments) const {
  return Vec2F{m_humanoid.mouthOffset(ignoreAdjustments)[0] * numericalDirection(m_humanoid.facingDirection()),
      m_humanoid.mouthOffset(ignoreAdjustments)[1]};
}

Vec2F Npc::feetOffset() const {
  return {m_humanoid.feetOffset()[0] * numericalDirection(m_humanoid.facingDirection()), m_humanoid.feetOffset()[1]};
}

Vec2F Npc::headArmorOffset() const {
  return {m_humanoid.headArmorOffset()[0] * numericalDirection(m_humanoid.facingDirection()), m_humanoid.headArmorOffset()[1]};
}

Vec2F Npc::chestArmorOffset() const {
  return {m_humanoid.chestArmorOffset()[0] * numericalDirection(m_humanoid.facingDirection()), m_humanoid.chestArmorOffset()[1]};
}

Vec2F Npc::backArmorOffset() const {
  return {m_humanoid.backArmorOffset()[0] * numericalDirection(m_humanoid.facingDirection()), m_humanoid.backArmorOffset()[1]};
}

Vec2F Npc::legsArmorOffset() const {
  return {m_humanoid.legsArmorOffset()[0] * numericalDirection(m_humanoid.facingDirection()), m_humanoid.legsArmorOffset()[1]};
}

RectF Npc::collisionArea() const {
  return m_movementController->collisionPoly().boundBox();
}

pair<ByteArray, uint64_t> Npc::writeNetState(uint64_t fromVersion, NetCompatibilityRules rules) {
  // client-side npcs error nearby vanilla NPC scripts because callScriptedEntity
  // for now, scrungle the collision poly to avoid their queries. hacky :(
  if (m_npcVariant.overrides && m_npcVariant.overrides.getBool("overrideNetPoly", false)) {
    if (auto mode = entityMode()) {
      if (*mode == EntityMode::Master && connectionForEntity(entityId()) != ServerConnectionId) {
        PolyF poly = m_movementController->collisionPoly();
        m_movementController->setCollisionPoly({ { 0.0f, -3.402823466e+38F }});
        auto result = m_netGroup.writeNetState(fromVersion, rules);
        m_movementController->setCollisionPoly(poly);
        return result;
      }
    }
  }

  return m_netGroup.writeNetState(fromVersion, rules);
}

void Npc::readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules) {
  m_netGroup.readNetState(data, interpolationTime, rules);
}

String Npc::description() const {
  return "Some funny looking person";
}

String Npc::species() const {
  return m_humanoid.identity().species;
}

Gender Npc::gender() const {
  return m_humanoid.identity().gender;
}

String Npc::npcType() const {
  return m_npcVariant.typeName;
}

Json Npc::scriptConfigParameter(String const& parameterName, Json const& defaultValue) const {
  return m_npcVariant.scriptConfig.query(parameterName, defaultValue);
}

Maybe<HitType> Npc::queryHit(DamageSource const& source) const {
  if (!inWorld() || !m_statusController->resourcePositive("health") || m_statusController->statPositive("invulnerable"))
    return {};

  if (m_tools->queryShieldHit(source))
    return HitType::ShieldHit;

  if (source.intersectsWithPoly(world()->geometry(), m_movementController->collisionBody()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Npc::hitPoly() const {
  return m_movementController->collisionBody();
}

List<DamageNotification> Npc::applyDamage(DamageRequest const& damage) {
  if (!inWorld())
    return {};

  auto notifications = m_statusController->applyDamageRequest(damage);

  float totalDamage = 0.0f;
  for (auto const& notification : notifications)
    totalDamage += notification.healthLost;

  if (totalDamage > 0 && m_hitDamageNotificationLimiter < m_hitDamageNotificationLimit) {
    m_scriptComponent.invoke("damage", JsonObject{
        {"sourceId", damage.sourceEntityId},
        {"damage", totalDamage},
        {"sourceDamage", damage.damage},
        {"sourceKind", damage.damageSourceKind}
      });
    m_hitDamageNotificationLimiter++;
  }

  return notifications;
}

List<DamageNotification> Npc::selfDamageNotifications() {
  return m_statusController->pullSelfDamageNotifications();
}

bool Npc::shouldDestroy() const {
  if (auto res = m_scriptComponent.invoke<bool>("shouldDie"))
    return *res;
  else if (!m_statusController->resourcePositive("health") || m_scriptComponent.error())
    return true;
  else
    return false;
}

void Npc::destroy(RenderCallback* renderCallback) {
  m_scriptComponent.invoke("die");

  if (isMaster() && !m_dropPools.get().empty()) {
    auto treasureDatabase = Root::singleton().treasureDatabase();
    for (auto const& treasureItem :
        treasureDatabase->createTreasure(staticRandomFrom(m_dropPools.get(), m_npcVariant.seed), m_npcVariant.level))
      world()->addEntity(ItemDrop::createRandomizedDrop(treasureItem, position()));
  }

  if (renderCallback && m_deathParticleBurst.get())
    renderCallback->addParticles(m_humanoid.particles(*m_deathParticleBurst.get()), position());

  m_songbook->stop();
}

void Npc::damagedOther(DamageNotification const& damage) {
  if (inWorld() && isMaster())
    m_statusController->damagedOther(damage);
}

void Npc::update(float dt, uint64_t) {
  if (!inWorld())
    return;

  m_movementController->setTimestep(dt);

  if (isMaster()) {
    m_scriptComponent.update(m_scriptComponent.updateDt(dt));

    if (inConflictingLoungeAnchor())
      m_movementController->resetAnchorState();

    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
      if (loungeAnchor->emote)
        requestEmote(*loungeAnchor->emote);
      m_statusController->setPersistentEffects("lounging", loungeAnchor->statusEffects);
      m_effectEmitter->addEffectSources("normal", loungeAnchor->effectEmitters);
      switch (loungeAnchor->orientation) {
        case LoungeOrientation::Sit:
          m_humanoid.setState(Humanoid::Sit);
          break;
        case LoungeOrientation::Lay:
          m_humanoid.setState(Humanoid::Lay);
          break;
        case LoungeOrientation::Stand:
          m_humanoid.setState(Humanoid::Idle); // currently the same as "standard"
          // idle, but this is lounging idle
          break;
        default:
          m_humanoid.setState(Humanoid::Idle);
      }
    } else {
      m_statusController->setPersistentEffects("lounging", {});
    }

    m_armor->effects(*m_effectEmitter);
    m_tools->effects(*m_effectEmitter);

    if (!m_disableWornArmor.get())
      m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
    m_statusController->setPersistentEffects("tools", m_tools->statusEffects());

    m_movementController->tickMaster(dt);
    m_statusController->tickMaster(dt);

    tickShared(dt);

    if (!is<LoungeAnchor>(m_movementController->entityAnchor())) {
      if (m_movementController->groundMovement()) {
        if (m_movementController->running())
          m_humanoid.setState(Humanoid::Run);
        else if (m_movementController->walking())
          m_humanoid.setState(Humanoid::Walk);
        else if (m_movementController->crouching())
          m_humanoid.setState(Humanoid::Duck);
        else
          m_humanoid.setState(Humanoid::Idle);
      } else if (m_movementController->liquidMovement()) {
        if (abs(m_movementController->xVelocity()) > 0)
          m_humanoid.setState(Humanoid::Swim);
        else
          m_humanoid.setState(Humanoid::SwimIdle);
      } else {
        if (m_movementController->yVelocity() > 0)
          m_humanoid.setState(Humanoid::Jump);
        else
          m_humanoid.setState(Humanoid::Fall);
      }
    }

    if (m_emoteCooldownTimer.tick(dt))
      m_emoteState = HumanoidEmote::Idle;
    if (m_danceCooldownTimer.tick(dt))
      m_dance = {};

    if (m_chatMessageUpdated) {
      auto state = Root::singleton().emoteProcessor()->detectEmotes(m_chatMessage.get());
      if (state != HumanoidEmote::Idle)
        addEmote(state);
      m_chatMessageUpdated = false;
    }

    if (m_blinkCooldownTimer.tick(dt)) {
      m_blinkCooldownTimer = GameTimer(Random::randf(m_blinkInterval[0], m_blinkInterval[1]));
      if (m_emoteState == HumanoidEmote::Idle)
        addEmote(HumanoidEmote::Blink);
    }

    m_humanoid.setEmoteState(m_emoteState);
    m_humanoid.setDance(m_dance);

  } else {
    m_netGroup.tickNetInterpolation(dt);
    m_movementController->tickSlave(dt);
    m_statusController->tickSlave(dt);

    tickShared(dt);
  }

  if (world()->isClient())
    SpatialLogger::logPoly("world", m_movementController->collisionBody(), {0, 255, 0, 255});
}

void Npc::render(RenderCallback* renderCallback) {
  EntityRenderLayer renderLayer = RenderLayerNpc;
  if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor()))
    renderLayer = loungeAnchor->loungeRenderLayer;

  m_tools->setupHumanoidHandItemDrawables(m_humanoid);

  DirectivesGroup humanoidDirectives;
  Vec2F scale = Vec2F::filled(1.f);
  for (auto& directives : m_statusController->parentDirectives().list()) {
    auto result = Humanoid::extractScaleFromDirectives(directives);
    scale = scale.piecewiseMultiply(result.first);
    humanoidDirectives.append(result.second);
  }
  m_humanoid.setScale(scale);

  for (auto& drawable : m_humanoid.render()) {
    drawable.translate(position());
    if (drawable.isImage())
      drawable.imagePart().addDirectivesGroup(humanoidDirectives, true);
    renderCallback->addDrawable(std::move(drawable), renderLayer);
  }

  renderCallback->addDrawables(m_statusController->drawables(), renderLayer);
  renderCallback->addParticles(m_statusController->pullNewParticles());
  renderCallback->addAudios(m_statusController->pullNewAudios());

  renderCallback->addParticles(m_npcVariant.splashConfig.doSplash(position(), m_movementController->velocity(), world()));

  m_tools->render(renderCallback, inToolRange(), m_shifting.get(), renderLayer);

  renderCallback->addDrawables(m_tools->renderObjectPreviews(aimPosition(), walkingDirection(), inToolRange(), favoriteColor()), renderLayer);

  m_effectEmitter->render(renderCallback);
  m_songbook->render(renderCallback);
}

void Npc::renderLightSources(RenderCallback* renderCallback) {
  renderCallback->addLightSources(lightSources());
}

void Npc::setPosition(Vec2F const& pos) {
  m_movementController->setPosition(pos);
}

float Npc::maxHealth() const {
  return *m_statusController->resourceMax("health");
}

float Npc::health() const {
  return m_statusController->resource("health");
}

DamageBarType Npc::damageBar() const {
  return DamageBarType::Default;
}

List<Drawable> Npc::portrait(PortraitMode mode) const {
  return m_humanoid.renderPortrait(mode);
}

String Npc::name() const {
  return m_npcVariant.humanoidIdentity.name;
}

Maybe<String> Npc::statusText() const {
  return m_statusText.get();
}

bool Npc::displayNametag() const {
  return m_displayNametag.get();
}

Vec3B Npc::nametagColor() const {
  return m_npcVariant.nametagColor;
}

Vec2F Npc::nametagOrigin() const {
  return mouthPosition(false);
}

String Npc::nametag() const {
  return name();
}

bool Npc::aggressive() const {
  return m_aggressive.get();
}

Maybe<LuaValue> Npc::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Npc::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

Vec2F Npc::getAbsolutePosition(Vec2F relativePosition) const {
  if (m_humanoid.facingDirection() == Direction::Left)
    relativePosition[0] *= -1;
  return m_movementController->position() + relativePosition;
}

void Npc::tickShared(float dt) {
  if (m_hitDamageNotificationLimiter)
    m_hitDamageNotificationLimiter--;

  m_songbook->update(*entityMode(), world());

  m_effectEmitter->setSourcePosition("normal", position());
  m_effectEmitter->setSourcePosition("mouth", position() + mouthOffset());
  m_effectEmitter->setSourcePosition("feet", position() + feetOffset());
  m_effectEmitter->setSourcePosition("headArmor", headArmorOffset() + position());
  m_effectEmitter->setSourcePosition("chestArmor", chestArmorOffset() + position());
  m_effectEmitter->setSourcePosition("legsArmor", legsArmorOffset() + position());
  m_effectEmitter->setSourcePosition("backArmor", backArmorOffset() + position());

  m_effectEmitter->setDirection(m_humanoid.facingDirection());
  m_effectEmitter->tick(dt, *entityMode());

  m_humanoid.setMovingBackwards(m_movementController->movingDirection() != m_movementController->facingDirection());
  m_humanoid.setFacingDirection(m_movementController->facingDirection());
  m_humanoid.setRotation(m_movementController->rotation());

  ActorMovementModifiers firingModifiers;
  if (auto fireableMain = as<FireableItem>(handItem(ToolHand::Primary))) {
    if (fireableMain->firing()) {
      if (fireableMain->stopWhileFiring())
        firingModifiers.movementSuppressed = true;
      else if (fireableMain->walkWhileFiring())
        firingModifiers.runningSuppressed = true;
    }
  }

  if (auto fireableAlt = as<FireableItem>(handItem(ToolHand::Alt))) {
    if (fireableAlt->firing()) {
      if (fireableAlt->stopWhileFiring())
        firingModifiers.movementSuppressed = true;
      else if (fireableAlt->walkWhileFiring())
        firingModifiers.runningSuppressed = true;
    }
  }

  m_armor->setupHumanoidClothingDrawables(m_humanoid, false);

  m_tools->suppressItems(!canUseTool());
  m_tools->tick(dt, m_shifting.get(), {});

  if (auto overrideDirection = m_tools->setupHumanoidHandItems(m_humanoid, position(), aimPosition()))
    m_movementController->controlFace(*overrideDirection);

  m_humanoid.animate(dt);
}

LuaCallbacks Npc::makeNpcCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("toAbsolutePosition", [this](Vec2F const& p) { return getAbsolutePosition(p); });

  callbacks.registerCallback("species", [this]() { return m_npcVariant.species; });

  callbacks.registerCallback("gender", [this]() { return GenderNames.getRight(m_humanoid.identity().gender); });

  callbacks.registerCallback("humanoidIdentity", [this]() { return m_humanoid.identity().toJson(); });

  callbacks.registerCallback("npcType", [this]() { return npcType(); });

  callbacks.registerCallback("seed", [this]() { return m_npcVariant.seed; });

  callbacks.registerCallback("level", [this]() { return m_npcVariant.level; });

  callbacks.registerCallback("dropPools", [this]() { return m_dropPools.get(); });

  callbacks.registerCallback("setDropPools", [this](StringList const& dropPools) { m_dropPools.set(dropPools); });

  callbacks.registerCallback("energy", [this]() { return m_statusController->resource("energy"); });

  callbacks.registerCallback("maxEnergy", [this]() { return m_statusController->resourceMax("energy"); });

  callbacks.registerCallback("say", [this](String line, Maybe<StringMap<String>> const& tags, Json const& config) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line, config);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("sayPortrait", [this](String line, String portrait, Maybe<StringMap<String>> const& tags, Json const& config) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line, config, portrait);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("emote", [this](String const& arg1) { addEmote(HumanoidEmoteNames.getLeft(arg1)); });

  callbacks.registerCallback("dance", [this](Maybe<String> const& danceName) { setDance(danceName); });

  callbacks.registerCallback("setInteractive", [this](bool interactive) { m_isInteractive.set(interactive); });

  callbacks.registerCallback("setLounging", [this](EntityId loungeableEntityId, Maybe<size_t> maybeAnchorIndex) {
      size_t anchorIndex = maybeAnchorIndex.value(0);
      auto loungeableEntity = world()->get<LoungeableEntity>(loungeableEntityId);
      if (!loungeableEntity || anchorIndex >= loungeableEntity->anchorCount()
          || !loungeableEntity->entitiesLoungingIn(anchorIndex).empty()
          || !loungeableEntity->loungeAnchor(anchorIndex))
        return false;

      m_movementController->setAnchorState({loungeableEntityId, anchorIndex});
      return true;
    });

  callbacks.registerCallback("resetLounging", [this]() { m_movementController->resetAnchorState(); });

  callbacks.registerCallback("isLounging", [this]() { return is<LoungeAnchor>(m_movementController->entityAnchor()); });

  callbacks.registerCallback("loungingIn", [this]() -> Maybe<EntityId> {
      auto loungingState = loungingIn();
      if (loungingState)
        return loungingState.value().entityId;
      else
        return {};
    });

  callbacks.registerCallback("setOfferedQuests", [this](Maybe<JsonArray> const& offeredQuests) {
      m_offeredQuests.set(offeredQuests.value().transformed(&QuestArcDescriptor::fromJson));
    });

  callbacks.registerCallback("setTurnInQuests", [this](Maybe<StringList> const& turnInQuests) {
      m_turnInQuests.set(StringSet::from(turnInQuests.value()));
    });

  callbacks.registerCallback("setItemSlot", [this](String const& slot, Json const& itemDescriptor) -> Json {
      return setItemSlot(slot, ItemDescriptor(itemDescriptor));
    });

  callbacks.registerCallback("getItemSlot", [this](String const& entry) -> Json {
      if (entry.equalsIgnoreCase("head"))
        return m_armor->headItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("headCosmetic"))
        return m_armor->headCosmeticItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("chest"))
        return m_armor->chestItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("chestCosmetic"))
        return m_armor->chestCosmeticItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("legs"))
        return m_armor->legsItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("legsCosmetic"))
        return m_armor->legsCosmeticItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("back"))
        return m_armor->backItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("backCosmetic"))
        return m_armor->backCosmeticItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("primary"))
        return m_tools->primaryHandItemDescriptor().toJson();
      else if (entry.equalsIgnoreCase("alt"))
        return m_tools->altHandItemDescriptor().toJson();
      else if (m_npcVariant.items.contains(entry))
        return m_npcVariant.items.get(entry).toJson();

      return {};
    });

  callbacks.registerCallback("disableWornArmor", [this](bool disable) { m_disableWornArmor.set(disable); });

  callbacks.registerCallback("beginPrimaryFire", [this]() { m_tools->beginPrimaryFire(); });
  callbacks.registerCallback("beginAltFire", [this]() { m_tools->beginAltFire(); });
  callbacks.registerCallback("endPrimaryFire", [this]() { m_tools->endPrimaryFire(); });
  callbacks.registerCallback("endAltFire", [this]() { m_tools->endAltFire(); });
  callbacks.registerCallback("setShifting", [this](bool shifting) { m_shifting.set(shifting); });
  callbacks.registerCallback("setDamageOnTouch", [this](bool damageOnTouch) { m_damageOnTouch.set(damageOnTouch); });

  callbacks.registerCallback("aimPosition", [this]() { return jsonFromVec2F(aimPosition()); });

  callbacks.registerCallback("setAimPosition", [this](Vec2F const& pos) {
      auto aimPosition = world()->geometry().diff(pos, position());
      m_xAimPosition.set(aimPosition[0]);
      m_yAimPosition.set(aimPosition[1]);
    });

  callbacks.registerCallback("setDeathParticleBurst", [this](Maybe<String> const& deathParticleBurst) {
      m_deathParticleBurst.set(deathParticleBurst);
    });

  callbacks.registerCallback("setStatusText", [this](Maybe<String> const& status) { m_statusText.set(status); });
  callbacks.registerCallback("setDisplayNametag", [this](bool display) { m_displayNametag.set(display); });

  callbacks.registerCallback("setPersistent", [this](bool persistent) { setPersistent(persistent); });

  callbacks.registerCallback("setKeepAlive", [this](bool keepAlive) { setKeepAlive(keepAlive); });

  callbacks.registerCallback("setDamageTeam", [this](Json const& team) { setTeam(EntityDamageTeam(team)); });

  callbacks.registerCallback("setAggressive", [this](bool aggressive) { m_aggressive.set(aggressive); });

  callbacks.registerCallback("setUniqueId", [this](Maybe<String> uniqueId) { setUniqueId(uniqueId); });

  return callbacks;
}

void Npc::setupNetStates() {
  m_netGroup.addNetElement(&m_xAimPosition);
  m_netGroup.addNetElement(&m_yAimPosition);

  m_xAimPosition.setFixedPointBase(0.0625);
  m_yAimPosition.setFixedPointBase(0.0625);
  m_xAimPosition.setInterpolator(lerp<float, float>);
  m_yAimPosition.setInterpolator(lerp<float, float>);

  m_netGroup.addNetElement(&m_uniqueIdNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_humanoidStateNetState);
  m_netGroup.addNetElement(&m_humanoidEmoteStateNetState);
  m_netGroup.addNetElement(&m_humanoidDanceNetState);

  m_netGroup.addNetElement(&m_newChatMessageEvent);
  m_netGroup.addNetElement(&m_chatMessage);
  m_netGroup.addNetElement(&m_chatPortrait);
  m_netGroup.addNetElement(&m_chatConfig);

  m_netGroup.addNetElement(&m_statusText);
  m_netGroup.addNetElement(&m_displayNametag);

  m_netGroup.addNetElement(&m_isInteractive);

  m_netGroup.addNetElement(&m_offeredQuests);
  m_netGroup.addNetElement(&m_turnInQuests);

  m_netGroup.addNetElement(&m_shifting);
  m_netGroup.addNetElement(&m_damageOnTouch);

  m_netGroup.addNetElement(&m_disableWornArmor);

  m_netGroup.addNetElement(&m_deathParticleBurst);

  m_netGroup.addNetElement(&m_dropPools);
  m_netGroup.addNetElement(&m_aggressive);

  m_netGroup.addNetElement(m_movementController.get());
  m_netGroup.addNetElement(m_effectEmitter.get());
  m_netGroup.addNetElement(m_statusController.get());
  m_netGroup.addNetElement(m_armor.get());
  m_netGroup.addNetElement(m_tools.get());
  m_songbook->setCompatibilityVersion(6);
  m_netGroup.addNetElement(m_songbook.get());

  m_netGroup.setNeedsStoreCallback(bind(&Npc::setNetStates, this));
  m_netGroup.setNeedsLoadCallback(bind(&Npc::getNetStates, this, _1));
}

void Npc::setNetStates() {
  m_uniqueIdNetState.set(uniqueId());
  m_teamNetState.set(getTeam());
  m_humanoidStateNetState.set(m_humanoid.state());
  m_humanoidEmoteStateNetState.set(m_humanoid.emoteState());
  m_humanoidDanceNetState.set(m_humanoid.dance());
}

void Npc::getNetStates(bool initial) {
  setUniqueId(m_uniqueIdNetState.get());
  setTeam(m_teamNetState.get());
  m_humanoid.setState(m_humanoidStateNetState.get());
  m_humanoid.setEmoteState(m_humanoidEmoteStateNetState.get());
  m_humanoid.setDance(m_humanoidDanceNetState.get());

  if (m_newChatMessageEvent.pullOccurred() && !initial) {
    m_chatMessageUpdated = true;
    if (m_chatPortrait.get().empty())
      m_pendingChatActions.append(SayChatAction{entityId(), m_chatMessage.get(), mouthPosition(), m_chatConfig.get()});
    else
      m_pendingChatActions.append(PortraitChatAction{
          entityId(), m_chatPortrait.get(), m_chatMessage.get(), mouthPosition(), m_chatConfig.get()});
  }
}

Vec2F Npc::mouthPosition() const {
  return mouthOffset(true) + position();
}

Vec2F Npc::mouthPosition(bool ignoreAdjustments) const {
  return mouthOffset(ignoreAdjustments) + position();
}

List<ChatAction> Npc::pullPendingChatActions() {
  return std::move(m_pendingChatActions);
}

void Npc::addChatMessage(String const& message, Json const& config, String const& portrait) {
  assert(!isSlave());
  m_chatMessage.set(message);
  m_chatPortrait.set(portrait);
  m_chatConfig.set(config);
  m_chatMessageUpdated = true;
  m_newChatMessageEvent.trigger();
  if (portrait.empty())
    m_pendingChatActions.append(SayChatAction{entityId(), message, mouthPosition(), config});
  else
    m_pendingChatActions.append(PortraitChatAction{entityId(), portrait, message, mouthPosition(), config});
}

void Npc::addEmote(HumanoidEmote const& emote) {
  assert(!isSlave());
  m_emoteState = emote;
  m_emoteCooldownTimer.reset();
}

void Npc::setDance(Maybe<String> const& danceName) {
  assert(!isSlave());
  m_dance = danceName;

  if (danceName.isValid()) {
    auto danceDatabase = Root::singleton().danceDatabase();
    DancePtr dance = danceDatabase->getDance(*danceName);
    m_danceCooldownTimer = GameTimer(dance->duration);
  }
}

bool Npc::isInteractive() const {
  return m_isInteractive.get();
}

InteractAction Npc::interact(InteractRequest const& request) {
  auto result = m_scriptComponent.invoke<Json>("interact",
      JsonObject{{"sourceId", request.sourceId}, {"sourcePosition", jsonFromVec2F(request.sourcePosition)}}).value();

  if (result.isNull())
    return {};

  if (result.isType(Json::Type::String))
    return InteractAction(result.toString(), entityId(), Json());

  return InteractAction(result.getString(0), entityId(), result.get(1));
}

RectF Npc::interactiveBoundBox() const {
  return m_movementController->collisionPoly().boundBox();
}

Maybe<EntityAnchorState> Npc::loungingIn() const {
  if (is<LoungeAnchor>(m_movementController->entityAnchor()))
    return m_movementController->anchorState();
  return {};
}

List<QuestArcDescriptor> Npc::offeredQuests() const {
  return m_offeredQuests.get();
}

StringSet Npc::turnInQuests() const {
  return m_turnInQuests.get();
}

Vec2F Npc::questIndicatorPosition() const {
  Vec2F pos = position() + m_questIndicatorOffset;
  pos[1] += interactiveBoundBox().yMax();
  return pos;
}

bool Npc::setItemSlot(String const& slot, ItemDescriptor itemDescriptor) {
  auto item = Root::singleton().itemDatabase()->item(ItemDescriptor(itemDescriptor), m_npcVariant.level, m_npcVariant.seed);

  if (slot.equalsIgnoreCase("head"))
    m_armor->setHeadItem(as<HeadArmor>(item));
  else if (slot.equalsIgnoreCase("headCosmetic"))
    m_armor->setHeadCosmeticItem(as<HeadArmor>(item));
  else if (slot.equalsIgnoreCase("chest"))
    m_armor->setChestItem(as<ChestArmor>(item));
  else if (slot.equalsIgnoreCase("chestCosmetic"))
    m_armor->setChestCosmeticItem(as<ChestArmor>(item));
  else if (slot.equalsIgnoreCase("legs"))
    m_armor->setLegsItem(as<LegsArmor>(item));
  else if (slot.equalsIgnoreCase("legsCosmetic"))
    m_armor->setLegsCosmeticItem(as<LegsArmor>(item));
  else if (slot.equalsIgnoreCase("back"))
    m_armor->setBackItem(as<BackArmor>(item));
  else if (slot.equalsIgnoreCase("backCosmetic"))
    m_armor->setBackCosmeticItem(as<BackArmor>(item));
  else if (slot.equalsIgnoreCase("primary"))
    m_tools->setItems(item, m_tools->altHandItem());
  else if (slot.equalsIgnoreCase("alt"))
    m_tools->setItems(m_tools->primaryHandItem(), item);
  else
    return false;

  return true;
}

bool Npc::canUseTool() const {
  return !shouldDestroy() && !loungingIn();
}

void Npc::disableWornArmor(bool disable) {
  m_disableWornArmor.set(disable);
}

List<LightSource> Npc::lightSources() const {
  List<LightSource> lights;
  lights.appendAll(m_tools->lightSources());
  lights.appendAll(m_statusController->lightSources());
  return lights;
}

Maybe<Json> Npc::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  Maybe<Json> result = m_scriptComponent.handleMessage(message, world()->connection() == sendingConnection, args);
  if (!result)
    result = m_statusController->receiveMessage(message, world()->connection() == sendingConnection, args);
  return result;
}

Vec2F Npc::armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const {
  return m_tools->armPosition(m_humanoid, hand, facingDirection, armAngle, offset);
}

Vec2F Npc::handOffset(ToolHand hand, Direction facingDirection) const {
  return m_tools->handOffset(m_humanoid, hand, facingDirection);
}

Vec2F Npc::handPosition(ToolHand hand, Vec2F const& handOffset) const {
  return m_tools->handPosition(hand, m_humanoid, handOffset);
}

ItemPtr Npc::handItem(ToolHand hand) const {
  if (hand == ToolHand::Primary)
    return m_tools->primaryHandItem();
  return m_tools->altHandItem();
}

Vec2F Npc::armAdjustment() const {
  return m_humanoid.armAdjustment();
}

Vec2F Npc::velocity() const {
  return m_movementController->velocity();
}

Vec2F Npc::aimPosition() const {
  return world()->geometry().xwrap(Vec2F(m_xAimPosition.get(), m_yAimPosition.get()) + position());
}

float Npc::interactRadius() const {
  return 9999;
}

Direction Npc::facingDirection() const {
  return m_movementController->facingDirection();
}

Direction Npc::walkingDirection() const {
  return m_movementController->movingDirection();
}

bool Npc::isAdmin() const {
  return false;
}

Color Npc::favoriteColor() const {
  return Color::White;
}

float Npc::beamGunRadius() const {
  return m_tools->beamGunRadius();
}

void Npc::addParticles(List<Particle> const&) {}

void Npc::addSound(String const&, float, float) {}

bool Npc::inToolRange() const {
  return true;
}

bool Npc::inToolRange(Vec2F const&) const {
  return true;
}

void Npc::addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) {
  m_statusController->addEphemeralEffects(statusEffects);
}

ActiveUniqueStatusEffectSummary Npc::activeUniqueStatusEffectSummary() const {
  return m_statusController->activeUniqueStatusEffectSummary();
}

float Npc::powerMultiplier() const {
  return m_statusController->stat("powerMultiplier");
}

bool Npc::fullEnergy() const {
  return *m_statusController->resourcePercentage("energy") >= 1.0;
}

float Npc::energy() const {
  return m_statusController->resource("energy");
}

bool Npc::energyLocked() const {
  return m_statusController->resourceLocked("energy");
}

bool Npc::consumeEnergy(float energy) {
  return m_statusController->overConsumeResource("energy", energy);
}

void Npc::queueUIMessage(String const&) {}

bool Npc::instrumentPlaying() {
  return m_songbook->instrumentPlaying();
}

void Npc::instrumentEquipped(String const& instrumentKind) {
  if (canUseTool())
    m_songbook->keepAlive(instrumentKind, mouthPosition());
}

void Npc::interact(InteractAction const&) {}

void Npc::addEffectEmitters(StringSet const& emitters) {
  m_effectEmitter->addEffectSources("normal", emitters);
}

void Npc::requestEmote(String const& emote) {
  if (!emote.empty()) {
    auto state = HumanoidEmoteNames.getLeft(emote);
    if (state != HumanoidEmote::Idle && (m_emoteState == HumanoidEmote::Idle || m_emoteState == HumanoidEmote::Blink))
      addEmote(state);
  }
}

ActorMovementController* Npc::movementController() {
  return m_movementController.get();
}

StatusController* Npc::statusController() {
  return m_statusController.get();
}

Songbook* Npc::songbook() {
  return m_songbook.get();
}

void Npc::setCameraFocusEntity(Maybe<EntityId> const&) {
  // players only
}

void Npc::playEmote(HumanoidEmote emote) {
  addEmote(emote);
}

List<DamageSource> Npc::damageSources() const {
  auto damageSources = m_tools->damageSources();

  if (m_damageOnTouch.get() && !m_npcVariant.touchDamageConfig.isNull()) {
    Json config = m_npcVariant.touchDamageConfig;
    if (!config.contains("poly") && !config.contains("line")) {
      config = config.set("poly", jsonFromPolyF(m_movementController->collisionPoly()));
    }
    DamageSource damageSource(config);
    if (auto damagePoly = damageSource.damageArea.ptr<PolyF>())
      damagePoly->rotate(m_movementController->rotation());
    damageSource.damage *= m_statusController->stat("powerMultiplier");
    damageSources.append(damageSource);
  }

  for (auto& damageSource : damageSources) {
    damageSource.sourceEntityId = entityId();
    damageSource.team = getTeam();
  }

  return damageSources;
}

List<PhysicsForceRegion> Npc::forceRegions() const {
  return m_tools->forceRegions();
}

}
