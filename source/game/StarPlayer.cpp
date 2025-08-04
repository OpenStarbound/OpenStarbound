#include "StarPlayer.hpp"
#include "StarEncode.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarSongbook.hpp"
#include "StarSongbookLuaBindings.hpp"
#include "StarEmoteProcessor.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarDamageManager.hpp"
#include "StarTools.hpp"
#include "StarItemDrop.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarArmors.hpp"
#include "StarPlayerFactory.hpp"
#include "StarAssets.hpp"
#include "StarPlayerInventory.hpp"
#include "StarTechController.hpp"
#include "StarClientContext.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemBag.hpp"
#include "StarEntitySplash.hpp"
#include "StarWorld.hpp"
#include "StarStatusController.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarPlayerBlueprints.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarPlayerCodexes.hpp"
#include "StarPlayerTech.hpp"
#include "StarPlayerCompanions.hpp"
#include "StarPlayerDeployment.hpp"
#include "StarPlayerLog.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarQuestManager.hpp"
#include "StarAiDatabase.hpp"
#include "StarStatistics.hpp"
#include "StarInspectionTool.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"

namespace Star {

EnumMap<Player::State> const Player::StateNames{
  {Player::State::Idle, "idle"},
  {Player::State::Walk, "walk"},
  {Player::State::Run, "run"},
  {Player::State::Jump, "jump"},
  {Player::State::Fall, "fall"},
  {Player::State::Swim, "swim"},
  {Player::State::SwimIdle, "swimIdle"},
  {Player::State::TeleportIn, "teleportIn"},
  {Player::State::TeleportOut, "teleportOut"},
  {Player::State::Crouch, "crouch"},
  {Player::State::Lounge, "lounge"}
};

Player::Player(PlayerConfigPtr config, Uuid uuid) {
  auto assets = Root::singleton().assets();

  m_config = config;
  m_client = nullptr;

  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;

  m_footstepTimer = 0.0f;
  m_teleportTimer = 0.0f;
  m_teleportAnimationType = "default";

  m_shifting = false;

  m_aimPosition = Vec2F();

  setUniqueId(uuid.hex());
  m_identity = m_config->defaultIdentity;
  m_identityUpdated = true;

  m_questManager = make_shared<QuestManager>(this);
  m_tools = make_shared<ToolUser>();
  m_armor = make_shared<ArmorWearer>();
  m_companions = make_shared<PlayerCompanions>(config->companionsConfig);

  for (auto& p : config->genericScriptContexts) {
    auto scriptComponent = make_shared<GenericScriptComponent>();
    scriptComponent->setScript(p.second);
    m_genericScriptContexts.set(p.first, scriptComponent);
  }

  // all of these are defaults and won't include the correct humanoid config for the species
  m_netHumanoid.addNetElement(make_shared<NetHumanoid>(m_identity, m_humanoidParameters, Json()));
  auto movementParameters = ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), humanoid()->playerMovementParameters().value(m_config->movementParameters)));
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet({"player"});
  m_movementController = make_shared<ActorMovementController>(movementParameters);
  m_zeroGMovementParameters = ActorMovementParameters(m_config->zeroGMovementParameters);

  m_techController = make_shared<TechController>();
  m_statusController = make_shared<StatusController>(m_config->statusControllerSettings);
  m_deployment = make_shared<PlayerDeployment>(m_config->deploymentConfig);

  m_inventory = make_shared<PlayerInventory>();
  m_blueprints = make_shared<PlayerBlueprints>();
  m_universeMap = make_shared<PlayerUniverseMap>();
  m_codexes = make_shared<PlayerCodexes>();
  m_techs = make_shared<PlayerTech>();
  m_log = make_shared<PlayerLog>();

  setModeType(PlayerMode::Casual);

  m_useDown = false;
  m_edgeTriggeredUse = false;
  setTeam(EntityDamageTeam(TeamType::Friendly));

  m_footstepVolumeVariance = assets->json("/sfx.config:footstepVolumeVariance").toFloat();
  m_landingVolume = assets->json("/sfx.config:landingVolume").toFloat();

  m_effectsAnimator = make_shared<NetworkedAnimator>(assets->fetchJson(m_config->effectsAnimator));
  m_effectEmitter = make_shared<EffectEmitter>();

  m_interactRadius = assets->json("/player.config:interactRadius").toFloat();

  m_walkIntoInteractBias = jsonToVec2F(assets->json("/player.config:walkIntoInteractBias"));

  if (m_landingVolume <= 1)
    m_landingVolume = 6;

  m_isAdmin = false;

  m_emoteCooldown = assets->json("/player.config:emoteCooldown").toFloat();
  m_blinkInterval = jsonToVec2F(assets->json("/player.config:blinkInterval"));

  m_emoteCooldownTimer = 0;
  m_blinkCooldownTimer = 0;

  m_chatMessageChanged = false;
  m_chatMessageUpdated = false;

  m_songbook = make_shared<Songbook>(species());

  m_lastDamagedOtherTimer = 0;
  m_lastDamagedTarget = NullEntityId;

  m_ageItemsTimer = GameTimer(assets->json("/player.config:ageItemsEvery").toFloat());

  refreshEquipment();

  m_foodLowThreshold = assets->json("/player.config:foodLowThreshold").toFloat();
  m_foodLowStatusEffects = assets->json("/player.config:foodLowStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);
  m_foodEmptyStatusEffects = assets->json("/player.config:foodEmptyStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);

  m_inCinematicStatusEffects = assets->json("/player.config:inCinematicStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_landingNoisePending = false;
  m_footstepPending = false;

  setKeepAlive(true);

  m_netGroup.addNetElement(&m_stateNetState);
  m_netGroup.addNetElement(&m_shiftingNetState);
  m_netGroup.addNetElement(&m_xAimPositionNetState);
  m_netGroup.addNetElement(&m_yAimPositionNetState);
  m_netGroup.addNetElement(&m_identityNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_landedNetState);
  m_netGroup.addNetElement(&m_chatMessageNetState);
  m_netGroup.addNetElement(&m_newChatMessageNetState);
  m_netGroup.addNetElement(&m_emoteNetState);

  m_xAimPositionNetState.setFixedPointBase(0.003125);
  m_yAimPositionNetState.setFixedPointBase(0.003125);
  m_yAimPositionNetState.setInterpolator(lerp<float, float>);

  m_netGroup.addNetElement(m_inventory.get());
  m_netGroup.addNetElement(m_tools.get());
  m_netGroup.addNetElement(m_armor.get());
  m_netGroup.addNetElement(m_songbook.get());
  m_netGroup.addNetElement(m_movementController.get());
  m_netGroup.addNetElement(m_effectEmitter.get());
  m_netGroup.addNetElement(m_effectsAnimator.get());
  m_netGroup.addNetElement(m_statusController.get());
  m_netGroup.addNetElement(m_techController.get());

  m_netHumanoid.setCompatibilityVersion(10);
  // m_netGroup.addNetElement(&m_netHumanoid);
  m_refreshedHumanoidParameters.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_refreshedHumanoidParameters);

  m_scriptedAnimationParameters.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_scriptedAnimationParameters);

  m_deathParticleBurst.setCompatibilityVersion(10);
  m_netGroup.addNetElement(&m_deathParticleBurst);

  m_netGroup.setNeedsLoadCallback(bind(&Player::getNetStates, this, _1));
  m_netGroup.setNeedsStoreCallback(bind(&Player::setNetStates, this));
}

Player::Player(PlayerConfigPtr config, ByteArray const& netStore, NetCompatibilityRules rules) : Player(config) {
  DataStreamBuffer ds(netStore);
  ds.setStreamCompatibilityVersion(rules);

  setUniqueId(ds.read<String>());

  ds.read(m_description);
  ds.read(m_modeType);
  ds.read(m_identity);
  if (rules.version() >= 10) {
    ds.read(m_humanoidParameters);
  }

  m_netHumanoid.clearNetElements();
  m_netHumanoid.addNetElement(make_shared<NetHumanoid>(m_identity, m_humanoidParameters, Json()));
  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), humanoid()->playerMovementParameters().value(m_config->movementParameters))));
  m_deathParticleBurst.set(humanoid()->defaultDeathParticles());
}


Player::Player(PlayerConfigPtr config, Json const& diskStore) : Player(config) {
  diskLoad(diskStore);
}

void Player::diskLoad(Json const& diskStore) {
  setUniqueId(diskStore.getString("uuid"));
  m_description = diskStore.getString("description");
  setModeType(PlayerModeNames.getLeft(diskStore.getString("modeType")));
  m_shipUpgrades = ShipUpgrades(diskStore.get("shipUpgrades"));
  m_blueprints = make_shared<PlayerBlueprints>(diskStore.get("blueprints"));
  m_universeMap = make_shared<PlayerUniverseMap>(diskStore.get("universeMap"));
  if (m_clientContext)
    m_universeMap->setServerUuid(m_clientContext->serverUuid());

  m_codexes = make_shared<PlayerCodexes>(diskStore.get("codexes"));
  m_techs = make_shared<PlayerTech>(diskStore.get("techs"));
  m_identity = HumanoidIdentity(diskStore.get("identity"));
  m_identityUpdated = true;

  setTeam(EntityDamageTeam(diskStore.get("team")));

  m_state = State::Idle;

  m_inventory->load(diskStore.get("inventory"));

  m_movementController->loadState(diskStore.get("movementController"));
  m_techController->diskLoad(diskStore.get("techController"));
  m_statusController->diskLoad(diskStore.get("statusController"));

  m_log = make_shared<PlayerLog>(diskStore.get("log"));

  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDef = speciesDatabase->species(m_identity.species);

  m_questManager->diskLoad(diskStore.get("quests", JsonObject{}));
  m_companions->diskLoad(diskStore.get("companions", JsonObject{}));
  m_deployment->diskLoad(diskStore.get("deployment", JsonObject{}));

  m_humanoidParameters = diskStore.getObject("humanoidParameters", JsonObject());

  m_netHumanoid.clearNetElements();
  m_netHumanoid.addNetElement(make_shared<NetHumanoid>(m_identity, m_humanoidParameters, Json()));
  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), humanoid()->playerMovementParameters().value(m_config->movementParameters))));
  m_effectsAnimator->setGlobalTag("effectDirectives", speciesDef->effectDirectives());
  m_deathParticleBurst.set(humanoid()->defaultDeathParticles());

  m_genericProperties = diskStore.getObject("genericProperties");

  m_armor->reset();
  refreshArmor();
  setNetArmorSecrets(true);

  m_codexes->learnInitialCodexes(species());

  m_aiState = AiState(diskStore.get("aiState", JsonObject{}));

  for (auto& script : m_genericScriptContexts)
    script.second->setScriptStorage({});

  for (auto& p : diskStore.get("genericScriptStorage", JsonObject{}).toObject()) {
    if (auto script = m_genericScriptContexts.maybe(p.first).value({})) {
      script->setScriptStorage(p.second.toObject());
    }
  }

  // Make sure to merge the stored player blueprints with what a new player
  // would get as default.
  for (auto const& descriptor : m_config->defaultBlueprints)
    m_blueprints->add(descriptor);
  for (auto const& descriptor : speciesDef->defaultBlueprints())
    m_blueprints->add(descriptor);
}

ClientContextPtr Player::clientContext() const {
  return m_clientContext;
}

void Player::setClientContext(ClientContextPtr clientContext) {
  m_clientContext = std::move(clientContext);
  if (m_clientContext && m_universeMap)
    m_universeMap->setServerUuid(m_clientContext->serverUuid());
}

StatisticsPtr Player::statistics() const {
  return m_statistics;
}

void Player::setStatistics(StatisticsPtr statistics) {
  m_statistics = statistics;
}

void Player::setUniverseClient(UniverseClient* client) {
  m_client = client;
  m_questManager->setUniverseClient(client);
}

UniverseClient* Player::universeClient() const {
  return m_client;
}

EntityType Player::entityType() const {
  return EntityType::Player;
}

ClientEntityMode Player::clientEntityMode() const {
  return ClientEntityMode::ClientPresenceMaster;
}

void Player::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);


  m_tools->init(this);
  m_movementController->init(world);
  m_movementController->setIgnorePhysicsEntities({entityId});
  m_statusController->init(this, m_movementController.get());
  m_techController->init(this, m_movementController.get(), m_statusController.get());
  auto speciesDefinition = Root::singleton().speciesDatabase()->species(m_identity.species);

  if (mode == EntityMode::Master) {
    m_scriptedAnimationParameters.clear();
    m_movementController->setRotation(0);
    m_statusController->setStatusProperty("ouchNoise", speciesDefinition->ouchNoise(m_identity.gender));
    m_emoteState = HumanoidEmote::Idle;
    m_questManager->init(world);
    m_companions->init(this, world);
    m_deployment->init(this, world);
    m_missionRadioMessages.clear();

    m_statusController->setPersistentEffects("species", speciesDefinition->statusEffects());

    for (auto& p : m_genericScriptContexts) {
      p.second->addActorMovementCallbacks(m_movementController.get());
      p.second->addCallbacks("player", LuaBindings::makePlayerCallbacks(this));
      p.second->addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_statusController.get()));
      p.second->addCallbacks("songbook", LuaBindings::makeSongbookCallbacks(m_songbook.get()));
      p.second->addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(humanoid()->networkedAnimator()));
      if (m_client)
        p.second->addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client));
      p.second->init(world);
    }

    for (auto& p : m_inventory->pullOverflow()) {
      world->addEntity(ItemDrop::createRandomizedDrop(p, m_movementController->position(), true));
    }

    setNetArmorSecrets();
  }

  if (world->isClient()) {
      m_scriptedAnimator.setScripts(humanoid()->animationScripts());
      m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(humanoid()->networkedAnimator(),
        [this](String const& name, Json const& defaultValue) -> Json {
          return m_scriptedAnimationParameters.value(name, defaultValue);
        }));
      m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
      m_scriptedAnimator.init(world);
  }

  m_xAimPositionNetState.setInterpolator(world->geometry().xLerpFunction());
  refreshEquipment();
}

void Player::uninit() {
  m_techController->uninit();
  m_movementController->uninit();
  m_tools->uninit();
  m_statusController->uninit();

  if (isMaster()) {
    m_questManager->uninit();
    m_companions->uninit();
    m_deployment->uninit();

    for (auto& p : m_genericScriptContexts) {
      p.second->uninit();
      p.second->removeCallbacks("animator");
      p.second->removeCallbacks("entity");
      p.second->removeCallbacks("player");
      p.second->removeCallbacks("mcontroller");
      p.second->removeCallbacks("status");
      p.second->removeCallbacks("songbook");
      p.second->removeCallbacks("world");
      if (m_client)
        p.second->removeCallbacks("celestial");
    }
  }
  if (world()->isClient()) {
    m_scriptedAnimator.uninit();
    m_scriptedAnimator.removeCallbacks("animationConfig");
    m_scriptedAnimator.removeCallbacks("entity");
  }

  Entity::uninit();
}

List<Drawable> Player::drawables() const {
  List<Drawable> drawables;

  if (!isTeleporting()) {
    drawables.appendAll(m_techController->backDrawables());
    if (!m_techController->parentHidden()) {
      m_tools->setupHumanoidHandItemDrawables(*humanoid());

      // Auto-detect any ?scalenearest and apply them as a direct scale on the Humanoid's drawables instead.
      DirectivesGroup humanoidDirectives;
      Vec2F scale = Vec2F::filled(1.f);
      auto extractScale = [&](List<Directives> const& list) {
        for (auto& directives : list) {
          auto result = Humanoid::extractScaleFromDirectives(directives);
          scale = scale.piecewiseMultiply(result.first);
          humanoidDirectives.append(result.second);
        }
      };
      extractScale(m_techController->parentDirectives().list());
      extractScale(m_statusController->parentDirectives().list());
      humanoid()->setScale(scale);

      for (auto& drawable : humanoid()->render()) {
        drawable.translate(position() + m_techController->parentOffset());
        if (drawable.isImage()) {
          drawable.imagePart().addDirectivesGroup(humanoidDirectives, true);

          if (auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
            if (auto& directives = anchor->directives)
              drawable.imagePart().addDirectives(*directives, true);
          }
        }
        drawables.append(std::move(drawable));
      }
    }
    drawables.appendAll(m_techController->frontDrawables());

    drawables.appendAll(m_statusController->drawables());

    drawables.appendAll(m_tools->renderObjectPreviews(aimPosition(), walkingDirection(), inToolRange(), favoriteColor()));
  }

  drawables.appendAll(m_effectsAnimator->drawables(position()));

  return drawables;
}

List<OverheadBar> Player::bars() const {
  return m_statusController->overheadBars();
}

List<Particle> Player::particles() {
  List<Particle> particles;
  particles.appendAll(m_config->splashConfig.doSplash(position(), m_movementController->velocity(), world()));
  particles.appendAll(take(m_callbackParticles));
  particles.appendAll(humanoid()->networkedAnimatorDynamicTarget()->pullNewParticles());
  particles.appendAll(m_techController->pullNewParticles());
  particles.appendAll(m_statusController->pullNewParticles());

  return particles;
}

void Player::addParticles(List<Particle> const& particles) {
  m_callbackParticles.appendAll(particles);
}

void Player::addSound(String const& sound, float volume, float pitch) {
  m_callbackSounds.emplaceAppend(sound, volume, pitch);
}

void Player::addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) {
  if (isSlave())
    throw PlayerException("Adding status effects to an entity can only be done directly on the master entity.");
  m_statusController->addEphemeralEffects(statusEffects);
}

ActiveUniqueStatusEffectSummary Player::activeUniqueStatusEffectSummary() const {
  return m_statusController->activeUniqueStatusEffectSummary();
}

float Player::powerMultiplier() const {
  return m_statusController->stat("powerMultiplier");
}

bool Player::isDead() const {
  return !m_statusController->resourcePositive("health");
}

void Player::kill() {
  m_statusController->setResource("health", 0);
}

bool Player::wireToolInUse() const {
  return (bool)as<WireTool>(m_tools->primaryHandItem());
}

void Player::setWireConnector(WireConnector* wireConnector) const {
  if (auto wireTool = as<WireTool>(m_tools->primaryHandItem()))
    wireTool->setConnector(wireConnector);
}

List<Drawable> Player::portrait(PortraitMode mode) const {
  if (isPermaDead())
    return humanoid()->renderSkull();
  if (invisible())
    return {};
  if (!inWorld())
    refreshHumanoid();
  return humanoid()->renderPortrait(mode);
}

bool Player::underwater() const {
  if (!inWorld())
    return false;
  else
    return world()->liquidLevel(Vec2I(position() + m_config->underwaterSensor)).level
        >= m_config->underwaterMinWaterLevel;
}

List<LightSource> Player::lightSources() const {
  List<LightSource> lights;
  lights.appendAll(m_tools->lightSources());
  lights.appendAll(m_statusController->lightSources());
  lights.appendAll(m_techController->lightSources());
  lights.appendAll(humanoid()->networkedAnimator()->lightSources());
  return lights;
}

RectF Player::metaBoundBox() const {
  return m_config->metaBoundBox;
}

Maybe<HitType> Player::queryHit(DamageSource const& source) const {
  if (!inWorld() || isDead() || m_isAdmin || isTeleporting() || m_statusController->statPositive("invulnerable"))
    return {};

  if (m_tools->queryShieldHit(source))
    return HitType::ShieldHit;

  if (source.intersectsWithPoly(world()->geometry(), m_movementController->collisionBody()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Player::hitPoly() const {
  return m_movementController->collisionBody();
}

List<DamageNotification> Player::applyDamage(DamageRequest const& request) {
  if (!inWorld() || isDead() || m_isAdmin)
    return {};

  return m_statusController->applyDamageRequest(request);
}

List<DamageNotification> Player::selfDamageNotifications() {
  return m_statusController->pullSelfDamageNotifications();
}

void Player::hitOther(EntityId targetEntityId, DamageRequest const& damageRequest) {
  if (!isMaster())
    return;

  m_statusController->hitOther(targetEntityId, damageRequest);
  if (as<DamageBarEntity>(world()->entity(targetEntityId))) {
    m_lastDamagedOtherTimer = 0;
    m_lastDamagedTarget = targetEntityId;
  }
}

void Player::damagedOther(DamageNotification const& damage) {
  if (!isMaster())
    return;

  m_statusController->damagedOther(damage);
}

List<DamageSource> Player::damageSources() const {
  return m_damageSources;
}

bool Player::shouldDestroy() const {
  return isDead();
}

void Player::destroy(RenderCallback* renderCallback) {
  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;
  if (renderCallback && m_deathParticleBurst.get())
    renderCallback->addParticles(humanoid()->particles(*m_deathParticleBurst.get()), position());

  if (isMaster()) {
    m_log->addDeathCount(1);

    if (!world()->disableDeathDrops()) {
      if (auto dropString = modeConfig().deathDropItemTypes.maybeLeft()) {
        if (*dropString == "all")
          dropEverything();
      } else {
        List<ItemType> dropList = modeConfig().deathDropItemTypes.right().transformed([](String typeName) {
            return ItemTypeNames.getLeft(typeName);
          });
        Set<ItemType> dropSet = Set<ItemType>::from(dropList);
        auto itemDb = Root::singleton().itemDatabase();
        dropSelectedItems([dropSet, itemDb](ItemPtr item) {
            return dropSet.contains(itemDb->itemType(item->name()));
          });
      }
    }
  }

  m_songbook->stop();
}

Maybe<EntityAnchorState> Player::loungingIn() const {
  if (is<LoungeAnchor>(m_movementController->entityAnchor()))
    return m_movementController->anchorState();
  return {};
}

bool Player::lounge(EntityId loungeableEntityId, size_t anchorIndex) {
  if (!canUseTool())
    return false;

  auto loungeableEntity = world()->get<LoungeableEntity>(loungeableEntityId);
  if (!loungeableEntity || anchorIndex >= loungeableEntity->anchorCount()
      || !loungeableEntity->entitiesLoungingIn(anchorIndex).empty()
      || !loungeableEntity->loungeAnchor(anchorIndex))
    return false;

  m_state = State::Lounge;
  m_movementController->setAnchorState({loungeableEntityId, anchorIndex});
  return true;
}

void Player::stopLounging() {
  if (loungingIn()) {
    m_movementController->resetAnchorState();
    m_state = State::Idle;
    m_statusController->setPersistentEffects("lounging", {});
  }
}

Vec2F Player::position() const {
  return m_movementController->position();
}

Vec2F Player::velocity() const {
  return m_movementController->velocity();
}

Vec2F Player::mouthOffset(bool ignoreAdjustments) const {
  return Vec2F(
      humanoid()->mouthOffset(ignoreAdjustments)[0] * numericalDirection(facingDirection()), humanoid()->mouthOffset(ignoreAdjustments)[1]);
}

Vec2F Player::feetOffset() const {
  return Vec2F(humanoid()->feetOffset()[0] * numericalDirection(facingDirection()), humanoid()->feetOffset()[1]);
}

Vec2F Player::headArmorOffset() const {
  return Vec2F(
      humanoid()->headArmorOffset()[0] * numericalDirection(facingDirection()), humanoid()->headArmorOffset()[1]);
}

Vec2F Player::chestArmorOffset() const {
  return Vec2F(
      humanoid()->chestArmorOffset()[0] * numericalDirection(facingDirection()), humanoid()->chestArmorOffset()[1]);
}

Vec2F Player::backArmorOffset() const {
  return Vec2F(
      humanoid()->backArmorOffset()[0] * numericalDirection(facingDirection()), humanoid()->backArmorOffset()[1]);
}

Vec2F Player::legsArmorOffset() const {
  return Vec2F(
      humanoid()->legsArmorOffset()[0] * numericalDirection(facingDirection()), humanoid()->legsArmorOffset()[1]);
}

Vec2F Player::mouthPosition() const {
  return position() + mouthOffset(true);
}

Vec2F Player::mouthPosition(bool ignoreAdjustments) const {
  return position() + mouthOffset(ignoreAdjustments);
}

RectF Player::collisionArea() const {
  return m_movementController->collisionPoly().boundBox();
}

void Player::revive(Vec2F const& footPosition) {
  if (!isDead())
    return;

  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_statusController->clearEphemeralEffects();

  endPrimaryFire();
  endAltFire();
  endTrigger();

  m_effectEmitter->reset();
  m_movementController->setPosition(footPosition - feetOffset());
  m_movementController->setVelocity(Vec2F());

  m_techController->reloadTech();

  float moneyCost = m_inventory->currency("money") * modeConfig().reviveCostPercentile;
  m_inventory->consumeCurrency("money", min((uint64_t)round(moneyCost), m_inventory->currency("money")));
}

bool Player::shifting() const {
  return m_shifting;
}

void Player::setShifting(bool shifting) {
  m_shifting = shifting;
}

void Player::special(int specialKey) {
  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->controllable) {
    auto anchorState = m_movementController->anchorState();
    if (auto loungeableEntity = world()->get<LoungeableEntity>(anchorState->entityId)) {
      if (specialKey == 1)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special1);
      else if (specialKey == 2)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special2);
      else if (specialKey == 3)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special3);
      return;
    }
  }
  m_techController->special(specialKey);
}

void Player::setMoveVector(Vec2F const& vec) {
  m_moveVector = vec;
}

void Player::moveLeft() {
  m_pendingMoves.add(MoveControlType::Left);
}

void Player::moveRight() {
  m_pendingMoves.add(MoveControlType::Right);
}

void Player::moveUp() {
  m_pendingMoves.add(MoveControlType::Up);
}

void Player::moveDown() {
  m_pendingMoves.add(MoveControlType::Down);
}

void Player::jump() {
  m_pendingMoves.add(MoveControlType::Jump);
}

void Player::dropItem() {
  if (!world())
    return;
  if (!canUseTool())
    return;

  Vec2F throwDirection = world()->geometry().diff(aimPosition(), position());
  for (auto& throwSlot : {m_inventory->primaryHeldSlot(), m_inventory->secondaryHeldSlot()}) {
    if (throwSlot) {
      if (auto drop = m_inventory->takeSlot(*throwSlot)) {
        world()->addEntity(ItemDrop::throwDrop(drop, position(), velocity(), throwDirection));
        break;
      }
    }
  }
}

Maybe<Json> Player::receiveMessage(ConnectionId fromConnection, String const& message, JsonArray const& args) {
  bool localMessage = fromConnection == world()->connection();
  if (message == "queueRadioMessage" && args.size() > 0) {
    float delay = 0;
    if (args.size() > 1 && args.get(1).canConvert(Json::Type::Float))
      delay = args.get(1).toFloat();

    queueRadioMessage(args.get(0), delay);
  } else if (message == "warp") {
    Maybe<String> animation;
    if (args.size() > 1)
      animation = args.get(1).toString();

    bool deploy = false;
    if (args.size() > 2)
      deploy = args.get(2).toBool();

    setPendingWarp(args.get(0).toString(), animation, deploy);
  } else if (message == "interruptRadioMessage") {
    m_interruptRadioMessage = true;
  } else if (message == "playCinematic" && args.size() > 0) {
    bool unique = false;
    if (args.size() > 1)
      unique = args.get(1).toBool();
    setPendingCinematic(args.get(0), unique);
  } else if (message == "playAltMusic" && args.size() > 0) {
    float fadeTime = args.size() > 1 ? args.get(1).toFloat() : 0.f;
    int loops = args.size() > 2 ? args.get(2).toInt() : -1;
    StringList trackList;
    if (args.get(0).canConvert(Json::Type::Array))
      trackList = jsonToStringList(args.get(0).toArray());
    else
      trackList = StringList();
    m_pendingAltMusic = pair<Maybe<pair<StringList, int>>, float>(make_pair(trackList, loops), fadeTime);
  } else if (message == "stopAltMusic") {
    float fadeTime = 0;
    if (args.size() > 0)
      fadeTime = args.get(0).toFloat();
    m_pendingAltMusic = pair<Maybe<pair<StringList, int>>, float>({}, fadeTime);
  } else if (message == "recordEvent") {
    statistics()->recordEvent(args.at(0).toString(), args.at(1));
  } else if (message == "addCollectable") {
    auto collection = args.get(0).toString();
    auto collectable = args.get(1).toString();
    if (Root::singleton().collectionDatabase()->hasCollectable(collection, collectable))
      addCollectable(collection, collectable);
  } else {
    Maybe<Json> result = m_tools->receiveMessage(message, localMessage, args);
    if (!result)
      result = m_statusController->receiveMessage(message, localMessage, args);
    if (!result)
      result = m_companions->receiveMessage(message, localMessage, args);
    if (!result)
      result = m_deployment->receiveMessage(message, localMessage, args);
    if (!result)
      result = m_techController->receiveMessage(message, localMessage, args);
    if (!result)
      result = m_questManager->receiveMessage(message, localMessage, args);
    for (auto& p : m_genericScriptContexts) {
      if (result)
        break;
      result = p.second->handleMessage(message, localMessage, args);
    }
    return result;
  }

  return {};
}

void Player::update(float dt, uint64_t) {
  m_movementController->setTimestep(dt);

  if (isMaster()) {
    if (m_emoteCooldownTimer) {
      m_emoteCooldownTimer -= dt;
      if (m_emoteCooldownTimer <= 0) {
        m_emoteCooldownTimer = 0;
        m_emoteState = HumanoidEmote::Idle;
      }
    }

    if (m_chatMessageUpdated) {
      auto state = Root::singleton().emoteProcessor()->detectEmotes(m_chatMessage);
      if (state != HumanoidEmote::Idle)
        addEmote(state);
      m_chatMessageUpdated = false;
    }

    m_blinkCooldownTimer -= dt;
    if (m_blinkCooldownTimer <= 0) {
      m_blinkCooldownTimer = Random::randf(m_blinkInterval[0], m_blinkInterval[1]);
      auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
      if (m_emoteState == HumanoidEmote::Idle && (!loungeAnchor || !loungeAnchor->emote))
        addEmote(HumanoidEmote::Blink);
    }

    m_lastDamagedOtherTimer += dt;

    if (m_movementController->zeroG())
      m_movementController->controlParameters(m_zeroGMovementParameters);

    if (isTeleporting()) {
      m_teleportTimer -= dt;
      if (m_teleportTimer <= 0 && m_state == State::TeleportIn) {
        m_state = State::Idle;
        m_effectsAnimator->burstParticleEmitter(m_teleportAnimationType + "Burst");
      }
    }

    if (!isTeleporting()) {
      processControls();

      m_questManager->update(dt);
      m_companions->update(dt);
      m_deployment->update(dt);

      bool edgeTriggeredUse = take(m_edgeTriggeredUse);

      m_inventory->cleanup();
      refreshEquipment();

      if (inConflictingLoungeAnchor())
        m_movementController->resetAnchorState();

      if (m_state == State::Lounge) {
        if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
          m_statusController->setPersistentEffects("lounging", loungeAnchor->statusEffects);
          addEffectEmitters(loungeAnchor->effectEmitters);
          if (loungeAnchor->emote)
            requestEmote(*loungeAnchor->emote);

          auto itemDatabase = Root::singleton().itemDatabase();
          if (auto headOverride = loungeAnchor->armorCosmeticOverrides.maybe("head")) {
            auto overrideItem = itemDatabase->item(ItemDescriptor(*headOverride));
            if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::HeadCosmetic))
              m_armor->setHeadCosmeticItem(as<HeadArmor>(overrideItem));
          }
          if (auto chestOverride = loungeAnchor->armorCosmeticOverrides.maybe("chest")) {
            auto overrideItem = itemDatabase->item(ItemDescriptor(*chestOverride));
            if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::ChestCosmetic))
              m_armor->setChestCosmeticItem(as<ChestArmor>(overrideItem));
          }
          if (auto legsOverride = loungeAnchor->armorCosmeticOverrides.maybe("legs")) {
            auto overrideItem = itemDatabase->item(ItemDescriptor(*legsOverride));
            if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::LegsCosmetic))
              m_armor->setLegsCosmeticItem(as<LegsArmor>(overrideItem));
          }
          if (auto backOverride = loungeAnchor->armorCosmeticOverrides.maybe("back")) {
            auto overrideItem = itemDatabase->item(ItemDescriptor(*backOverride));
            if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::BackCosmetic))
              m_armor->setBackCosmeticItem(as<BackArmor>(overrideItem));
          }
        } else {
          m_state = State::Idle;
          m_movementController->resetAnchorState();
        }
      } else {
        m_movementController->resetAnchorState();
        m_statusController->setPersistentEffects("lounging", {});
      }

      if (!forceNude())
        m_armor->effects(*m_effectEmitter);

      m_tools->effects(*m_effectEmitter);

      auto aimRelative = world()->geometry().diff(m_aimPosition, position()); // dumb, but due to how things are ordered
      m_movementController->tickMaster(dt);
      m_aimPosition = position() + aimRelative;                               // it's gonna have to be like this for now

      m_techController->tickMaster(dt);

      for (auto& p : m_genericScriptContexts)
        p.second->update(p.second->updateDt(dt));

      if (edgeTriggeredUse) {
        auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor());
        bool useTool = canUseTool();
        if (anchor && (!useTool || anchor->controllable))
          m_movementController->resetAnchorState();
        else if (useTool) {
          if (auto ie = bestInteractionEntity(true))
            interactWithEntity(ie);
        }
      }

      m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
      m_statusController->setPersistentEffects("tools", m_tools->statusEffects());

      if (!m_techController->techOverridden())
        m_techController->setLoadedTech(m_techs->equippedTechs().values());

      if (!isDead())
        m_statusController->tickMaster(dt);

      if (!modeConfig().hunger)
        m_statusController->resetResource("food");

      if (!m_statusController->resourcePositive("food"))
        m_statusController->setPersistentEffects("hunger", m_foodEmptyStatusEffects);
      else if (m_statusController->resourcePercentage("food").value() <= m_foodLowThreshold)
        m_statusController->setPersistentEffects("hunger", m_foodLowStatusEffects);
      else
        m_statusController->setPersistentEffects("hunger", {});

      for (auto& pair : m_delayedRadioMessages) {
        if (pair.first.tick(dt))
          queueRadioMessage(pair.second);
      }
      m_delayedRadioMessages.filter([](pair<GameTimer, RadioMessage>& pair) { return !pair.first.ready(); });
    }

    if (m_isAdmin) {
      m_statusController->resetResource("health");
      m_statusController->resetResource("energy");
      m_statusController->resetResource("food");
      m_statusController->resetResource("breath");
    }

    m_log->addPlayTime(GlobalTimestep);

    if (m_ageItemsTimer.wrapTick(dt)) {
      auto itemDatabase = Root::singleton().itemDatabase();
      m_inventory->forEveryItem([&](InventorySlot const&, ItemPtr& item) {
          itemDatabase->ageItem(item, m_ageItemsTimer.time);
        });
    }

    for (auto& tool : {m_tools->primaryHandItem(), m_tools->altHandItem()}) {
      if (auto inspectionTool = as<InspectionTool>(tool)) {
        for (auto& ir : inspectionTool->pullInspectionResults()) {
          if (ir.objectName) {
            m_questManager->receiveMessage("objectScanned", true, {*ir.objectName, *ir.entityId});
            m_log->addScannedObject(*ir.objectName);
          }

          addChatMessage(ir.message, JsonObject{
            {"message", JsonObject{
              {"context", JsonObject{{"mode", "RadioMessage"}}},
              {"fromConnection", world()->connection()},
              {"text", ir.message}
            }}
          });
        }
      }
    }

    m_interestingObjects = m_questManager->interestingObjects();

  } else {
    m_netGroup.tickNetInterpolation(dt);
    m_movementController->tickSlave(dt);
    m_techController->tickSlave(dt);
    m_statusController->tickSlave(dt);
  }

  humanoid()->setRotation(m_movementController->rotation());

  bool suppressedItems = !canUseTool();

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->dance)
    humanoid()->setDance(*loungeAnchor->dance);
  else if ((!suppressedItems && (m_tools->primaryHandItem() || m_tools->altHandItem()))
    || humanoid()->danceCyclicOrEnded() || m_movementController->running())
    humanoid()->setDance({});

  bool isClient = world()->isClient();

  m_tools->suppressItems(suppressedItems);
  m_tools->tick(dt, m_shifting, m_pendingMoves);

  Direction facingDirection = m_movementController->facingDirection();

  auto overrideFacingDirection = m_tools->setupHumanoidHandItems(*humanoid(), position(), aimPosition());
  if (overrideFacingDirection)
    m_movementController->controlFace(facingDirection = *overrideFacingDirection);

  humanoid()->setFacingDirection(facingDirection);
  humanoid()->setMovingBackwards(facingDirection != m_movementController->movingDirection());

  refreshHumanoid();

  auto scale = Mat3F::scaling(Vec2F(facingDirection == Direction::Right ? 1.f : -1.f, 1.f));
  m_effectsAnimator->setTransformationGroup("flip", scale);

  if (m_state == State::Walk || m_state == State::Run) {
    if ((m_footstepTimer += dt) > m_config->footstepTiming) {
      m_footstepPending = true;
      m_footstepTimer = 0.0;
    }
  }

  if (isClient) {
    m_effectsAnimator->update(dt, &m_effectsAnimatorDynamicTarget);
    m_effectsAnimatorDynamicTarget.updatePosition(position() + m_techController->parentOffset());
  } else {
    m_effectsAnimator->update(dt, nullptr);
  }

  if (!isTeleporting())
    processStateChanges(dt);

  m_damageSources = m_tools->damageSources();
  for (auto& damageSource : m_damageSources) {
    damageSource.sourceEntityId = entityId();
    damageSource.team = getTeam();
  }

  m_songbook->update(*entityMode(), world());

  m_effectEmitter->setSourcePosition("normal", position());
  m_effectEmitter->setSourcePosition("mouth", mouthOffset() + position());
  m_effectEmitter->setSourcePosition("feet", feetOffset() + position());
  m_effectEmitter->setSourcePosition("headArmor", headArmorOffset() + position());
  m_effectEmitter->setSourcePosition("chestArmor", chestArmorOffset() + position());
  m_effectEmitter->setSourcePosition("legsArmor", legsArmorOffset() + position());
  m_effectEmitter->setSourcePosition("backArmor", backArmorOffset() + position());

  m_effectEmitter->setSourcePosition("primary", handPosition(ToolHand::Primary) + position());
  m_effectEmitter->setSourcePosition("alt", handPosition(ToolHand::Alt) + position());

  m_effectEmitter->setDirection(facingDirection);

  m_effectEmitter->tick(dt, *entityMode());

  if (isClient) {
    bool calculateHeadRotation = isMaster();
    if (!calculateHeadRotation) {
      auto headRotationProperty = getSecretProperty("humanoid.headRotation");
      if (headRotationProperty.isType(Json::Type::Float)) {
        humanoid()->setHeadRotation(headRotationProperty.toFloat());
      } else
        calculateHeadRotation = true;
    }
    if (calculateHeadRotation) { // master or not an OpenStarbound player
      float headRotation = 0.f;
      if (Humanoid::globalHeadRotation() && (humanoid()->handHoldingItem(ToolHand::Primary) || humanoid()->handHoldingItem(ToolHand::Alt) || humanoid()->dance())) {
        auto primary = m_tools->primaryHandItem();
        auto alt = m_tools->altHandItem();
        String const disableFlag = "disableHeadRotation";
        auto statusFlag = m_statusController->statusProperty(disableFlag);
        if (!(statusFlag.isType(Json::Type::Bool) && statusFlag.toBool())
         && !(primary && primary->instanceValue(disableFlag))
         && !(alt && alt->instanceValue(disableFlag))) {
          auto diff = world()->geometry().diff(aimPosition(), mouthPosition());
          diff.setX(fabsf(diff.x()));
          headRotation = diff.angle() * .25f * numericalDirection(humanoid()->facingDirection());
        }
      }
      humanoid()->setHeadRotation(headRotation);
      if (isMaster())
        setSecretProperty("humanoid.headRotation", headRotation);
    }
  }

  m_pendingMoves.clear();

  if (isClient)
    SpatialLogger::logPoly("world", m_movementController->collisionBody(), isMaster() ? Color::Orange.toRgba() : Color::Yellow.toRgba());
}

float Player::timeSinceLastGaveDamage() const {
  return m_lastDamagedOtherTimer;
}

EntityId Player::lastDamagedTarget() const {
  return m_lastDamagedTarget;
}

void Player::render(RenderCallback* renderCallback) {
  if (invisible()) {
    m_techController->pullNewAudios();
    m_techController->pullNewParticles();
    m_statusController->pullNewAudios();
    m_statusController->pullNewParticles();

    humanoid()->networkedAnimatorDynamicTarget()->pullNewAudios();
    humanoid()->networkedAnimatorDynamicTarget()->pullNewParticles();
    return;
  }

  Vec2I footstepSensor = Vec2I((m_config->footstepSensor + m_movementController->position()).floor());
  String footstepSound = getFootstepSound(footstepSensor);

  if (!footstepSound.empty() && !m_techController->parentState() && !m_techController->parentHidden()) {
    auto footstepAudio = Root::singleton().assets()->audio(footstepSound);
    if (m_landingNoisePending) {
      auto landingNoise = make_shared<AudioInstance>(*footstepAudio);
      landingNoise->setPosition(position() + feetOffset());
      landingNoise->setVolume(m_landingVolume);
      renderCallback->addAudio(std::move(landingNoise));
    }

    if (m_footstepPending) {
      auto stepNoise = make_shared<AudioInstance>(*footstepAudio);
      stepNoise->setPosition(position() + feetOffset());
      stepNoise->setVolume(1 - Random::randf(0, m_footstepVolumeVariance));
      renderCallback->addAudio(std::move(stepNoise));
    }
  } else {
    m_footstepTimer = m_config->footstepTiming;
  }
  m_footstepPending = false;
  m_landingNoisePending = false;

  renderCallback->addAudios(m_effectsAnimatorDynamicTarget.pullNewAudios());
  renderCallback->addParticles(m_effectsAnimatorDynamicTarget.pullNewParticles());

  renderCallback->addAudios(m_techController->pullNewAudios());
  renderCallback->addAudios(m_statusController->pullNewAudios());
  renderCallback->addAudios(humanoid()->networkedAnimatorDynamicTarget()->pullNewAudios());

  for (auto const& p : take(m_callbackSounds)) {
    auto audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(get<0>(p)));
    audio->setVolume(get<1>(p));
    audio->setPitchMultiplier(get<2>(p));
    audio->setPosition(position());
    renderCallback->addAudio(std::move(audio));
  }

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  EntityRenderLayer renderLayer = loungeAnchor ? loungeAnchor->loungeRenderLayer : RenderLayerPlayer;

  renderCallback->addDrawables(drawables(), renderLayer);
  if (!isTeleporting())
    renderCallback->addOverheadBars(bars(), position());
  renderCallback->addParticles(particles());

  m_tools->render(renderCallback, inToolRange(), m_shifting, renderLayer);

  m_effectEmitter->render(renderCallback);
  m_songbook->render(renderCallback);

  if (isMaster())
    m_deployment->render(renderCallback, position());
}

void Player::renderLightSources(RenderCallback* renderCallback) {
  renderCallback->addLightSources(lightSources());
  m_deployment->renderLightSources(renderCallback);
}

Json Player::getGenericProperty(String const& name, Json const& defaultValue) const {
  return m_genericProperties.value(name, defaultValue);
}

void Player::setGenericProperty(String const& name, Json const& value) {
  if (value.isNull())
    m_genericProperties.erase(name);
  else
    m_genericProperties.set(name, value);
}

PlayerInventoryPtr Player::inventory() const {
  return m_inventory;
}

uint64_t Player::itemsCanHold(ItemPtr const& items) const {
  return m_inventory->itemsCanFit(items);
}

ItemPtr Player::pickupItems(ItemPtr const& items, bool silent) {
  if (isDead() || !items || m_inventory->itemsCanFit(items) == 0)
    return items;

  triggerPickupEvents(items);

  if (!silent) {
    if (items->pickupSound().size()) {
      m_effectsAnimator->setSoundPool("pickup", {items->pickupSound()});
      float pitch = 1.f - ((float)items->count() / (float)items->maxStack()) * 0.5f;
      m_effectsAnimator->setSoundPitchMultiplier("pickup", clamp(pitch * Random::randf(0.8f, 1.2f), 0.f, 2.f));
      m_effectsAnimator->playSound("pickup");
    }
    auto itemDb = Root::singleton().itemDatabase();
    queueItemPickupMessage(itemDb->itemShared(items->descriptor()));
  }

  return m_inventory->addItems(items);
}

void Player::giveItem(ItemPtr const& item) {
  if (auto spill = pickupItems(item))
    world()->addEntity(ItemDrop::createRandomizedDrop(spill->descriptor(), position()));
}

void Player::triggerPickupEvents(ItemPtr const& item) {
  if (item) {
    for (auto b : item->learnBlueprintsOnPickup())
      addBlueprint(b);

    for (auto pair : item->collectablesOnPickup())
      addCollectable(pair.first, pair.second);

    for (auto m : item->instanceValue("radioMessagesOnPickup", JsonArray()).iterateArray()) {
      if (m.isType(Json::Type::Array)) {
        if (m.size() >= 2 && m.get(1).canConvert(Json::Type::Float))
          queueRadioMessage(m.get(0), m.get(1).toFloat());
      } else {
        queueRadioMessage(m);
      }
    }

    if (auto cinematic = item->instanceValue("cinematicOnPickup", Json()))
      setPendingCinematic(cinematic, true);

    for (auto const& quest : item->pickupQuestTemplates()) {
      if (m_questManager->canStart(quest))
        m_questManager->offer(make_shared<Quest>(quest, 0, this));
    }

    if (auto consume = item->instanceValue("consumeOnPickup", Json())) {
      if (consume.toBool())
        item->consume(item->count());
    }

    statistics()->recordEvent("item", JsonObject{
        {"itemName", item->name()},
        {"count", item->count()},
        {"category", item->instanceValue("eventCategory", item->category())}
      });
  }
}

ItemPtr Player::essentialItem(EssentialItem essentialItem) const {
  return m_inventory->essentialItem(essentialItem);
}

bool Player::hasItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  return m_inventory->hasItem(descriptor, exactMatch);
}

uint64_t Player::hasCountOfItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  return m_inventory->hasCountOfItem(descriptor, exactMatch);
}

ItemDescriptor Player::takeItem(ItemDescriptor const& descriptor, bool consumePartial, bool exactMatch) {
  return m_inventory->takeItems(descriptor, consumePartial, exactMatch);
}

void Player::giveItem(ItemDescriptor const& descriptor) {
  giveItem(Root::singleton().itemDatabase()->item(descriptor));
}

void Player::clearSwap() {
  // If we cannot put the swap slot back into the bag, then just drop it in the
  // world.
  if (!m_inventory->clearSwap()) {
    if (auto world = worldPtr())
      world->addEntity(ItemDrop::createRandomizedDrop(m_inventory->takeSlot(SwapSlot()), position()));
  }

  // Interrupt all firing in case the item being dropped was in use.
  endPrimaryFire();
  endAltFire();
  endTrigger();
}

void Player::refreshItems() {
  if (isSlave())
    return;

  m_tools->setItems(m_inventory->primaryHeldItem(), m_inventory->secondaryHeldItem());
}

void Player::refreshArmor() {
  if (isSlave())
    return;

  bool shouldSetArmorSecrets = m_clientContext && m_clientContext->netCompatibilityRules().version() < 9;
  for (uint8_t i = 0; i != 20; ++i) {
    auto slot = (EquipmentSlot)i;
    auto item = m_inventory->equipment(slot);
    bool visible = m_inventory->equipmentVisibility(slot);
    if (m_armor->setItem(i, item, visible)) {
      if (slot >= EquipmentSlot::Cosmetic1 && shouldSetArmorSecrets)
        setNetArmorSecret(slot, item, visible);
    }
  }
}

void Player::refreshHumanoid() const {
  try {
    if (m_armor->setupHumanoid(*humanoid(), forceNude())) {
      m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), humanoid()->playerMovementParameters().value(m_config->movementParameters))));
    }
  }
  catch (std::exception const&) {
    if (isMaster()) // it's your problem,
      throw;        // deal with it!
  }
}

void Player::refreshEquipment() {
  refreshArmor();
  refreshItems();
}

PlayerBlueprintsPtr Player::blueprints() const {
  return m_blueprints;
}

bool Player::addBlueprint(ItemDescriptor const& descriptor, bool showFailure) {
  if (descriptor.isNull())
    return false;

  auto itemDb = Root::singleton().itemDatabase();
  auto item = itemDb->item(descriptor);
  auto assets = Root::singleton().assets();
  if (!m_blueprints->isKnown(descriptor)) {
    m_blueprints->add(descriptor);
    queueUIMessage(assets->json("/player.config:blueprintUnlock").toString().replace("<ItemName>", item->friendlyName()));
    return true;
  } else if (showFailure) {
    queueUIMessage(assets->json("/player.config:blueprintAlreadyKnown").toString().replace("<ItemName>", item->friendlyName()));
  }

  return false;
}

bool Player::blueprintKnown(ItemDescriptor const& descriptor) const {
  if (descriptor.isNull())
    return false;

  return m_blueprints->isKnown(descriptor);
}

bool Player::addCollectable(String const& collectionName, String const& collectableName) {
  if (m_log->addCollectable(collectionName, collectableName)) {
    auto collectionDatabase = Root::singleton().collectionDatabase();

    auto collection = collectionDatabase->collection(collectionName);
    auto collectable = collectionDatabase->collectable(collectionName, collectableName);
    queueUIMessage(Root::singleton().assets()->json("/player.config:collectableUnlock").toString().replace("<collectable>", collectable.title).replace("<collection>", collection.title));
    return true;
  } else {
    return false;
  }
}

PlayerUniverseMapPtr Player::universeMap() const {
  return m_universeMap;
}

PlayerCodexesPtr Player::codexes() const {
  return m_codexes;
}

PlayerTechPtr Player::techs() const {
  return m_techs;
}

void Player::overrideTech(Maybe<StringList> const& techModules) {
  if (techModules)
    m_techController->setOverrideTech(*techModules);
  else
    m_techController->clearOverrideTech();
}

bool Player::techOverridden() const {
  return m_techController->techOverridden();
}

PlayerCompanionsPtr Player::companions() const {
  return m_companions;
}

PlayerLogPtr Player::log() const {
  return m_log;
}

InteractiveEntityPtr Player::bestInteractionEntity(bool includeNearby) {
  if (!inWorld())
    return {};

  InteractiveEntityPtr interactiveEntity;
  if (auto entity = world()->getInteractiveInRange(m_aimPosition, isAdmin() ? m_aimPosition : position(), m_interactRadius)) {
    interactiveEntity = entity;
  } else if (includeNearby) {
    Vec2F interactBias = m_walkIntoInteractBias;
    if (facingDirection() == Direction::Left)
      interactBias[0] *= -1;
    Vec2F pos = position() + interactBias;

    if (auto entity = world()->getInteractiveInRange(pos, position(), m_interactRadius))
      interactiveEntity = entity;
  }

  if (interactiveEntity && (isAdmin() || world()->canReachEntity(position(), interactRadius(), interactiveEntity->entityId())))
    return interactiveEntity;
  return {};
}

void Player::interactWithEntity(InteractiveEntityPtr entity) {
  bool questIntercepted = false;
  for (auto quest : m_questManager->listActiveQuests()) {
    if (quest->interactWithEntity(entity->entityId()))
      questIntercepted = true;
  }
  if (questIntercepted)
    return;

  bool anyTurnedIn = false;

  for (auto questId : entity->turnInQuests()) {
    if (m_questManager->canTurnIn(questId)) {
      auto const& quest = m_questManager->getQuest(questId);
      quest->setEntityParameter("questReceiver", entity);
      m_questManager->getQuest(questId)->complete();
      anyTurnedIn = true;
    }
  }

  if (anyTurnedIn)
    return;

  for (auto questArc : entity->offeredQuests()) {
    if (m_questManager->canStart(questArc)) {
      auto quest = make_shared<Quest>(questArc, 0, this);
      quest->setWorldId(clientContext()->playerWorldId());
      quest->setServerUuid(clientContext()->serverUuid());
      quest->setEntityParameter("questGiver", entity);
      m_questManager->offer(quest);
      return;
    }
  }

  m_pendingInteractActions.append(world()->interact(InteractRequest{
        entityId(), position(), entity->entityId(), aimPosition()}));
}

void Player::aim(Vec2F const& position) {
  m_techController->setAimPosition(position);
  m_aimPosition = position;
}

Vec2F Player::aimPosition() const {
  return m_aimPosition;
}

Vec2F Player::armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const {
  return m_tools->armPosition(*humanoid(), hand, facingDirection, armAngle, offset);
}

Vec2F Player::handOffset(ToolHand hand, Direction facingDirection) const {
  return m_tools->handOffset(*humanoid(), hand, facingDirection);
}

Vec2F Player::handPosition(ToolHand hand, Vec2F const& handOffset) const {
  return m_tools->handPosition(hand, *humanoid(), handOffset);
}

ItemPtr Player::handItem(ToolHand hand) const {
  if (hand == ToolHand::Primary)
    return m_tools->primaryHandItem();
  else
    return m_tools->altHandItem();
}

Vec2F Player::armAdjustment() const {
  return humanoid()->armAdjustment();
}

void Player::setCameraFocusEntity(Maybe<EntityId> const& cameraFocusEntity) {
  m_cameraFocusEntity = cameraFocusEntity;
}

void Player::playEmote(HumanoidEmote emote) {
  addEmote(emote);
}

bool Player::canUseTool() const {
  bool canUse = !isDead() && !isTeleporting() && !m_techController->toolUsageSuppressed();
  if (canUse) {
    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor()))
      if (loungeAnchor->suppressTools.value(loungeAnchor->controllable))
        return false;
  }
  return canUse;
}

void Player::beginPrimaryFire() {
  m_techController->beginPrimaryFire();
  m_tools->beginPrimaryFire();
}

void Player::beginAltFire() {
  m_techController->beginAltFire();
  m_tools->beginAltFire();
}

void Player::endPrimaryFire() {
  m_techController->endPrimaryFire();
  m_tools->endPrimaryFire();
}

void Player::endAltFire() {
  m_techController->endAltFire();
  m_tools->endAltFire();
}

void Player::beginTrigger() {
  if (!m_useDown)
    m_edgeTriggeredUse = true;
  m_useDown = true;
}

void Player::endTrigger() {
  m_useDown = false;
}

float Player::toolRadius() const {
  auto radius = m_tools->toolRadius();
  if (radius)
    return *radius;
  return interactRadius();
}

float Player::interactRadius() const {
  return m_interactRadius;
}

void Player::setInteractRadius(float interactRadius) {
  m_interactRadius = interactRadius;
}

List<InteractAction> Player::pullInteractActions() {
  List<InteractAction> results;
  eraseWhere(m_pendingInteractActions, [&results](auto& promise) {
      if (auto res = promise.result())
        results.append(res.take());
      return promise.finished();
    });
  return results;
}

uint64_t Player::currency(String const& currencyType) const {
  return m_inventory->currency(currencyType);
}

float Player::health() const {
  return m_statusController->resource("health");
}

float Player::maxHealth() const {
  return *m_statusController->resourceMax("health");
}

DamageBarType Player::damageBar() const {
  return DamageBarType::Default;
}

float Player::healthPercentage() const {
  return *m_statusController->resourcePercentage("health");
}

float Player::energy() const {
  return m_statusController->resource("energy");
}

float Player::maxEnergy() const {
  return *m_statusController->resourceMax("energy");
}

float Player::energyPercentage() const {
  return *m_statusController->resourcePercentage("energy");
}

float Player::energyRegenBlockPercent() const {
  return *m_statusController->resourcePercentage("energyRegenBlock");
}

bool Player::fullEnergy() const {
  return energy() >= maxEnergy();
}

bool Player::energyLocked() const {
  return m_statusController->resourceLocked("energy");
}

bool Player::consumeEnergy(float energy) {
  if (m_isAdmin)
    return true;
  return m_statusController->overConsumeResource("energy", energy);
}

float Player::foodPercentage() const {
  return *m_statusController->resourcePercentage("food");
}

float Player::breath() const {
  return m_statusController->resource("breath");
}

float Player::maxBreath() const {
  return *m_statusController->resourceMax("breath");
}

float Player::protection() const {
  return m_statusController->stat("protection");
}

bool Player::forceNude() const {
  return m_statusController->statPositive("nude");
}

String Player::description() const {
  return m_description;
}

void Player::setDescription(String const& description) {
  m_description = description;
}

Direction Player::walkingDirection() const {
  return m_movementController->movingDirection();
}

Direction Player::facingDirection() const {
  return m_movementController->facingDirection();
}

pair<ByteArray, uint64_t> Player::writeNetState(uint64_t fromVersion, NetCompatibilityRules rules) {
  return m_netGroup.writeNetState(fromVersion, rules);
}

void Player::readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules) {
  m_netGroup.readNetState(data, interpolationTime, rules);
}

void Player::enableInterpolation(float) {
  m_netGroup.enableNetInterpolation();
}

void Player::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

void Player::processControls() {
  bool run = !m_shifting && !m_statusController->statPositive("encumberance");

  bool useMoveVector = m_moveVector.x() != 0.0f;
  if (useMoveVector) {
    for (auto move : m_pendingMoves) {
      if (move == MoveControlType::Left || move == MoveControlType::Right) {
        useMoveVector = false;
        break;
      }
    }
  }

  if (useMoveVector) {
    m_pendingMoves.insert(m_moveVector.x() < 0.0f ? MoveControlType::Left : MoveControlType::Right);
    m_movementController->setMoveSpeedMultiplier(clamp(abs(m_moveVector.x()), 0.0f, 1.0f));
  }
  else
    m_movementController->setMoveSpeedMultiplier(1.0f);

  if (auto fireableMain = as<FireableItem>(m_tools->primaryHandItem())) {
    if (fireableMain->inUse() && fireableMain->walkWhileFiring())
      run = false;
  }

  if (auto fireableAlt = as<FireableItem>(m_tools->altHandItem())) {
    if (fireableAlt->inUse() && fireableAlt->walkWhileFiring())
      run = false;
  }

  bool move = true;

  if (auto fireableMain = as<FireableItem>(m_tools->primaryHandItem())) {
    if (fireableMain->inUse() && fireableMain->stopWhileFiring())
      move = false;
  }

  if (auto fireableAlt = as<FireableItem>(m_tools->altHandItem())) {
    if (fireableAlt->inUse() && fireableAlt->stopWhileFiring())
      move = false;
  }

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->controllable) {
    auto anchorState = m_movementController->anchorState();
    if (auto loungeableEntity = world()->get<LoungeableEntity>(anchorState->entityId)) {
      for (auto move : m_pendingMoves) {
        if (move == MoveControlType::Up)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Up);
        else if (move == MoveControlType::Down)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Down);
        else if (move == MoveControlType::Left)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Left);
        else if (move == MoveControlType::Right)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Right);
        else if (move == MoveControlType::Jump)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Jump);
      }
      if (m_tools->firingPrimary())
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::PrimaryFire);
      if (m_tools->firingAlt())
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::AltFire);
      if (m_shifting)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Walk);
      loungeableEntity->loungeAim(anchorState->positionIndex, m_aimPosition);
    }
    move = false;
  }

  m_techController->setShouldRun(run);

  if (move) {
    for (auto move : m_pendingMoves) {
      switch (move) {
        case MoveControlType::Right:
          m_techController->moveRight();
          break;
        case MoveControlType::Left:
          m_techController->moveLeft();
          break;
        case MoveControlType::Up:
          m_techController->moveUp();
          break;
        case MoveControlType::Down:
          m_techController->moveDown();
          break;
        case MoveControlType::Jump:
          m_techController->jump();
          break;
      }
    }
  }

  if (m_state == State::Lounge && !m_pendingMoves.empty() && move)
    stopLounging();
}

void Player::processStateChanges(float dt) {
  if (isMaster()) {

    // Set the current player state based on what movement controller tells us
    // we're doing and do some state transition logic
    State oldState = m_state;

    if (m_movementController->zeroG()) {
      if (m_movementController->flying())
        m_state = State::Swim;
      else if (m_state != State::Lounge)
        m_state = State::SwimIdle;
    } else if (m_movementController->groundMovement()) {
      if (m_movementController->running()) {
        m_state = State::Run;
      } else if (m_movementController->walking()) {
        m_state = State::Walk;
      } else if (m_movementController->crouching()) {
        m_state = State::Crouch;
      } else {
        if (m_state != State::Lounge)
          m_state = State::Idle;
      }
    } else if (m_movementController->liquidMovement()) {
      if (m_movementController->jumping()) {
        m_state = State::Swim;
      } else {
        if (m_state != State::Lounge)
          m_state = State::SwimIdle;
      }
    } else {
      if (m_movementController->jumping()) {
        m_state = State::Jump;
      } else {
        if (m_movementController->falling()) {
          m_state = State::Fall;
        }
        if (m_movementController->velocity()[1] > 0) {
          if (m_state != State::Lounge)
            m_state = State::Jump;
        }
      }
    }

    if (m_moveVector.x() != 0.0f && (m_state == State::Run))
        m_state = abs(m_moveVector.x()) > 0.5f ? State::Run : State::Walk;

    if (m_state == State::Jump && (oldState == State::Idle || oldState == State::Run || oldState == State::Walk || oldState == State::Crouch))
      m_effectsAnimator->burstParticleEmitter("jump");

    if (!m_movementController->isNullColliding()) {
      if (oldState == State::Fall && oldState != m_state && m_state != State::Swim && m_state != State::SwimIdle
          && m_state != State::Jump) {
        m_effectsAnimator->burstParticleEmitter("landing");
        m_landedNetState.trigger();
        m_landingNoisePending = true;
      }
    }
  }

  humanoid()->animate(dt);
  m_scriptedAnimator.update();

  if (auto techState = m_techController->parentState()) {
    if (techState == TechController::ParentState::Stand) {
      humanoid()->setState(Humanoid::Idle);
    } else if (techState == TechController::ParentState::Fly) {
      humanoid()->setState(Humanoid::Jump);
    } else if (techState == TechController::ParentState::Fall) {
      humanoid()->setState(Humanoid::Fall);
    } else if (techState == TechController::ParentState::Sit) {
      humanoid()->setState(Humanoid::Sit);
    } else if (techState == TechController::ParentState::Lay) {
      humanoid()->setState(Humanoid::Lay);
    } else if (techState == TechController::ParentState::Duck) {
      humanoid()->setState(Humanoid::Duck);
    } else if (techState == TechController::ParentState::Walk) {
      humanoid()->setState(Humanoid::Walk);
    } else if (techState == TechController::ParentState::Run) {
      humanoid()->setState(Humanoid::Run);
    } else if (techState == TechController::ParentState::Swim) {
      humanoid()->setState(Humanoid::Swim);
    } else if (techState == TechController::ParentState::SwimIdle) {
      humanoid()->setState(Humanoid::SwimIdle);
    }
  } else {
    auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
    if (m_state == State::Idle) {
      humanoid()->setState(Humanoid::Idle);
    } else if (m_state == State::Walk) {
      humanoid()->setState(Humanoid::Walk);
    } else if (m_state == State::Run) {
      humanoid()->setState(Humanoid::Run);
    } else if (m_state == State::Jump) {
      humanoid()->setState(Humanoid::Jump);
    } else if (m_state == State::Fall) {
      humanoid()->setState(Humanoid::Fall);
    } else if (m_state == State::Swim) {
      humanoid()->setState(Humanoid::Swim);
    } else if (m_state == State::SwimIdle) {
      humanoid()->setState(Humanoid::SwimIdle);
    } else if (m_state == State::Crouch) {
      humanoid()->setState(Humanoid::Duck);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Sit) {
      humanoid()->setState(Humanoid::Sit);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Lay) {
      humanoid()->setState(Humanoid::Lay);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Stand) {
      humanoid()->setState(Humanoid::Idle);
    }
  }

  humanoid()->setEmoteState(m_emoteState);
}

String Player::getFootstepSound(Vec2I const& sensor) const {
  auto materialDatabase = Root::singleton().materialDatabase();

  String fallback = materialDatabase->defaultFootstepSound();
  List<Vec2I> scanOrder{{0, 0}, {0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}};
  for (auto subSensor : scanOrder) {
    String footstepSound = materialDatabase->footstepSound(world()->material(sensor + subSensor, TileLayer::Foreground),
        world()->mod(sensor + subSensor, TileLayer::Foreground));
    if (!footstepSound.empty()) {
      if (footstepSound != fallback) {
        return footstepSound;
      }
    }
  }
  return fallback;
}

bool Player::inInteractionRange() const {
  return inInteractionRange(centerOfTile(aimPosition()));
}

bool Player::inInteractionRange(Vec2F aimPos) const {
  return isAdmin() || world()->geometry().diff(aimPos, position()).magnitude() < interactRadius();
}

bool Player::inToolRange() const {
  return inToolRange(centerOfTile(aimPosition()));
}

bool Player::inToolRange(Vec2F const& aimPos) const {
  return isAdmin() || world()->geometry().diff(aimPos, position()).magnitude() < toolRadius();
}

void Player::getNetStates(bool initial) {
  m_state = (State)m_stateNetState.get();
  m_shifting = m_shiftingNetState.get();
  m_aimPosition[0] = m_xAimPositionNetState.get();
  m_aimPosition[1] = m_yAimPositionNetState.get();

  if (m_identityNetState.pullUpdated() && !initial) {
    auto newIdentity = m_identityNetState.get();
    if ((m_identity.species == newIdentity.species) && (m_identity.imagePath == newIdentity.imagePath)) {
      humanoid()->setIdentity(newIdentity);
    }
    m_identity = newIdentity;
  }
  if (m_refreshedHumanoidParameters.pullOccurred() && !initial) {
    refreshHumanoidParameters();
  }

  setTeam(m_teamNetState.get());

  if (m_landedNetState.pullOccurred() && !initial)
    m_landingNoisePending = true;

  if (m_newChatMessageNetState.pullOccurred() && !initial) {
    m_chatMessage = m_chatMessageNetState.get();
    m_chatMessageUpdated = true;
    m_pendingChatActions.append(SayChatAction{entityId(), m_chatMessage, m_movementController->position()});
  }

  m_emoteState = HumanoidEmoteNames.getLeft(m_emoteNetState.get());

  getNetArmorSecrets();
}

void Player::setNetStates() {
  m_stateNetState.set((unsigned)m_state);
  m_shiftingNetState.set(m_shifting);
  m_xAimPositionNetState.set(m_aimPosition[0]);
  m_yAimPositionNetState.set(m_aimPosition[1]);

  if (m_identityUpdated) {
    m_identityNetState.push(m_identity);
    m_identityUpdated = false;
  }

  m_teamNetState.set(getTeam());

  if (m_chatMessageChanged) {
    m_chatMessageChanged = false;
    m_chatMessageNetState.push(m_chatMessage);
    m_newChatMessageNetState.trigger();
  }

  m_emoteNetState.set(HumanoidEmoteNames.getRight(m_emoteState));
}

void Player::setNetArmorSecret(EquipmentSlot slot, ArmorItemPtr const& armor, bool visible) {
  String const& slotName = EquipmentSlotNames.getRight(slot);
  ItemDescriptor descriptor = visible ? itemSafeDescriptor(armor) : ItemDescriptor();
  setSecretProperty(strf("armorWearer.{}.data", slotName), descriptor.diskStore());
  if (m_armorSecretNetVersions.empty())
    setSecretProperty("armorWearer.replicating", true);
  setSecretProperty(strf("armorWearer.{}.version", slotName), ++m_armorSecretNetVersions[slot]);
}

void Player::setNetArmorSecrets(bool includeEmpty) {
  if (m_clientContext && m_clientContext->netCompatibilityRules().version() < 9) {
    for (uint8_t i = 0; i != 12; ++i) {
      auto slot = EquipmentSlot((uint8_t)EquipmentSlot::Cosmetic1 + i);
      auto item = as<ArmorItem>(m_inventory->itemsAt(slot));
      bool visible = m_inventory->equipmentVisibility(slot);
      if ((item && visible) || includeEmpty)
        setNetArmorSecret(slot, item, visible);
    }
  }
}

void Player::getNetArmorSecrets() {
  if (isSlave() && getSecretPropertyPtr("armorWearer.replicating") != nullptr) {
    auto itemDatabase = Root::singleton().itemDatabase();

    for (uint8_t i = 0; i != 12; ++i) {
      auto slot = EquipmentSlot((uint8_t)EquipmentSlot::Cosmetic1 + i);
      String const& slotName = EquipmentSlotNames.getRight(slot);
      auto& curVersion = m_armorSecretNetVersions[slot];
      uint64_t newVersion = 0;
      if (auto jVersion = getSecretProperty(strf("armorWearer.{}.version", slotName), 0); jVersion.isType(Json::Type::Int))
        newVersion = jVersion.toUInt();
      if (newVersion > curVersion) {
        curVersion = newVersion;
        ArmorItemPtr item;
        itemDatabase->diskLoad(getSecretProperty(strf("armorWearer.{}.data", slotName)), item);
        m_inventory->setItem(slot, item);
        m_armor->setCosmeticItem(i, item);
      }
    }
  }
}

void Player::setAdmin(bool isAdmin) {
  m_isAdmin = isAdmin;
}

bool Player::isAdmin() const {
  return m_isAdmin;
}

void Player::setFavoriteColor(Color color) {
  m_identity.color = color.toRgba();
  updateIdentity();
}

Color Player::favoriteColor() const {
  return Color::rgba(m_identity.color);
}

bool Player::isTeleporting() const {
  return (m_state == State::TeleportIn) || (m_state == State::TeleportOut);
}

bool Player::isTeleportingOut() const {
  return inWorld() && (m_state == State::TeleportOut) && m_teleportTimer >= 0.0f;
}

bool Player::canDeploy() {
  return m_deployment->canDeploy();
}

void Player::deployAbort(String const& animationType) {
  m_teleportAnimationType = animationType;
  m_deployment->setDeploying(false);
}

bool Player::isDeploying() const {
  return m_deployment->isDeploying();
}

bool Player::isDeployed() const {
  return m_deployment->isDeployed();
}

void Player::setBusyState(PlayerBusyState busyState) {
  m_effectsAnimator->setState("busy", PlayerBusyStateNames.getRight(busyState));
}

void Player::teleportOut(String const& animationType, bool deploy) {
  m_state = State::TeleportOut;
  m_teleportAnimationType = animationType;
  m_effectsAnimator->setState("teleport", m_teleportAnimationType + "Out");
  m_deployment->setDeploying(deploy);
  m_deployment->teleportOut();
  m_teleportTimer = deploy ? m_config->deployOutTime : m_config->teleportOutTime;
}

void Player::teleportIn() {
  m_state = State::TeleportIn;
  m_effectsAnimator->setState("teleport", m_teleportAnimationType + "In");
  m_teleportTimer = m_deployment->isDeployed() ? m_config->deployInTime : m_config->teleportInTime;

  auto statusEffects = Root::singleton().assets()->json("/player.config:teleportInStatusEffects").toArray().transformed(jsonToEphemeralStatusEffect);
  m_statusController->addEphemeralEffects(statusEffects);
}

void Player::teleportAbort() {
  m_state = State::TeleportIn;
  m_effectsAnimator->setState("teleport", "abort");
  m_deployment->setDeploying(m_deployment->isDeployed());
  m_teleportTimer = m_config->teleportInTime;
}

void Player::moveTo(Vec2F const& footPosition) {
  m_movementController->setPosition(footPosition - feetOffset());
  m_movementController->setVelocity(Vec2F());
}

ItemPtr Player::primaryHandItem() const {
  return m_tools->primaryHandItem();
}

ItemPtr Player::altHandItem() const {
  return m_tools->altHandItem();
}

Uuid Player::uuid() const {
  return Uuid(*uniqueId());
}

PlayerMode Player::modeType() const {
  return m_modeType;
}

void Player::setModeType(PlayerMode mode) {
  m_modeType = mode;

  auto assets = Root::singleton().assets();
  m_modeConfig = PlayerModeConfig(assets->json("/playermodes.config").get(PlayerModeNames.getRight(mode)));
}

PlayerModeConfig Player::modeConfig() const {
  return m_modeConfig;
}

ShipUpgrades Player::shipUpgrades() {
  return m_shipUpgrades;
}

void Player::setShipUpgrades(ShipUpgrades shipUpgrades) {
  m_shipUpgrades = std::move(shipUpgrades);
}

void Player::applyShipUpgrades(Json const& upgrades) {
  if (m_clientContext->playerUuid() == uuid())
    m_clientContext->rpcInterface()->invokeRemote("ship.applyShipUpgrades", upgrades);
  else
    m_shipUpgrades.apply(upgrades);
}

String Player::name() const {
  return m_identity.name;
}

void Player::setName(String const& name) {
  m_identity.name = name;
  updateIdentity();
}

Maybe<String> Player::statusText() const {
  return {};
}

bool Player::displayNametag() const {
  return true;
}

Vec3B Player::nametagColor() const {
  auto assets = Root::singleton().assets();
  return jsonToVec3B(assets->json("/player.config:nametagColor"));
}

Vec2F Player::nametagOrigin() const {
  return mouthPosition(false);
}

String Player::nametag() const {
  if (auto jNametag = getSecretProperty("nametag"); jNametag.isType(Json::Type::String))
    return jNametag.toString();
  else
    return name();
}

void Player::setNametag(Maybe<String> nametag) {
  setSecretProperty("nametag", nametag ? Json(*nametag) : Json());
}

void Player::updateIdentity() {
  m_identityUpdated = true;
  auto oldIdentity = humanoid()->identity();
  if ((m_identity.species != oldIdentity.species) || (m_identity.imagePath != oldIdentity.imagePath)) {
    refreshHumanoidParameters();
  } else {
    humanoid()->setIdentity(m_identity);
  }
}

void Player::setHumanoidParameter(String key, Maybe<Json> value) {
  if (value.isValid())
    m_humanoidParameters.set(key, value.value());
  else
    m_humanoidParameters.erase(key);

  m_netHumanoid.netElements().last()->setHumanoidParameters(m_humanoidParameters);
}

Maybe<Json> Player::getHumanoidParameter(String key) {
  return m_humanoidParameters.maybe(key);
}

void Player::setHumanoidParameters(JsonObject parameters) {
  m_humanoidParameters = parameters;

  m_netHumanoid.netElements().last()->setHumanoidParameters(m_humanoidParameters);
}

JsonObject Player::getHumanoidParameters() {
  return m_humanoidParameters;
}

void Player::setBodyDirectives(String const& directives)
{ m_identity.bodyDirectives = directives; updateIdentity(); }

void Player::setEmoteDirectives(String const& directives)
{ m_identity.emoteDirectives = directives; updateIdentity(); }

void Player::setHairGroup(String const& group)
{ m_identity.hairGroup = group; updateIdentity(); }

void Player::setHairType(String const& type)
{ m_identity.hairType = type; updateIdentity(); }

void Player::setHairDirectives(String const& directives)
{ m_identity.hairDirectives = directives; updateIdentity(); }

void Player::setFacialHairGroup(String const& group)
{ m_identity.facialHairGroup = group; updateIdentity(); }

void Player::setFacialHairType(String const& type)
{ m_identity.facialHairType = type; updateIdentity(); }

void Player::setFacialHairDirectives(String const& directives)
{ m_identity.facialHairDirectives = directives; updateIdentity(); }

void Player::setFacialMaskGroup(String const& group)
{ m_identity.facialMaskGroup = group; updateIdentity(); }

void Player::setFacialMaskType(String const& type)
{ m_identity.facialMaskType = type; updateIdentity(); }

void Player::setFacialMaskDirectives(String const& directives)
{ m_identity.facialMaskDirectives = directives; updateIdentity(); }

void Player::setHair(String const& group, String const& type, String const& directives) {
  m_identity.hairGroup = group;
  m_identity.hairType = type;
  m_identity.hairDirectives = directives;
  updateIdentity();
}

void Player::setFacialHair(String const& group, String const& type, String const& directives) {
  m_identity.facialHairGroup = group;
  m_identity.facialHairType = type;
  m_identity.facialHairDirectives = directives;
  updateIdentity();
}

void Player::setFacialMask(String const& group, String const& type, String const& directives) {
  m_identity.facialMaskGroup = group;
  m_identity.facialMaskType = type;
  m_identity.facialMaskDirectives = directives;
  updateIdentity();
}

void Player::setSpecies(String const& species) {
  Root::singleton().speciesDatabase()->species(species); // throw if non-existent
  m_identity.species = species;
  updateIdentity();
}

Gender Player::gender() const {
  return m_identity.gender;
}

void Player::setGender(Gender const& gender) {
  m_identity.gender = gender;
  updateIdentity();
}

String Player::species() const {
  return m_identity.species;
}

void Player::setPersonality(Personality const& personality) {
  m_identity.personality = personality;
  updateIdentity();
}

void Player::setImagePath(Maybe<String> const& imagePath) {
  m_identity.imagePath = imagePath;
  updateIdentity();
}

HumanoidPtr Player::humanoid() {
  return m_netHumanoid.netElements().last()->humanoid();
}
HumanoidPtr Player::humanoid() const {
  return m_netHumanoid.netElements().last()->humanoid();
}

HumanoidIdentity const& Player::identity() const {
  return m_identity;
}

void Player::setIdentity(HumanoidIdentity identity) {
  m_identity = std::move(identity);
  updateIdentity();
}

List<String> Player::pullQueuedMessages() {
  return take(m_queuedMessages);
}

List<ItemPtr> Player::pullQueuedItemDrops() {
  return take(m_queuedItemPickups);
}

void Player::queueUIMessage(String const& message) {
  if (!isSlave())
    m_queuedMessages.append(message);
}

void Player::queueItemPickupMessage(ItemPtr const& item) {
  if (!isSlave())
    m_queuedItemPickups.append(item);
}

void Player::addChatMessage(String const& message, Json const& config) {
  starAssert(!isSlave());
  m_chatMessage = message;
  m_chatMessageUpdated = true;
  m_chatMessageChanged = true;
  m_pendingChatActions.append(SayChatAction{entityId(), message, mouthPosition(), config});
}

void Player::addEmote(HumanoidEmote const& emote, Maybe<float> emoteCooldown) {
  starAssert(!isSlave());
  m_emoteState = emote;
  m_emoteCooldownTimer = emoteCooldown.value(m_emoteCooldown);
}

pair<HumanoidEmote, float> Player::currentEmote() const {
  return make_pair(m_emoteState, m_emoteCooldownTimer);
}

Player::State Player::currentState() const {
  return m_state;
}

List<ChatAction> Player::pullPendingChatActions() {
  return take(m_pendingChatActions);
}

Maybe<String> Player::inspectionLogName() const {
  auto identifier = uniqueId();
  if (String* str = identifier.ptr()) {
    auto hash = XXH3_128bits(str->utf8Ptr(), str->utf8Size());
    return String("Player #") + hexEncode((const char*)&hash, sizeof(hash));
  }
  return identifier;
}

Maybe<String> Player::inspectionDescription(String const&) const {
  return m_description;
}

float Player::beamGunRadius() const {
  return m_tools->beamGunRadius();
}

bool Player::instrumentPlaying() {
  return m_songbook->instrumentPlaying();
}

void Player::instrumentEquipped(String const& instrumentKind) {
  if (canUseTool())
    m_songbook->keepAlive(instrumentKind, mouthPosition());
}

void Player::interact(InteractAction const& action) {
  starAssert(!isSlave());
  m_pendingInteractActions.append(RpcPromise<InteractAction>::createFulfilled(action));
}

void Player::addEffectEmitters(StringSet const& emitters) {
  starAssert(!isSlave());
  m_effectEmitter->addEffectSources("normal", emitters);
}

void Player::requestEmote(String const& emote) {
  auto state = HumanoidEmoteNames.getLeft(emote);
  if (state != HumanoidEmote::Idle
      && (m_emoteState == state || m_emoteState == HumanoidEmote::Idle || m_emoteState == HumanoidEmote::Blink))
    addEmote(state);
}

ActorMovementController* Player::movementController() {
  return m_movementController.get();
}

StatusController* Player::statusController() {
  return m_statusController.get();
}

List<PhysicsForceRegion> Player::forceRegions() const {
  return m_tools->forceRegions();
}


StatusControllerPtr Player::statusControllerPtr() {
  return m_statusController;
}

ActorMovementControllerPtr Player::movementControllerPtr() {
  return m_movementController;
}

PlayerConfigPtr Player::config() {
  return m_config;
}

SongbookPtr Player::songbook() const {
  return m_songbook;
}

QuestManagerPtr Player::questManager() const {
  return m_questManager;
}

Json Player::diskStore() {
  JsonObject genericScriptStorage;
  for (auto& p : m_genericScriptContexts) {
    auto scriptStorage = p.second->getScriptStorage();
    if (!scriptStorage.empty())
      genericScriptStorage[p.first] = std::move(scriptStorage);
  }

  return JsonObject{
    {"uuid", *uniqueId()},
    {"description", m_description},
    {"modeType", PlayerModeNames.getRight(m_modeType)},
    {"shipUpgrades", m_shipUpgrades.toJson()},
    {"blueprints", m_blueprints->toJson()},
    {"universeMap", m_universeMap->toJson()},
    {"codexes", m_codexes->toJson()},
    {"techs", m_techs->toJson()},
    {"identity", m_identity.toJson()},
    {"team", getTeam().toJson()},
    {"inventory", m_inventory->store()},
    {"movementController", m_movementController->storeState()},
    {"techController", m_techController->diskStore()},
    {"statusController", m_statusController->diskStore()},
    {"log", m_log->toJson()},
    {"aiState", m_aiState.toJson()},
    {"quests", m_questManager->diskStore()},
    {"companions", m_companions->diskStore()},
    {"deployment", m_deployment->diskStore()},
    {"genericProperties", m_genericProperties},
    {"genericScriptStorage", genericScriptStorage},
    {"humanoidParameters", m_humanoidParameters},
  };
}

ByteArray Player::netStore(NetCompatibilityRules rules) {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);

  ds.write(*uniqueId());
  ds.write(m_description);
  ds.write(m_modeType);
  ds.write(m_identity);
  if (rules.version() >= 10)
    ds.write(m_humanoidParameters);

  return ds.data();
}

void Player::finalizeCreation() {
  m_blueprints = make_shared<PlayerBlueprints>();
  m_techs = make_shared<PlayerTech>();

  auto itemDatabase = Root::singleton().itemDatabase();
  for (auto const& descriptor : m_config->defaultItems)
    m_inventory->addItems(itemDatabase->item(descriptor));

  for (auto const& descriptor : Root::singleton().speciesDatabase()->species(m_identity.species)->defaultItems())
    m_inventory->addItems(itemDatabase->item(descriptor));

  for (auto const& descriptor : m_config->defaultBlueprints)
    m_blueprints->add(descriptor);

  for (auto const& descriptor : Root::singleton().speciesDatabase()->species(m_identity.species)->defaultBlueprints())
    m_blueprints->add(descriptor);

  refreshEquipment();

  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_effectEmitter->reset();

  m_description = strf("This {} seems to have nothing to say for {}self.",
    m_identity.gender == Gender::Male ? "guy" : "gal",
    m_identity.gender == Gender::Male ? "him" : "her");
}

bool Player::invisible() const {
  return m_statusController->statPositive("invisible");
}

void Player::animatePortrait(float dt) {
  humanoid()->animate(dt);
  if (m_emoteCooldownTimer) {
    m_emoteCooldownTimer -= dt;
    if (m_emoteCooldownTimer <= 0) {
      m_emoteCooldownTimer = 0;
      m_emoteState = HumanoidEmote::Idle;
    }
  }
  humanoid()->setEmoteState(m_emoteState);
}

bool Player::isOutside() {
  if (!inWorld())
    return false;
  return !world()->isUnderground(position())
      && !world()->tileIsOccupied(Vec2I::floor(mouthPosition()), TileLayer::Background);
}

void Player::dropSelectedItems(function<bool(ItemPtr)> filter) {
  if (!world())
    return;

  m_inventory->forEveryItem([&](InventorySlot const&, ItemPtr& item) {
      if (item && (!filter || filter(item)))
        world()->addEntity(ItemDrop::throwDrop(take(item), position(), velocity(), Vec2F::withAngle(Random::randf(-Constants::pi, Constants::pi)), true));
    });
}

void Player::dropEverything() {
  dropSelectedItems({});
}

bool Player::isPermaDead() const {
  if (!isDead())
    return false;
  return modeConfig().permadeath;
}

bool Player::interruptRadioMessage() {
  if (m_interruptRadioMessage) {
    m_interruptRadioMessage = false;
    return true;
  }
  return false;
}

Maybe<RadioMessage> Player::pullPendingRadioMessage() {
  if (m_pendingRadioMessages.count()) {
    if (m_pendingRadioMessages.at(0).unique)
      m_log->addRadioMessage(m_pendingRadioMessages.at(0).messageId);
    return m_pendingRadioMessages.takeFirst();
  }
  return {};
}

void Player::queueRadioMessage(Json const& messageConfig, float delay) {
  RadioMessage message;
  try {
    message = Root::singleton().radioMessageDatabase()->createRadioMessage(messageConfig);

    if (message.type == RadioMessageType::Tutorial && !Root::singleton().configuration()->get("tutorialMessages").toBool())
      return;

    // non-absolute portrait image paths are assumed to be a frame name within the player's species-specific AI
    if (!message.portraitImage.empty() && message.portraitImage[0] != '/')
      message.portraitImage = Root::singleton().aiDatabase()->portraitImage(species(), message.portraitImage);
  } catch (RadioMessageDatabaseException const& e) {
    Logger::error("Couldn't queue radio message '{}': {}", messageConfig, e.what());
    return;
  }

  if (m_log->radioMessages().contains(message.messageId)) {
    return;
  } else {
    if (message.type == RadioMessageType::Mission) {
      if (m_missionRadioMessages.contains(message.messageId))
        return;
      else
        m_missionRadioMessages.add(message.messageId);
    }

    for (RadioMessage const& pendingMessage : m_pendingRadioMessages) {
      if (pendingMessage.messageId == message.messageId)
        return;
    }
    for (auto& delayedMessagePair : m_delayedRadioMessages) {
      if (delayedMessagePair.second.messageId == message.messageId) {
        if (delay == 0)
          delayedMessagePair.first.setDone();
        return;
      }
    }
  }

  if (delay > 0) {
    m_delayedRadioMessages.append(pair<GameTimer, RadioMessage>{GameTimer(delay), message});
  } else {
    queueRadioMessage(message);
  }
}

void Player::queueRadioMessage(RadioMessage message) {
  if (message.important) {
    m_interruptRadioMessage = true;
    m_pendingRadioMessages.prepend(message);
  } else {
    m_pendingRadioMessages.append(message);
  }
}

Maybe<Json> Player::pullPendingCinematic() {
  if (m_pendingCinematic && m_pendingCinematic->isType(Json::Type::String))
    m_log->addCinematic(m_pendingCinematic->toString());
  return take(m_pendingCinematic);
}

void Player::setPendingCinematic(Json const& cinematic, bool unique) {
  if (unique && cinematic.isType(Json::Type::String) && m_log->cinematics().contains(cinematic.toString()))
    return;
  m_pendingCinematic = cinematic;
}

void Player::setInCinematic(bool inCinematic) {
  if (inCinematic)
    m_statusController->setPersistentEffects("cinematic", m_inCinematicStatusEffects);
  else
    m_statusController->setPersistentEffects("cinematic", {});
}

Maybe<pair<Maybe<pair<StringList, int>>, float>> Player::pullPendingAltMusic() {
  if (m_pendingAltMusic)
    return m_pendingAltMusic.take();
  return {};
}

Maybe<PlayerWarpRequest> Player::pullPendingWarp() {
  if (m_pendingWarp)
    return m_pendingWarp.take();
  return {};
}

void Player::setPendingWarp(String const& action, Maybe<String> const& animation, bool deploy) {
  m_pendingWarp = PlayerWarpRequest{action, animation, deploy};
}

Maybe<pair<Json, RpcPromiseKeeper<Json>>> Player::pullPendingConfirmation() {
  if (m_pendingConfirmations.count() > 0)
    return m_pendingConfirmations.takeFirst();
  return {};
}

void Player::queueConfirmation(Json const& dialogConfig, RpcPromiseKeeper<Json> const& resultPromise) {
  m_pendingConfirmations.append(make_pair(dialogConfig, resultPromise));
}

AiState const& Player::aiState() const {
  return m_aiState;
}

AiState& Player::aiState() {
  return m_aiState;
}

bool Player::inspecting() const {
  return is<InspectionTool>(m_tools->primaryHandItem()) || is<InspectionTool>(m_tools->altHandItem());
}

EntityHighlightEffect Player::inspectionHighlight(InspectableEntityPtr const& inspectableEntity) const {
  auto inspectionTool = as<InspectionTool>(m_tools->primaryHandItem());
  if (!inspectionTool)
    inspectionTool = as<InspectionTool>(m_tools->altHandItem());

  if (!inspectionTool)
    return EntityHighlightEffect();

  if (auto name = inspectableEntity->inspectionLogName()) {
    auto ehe = EntityHighlightEffect();
    ehe.level = inspectionTool->inspectionHighlightLevel(inspectableEntity);
    if (ehe.level > 0) {
      if (m_interestingObjects.contains(*name))
        ehe.type = EntityHighlightEffectType::Interesting;
      else if (m_log->scannedObjects().contains(*name))
        ehe.type = EntityHighlightEffectType::Inspected;
      else
        ehe.type = EntityHighlightEffectType::Inspectable;
    }
    return ehe;
  }

  return EntityHighlightEffect();
}

Vec2F Player::cameraPosition() {
  if (inWorld()) {
    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
      if (loungeAnchor->cameraFocus) {
        if (auto anchoredEntity = world()->entity(m_movementController->anchorState()->entityId))
          return anchoredEntity->position();
      }
    }

    if (m_cameraFocusEntity) {
      if (auto focusedEntity = world()->entity(*m_cameraFocusEntity))
        return focusedEntity->position();
      else
        m_cameraFocusEntity = {};
    }
  }
  return position();
}

NetworkedAnimatorPtr Player::effectsAnimator() {
  return m_effectsAnimator;
}

const String secretProprefix = "\0JsonProperty\0"s;

Maybe<StringView> Player::getSecretPropertyView(String const& name) const {
  if (auto tag = m_effectsAnimator->globalTagPtr(secretProprefix + name)) {
    auto& view = tag->utf8();
    DataStreamExternalBuffer buffer(view.data(), view.size());
    try {
      uint8_t typeIndex = buffer.read<uint8_t>() - 1;
      if ((Json::Type)typeIndex == Json::Type::String) {
        size_t len = buffer.readVlqU();
        size_t pos = buffer.pos();
        if (pos + len == buffer.size())
          return StringView(buffer.ptr() + pos, len);
      }
    }
    catch (StarException const& e) {}
  }

  return {};
}

String const* Player::getSecretPropertyPtr(String const& name) const {
  return m_effectsAnimator->globalTagPtr(secretProprefix + name);
}

Json Player::getSecretProperty(String const& name, Json defaultValue) const {
  if (auto tag = m_effectsAnimator->globalTagPtr(secretProprefix + name)) {
    DataStreamExternalBuffer buffer(tag->utf8Ptr(), tag->utf8Size());
    try
      { return buffer.read<Json>(); }
    catch (StarException const& e)
      { Logger::error("Exception reading secret player property '{}': {}", name, e.what()); }
  }

  return defaultValue;
}

void Player::setSecretProperty(String const& name, Json const& value) {
  if (value) {
    DataStreamBuffer ds;
    ds.write(value);
    auto& data = ds.data();
    m_effectsAnimator->setGlobalTag(secretProprefix + name, String(data.ptr(), data.size()));
  }
  else
    m_effectsAnimator->removeGlobalTag(secretProprefix + name);
}

void Player::refreshHumanoidParameters() {
  auto speciesDatabase = Root::singleton().speciesDatabase();
  auto speciesDef = speciesDatabase->species(m_identity.species);

  if (isMaster() || !inWorld()) {
    m_refreshedHumanoidParameters.trigger();
    m_netHumanoid.clearNetElements();
    m_netHumanoid.addNetElement(make_shared<NetHumanoid>(m_identity, m_humanoidParameters, Json()));
    m_effectsAnimator->setGlobalTag("effectDirectives", speciesDef->effectDirectives());
    m_deathParticleBurst.set(humanoid()->defaultDeathParticles());
    m_statusController->setStatusProperty("ouchNoise", speciesDef->ouchNoise(m_identity.gender));
    m_scriptedAnimationParameters.clear();
  } else {
    m_humanoidParameters = m_netHumanoid.netElements().last()->humanoidParameters();
  }
  auto armor = m_armor->diskStore();
  m_armor->reset();
  m_armor->diskLoad(armor);
  m_armor->setupHumanoid(*humanoid(), forceNude());

  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(humanoid()->defaultMovementParameters(), humanoid()->playerMovementParameters().value(m_config->movementParameters))));

  if (inWorld()) {
    if (isMaster()) {
      for (auto& p : m_genericScriptContexts) {
        if (p.second->initialized()) {
          p.second->removeCallbacks("animator");
          p.second->addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(humanoid()->networkedAnimator()));
          p.second->invoke("refreshHumanoidParameters");
        }
      }
    }
    if (world()->isClient()) {
      m_scriptedAnimator.uninit();
      m_scriptedAnimator.removeCallbacks("animationConfig");
      m_scriptedAnimator.removeCallbacks("entity");

      m_scriptedAnimator.setScripts(humanoid()->animationScripts());
      m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(humanoid()->networkedAnimator(),
        [this](String const& name, Json const& defaultValue) -> Json {
          return m_scriptedAnimationParameters.value(name, defaultValue);
        }));
      m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
      m_scriptedAnimator.init(world());
    }
  }
}

void Player::setAnimationParameter(String name, Json value) {
  m_scriptedAnimationParameters.set(std::move(name), std::move(value));
}

}
