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
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"

namespace Star {

Npc::Npc(NpcVariant const& npcVariant) {

  m_netHumanoid.addNetElement(make_shared<NetHumanoid>(npcVariant.humanoidIdentity, npcVariant.humanoidParameters, npcVariant.uniqueHumanoidConfig ? npcVariant.humanoidConfig : Json()));
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
  auto movementParameters = ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), m_npcVariant.movementParameters));
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet({"npc"});
  m_movementController = make_shared<ActorMovementController>(movementParameters);
  m_identityUpdated = false;
  m_deathParticleBurst.set(humanoid()->defaultDeathParticles());

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
  humanoid()->setState(Humanoid::StateNames.getLeft(diskStore.getString("humanoidState")));
  humanoid()->setEmoteState(HumanoidEmoteNames.getLeft(diskStore.getString("humanoidEmoteState")));
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
    {"humanoidState", Humanoid::StateNames.getRight(humanoid()->state())},
    {"humanoidEmoteState", HumanoidEmoteNames.getRight(humanoid()->emoteState())},
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

  m_armor->setupHumanoid(*humanoid(), forceNude());

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
    m_scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(humanoid()->networkedAnimator()));
    m_scriptComponent.addActorMovementCallbacks(m_movementController.get());
    m_scriptComponent.init(world);
  }
  if (world->isClient()) {
    m_scriptedAnimator.setScripts(humanoid()->animationScripts());
    m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(humanoid()->networkedAnimator(),
      [this](String const& name, Json const& defaultValue) -> Json {
        return m_scriptedAnimationParameters.value(name, defaultValue);
      }));
    m_scriptedAnimator.addCallbacks("config",
        LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def)
            { return m_npcVariant.scriptConfig.query(name, def); }));
    m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptedAnimator.init(world);
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
    m_scriptComponent.removeCallbacks("animator");
    m_scriptComponent.removeActorMovementCallbacks();
  }
  if (world()->isClient()) {
    m_scriptedAnimator.uninit();
    m_scriptedAnimator.removeCallbacks("animationConfig");
    m_scriptedAnimator.removeCallbacks("config");
    m_scriptedAnimator.removeCallbacks("entity");
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
  return Vec2F{humanoid()->mouthOffset(ignoreAdjustments)[0] * numericalDirection(humanoid()->facingDirection()),
      humanoid()->mouthOffset(ignoreAdjustments)[1]};
}

Vec2F Npc::feetOffset() const {
  return {humanoid()->feetOffset()[0] * numericalDirection(humanoid()->facingDirection()), humanoid()->feetOffset()[1]};
}

Vec2F Npc::headArmorOffset() const {
  return {humanoid()->headArmorOffset()[0] * numericalDirection(humanoid()->facingDirection()), humanoid()->headArmorOffset()[1]};
}

Vec2F Npc::chestArmorOffset() const {
  return {humanoid()->chestArmorOffset()[0] * numericalDirection(humanoid()->facingDirection()), humanoid()->chestArmorOffset()[1]};
}

Vec2F Npc::backArmorOffset() const {
  return {humanoid()->backArmorOffset()[0] * numericalDirection(humanoid()->facingDirection()), humanoid()->backArmorOffset()[1]};
}

Vec2F Npc::legsArmorOffset() const {
  return {humanoid()->legsArmorOffset()[0] * numericalDirection(humanoid()->facingDirection()), humanoid()->legsArmorOffset()[1]};
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
  return m_npcVariant.description.value("Some funny looking person");
}

String Npc::species() const {
  return m_npcVariant.humanoidIdentity.species;
}

Gender Npc::gender() const {
  return m_npcVariant.humanoidIdentity.gender;
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
    renderCallback->addParticles(humanoid()->particles(*m_deathParticleBurst.get()), position());

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
      auto anchorState = m_movementController->anchorState();
      if (auto loungeableEntity = world()->get<LoungeableEntity>(anchorState->entityId))
        for (auto control : m_LoungeControlsHeld)
          loungeableEntity->loungeControl(anchorState->positionIndex, control);

      if (loungeAnchor->emote)
        requestEmote(*loungeAnchor->emote);
      m_statusController->setPersistentEffects("lounging", loungeAnchor->statusEffects, m_movementController->anchorState()->entityId);
      m_effectEmitter->addEffectSources("normal", loungeAnchor->effectEmitters);
      switch (loungeAnchor->orientation) {
        case LoungeOrientation::Sit:
          humanoid()->setState(Humanoid::Sit);
          break;
        case LoungeOrientation::Lay:
          humanoid()->setState(Humanoid::Lay);
          break;
        case LoungeOrientation::Stand:
          humanoid()->setState(Humanoid::Idle); // currently the same as "standard"
          // idle, but this is lounging idle
          break;
        default:
          humanoid()->setState(Humanoid::Idle);
      }
    } else {
      m_statusController->setPersistentEffects("lounging", {});
    }

    m_armor->effects(*m_effectEmitter);
    m_tools->effects(*m_effectEmitter);

    m_statusController->setPersistentEffects("armor", m_armor->statusEffects(m_disableWornArmor.get()));
    m_statusController->setPersistentEffects("tools", m_tools->statusEffects());

    m_movementController->tickMaster(dt);
    m_statusController->tickMaster(dt);

    tickShared(dt);

    if (!is<LoungeAnchor>(m_movementController->entityAnchor())) {
      if (m_movementController->groundMovement()) {
        if (m_movementController->running())
          humanoid()->setState(Humanoid::Run);
        else if (m_movementController->walking())
          humanoid()->setState(Humanoid::Walk);
        else if (m_movementController->crouching())
          humanoid()->setState(Humanoid::Duck);
        else
          humanoid()->setState(Humanoid::Idle);
      } else if (m_movementController->liquidMovement()) {
        if (abs(m_movementController->xVelocity()) > 0)
          humanoid()->setState(Humanoid::Swim);
        else
          humanoid()->setState(Humanoid::SwimIdle);
      } else {
        if (m_movementController->yVelocity() > 0)
          humanoid()->setState(Humanoid::Jump);
        else
          humanoid()->setState(Humanoid::Fall);
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

    humanoid()->setEmoteState(m_emoteState);
    humanoid()->setDance(m_dance);

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
  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor)
    renderLayer = loungeAnchor->loungeRenderLayer;

  if (!loungeAnchor ||(!loungeAnchor->usePartZLevel && !loungeAnchor->hidden) ) {
    renderCallback->addDrawables(drawables(position()), renderLayer);
  }
  renderCallback->addDrawables(m_tools->renderObjectPreviews(aimPosition(), walkingDirection(), inToolRange(), favoriteColor()), renderLayer);

  if (loungeAnchor && loungeAnchor->hidden) {
    m_statusController->pullNewParticles();
    m_npcVariant.splashConfig.doSplash(position(), m_movementController->velocity(), world());
    m_humanoidDynamicTarget.pullNewParticles();
    m_humanoidDynamicTarget.pullNewAudios();
  } else {
    renderCallback->addParticles(m_statusController->pullNewParticles());
    renderCallback->addParticles(m_npcVariant.splashConfig.doSplash(position(), m_movementController->velocity(), world()));
    renderCallback->addParticles(m_humanoidDynamicTarget.pullNewParticles());
    renderCallback->addAudios(m_humanoidDynamicTarget.pullNewAudios());

  }


  renderCallback->addAudios(m_statusController->pullNewAudios());

  m_tools->render(renderCallback, inToolRange(), m_shifting.get(), renderLayer);

  m_effectEmitter->render(renderCallback);
  m_songbook->render(renderCallback);
}

List<Drawable> Npc::drawables(Vec2F position) {
  List<Drawable> drawables;
  m_tools->setupHumanoidHandItemDrawables(*humanoid());
  auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor());

  DirectivesGroup humanoidDirectives;
  Vec2F scale = Vec2F::filled(1.f);
  for (auto& directives : m_statusController->parentDirectives().list()) {
    auto result = Humanoid::extractScaleFromDirectives(directives);
    scale = scale.piecewiseMultiply(result.first);
    humanoidDirectives.append(result.second);
  }
  humanoid()->setScale(scale);

  for (auto& drawable : humanoid()->render(true, true, (!anchor || !anchor->usePartZLevel), true)) {
    drawable.translate(position);
    if (drawable.isImage()) {
      drawable.imagePart().addDirectivesGroup(humanoidDirectives, true);

      if (anchor) {
        if (auto& directives = anchor->directives)
          drawable.imagePart().addDirectives(*directives, true);
      }
    }
    drawables.append(std::move(drawable));
  }

  drawables.appendAll(m_statusController->drawables(position));

  return drawables;
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
  return humanoid()->renderPortrait(mode);
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
  if (humanoid()->facingDirection() == Direction::Left)
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

  m_effectEmitter->setDirection(humanoid()->facingDirection());
  m_effectEmitter->tick(dt, *entityMode());

  humanoid()->setMovingBackwards(m_movementController->movingDirection() != m_movementController->facingDirection());
  humanoid()->setFacingDirection(m_movementController->facingDirection());
  humanoid()->setRotation(m_movementController->rotation());

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

  m_armor->setupHumanoid(*humanoid(), forceNude());

  m_tools->suppressItems(!canUseTool());
  m_tools->tick(dt, m_shifting.get(), {});

  if (auto overrideDirection = m_tools->setupHumanoidHandItems(*humanoid(), position(), aimPosition()))
    m_movementController->controlFace(*overrideDirection);

  if (world()->isClient()) {
    humanoid()->animate(dt, &m_humanoidDynamicTarget);
    m_humanoidDynamicTarget.updatePosition(position());
  } else {
    humanoid()->animate(dt, {});
  }
  m_scriptedAnimator.update();
}

LuaCallbacks Npc::makeNpcCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("toAbsolutePosition", [this](Vec2F const& p) { return getAbsolutePosition(p); });

  callbacks.registerCallback("species", [this]() { return m_npcVariant.species; });

  callbacks.registerCallback("gender", [this]() { return GenderNames.getRight(humanoid()->identity().gender); });

  callbacks.registerCallback("humanoidIdentity", [this]() { return humanoid()->identity().toJson(); });
  callbacks.registerCallback("setHumanoidIdentity", [this](Json const& id) { setIdentity(HumanoidIdentity(id)); });
  callbacks.registerCallback("setHumanoidParameter", [this](String key, Maybe<Json> value) { setHumanoidParameter(key, value); });
  callbacks.registerCallback("getHumanoidParameter", [this](String key) -> Maybe<Json> { return getHumanoidParameter(key); });
  callbacks.registerCallback("setHumanoidParameters", [this](JsonObject parameters) { setHumanoidParameters(parameters); });
  callbacks.registerCallback("getHumanoidParameters", [this]() -> JsonObject { return getHumanoidParameters(); });
  callbacks.registerCallback("refreshHumanoidParameters", [this]() { refreshHumanoidParameters(); });
  callbacks.registerCallback("humanoidConfig", [this](bool withOverrides) -> Json { return humanoid()->humanoidConfig(withOverrides); });

  callbacks.registerCallback(   "bodyDirectives", [this]()   { return identity().bodyDirectives;      });
  callbacks.registerCallback("setBodyDirectives", [this](String const& str) { setBodyDirectives(str); });

  callbacks.registerCallback(   "emoteDirectives", [this]()   { return identity().emoteDirectives;      });
  callbacks.registerCallback("setEmoteDirectives", [this](String const& str) { setEmoteDirectives(str); });

  callbacks.registerCallback(   "hairGroup",      [this]()   { return identity().hairGroup;      });
  callbacks.registerCallback("setHairGroup",      [this](String const& str) { setHairGroup(str); });
  callbacks.registerCallback(   "hairType",       [this]()   { return identity().hairType;      });
  callbacks.registerCallback("setHairType",       [this](String const& str) { setHairType(str); });
  callbacks.registerCallback(   "hairDirectives", [this]()   { return identity().hairDirectives;     });
  callbacks.registerCallback("setHairDirectives", [this](String const& str) { setHairDirectives(str); });

  callbacks.registerCallback(   "facialHairGroup",      [this]()   { return identity().facialHairGroup;      });
  callbacks.registerCallback("setFacialHairGroup",      [this](String const& str) { setFacialHairGroup(str); });
  callbacks.registerCallback(   "facialHairType",       [this]()   { return identity().facialHairType;      });
  callbacks.registerCallback("setFacialHairType",       [this](String const& str) { setFacialHairType(str); });
  callbacks.registerCallback(   "facialHairDirectives", [this]()   { return identity().facialHairDirectives;      });
  callbacks.registerCallback("setFacialHairDirectives", [this](String const& str) { setFacialHairDirectives(str); });

  callbacks.registerCallback("hair", [this]() {
    HumanoidIdentity const& humanoidIdentity = identity();
    return luaTupleReturn(humanoidIdentity.hairGroup, humanoidIdentity.hairType, humanoidIdentity.hairDirectives);
  });

  callbacks.registerCallback("facialHair", [this]() {
    HumanoidIdentity const& humanoidIdentity = identity();
    return luaTupleReturn(humanoidIdentity.facialHairGroup, humanoidIdentity.facialHairType, humanoidIdentity.facialHairDirectives);
  });

  callbacks.registerCallback("facialMask", [this]() {
    HumanoidIdentity const& humanoidIdentity = identity();
    return luaTupleReturn(humanoidIdentity.facialMaskGroup, humanoidIdentity.facialMaskType, humanoidIdentity.facialMaskDirectives);
  });

  callbacks.registerCallback("setFacialHair", [this](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      setFacialHair(*group, *type, *directives);
    else {
      if (group)      setFacialHairGroup(*group);
      if (type)       setFacialHairType(*type);
      if (directives) setFacialHairDirectives(*directives);
    }
  });

  callbacks.registerCallback("setFacialMask", [this](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      setFacialMask(*group, *type, *directives);
    else {
      if (group)      setFacialMaskGroup(*group);
      if (type)       setFacialMaskType(*type);
      if (directives) setFacialMaskDirectives(*directives);
    }
  });

  callbacks.registerCallback("setHair", [this](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      setHair(*group, *type, *directives);
    else {
      if (group)      setHairGroup(*group);
      if (type)       setHairType(*type);
      if (directives) setHairDirectives(*directives);
    }
  });

  callbacks.registerCallback(   "description", [this]()                          { return description(); });
  callbacks.registerCallback("setDescription", [this](String const& description) { setDescription(description); });

  callbacks.registerCallback(   "name", [this]()                   { return name(); });
  callbacks.registerCallback("setName", [this](String const& name) { setName(name); });

  callbacks.registerCallback("setSpecies", [this](String const& species) { setSpecies(species); });

  callbacks.registerCallback(   "imagePath", [this]()                        { return identity().imagePath;    });
  callbacks.registerCallback("setImagePath", [this](Maybe<String> const& imagePath) { setImagePath(imagePath); });

  callbacks.registerCallback("setGender", [this](String const& gender) { setGender(GenderNames.getLeft(gender)); });

  callbacks.registerCallback(   "personality", [this]() { return jsonFromPersonality(identity().personality); });
  callbacks.registerCallback("setPersonality", [this](Json const& personalityConfig) {
    Personality const& oldPersonality = identity().personality;
    Personality newPersonality = oldPersonality;
    setPersonality(parsePersonality(newPersonality, personalityConfig));
  });

  callbacks.registerCallback(   "favoriteColor", [this]()            { return favoriteColor();  });
  callbacks.registerCallback("setFavoriteColor", [this](Color color) { setFavoriteColor(color); });


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

  callbacks.registerCallback("resetLounging", [this]() {
    auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor());
    if (anchor && anchor->dismountable) {
      m_movementController->resetAnchorState();
    }
  });

  callbacks.registerCallback("isLounging", [this]() { return is<LoungeAnchor>(m_movementController->entityAnchor()); });

  callbacks.registerCallback("setLoungeControlHeld", [this](String control, bool held) {
    if (held)
      m_LoungeControlsHeld.add(LoungeControlNames.getLeft(control));
    else
      m_LoungeControlsHeld.remove(LoungeControlNames.getLeft(control));
  });
  callbacks.registerCallback("isLoungeControlHeld", [this](String control) -> bool {
    return m_LoungeControlsHeld.contains(LoungeControlNames.getLeft(control));
  });

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
      if (auto equipmentSlot = EquipmentSlotNames.leftPtr(entry)) {
        return m_armor->itemDescriptor((uint8_t)*equipmentSlot).toJson();
      } else if (entry.equalsIgnoreCase("primary"))
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

  callbacks.registerCallback("setAnimationParameter", [this](String name, Json value) {
    m_scriptedAnimationParameters.set(std::move(name), std::move(value));
  });

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

  m_identityNetState.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_identityNetState);
  m_refreshedHumanoidParameters.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_refreshedHumanoidParameters);

  m_netHumanoid.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_netHumanoid);

  m_scriptedAnimationParameters.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_scriptedAnimationParameters);

  m_netGroup.setNeedsStoreCallback(bind(&Npc::setNetStates, this));
  m_netGroup.setNeedsLoadCallback(bind(&Npc::getNetStates, this, _1));
}

void Npc::setNetStates() {
  m_uniqueIdNetState.set(uniqueId());
  m_teamNetState.set(getTeam());
  m_humanoidStateNetState.set(humanoid()->state());
  m_humanoidEmoteStateNetState.set(humanoid()->emoteState());
  m_humanoidDanceNetState.set(humanoid()->dance());

  if (m_identityUpdated) {
    m_identityNetState.push(m_npcVariant.humanoidIdentity);
    m_identityUpdated = false;
  }
}

void Npc::getNetStates(bool initial) {
  setUniqueId(m_uniqueIdNetState.get());
  setTeam(m_teamNetState.get());
  humanoid()->setState(m_humanoidStateNetState.get());
  humanoid()->setEmoteState(m_humanoidEmoteStateNetState.get());
  humanoid()->setDance(m_humanoidDanceNetState.get());

  if ((m_identityNetState.pullUpdated()) && !initial) {
    auto newIdentity = m_identityNetState.get();
    if ((m_npcVariant.humanoidIdentity.species == newIdentity.species) && (m_npcVariant.humanoidIdentity.imagePath == newIdentity.imagePath)) {
      humanoid()->setIdentity(newIdentity);
    }
    m_npcVariant.humanoidIdentity = newIdentity;
  }
  if (m_refreshedHumanoidParameters.pullOccurred() && !initial) {
    refreshHumanoidParameters();
  }

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

  if (auto equipmentSlot = EquipmentSlotNames.leftPtr(slot)) {
    m_armor->setItem((uint8_t)*equipmentSlot, as<ArmorItem>(item));
  } else if (slot.equalsIgnoreCase("primary"))
    m_tools->setItems(item, m_tools->altHandItem());
  else if (slot.equalsIgnoreCase("alt"))
    m_tools->setItems(m_tools->primaryHandItem(), item);
  else
    return false;

  return true;
}

bool Npc::canUseTool() const {
  bool canUse = !shouldDestroy() && !m_statusController->toolUsageSuppressed();
  if (canUse) {
    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor()))
      if (loungeAnchor->suppressTools.value(loungeAnchor->controllable))
        return false;
  }
  return canUse;
}

void Npc::disableWornArmor(bool disable) {
  m_disableWornArmor.set(disable);
}

List<LightSource> Npc::lightSources() const {
  List<LightSource> lights;
  lights.appendAll(m_tools->lightSources());
  lights.appendAll(m_statusController->lightSources());
  lights.appendAll(humanoid()->networkedAnimator()->lightSources());
  return lights;
}

Maybe<Json> Npc::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  Maybe<Json> result = m_scriptComponent.handleMessage(message, world()->connection() == sendingConnection, args);
  if (!result)
    result = m_statusController->receiveMessage(message, world()->connection() == sendingConnection, args);
  return result;
}

Vec2F Npc::armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const {
  return m_tools->armPosition(*humanoid(), hand, facingDirection, armAngle, offset);
}

Vec2F Npc::handOffset(ToolHand hand, Direction facingDirection) const {
  return m_tools->handOffset(*humanoid(), hand, facingDirection);
}

Vec2F Npc::handPosition(ToolHand hand, Vec2F const& handOffset) const {
  return m_tools->handPosition(hand, *humanoid(), handOffset);
}

ItemPtr Npc::handItem(ToolHand hand) const {
  if (hand == ToolHand::Primary)
    return m_tools->primaryHandItem();
  return m_tools->altHandItem();
}

Vec2F Npc::armAdjustment() const {
  return humanoid()->armAdjustment();
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
  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());

  if (m_damageOnTouch.get() && !m_npcVariant.touchDamageConfig.isNull() && !(loungeAnchor && loungeAnchor->suppressTools)) {
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


HumanoidIdentity const& Npc::identity() const {
  return m_npcVariant.humanoidIdentity;
}

void Npc::updateIdentity() {
  m_identityUpdated = true;
  auto oldIdentity = humanoid()->identity();
  if ((m_npcVariant.humanoidIdentity.species != oldIdentity.species) || (m_npcVariant.humanoidIdentity.imagePath != oldIdentity.imagePath)) {
    refreshHumanoidParameters();
  } else {
    humanoid()->setIdentity(m_npcVariant.humanoidIdentity);
  }
}

void Npc::setIdentity(HumanoidIdentity identity) {
  m_npcVariant.humanoidIdentity = std::move(identity);
  updateIdentity();
}

void Npc::setHumanoidParameter(String key, Maybe<Json> value) {
  if (value.isValid())
    m_npcVariant.humanoidParameters.set(key, value.value());
  else
    m_npcVariant.humanoidParameters.erase(key);

  m_netHumanoid.netElements().last()->setHumanoidParameters(m_npcVariant.humanoidParameters);
}

Maybe<Json> Npc::getHumanoidParameter(String key) {
  return m_npcVariant.humanoidParameters.maybe(key);
}

void Npc::setHumanoidParameters(JsonObject parameters) {
  m_npcVariant.humanoidParameters = parameters;

  m_netHumanoid.netElements().last()->setHumanoidParameters(m_npcVariant.humanoidParameters);
}


JsonObject Npc::getHumanoidParameters() {
  return m_npcVariant.humanoidParameters;
}

void Npc::setBodyDirectives(String const& directives)
{ m_npcVariant.humanoidIdentity.bodyDirectives = directives; updateIdentity(); }

void Npc::setEmoteDirectives(String const& directives)
{ m_npcVariant.humanoidIdentity.emoteDirectives = directives; updateIdentity(); }

void Npc::setHairGroup(String const& group)
{ m_npcVariant.humanoidIdentity.hairGroup = group; updateIdentity(); }

void Npc::setHairType(String const& type)
{ m_npcVariant.humanoidIdentity.hairType = type; updateIdentity(); }

void Npc::setHairDirectives(String const& directives)
{ m_npcVariant.humanoidIdentity.hairDirectives = directives; updateIdentity(); }

void Npc::setFacialHairGroup(String const& group)
{ m_npcVariant.humanoidIdentity.facialHairGroup = group; updateIdentity(); }

void Npc::setFacialHairType(String const& type)
{ m_npcVariant.humanoidIdentity.facialHairType = type; updateIdentity(); }

void Npc::setFacialHairDirectives(String const& directives)
{ m_npcVariant.humanoidIdentity.facialHairDirectives = directives; updateIdentity(); }

void Npc::setFacialMaskGroup(String const& group)
{ m_npcVariant.humanoidIdentity.facialMaskGroup = group; updateIdentity(); }

void Npc::setFacialMaskType(String const& type)
{ m_npcVariant.humanoidIdentity.facialMaskType = type; updateIdentity(); }

void Npc::setFacialMaskDirectives(String const& directives)
{ m_npcVariant.humanoidIdentity.facialMaskDirectives = directives; updateIdentity(); }

void Npc::setHair(String const& group, String const& type, String const& directives) {
  m_npcVariant.humanoidIdentity.hairGroup = group;
  m_npcVariant.humanoidIdentity.hairType = type;
  m_npcVariant.humanoidIdentity.hairDirectives = directives;
  updateIdentity();
}

void Npc::setFacialHair(String const& group, String const& type, String const& directives) {
  m_npcVariant.humanoidIdentity.facialHairGroup = group;
  m_npcVariant.humanoidIdentity.facialHairType = type;
  m_npcVariant.humanoidIdentity.facialHairDirectives = directives;
  updateIdentity();
}

void Npc::setFacialMask(String const& group, String const& type, String const& directives) {
  m_npcVariant.humanoidIdentity.facialMaskGroup = group;
  m_npcVariant.humanoidIdentity.facialMaskType = type;
  m_npcVariant.humanoidIdentity.facialMaskDirectives = directives;
  updateIdentity();
}

void Npc::setSpecies(String const& species) {
  m_npcVariant.humanoidIdentity.species = species;
  updateIdentity();
}

void Npc::setGender(Gender const& gender) {
  m_npcVariant.humanoidIdentity.gender = gender;
  updateIdentity();
}

void Npc::setPersonality(Personality const& personality) {
  m_npcVariant.humanoidIdentity.personality = personality;
  updateIdentity();
}

void Npc::setImagePath(Maybe<String> const& imagePath) {
  m_npcVariant.humanoidIdentity.imagePath = imagePath;
  updateIdentity();
}

void Npc::setFavoriteColor(Color color) {
  m_npcVariant.humanoidIdentity.color = color.toRgba();
  updateIdentity();
}

void Npc::setName(String const& name) {
  m_npcVariant.humanoidIdentity.name = name;
  updateIdentity();
}

void Npc::setDescription(String const& description) {
  m_npcVariant.description = description;
}

HumanoidPtr Npc::humanoid() {
  return m_netHumanoid.netElements().last()->humanoid();
}
HumanoidPtr Npc::humanoid() const {
  return m_netHumanoid.netElements().last()->humanoid();
}

void Npc::refreshHumanoidParameters() {
  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDef = speciesDatabase->species(m_npcVariant.humanoidIdentity.species);

  if (isMaster()) {
    m_refreshedHumanoidParameters.trigger();
    m_scriptedAnimationParameters.clear();
    m_netHumanoid.clearNetElements();
    m_netHumanoid.addNetElement(make_shared<NetHumanoid>(m_npcVariant.humanoidIdentity, m_npcVariant.humanoidParameters, m_npcVariant.uniqueHumanoidConfig ? m_npcVariant.humanoidConfig : Json()));
    m_deathParticleBurst.set(humanoid()->defaultDeathParticles());
  }else {
    m_npcVariant.humanoidParameters = m_netHumanoid.netElements().last()->humanoidParameters();
  }

  auto armor = m_armor->diskStore();
  m_armor->reset();
  m_armor->diskLoad(armor);
  m_armor->setupHumanoid(*humanoid(), forceNude());

  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), m_npcVariant.movementParameters)));

  if (inWorld()) {
    if (isMaster()) {
      if (m_scriptComponent.initialized()) {
        m_scriptComponent.removeCallbacks("animator");
        m_scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(humanoid()->networkedAnimator()));
        m_scriptComponent.invoke("refreshHumanoidParameters");
      }
    }
    if (world()->isClient() && m_scriptedAnimator.initialized()) {
      m_scriptedAnimator.uninit();
      m_scriptedAnimator.removeCallbacks("animationConfig");
      m_scriptedAnimator.removeCallbacks("config");
      m_scriptedAnimator.removeCallbacks("entity");

      m_scriptedAnimator.setScripts(humanoid()->animationScripts());
      m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(humanoid()->networkedAnimator(),
        [this](String const& name, Json const& defaultValue) -> Json {
          return m_scriptedAnimationParameters.value(name, defaultValue);
        }));
      m_scriptedAnimator.addCallbacks("config",
          LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def)
              { return m_npcVariant.scriptConfig.query(name, def); }));
      m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
      m_scriptedAnimator.init(world());
    }
  }
}

bool Npc::forceNude() const {
  return m_statusController->statPositive("nude");
}

}
