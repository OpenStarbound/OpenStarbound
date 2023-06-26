#include "StarMonster.hpp"
#include "StarWorld.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarDamageManager.hpp"
#include "StarDamageDatabase.hpp"
#include "StarTreasure.hpp"
#include "StarJsonExtra.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarScriptedAnimatorLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarBehaviorLuaBindings.hpp"
#include "StarStoredFunctions.hpp"
#include "StarItemDrop.hpp"
#include "StarAssets.hpp"
#include "StarTime.hpp"
#include "StarStatusController.hpp"

namespace Star {

Monster::Monster(MonsterVariant const& monsterVariant, Maybe<float> level) {
  m_monsterLevel = level;

  m_damageOnTouch = false;
  m_aggressive = false;

  m_knockedOut = false;
  m_knockoutTimer = 0.0;

  m_dropPool = monsterVariant.dropPoolConfig;

  m_monsterVariant = monsterVariant;

  m_questIndicatorOffset = jsonToVec2F(Root::singleton().assets()->json("/quests/quests.config:defaultIndicatorOffset"));

  setTeam(EntityDamageTeam(m_monsterVariant.damageTeamType, m_monsterVariant.damageTeam));

  m_networkedAnimator = NetworkedAnimator(m_monsterVariant.animatorConfig);
  for (auto const& pair : m_monsterVariant.animatorPartTags)
    m_networkedAnimator.setPartTag(pair.first, "partImage", pair.second);
  m_networkedAnimator.setZoom(m_monsterVariant.animatorZoom);
  auto colorSwap = m_monsterVariant.colorSwap.value(Root::singleton().monsterDatabase()->colorSwap(m_monsterVariant.parameters.getString("colors", "default"), m_monsterVariant.seed));
  if (!colorSwap.empty())
    m_networkedAnimator.setProcessingDirectives(imageOperationToString(ColorReplaceImageOperation{colorSwap}));

  m_statusController = make_shared<StatusController>(m_monsterVariant.statusSettings);

  m_scriptComponent.setScripts(m_monsterVariant.parameters.optArray("scripts").apply(jsonToStringList).value(m_monsterVariant.scripts));
  m_scriptComponent.setUpdateDelta(m_monsterVariant.initialScriptDelta);

  auto movementParameters = ActorMovementParameters::sensibleDefaults().merge(ActorMovementParameters(monsterVariant.movementSettings));
  if (movementParameters.standingPoly)
    movementParameters.standingPoly->scale(m_monsterVariant.animatorZoom);
  if (movementParameters.crouchingPoly)
    movementParameters.crouchingPoly->scale(m_monsterVariant.animatorZoom);
  *movementParameters.walkSpeed *= monsterVariant.walkMultiplier;
  *movementParameters.runSpeed *= monsterVariant.runMultiplier;
  *movementParameters.airJumpProfile.jumpSpeed *= monsterVariant.jumpMultiplier;
  *movementParameters.liquidJumpProfile.jumpSpeed *= monsterVariant.jumpMultiplier;
  *movementParameters.mass *= monsterVariant.weightMultiplier;
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet{"monster"};
  m_movementController = make_shared<ActorMovementController>(movementParameters);

  setPersistent(m_monsterVariant.persistent);

  setupNetStates();
  setNetStates();
}

Monster::Monster(Json const& diskStore)
  : Monster(Root::singleton().monsterDatabase()->readMonsterVariantFromJson(diskStore.get("monsterVariant"))) {
  m_monsterLevel = diskStore.optFloat("monsterLevel");
  m_movementController->loadState(diskStore.get("movementState"));
  m_statusController->diskLoad(diskStore.get("statusController"));
  m_damageOnTouch = diskStore.getBool("damageOnTouch");
  m_aggressive = diskStore.getBool("aggressive");
  m_deathParticleBurst = diskStore.getString("deathParticleBurst");
  m_deathSound = diskStore.getString("deathSound");
  m_activeSkillName = diskStore.getString("activeSkillName");
  m_dropPool = diskStore.get("dropPool");
  m_effectEmitter.fromJson(diskStore.get("effectEmitter"));
  m_scriptComponent.setScriptStorage(diskStore.getObject("scriptStorage"));

  setUniqueId(diskStore.optString("uniqueId"));
  if (diskStore.contains("team"))
    setTeam(EntityDamageTeam(diskStore.get("team")));
}

Json Monster::diskStore() const {
  return JsonObject{
    {"monsterLevel", jsonFromMaybe(m_monsterLevel)},
    {"movementState", m_movementController->storeState()},
    {"statusController", m_statusController->diskStore()},
    {"damageOnTouch", m_damageOnTouch},
    {"aggressive", aggressive()},
    {"deathParticleBurst", m_deathParticleBurst},
    {"deathSound", m_deathSound},
    {"activeSkillName", m_activeSkillName},
    {"dropPool", m_dropPool},
    {"effectEmitter", m_effectEmitter.toJson()},
    {"monsterVariant", Root::singleton().monsterDatabase()->writeMonsterVariantToJson(m_monsterVariant)},
    {"scriptStorage", m_scriptComponent.getScriptStorage()},
    {"uniqueId", jsonFromMaybe(uniqueId())},
    {"team", getTeam().toJson()}
  };
}

ByteArray Monster::netStore() {
  return Root::singleton().monsterDatabase()->writeMonsterVariant(m_monsterVariant);
}

EntityType Monster::entityType() const {
  return EntityType::Monster;
}

ClientEntityMode Monster::clientEntityMode() const {
  return m_monsterVariant.clientEntityMode;
}

void Monster::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);

  m_movementController->init(world);
  m_movementController->setIgnorePhysicsEntities({entityId});
  m_statusController->init(this, m_movementController.get());

  if (!m_monsterLevel)
    m_monsterLevel = world->threatLevel();

  if (isMaster()) {
    auto functionDatabase = Root::singleton().functionDatabase();
    float healthMultiplier = m_monsterVariant.healthMultiplier * functionDatabase->function(m_monsterVariant.healthLevelFunction)->evaluate(*m_monsterLevel);
    m_statusController->setPersistentEffects("innate", {StatModifier(StatBaseMultiplier{"maxHealth", healthMultiplier})});

    m_scriptComponent.addCallbacks("monster", makeMonsterCallbacks());
    m_scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
        return m_monsterVariant.parameters.query(name, def);
      }));
    m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptComponent.addCallbacks("animator", LuaBindings::makeNetworkedAnimatorCallbacks(&m_networkedAnimator));
    m_scriptComponent.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_statusController.get()));
    m_scriptComponent.addCallbacks("behavior", LuaBindings::makeBehaviorLuaCallbacks(&m_behaviors));
    m_scriptComponent.addActorMovementCallbacks(m_movementController.get());
    m_scriptComponent.init(world);
  }

  if (world->isClient()) {
    m_scriptedAnimator.setScripts(m_monsterVariant.animationScripts);

    m_scriptedAnimator.addCallbacks("animationConfig", LuaBindings::makeScriptedAnimatorCallbacks(&m_networkedAnimator,
      [this](String const& name, Json const& defaultValue) -> Json {
        return m_scriptedAnimationParameters.value(name, defaultValue);
      }));
    m_scriptedAnimator.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
        return m_monsterVariant.parameters.query(name, def);
      }));
    m_scriptedAnimator.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
    m_scriptedAnimator.init(world);
  }

  setPosition(position());
}

void Monster::uninit() {
  if (isMaster()) {
    m_scriptComponent.uninit();
    m_scriptComponent.removeCallbacks("monster");
    m_scriptComponent.removeCallbacks("config");
    m_scriptComponent.removeCallbacks("entity");
    m_scriptComponent.removeCallbacks("animator");
    m_scriptComponent.removeCallbacks("status");
    m_scriptComponent.removeActorMovementCallbacks();
  }
  if (world()->isClient()) {
    m_scriptedAnimator.removeCallbacks("animationConfig");
    m_scriptedAnimator.removeCallbacks("config");
    m_scriptedAnimator.removeCallbacks("entity");
  }
  m_statusController->uninit();
  m_movementController->uninit();
  Entity::uninit();
}

Vec2F Monster::mouthOffset() const {
  return getAbsolutePosition(m_monsterVariant.mouthOffset) - position();
}

Vec2F Monster::feetOffset() const {
  return getAbsolutePosition(m_monsterVariant.feetOffset) - position();
}

Vec2F Monster::position() const {
  return m_movementController->position();
}

RectF Monster::metaBoundBox() const {
  return m_monsterVariant.metaBoundBox;
}

RectF Monster::collisionArea() const {
  return m_movementController->collisionPoly().boundBox();
}

Vec2F Monster::velocity() const {
  return m_movementController->velocity();
}

pair<ByteArray, uint64_t> Monster::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Monster::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

void Monster::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Monster::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

String Monster::description() const {
  return m_monsterVariant.description.value("Some indescribable horror");
}

Maybe<HitType> Monster::queryHit(DamageSource const& source) const {
  if (!inWorld() || m_knockedOut || m_statusController->statPositive("invulnerable"))
    return {};

  if (source.intersectsWithPoly(world()->geometry(), hitPoly().get()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Monster::hitPoly() const {
  PolyF hitBody = m_monsterVariant.selfDamagePoly;
  hitBody.rotate(m_movementController->rotation());
  hitBody.translate(position());
  return hitBody;
}

List<DamageNotification> Monster::applyDamage(DamageRequest const& damage) {
  if (!inWorld())
    return {};

  auto notifications = m_statusController->applyDamageRequest(damage);

  float totalDamage = 0.0f;
  for (auto const& notification : notifications)
    totalDamage += notification.healthLost;

  if (totalDamage > 0.0f) {
    m_scriptComponent.invoke("damage", JsonObject{
        {"sourceId", damage.sourceEntityId},
        {"damage", totalDamage},
        {"sourceDamage", damage.damage},
        {"sourceKind", damage.damageSourceKind}
      });
  }

  if (!m_statusController->resourcePositive("health"))
    m_deathDamageSourceKinds.add(damage.damageSourceKind);

  return notifications;
}

List<DamageNotification> Monster::selfDamageNotifications() {
  return m_statusController->pullSelfDamageNotifications();
}

List<DamageSource> Monster::damageSources() const {
  List<DamageSource> damageSources = m_damageSources.get();

  float levelPowerMultiplier = Root::singleton().functionDatabase()->function(m_monsterVariant.powerLevelFunction)->evaluate(*m_monsterLevel);
  if (m_damageOnTouch && !m_monsterVariant.touchDamageConfig.isNull()) {
    DamageSource damageSource(m_monsterVariant.touchDamageConfig);
    if (auto damagePoly = damageSource.damageArea.ptr<PolyF>())
      damagePoly->rotate(m_movementController->rotation());
    damageSource.damage *= m_monsterVariant.touchDamageMultiplier * levelPowerMultiplier * m_statusController->stat("powerMultiplier");
    damageSource.sourceEntityId = entityId();
    damageSource.team = getTeam();
    damageSources.append(damageSource);
  }

  auto animationDamageParts = m_monsterVariant.animationDamageParts;
  for (auto pair : m_monsterVariant.animationDamageParts) {
    if (!m_animationDamageParts.get().contains(pair.first))
      continue;

    String anchorPart = pair.second.getString("anchorPart");
    DamageSource ds = DamageSource(pair.second.get("damageSource"));
    ds.damage *= levelPowerMultiplier * m_statusController->stat("powerMultiplier");
    ds.damageArea.call([this,&anchorPart](auto& poly) {
      poly.transform(m_networkedAnimator.partTransformation(anchorPart));
      if (m_networkedAnimator.flipped())
        poly.flipHorizontal(m_networkedAnimator.flippedRelativeCenterLine());
    });
    if (ds.knockback.is<Vec2F>()) {
      Vec2F knockback = ds.knockback.get<Vec2F>();
      knockback = m_networkedAnimator.partTransformation(anchorPart).transformVec2(knockback);
      if (m_networkedAnimator.flipped())
        knockback = Vec2F(-knockback[0], knockback[1]);
      ds.knockback = knockback;
    }

    List<DamageSource> partSources;
    if (auto line = ds.damageArea.maybe<Line2F>()) {
      if (pair.second.getBool("checkLineCollision", false)) {
        Line2F worldLine = line.value().translated(position());
        float length = worldLine.length();

        auto bounces = pair.second.getInt("bounces", 0);
        while (auto collision = world()->lineTileCollisionPoint(worldLine.min(), worldLine.max())) {
          worldLine = Line2F(worldLine.min(), collision.value().first);
          ds.damageArea = worldLine.translated(-position());
          length = length - worldLine.length();

          if (--bounces >= 0 && length > 0.0f) {
            partSources.append(ds);
            ds = DamageSource(ds);
            Vec2F dir = worldLine.direction();
            Vec2F normal = Vec2F((*collision).second);
            Vec2F reflection = dir - (2 * dir.piecewiseMultiply(normal).sum() * normal);
            if (ds.knockback.is<Vec2F>())
              ds.knockback = ds.knockback.get<Vec2F>().rotate(reflection.angleBetween(worldLine.direction()));

            worldLine.min() = (*collision).first;
            worldLine.max() = worldLine.min() + (reflection * length);
            ds.damageArea = worldLine.translated(-position());
          } else {
            break;
          }
        }
        partSources.append(ds);
      }
    } else {
      partSources.append(ds);
    }
    damageSources.appendAll(partSources);
  }

  return damageSources;
}

bool Monster::shouldDie() {
  if (auto res = m_scriptComponent.invoke<bool>("shouldDie"))
    return *res;
  else if (!m_statusController->resourcePositive("health") || m_scriptComponent.error())
    return true;
  else
    return false;
}

void Monster::knockout() {
  m_knockedOut = true;
  m_knockoutTimer = m_monsterVariant.parameters.getFloat("knockoutTime", 1.0f);

  m_damageOnTouch = false;

  String knockoutEffect = m_monsterVariant.parameters.getString("knockoutEffect");
  if (!knockoutEffect.empty())
    m_networkedAnimator.setEffectEnabled(knockoutEffect, true);

  auto knockoutAnimationStates = m_monsterVariant.parameters.getObject("knockoutAnimationStates", JsonObject());
  for (auto pair : knockoutAnimationStates)
    m_networkedAnimator.setState(pair.first, pair.second.toString());
}

bool Monster::shouldDestroy() const {
  return m_knockedOut && m_knockoutTimer <= 0;
}

void Monster::destroy(RenderCallback* renderCallback) {
  m_scriptComponent.invoke("die");

  if (isMaster() && !m_dropPool.isNull()) {
    auto treasureDatabase = Root::singleton().treasureDatabase();

    String treasurePool;
    if (m_dropPool.isType(Json::Type::String)) {
      treasurePool = m_dropPool.toString();
    } else {
      // Check to see whether any of the damage types that were used to cause
      // death are in the damage pool map, if so spawn treasure from that,
      // otherwise set the treasure pool to the "default" entry.

      for (auto const& damageSourceKind : m_deathDamageSourceKinds) {
        if (m_dropPool.contains(damageSourceKind))
          treasurePool = m_dropPool.getString(damageSourceKind);
      }

      if (treasurePool.empty())
        treasurePool = m_dropPool.getString("default");
    }

    for (auto const& treasureItem : treasureDatabase->createTreasure(treasurePool, *m_monsterLevel))
      world()->addEntity(ItemDrop::createRandomizedDrop(treasureItem, position()));
  }

  if (renderCallback) {
    if (!m_deathParticleBurst.empty())
      m_networkedAnimator.burstParticleEmitter(m_deathParticleBurst);

    if (!m_deathSound.empty())
      m_networkedAnimator.playSound(m_deathSound);

    m_networkedAnimator.update(0.0, &m_networkedAnimatorDynamicTarget);

    renderCallback->addAudios(m_networkedAnimatorDynamicTarget.pullNewAudios());
    renderCallback->addParticles(m_networkedAnimatorDynamicTarget.pullNewParticles());
    renderCallback->addParticles(m_statusController->pullNewParticles());
  }

  m_deathDamageSourceKinds.clear();

  if (isMaster())
    setNetStates();
}

List<LightSource> Monster::lightSources() const {
  auto lightSources = m_networkedAnimator.lightSources(position());
  lightSources.appendAll(m_statusController->lightSources());
  return lightSources;
}

void Monster::hitOther(EntityId targetEntityId, DamageRequest const& damageRequest) {
  if (inWorld() && isMaster())
    m_statusController->hitOther(targetEntityId, damageRequest);
}

void Monster::damagedOther(DamageNotification const& damage) {
  if (inWorld() && isMaster())
    m_statusController->damagedOther(damage);
}

void Monster::update(uint64_t) {
  if (!inWorld())
    return;

  if (isMaster()) {
    m_networkedAnimator.setFlipped((m_movementController->facingDirection() == Direction::Left) != m_monsterVariant.reversed);

    if (m_knockedOut) {
      m_knockoutTimer -= WorldTimestep;
    } else {
      if (m_scriptComponent.updateReady())
        m_physicsForces.set({});
      m_scriptComponent.update(m_scriptComponent.updateDt());

      if (shouldDie())
        knockout();
    }

    m_movementController->tickMaster();

    m_statusController->tickMaster();
    updateStatus();
  } else {
    m_netGroup.tickNetInterpolation(WorldTimestep);

    m_statusController->tickSlave();
    updateStatus();

    m_movementController->tickSlave();
  }

  if (world()->isServer()) {
    m_networkedAnimator.update(WorldTimestep, nullptr);
  } else {
    m_networkedAnimator.update(WorldTimestep, &m_networkedAnimatorDynamicTarget);
    m_networkedAnimatorDynamicTarget.updatePosition(position());

    m_scriptedAnimator.update();
  }

  SpatialLogger::logPoly("world", m_movementController->collisionBody(), {255, 0, 0, 255});
}

void Monster::render(RenderCallback* renderCallback) {
  for (auto& drawable : m_networkedAnimator.drawables(position())) {
    if (drawable.isImage())
      drawable.imagePart().addDirectivesGroup(m_statusController->parentDirectives(), true);
    renderCallback->addDrawable(move(drawable), m_monsterVariant.renderLayer);
  }

  renderCallback->addAudios(m_networkedAnimatorDynamicTarget.pullNewAudios());
  renderCallback->addParticles(m_networkedAnimatorDynamicTarget.pullNewParticles());

  renderCallback->addLightSources(m_networkedAnimator.lightSources(position()));

  renderCallback->addDrawables(m_statusController->drawables(), m_monsterVariant.renderLayer);
  renderCallback->addLightSources(m_statusController->lightSources());
  renderCallback->addParticles(m_statusController->pullNewParticles());
  renderCallback->addAudios(m_statusController->pullNewAudios());

  m_effectEmitter.render(renderCallback);

  for (auto drawablePair : m_scriptedAnimator.drawables())
    renderCallback->addDrawable(drawablePair.first, drawablePair.second.value(m_monsterVariant.renderLayer));
  renderCallback->addLightSources(m_scriptedAnimator.lightSources());
  renderCallback->addAudios(m_scriptedAnimator.pullNewAudios());
  renderCallback->addParticles(m_scriptedAnimator.pullNewParticles());
}

void Monster::setPosition(Vec2F const& pos) {
  m_movementController->setPosition(pos);
}

Maybe<Json> Monster::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  Maybe<Json> result = m_scriptComponent.handleMessage(message, world()->connection() == sendingConnection, args);
  if (!result)
    result = m_statusController->receiveMessage(message, world()->connection() == sendingConnection, args);
  return result;
}

float Monster::maxHealth() const {
  return *m_statusController->resourceMax("health");
}

float Monster::health() const {
  return m_statusController->resource("health");
}

DamageBarType Monster::damageBar() const {
  return m_damageBar.get();
}

Vec2F Monster::getAbsolutePosition(Vec2F relativePosition) const {
  if (m_movementController->facingDirection() == Direction::Left)
    relativePosition[0] *= -1;
  if (m_movementController->rotation() != 0)
    relativePosition = relativePosition.rotate(m_movementController->rotation());
  return m_movementController->position() + relativePosition;
}

void Monster::updateStatus() {
  m_effectEmitter.setSourcePosition("normal", position());
  m_effectEmitter.setSourcePosition("mouth", position() + mouthOffset());
  m_effectEmitter.setSourcePosition("feet", position() + feetOffset());
  m_effectEmitter.setDirection(m_movementController->facingDirection());
  m_effectEmitter.tick(*entityMode());
}

LuaCallbacks Monster::makeMonsterCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("type", [this]() {
      return m_monsterVariant.type;
    });

  callbacks.registerCallback("seed", [this]() {
      return strf("%d", m_monsterVariant.seed);
    });

  callbacks.registerCallback("uniqueParameters", [this]() {
      return m_monsterVariant.uniqueParameters;
    });

  callbacks.registerCallback("level", [this]() {
      return *m_monsterLevel;
    });

  callbacks.registerCallback("setDamageOnTouch", [this](bool arg1) {
      m_damageOnTouch = arg1;
    });

  callbacks.registerCallback("setDamageSources", [this](Maybe<JsonArray> const& damageSources) {
      m_damageSources.set(damageSources.value().transformed(construct<DamageSource>()));
    });

  callbacks.registerCallback("setDamageParts", [this](StringSet const& parts) {
      m_animationDamageParts.set(parts);
    });

  callbacks.registerCallback("setAggressive", [this](bool arg1) {
      m_aggressive = arg1;
    });

  callbacks.registerCallback("setActiveSkillName", [this](Maybe<String> const& activeSkillName) {
      m_activeSkillName = activeSkillName.value();
    });

  callbacks.registerCallback("setDropPool", [this](Json dropPool) {
      m_dropPool = move(dropPool);
    });

  callbacks.registerCallback("toAbsolutePosition", [this](Vec2F const& p) {
      return getAbsolutePosition(p);
    });

  callbacks.registerCallback("mouthPosition", [this]() {
      return mouthPosition();
    });

  // This callback is registered here rather than in
  // makeActorMovementControllerCallbacks
  // because it requires access to world
  callbacks.registerCallback("flyTo", [this](Vec2F const& arg1) {
      m_movementController->controlFly(world()->geometry().diff(arg1, position()));
    });

  callbacks.registerCallback("setDeathParticleBurst", [this](Maybe<String> const& arg1) {
      m_deathParticleBurst = arg1.value();
    });

  callbacks.registerCallback("setDeathSound", [this](Maybe<String> const& arg1) {
      m_deathSound = arg1.value();
    });

  callbacks.registerCallback("setPhysicsForces", [this](JsonArray const& forces) {
      m_physicsForces.set(forces.transformed(jsonToPhysicsForceRegion));
    });

  callbacks.registerCallback("setName", [this](String const& name) {
      m_name.set(name);
    });
  callbacks.registerCallback("setDisplayNametag", [this](bool display) {
      m_displayNametag.set(display);
    });

  callbacks.registerCallback("say", [this](String line, Maybe<StringMap<String>> const& tags) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("sayPortrait", [this](String line, String portrait, Maybe<StringMap<String>> const& tags) {
      if (tags)
        line = line.replaceTags(*tags, false);

      if (!line.empty()) {
        addChatMessage(line, portrait);
        return true;
      }

      return false;
    });

  callbacks.registerCallback("setDamageTeam", [this](Json const& team) {
      setTeam(EntityDamageTeam(team));
    });

  callbacks.registerCallback("setUniqueId", [this](Maybe<String> uniqueId) {
      setUniqueId(uniqueId);
    });

  callbacks.registerCallback("setDamageBar", [this](String const& damageBarType) {
      m_damageBar.set(DamageBarTypeNames.getLeft(damageBarType));
    });

  callbacks.registerCallback("setInteractive", [this](bool interactive) {
      m_interactive.set(interactive);
    });

  callbacks.registerCallback("setAnimationParameter", [this](String name, Json value) {
      m_scriptedAnimationParameters.set(move(name), move(value));
    });

  return callbacks;
}

void Monster::addChatMessage(String const& message, String const& portrait) {
  m_chatMessage.set(message);
  m_chatPortrait.set(portrait);
  m_newChatMessageEvent.trigger();
  if (portrait.empty())
    m_pendingChatActions.append(SayChatAction{entityId(), message, mouthPosition()});
  else
    m_pendingChatActions.append(PortraitChatAction{entityId(), portrait, message, mouthPosition()});
}

void Monster::setupNetStates() {
  m_netGroup.addNetElement(&m_uniqueIdNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_monsterLevelNetState);
  m_netGroup.addNetElement(&m_damageOnTouchNetState);
  m_netGroup.addNetElement(&m_damageSources);
  m_netGroup.addNetElement(&m_aggressiveNetState);
  m_netGroup.addNetElement(&m_knockedOutNetState);
  m_netGroup.addNetElement(&m_deathParticleBurstNetState);
  m_netGroup.addNetElement(&m_deathSoundNetState);
  m_netGroup.addNetElement(&m_activeSkillNameNetState);
  m_netGroup.addNetElement(&m_name);
  m_netGroup.addNetElement(&m_displayNametag);
  m_netGroup.addNetElement(&m_dropPoolNetState);
  m_netGroup.addNetElement(&m_physicsForces);

  m_netGroup.addNetElement(&m_networkedAnimator);
  m_netGroup.addNetElement(m_movementController.get());
  m_netGroup.addNetElement(m_statusController.get());
  m_netGroup.addNetElement(&m_effectEmitter);

  m_netGroup.addNetElement(&m_newChatMessageEvent);
  m_netGroup.addNetElement(&m_chatMessage);
  m_netGroup.addNetElement(&m_chatPortrait);

  m_netGroup.addNetElement(&m_damageBar);
  m_netGroup.addNetElement(&m_interactive);

    // don't interpolate scripted animation parameters or animationdamageparts
  m_netGroup.addNetElement(&m_animationDamageParts, false);
  m_netGroup.addNetElement(&m_scriptedAnimationParameters, false);

  m_netGroup.setNeedsLoadCallback(bind(&Monster::getNetStates, this, _1));
  m_netGroup.setNeedsStoreCallback(bind(&Monster::setNetStates, this));
}

void Monster::setNetStates() {
  m_uniqueIdNetState.set(uniqueId());
  m_teamNetState.set(getTeam());
  m_monsterLevelNetState.set(m_monsterLevel);
  m_damageOnTouchNetState.set(m_damageOnTouch);
  m_aggressiveNetState.set(aggressive());
  m_knockedOutNetState.set(m_knockedOut);
  m_deathParticleBurstNetState.set(m_deathParticleBurst);
  m_deathSoundNetState.set(m_deathSound);
  m_activeSkillNameNetState.set(m_activeSkillName);
  m_dropPoolNetState.set(m_dropPool);
}

void Monster::getNetStates(bool initial) {
  setUniqueId(m_uniqueIdNetState.get());
  setTeam(m_teamNetState.get());
  m_monsterLevel = m_monsterLevelNetState.get();
  m_damageOnTouch = m_damageOnTouchNetState.get();
  m_aggressive = m_aggressiveNetState.get();
  m_knockedOut = m_knockedOutNetState.get();
  if (m_deathParticleBurstNetState.pullUpdated())
    m_deathParticleBurst = m_deathParticleBurstNetState.get();
  if (m_deathSoundNetState.pullUpdated())
    m_deathSound = m_deathSoundNetState.get();
  if (m_activeSkillNameNetState.pullUpdated())
    m_activeSkillName = m_activeSkillNameNetState.get();
  if (m_dropPoolNetState.pullUpdated())
    m_dropPool = m_dropPoolNetState.get();

  if (m_newChatMessageEvent.pullOccurred() && !initial) {
    if (m_chatPortrait.get().empty())
      m_pendingChatActions.append(SayChatAction{entityId(), m_chatMessage.get(), mouthPosition()});
    else
      m_pendingChatActions.append(
          PortraitChatAction{entityId(), m_chatPortrait.get(), m_chatMessage.get(), mouthPosition()});
  }
}

float Monster::monsterLevel() const {
  return *m_monsterLevel;
}

Monster::SkillInfo Monster::activeSkillInfo() const {
  SkillInfo skillInfo;

  if (!m_activeSkillName.empty()) {
    auto monsterDatabase = Root::singleton().monsterDatabase();
    auto monsterSkillInfo = monsterDatabase->skillInfo(m_activeSkillName);
    skillInfo.label = monsterSkillInfo.first;
    skillInfo.image = monsterSkillInfo.second;
  }

  return skillInfo;
}

List<Drawable> Monster::portrait(PortraitMode) const {
  if (m_monsterVariant.portraitIcon) {
    return {Drawable::makeImage(*m_monsterVariant.portraitIcon, 1.0f, true, Vec2F())};
  } else {
    auto animator = m_networkedAnimator;
    animator.setFlipped(!m_monsterVariant.reversed);
    auto drawables = animator.drawables();
    Drawable::scaleAll(drawables, TilePixels);
    return drawables;
  }
}

String Monster::name() const {
  return m_name.get().orMaybe(m_monsterVariant.shortDescription).value("");
}

String Monster::typeName() const {
  return m_monsterVariant.type;
}

MonsterVariant Monster::monsterVariant() const {
  return m_monsterVariant;
}

Maybe<String> Monster::statusText() const {
  return {};
}

bool Monster::displayNametag() const {
  return m_displayNametag.get();
}

Vec3B Monster::nametagColor() const {
  return m_monsterVariant.nametagColor;
}

Vec2F Monster::nametagOrigin() const {
  return mouthPosition(false);
}

bool Monster::aggressive() const {
  return m_aggressive;
}

Maybe<LuaValue> Monster::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Monster::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

Vec2F Monster::mouthPosition() const {
  return mouthOffset() + position();
}

Vec2F Monster::mouthPosition(bool) const {
  return mouthPosition();
}

List<ChatAction> Monster::pullPendingChatActions() {
  return std::move(m_pendingChatActions);
}

List<PhysicsForceRegion> Monster::forceRegions() const {
  return m_physicsForces.get();
}

InteractAction Monster::interact(InteractRequest const& request) {
  auto result = m_scriptComponent.invoke<Json>("interact", JsonObject{{"sourceId", request.sourceId}, {"sourcePosition", jsonFromVec2F(request.sourcePosition)}}).value();

  if (result.isNull())
    return {};

  if (result.isType(Json::Type::String))
    return InteractAction(result.toString(), entityId(), {});

  return InteractAction(result.getString(0), entityId(), result.get(1));
}

bool Monster::isInteractive() const {
  return m_interactive.get();
}

Vec2F Monster::questIndicatorPosition() const {
  Vec2F pos = position() + m_questIndicatorOffset;
  pos[1] += collisionArea().yMax();
  return pos;
}

}
