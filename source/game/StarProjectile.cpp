#include "StarProjectile.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorld.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarMonster.hpp"
#include "StarStoredFunctions.hpp"
#include "StarDamageDatabase.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarAssets.hpp"
#include "StarItemDrop.hpp"
#include "StarIterator.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarMovementControllerLuaBindings.hpp"
#include "StarParticleDatabase.hpp"

namespace Star {

Projectile::Projectile(ProjectileConfigPtr const& config, Json const& parameters) {
  m_config = config;
  m_parameters = parameters;

  setup();
}

Projectile::Projectile(ProjectileConfigPtr const& config, DataStreamBuffer& data) {
  m_config = config;
  data.read(m_parameters);
  setup();

  EntityId sourceEntity = data.readVlqI();
  bool trackSourceEntity = data.read<bool>();
  setSourceEntity(sourceEntity, trackSourceEntity);

  data.read(m_initialSpeed);
  data.read(m_powerMultiplier);
  setTeam(data.read<EntityDamageTeam>());
}

ByteArray Projectile::netStore() const {
  DataStreamBuffer ds;

  ds.write(m_config->typeName);
  ds.write(m_parameters);

  ds.viwrite(m_sourceEntity);
  ds.write(m_trackSourceEntity);

  ds.write(m_initialSpeed);
  ds.write(m_powerMultiplier);
  ds.write(getTeam());

  return ds.data();
}

EntityType Projectile::entityType() const {
  return EntityType::Projectile;
}

void Projectile::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);
  m_movementController->init(world);
  m_movementController->setIgnorePhysicsEntities({entityId});

  m_timeToLive = m_parameters.getFloat("timeToLive", m_config->timeToLive);
  setSourceEntity(m_sourceEntity, m_trackSourceEntity);

  m_periodicActions.clear();
  if (m_parameters.contains("periodicActions")) {
    for (auto const& c : m_parameters.getArray("periodicActions", {}))
      m_periodicActions.append(make_tuple(GameTimer(c.getFloat("time")), c.getBool("repeat", true), c));
  } else {
    for (auto const& periodicAction : m_config->periodicActions)
      m_periodicActions.append(make_tuple(GameTimer(get<0>(periodicAction)), get<1>(periodicAction), get<2>(periodicAction)));
  }

  if (isMaster()) {
    if (!m_config->scripts.empty()) {
      m_scriptComponent.setScripts(m_config->scripts);
      m_scriptComponent.setUpdateDelta(m_parameters.getUInt("scriptDelta", m_config->config.getUInt("scriptDelta", 1)));

      m_scriptComponent.addCallbacks("projectile", makeProjectileCallbacks());
      m_scriptComponent.addCallbacks("config", LuaBindings::makeConfigCallbacks([this](String const& name, Json const& def) {
           return m_parameters.query(name, m_config->config.query(name, def));
         }));
      m_scriptComponent.addCallbacks("entity", LuaBindings::makeEntityCallbacks(this));
      m_scriptComponent.addCallbacks("mcontroller", LuaBindings::makeMovementControllerCallbacks(m_movementController.get()));
      m_scriptComponent.init(world);
    }
  }
  m_travelLine = Line2F(position(), position());

  if (auto referenceVelocity = m_parameters.opt("referenceVelocity"))
    setReferenceVelocity(referenceVelocity.apply(jsonToVec2F));

  if (world->isClient() && !m_persistentAudioFile.empty()) {
    m_persistentAudio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(m_persistentAudioFile));
    m_persistentAudio->setLoops(-1);
    m_persistentAudio->setPosition(position());
    m_pendingRenderables.append(m_persistentAudio);
  }
}

void Projectile::uninit() {
  if (m_persistentAudio)
    m_persistentAudio->stop();
  m_movementController->uninit();
  if (isMaster()) {
    if (!m_config->scripts.empty()) {
      m_scriptComponent.uninit();
      m_scriptComponent.removeCallbacks("projectile");
      m_scriptComponent.removeCallbacks("config");
      m_scriptComponent.removeCallbacks("entity");
      m_scriptComponent.removeCallbacks("mcontroller");
    }
  }
  Entity::uninit();
}

String Projectile::typeName() const {
  return m_config->typeName;
}

String Projectile::description() const {
  return m_config->description;
}

Vec2F Projectile::position() const {
  return m_movementController->position();
}

RectF Projectile::metaBoundBox() const {
  return m_config->boundBox;
}

pair<ByteArray, uint64_t> Projectile::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Projectile::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(move(data), interpolationTime);
}

void Projectile::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
}

void Projectile::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

bool Projectile::shouldDestroy() const {
  if (auto res = m_scriptComponent.invoke<bool>("shouldDestroy"))
    return *res;
  return m_timeToLive <= 0.0f;
}

void Projectile::destroy(RenderCallback* renderCallback) {
  for (auto const& action : m_parameters.getArray("actionOnReap", m_config->actionOnReap))
    processAction(action);

  if (m_collision) {
    for (auto const& action : m_parameters.getArray("actionOnHit", m_config->actionOnHit))
      processAction(action);
  } else {
    for (auto const& action : m_parameters.getArray("actionOnTimeout", m_config->actionOnTimeout))
      processAction(action);
  }

  if (renderCallback)
    renderPendingRenderables(renderCallback);

  m_scriptComponent.invoke("destroy");
}

List<DamageSource> Projectile::damageSources() const {
  if (m_onlyHitTerrain)
    return {};

  float time_per_frame = m_animationCycle / m_config->frameNumber;
  if ((m_config->intangibleWindup && m_animationTimer < time_per_frame * m_config->windupFrames)
      || (m_config->intangibleWinddown && m_timeToLive < time_per_frame * m_config->winddownFrames))
    return {};

  EntityDamageTeam sourceTeam = getTeam();

  auto statusEffects = m_config->statusEffects;
  statusEffects.appendAll(m_parameters.getArray("statusEffects", {}).transformed(jsonToEphemeralStatusEffect));

  float knockbackMagnitude = m_parameters.getFloat("knockback", m_config->knockback);

  DamageSource::Knockback knockback;
  if (m_parameters.getBool("knockbackDirectional", m_config->knockbackDirectional))
    knockback = Vec2F::withAngle(m_movementController->rotation()) * knockbackMagnitude;
  else
    knockback = knockbackMagnitude;

  List<DamageSource> res;
  auto addDamageSource = [&](DamageSource::DamageArea damageArea) {
    res.append(DamageSource(m_damageType, damageArea, m_power * m_powerMultiplier, true, m_sourceEntity, sourceTeam,
        m_damageRepeatGroup, m_damageRepeatTimeout, m_damageKind, statusEffects, knockback, m_rayCheckToSource));
  };

  Vec2F positionDelta = world()->geometry().diff(m_travelLine.min(), m_travelLine.max());
  static float const MinimumDamageLineDelta = 0.1f;
  bool useDamageLine = positionDelta.magnitudeSquared() >= square(MinimumDamageLineDelta);
  if (useDamageLine)
    addDamageSource(Line2F(positionDelta, Vec2F()));

  if (!m_config->damagePoly.isNull()) {
    PolyF damagePoly = m_config->damagePoly;
    if (m_config->flippable) {
      auto angleSide = getAngleSide(m_movementController->rotation(), true);
      if (angleSide.second == Direction::Left)
        damagePoly.flipHorizontal(0);
      damagePoly.rotate(angleSide.first);
    } else {
      damagePoly.rotate(m_movementController->rotation());
    }
    addDamageSource(damagePoly);
  } else if (!useDamageLine) {
    addDamageSource(PolyF(RectF::withCenter(Vec2F(), Vec2F::filled(MinimumDamageLineDelta))));
  }

  return res;
}

void Projectile::hitOther(EntityId entity, DamageRequest const&) {
  if (!m_parameters.getBool("piercing", m_config->piercing)) {
    auto victimEntity = world()->entity(entity);
    if (!victimEntity || (victimEntity->getTeam().type != TeamType::Passive && victimEntity->getTeam().type != TeamType::Environment)) {
      if (victimEntity) {
        if (auto hitPoly = victimEntity->hitPoly()) {
          auto geometry = world()->geometry();
          Vec2F checkVec = m_movementController->velocity().normalized() * 5;
          Vec2F nearMin = geometry.nearestTo(hitPoly->center(), m_movementController->position() - checkVec);
          if (auto intersection = hitPoly->lineIntersection(Line2F(nearMin, nearMin + checkVec * 2)))
            m_movementController->setPosition(intersection->point);
        }
      }
      m_movementController->setVelocity({0, 0});
      m_collision = true;
      m_timeToLive = 0.0f;
    }
  }
  m_scriptComponent.invoke("hit", entity);
}

void Projectile::update(float dt, uint64_t) {
  m_movementController->setTimestep(dt);

  if (isMaster()) {
    m_timeToLive -= dt;
    if (m_timeToLive < 0)
      m_timeToLive = 0;

    m_effectEmitter->addEffectSources("normal", m_config->emitters);

    if (m_referenceVelocity)
      m_movementController->setVelocity(m_movementController->velocity() - *m_referenceVelocity);

    m_scriptComponent.update(m_scriptComponent.updateDt(dt));
    m_movementController->accelerate(m_movementController->velocity().normalized() * m_acceleration);

    if (m_referenceVelocity)
      m_movementController->setVelocity(m_movementController->velocity() + *m_referenceVelocity);

    m_movementController->tickMaster(dt);
    m_travelLine.min() = m_travelLine.max();
    m_travelLine.max() = m_movementController->position();

    tickShared(dt);

    if (m_trackSourceEntity) {
      if (auto sourceEntity = world()->entity(m_sourceEntity)) {
        Vec2F newEntityPosition = sourceEntity->position();
        m_movementController->translate(newEntityPosition - m_lastEntityPosition);
        m_lastEntityPosition = newEntityPosition;
      } else {
        m_trackSourceEntity = false;
      }
    }

    if (m_movementController->atWorldLimit())
      m_timeToLive = 0.0f;

    if ((m_movementController->isColliding() || m_movementController->stickingDirection()) && !m_movementController->isNullColliding()) {
      if (!m_wasColliding)
        m_collisionEvent.trigger();
      m_wasColliding = true;
    } else {
      m_wasColliding = false;
    }

    if (m_movementController->isColliding()) {
      if (m_movementController->isNullColliding()) {
        // Don't trigger collision action, just silently die if we collide with a
        // null block.
        m_timeToLive = 0.0f;
      } else if (m_bounces != 0) {
        m_scriptComponent.invoke("bounce");
        if (m_bounces > 0)
          --m_bounces;
      } else if (m_falldown && !(m_movementController->onGround() || m_movementController->isCollisionStuck() || m_movementController->stickingDirection())) {
        // Wait til this projectile actually hits the ground before dying

      } else if (!m_movementController->stickingDirection()) {
        m_collision = true;
        m_timeToLive = 0.0f;
        // Move slightly less than one tile unit in the direction that the projectile
        // has most recently moved to find the collision tile.  This is *not* perfect by any means.
        m_collisionTile = Vec2I::floor(m_movementController->position() + m_travelLine.direction() * 0.9f);

        m_lastNonCollidingTile = Vec2I::floor(m_movementController->position());
        for (float i = 0; i < 1.51f; i += 0.5f) {
          auto pos = Vec2I::floor(m_movementController->position() + m_travelLine.direction() * -i);
          if (world()->material(pos, TileLayer::Foreground) == EmptyMaterialId) {
            m_lastNonCollidingTile = pos;
            break;
          }
        }
      }
    }

    if (!m_collision && m_hydrophobic) {
      auto liquid = world()->liquidLevel(Vec2I::floor(position()));
      if (liquid.level > 0.5f) {
        m_collision = true;
        m_timeToLive = 0.0f;
        m_collisionTile = Vec2I::floor(position());
        m_lastNonCollidingTile = m_collisionTile;
      }
    }
  } else {
    m_netGroup.tickNetInterpolation(WorldTimestep);
    m_movementController->tickSlave(dt);
    m_travelLine.min() = m_travelLine.max();
    m_travelLine.max() = m_movementController->position();

    m_timeToLive -= dt;

    tickShared(dt);
  }

  if (world()->isClient())
    SpatialLogger::logPoly("world", m_movementController->collisionBody(), Color::Red.toRgba());
}

void Projectile::render(RenderCallback* renderCallback) {
  renderPendingRenderables(renderCallback);

  if (m_persistentAudio)
    m_persistentAudio->setPosition(position());

  m_effectEmitter->render(renderCallback);

  String image = strf("{}:{}{}", m_config->image, m_frame, m_imageSuffix);
  Drawable drawable = Drawable::makeImage(image, 1.0f / TilePixels, true, Vec2F());
  drawable.imagePart().addDirectives(m_imageDirectives, true);
  if (m_config->flippable) {
    auto angleSide = getAngleSide(m_movementController->rotation(), true);
    if (angleSide.second == Direction::Left)
      drawable.scale(Vec2F(-1, 1));
    drawable.rotate(angleSide.first);
  } else {
    drawable.rotate(m_movementController->rotation());
  }
  drawable.fullbright = m_config->fullbright;
  drawable.translate(position());
  renderCallback->addDrawable(move(drawable), m_config->renderLayer);
}

void Projectile::renderLightSources(RenderCallback* renderCallback) {
  for (auto renderable : m_pendingRenderables) {
    if (renderable.is<LightSource>())
      renderCallback->addLightSource(renderable.get<LightSource>());
  }
  renderCallback->addLightSource({ position(), m_config->lightColor, m_config->pointLight, 0.0f, 0.0f, 0.0f });
}

Maybe<Json> Projectile::receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) {
  return m_scriptComponent.handleMessage(message, sendingConnection == world()->connection(), args);
}

Maybe<LuaValue> Projectile::callScript(String const& func, LuaVariadic<LuaValue> const& args) {
  return m_scriptComponent.invoke(func, args);
}

Maybe<LuaValue> Projectile::evalScript(String const& code) {
  return m_scriptComponent.eval(code);
}

String Projectile::projectileType() const {
  return m_config->typeName;
}

void Projectile::setReferenceVelocity(Maybe<Vec2F> const& velocity) {
  m_movementController->setVelocity(m_movementController->velocity() - m_referenceVelocity.value());
  m_referenceVelocity = velocity;
  m_movementController->setVelocity(m_movementController->velocity() + velocity.value());
  m_effectEmitter->setBaseVelocity(velocity.value());
}

float Projectile::initialSpeed() const {
  return m_initialSpeed;
}

void Projectile::setInitialSpeed(float speed) {
  m_initialSpeed = speed;
}

void Projectile::setInitialPosition(Vec2F const& position) {
  m_movementController->setPosition(position);
}

void Projectile::setInitialDirection(Vec2F const& direction) {
  m_movementController->setVelocity(vnorm(direction) * m_initialSpeed + m_referenceVelocity.value());
  m_movementController->setRotation(direction.angle());
}

void Projectile::setInitialVelocity(Vec2F const& velocity) {
  m_movementController->setVelocity(velocity + m_referenceVelocity.value());
  m_movementController->setRotation(velocity.angle());
}

void Projectile::setSourceEntity(EntityId source, bool trackSource) {
  m_sourceEntity = source;
  m_trackSourceEntity = trackSource;
  if (inWorld()) {
    if (auto sourceEntity = world()->entity(source)) {
      m_lastEntityPosition = sourceEntity->position();
      if (!m_damageTeam)
        setTeam(sourceEntity->getTeam());
    } else {
      m_sourceEntity = NullEntityId;
      m_trackSourceEntity = false;
    }
  }
}

float Projectile::powerMultiplier() const {
  return m_powerMultiplier;
}

void Projectile::setPowerMultiplier(float powerMultiplier) {
  m_powerMultiplier = powerMultiplier;
}

EntityId Projectile::sourceEntity() const {
  return m_sourceEntity;
}

List<PersistentStatusEffect> Projectile::statusEffects() const {
  return m_config->persistentStatusEffects;
}

PolyF Projectile::statusEffectArea() const {
  return m_config->statusEffectArea;
}

List<PhysicsForceRegion> Projectile::forceRegions() const {
  List<PhysicsForceRegion> forces;
  for (auto const& p : m_physicsForces) {
    if (p.second.enabled.get()) {
      PhysicsForceRegion forceRegion = p.second.forceRegion;
      forceRegion.call([pos = position()](auto& fr) { fr.translate(pos); });
      forces.append(move(forceRegion));
    }
  }
  return forces;
}

size_t Projectile::movingCollisionCount() const {
  return m_physicsCollisions.size();
}

Maybe<PhysicsMovingCollision> Projectile::movingCollision(size_t positionIndex) const {
  auto const& mc = m_physicsCollisions.valueAt(positionIndex);
  if (!mc.enabled.get())
    return {};
  PhysicsMovingCollision collision = mc.movingCollision;
  collision.translate(position());
  return collision;
}

List<Particle> Projectile::sparkBlock(World* world, Vec2I const& position, Vec2F const& damageSource) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto materialDatabase = root.materialDatabase();

  auto blockDamageParticle = Particle(assets->json("/client.config:blockDamageParticle"));
  auto blockDamageVariance = Particle(assets->json("/client.config:blockDamageParticleVariance"));

  List<Particle> result;
  for (auto layer : {TileLayer::Background, TileLayer::Foreground}) {
    auto material = world->material(position, layer);
    auto hueShift = world->materialHueShift(position, layer);
    if (isRealMaterial(material)) {
      auto particle = blockDamageParticle;
      particle.position += centerOfTile(position);
      particle.velocity = particle.velocity.magnitude() * vnorm(world->geometry().diff(damageSource, particle.position));
      particle.color = materialDatabase->materialParticleColor(material, hueShift);
      particle.applyVariance(blockDamageVariance);

      particle.approach += Vec2F(0.0f, 5.0f);
      particle.velocity += Vec2F(Random::randf() - 0.5f, 5.0f + Random::randf());
      particle.velocity += 10.0f * Vec2F(1.0f - 2.0f * Random::randf(), 1.0f - 2.0f * Random::randf());
      particle.finalVelocity = 0.5f * (particle.finalVelocity + Vec2F(Random::randf() - 0.5f, -20.0f + Random::randf()));
      particle.trail = true;

      result.append(move(particle));
    }
  }
  return result;
}

int Projectile::getFrame() const {
  float time_per_frame = m_animationCycle / m_config->frameNumber;

  if (m_config->animationLoops) {
    if (m_animationTimer < time_per_frame * m_config->windupFrames) {
      return floor(m_animationTimer / time_per_frame);
    } else if (m_timeToLive < time_per_frame * m_config->winddownFrames) {
      return m_config->windupFrames + m_config->frameNumber
          + clamp<int>((time_per_frame * m_config->winddownFrames - m_timeToLive) / time_per_frame, 0, m_config->winddownFrames - 1);
    } else {
      float time_within_cycle = std::fmod(m_animationTimer, m_animationCycle);
      return m_config->windupFrames + floor(time_within_cycle / time_per_frame);
    }
  } else {
    return clamp<int>(m_animationTimer / time_per_frame, 0, m_config->frameNumber - 1);
  }
}

void Projectile::setFrame(int frame) {
  m_frame = frame;
}

String Projectile::drawableFrame() {
  String str = strf("{}:{}{}", m_config->image, m_frame, m_imageSuffix);
  return m_imageDirectives.addToString(str);
}

bool Projectile::ephemeral() const {
  return true;
}

ClientEntityMode Projectile::clientEntityMode() const {
  return m_config->clientEntityMode;
}

bool Projectile::masterOnly() const {
  return m_config->masterOnly;
}

void Projectile::processAction(Json const& action) {
  Json parameters;
  String command;

  if (action.type() == Json::Type::Object) {
    parameters = action;
    command = parameters.getString("action").toLower();
  } else {
    command = action.toString().toLower();
  }

  auto doWithDelay = [this](int stepsDelay, WorldAction function) {
    if (stepsDelay == 0)
      function(world());
    else
      world()->timer(stepsDelay, function);
  };

  if (command == "tile") {
    if (isSlave())
      return;

    auto materialDatabase = Root::singleton().materialDatabase();
    List<MaterialId> tileDrops;
    unsigned totalDrops = 0;
    for (auto sets : parameters.getArray("materials")) {
      unsigned numDrops = sets.getUInt("quantity", 1);
      auto mat = materialDatabase->materialId(sets.getString("kind"));
      for (unsigned i = 0; i < numDrops; i++)
        tileDrops.push_back(mat);
      totalDrops += numDrops;
    }

    List<Vec2I> openSpaces = world()->findEmptyTiles(m_lastNonCollidingTile, parameters.getInt("radius", 2), totalDrops);
    if (openSpaces.size() < totalDrops)
      Logger::debug("Couldn't find a place for all the tile drops. {} drops requested, {} spaces found.", totalDrops, openSpaces.size());

    bool allowEntityOverlap = parameters.getBool("allowEntityOverlap", true);

    Random::shuffle(tileDrops);
    for (auto& tile : zip(openSpaces, tileDrops)) {
      if (!world()->modifyTile(std::get<0>(tile), PlaceMaterial{TileLayer::Foreground, std::get<1>(tile), MaterialHue()}, allowEntityOverlap)) {
        auto itemDrop = ItemDrop::createRandomizedDrop(materialDatabase->materialItemDrop(std::get<1>(tile)), (Vec2F)std::get<0>(tile));
        world()->addEntity(itemDrop);
      }
    }

  } else if (command == "applysurfacemod") {
    if (isSlave())
      return;

    auto materialDatabase = Root::singleton().materialDatabase();
    Maybe<ModId> previousMod =
        parameters.optString("previousMod").apply(bind(&MaterialDatabase::modId, materialDatabase, _1));
    ModId newMod = materialDatabase->modId(parameters.getString("newMod"));
    int radius = parameters.getInt("radius", 0);
    float chance = parameters.getFloat("chance", 1.0f);

    Set<TileLayer> layers;
    if (parameters.getBool("foreground", true))
      layers.add(TileLayer::Foreground);
    if (parameters.getBool("background", false))
      layers.add(TileLayer::Background);

    for (auto layer : layers) {
      // Go in vertical lines for each column, stop at the first non-emppty
      // material in each column.
      for (int x = m_collisionTile[0] - radius; x <= m_collisionTile[0] + radius; ++x) {
        if (world()->material({x, m_collisionTile[1] + radius + 1}, layer) == EmptyMaterialId) {
          for (int y = m_collisionTile[1] + radius; y >= m_collisionTile[1] - radius; --y) {
            auto mat = world()->material({x, y}, layer);
            if (Random::randf() <= chance) {
              if (isRealMaterial(mat)) {
                auto mod = world()->mod({x, y}, layer);
                if (!previousMod || *previousMod == mod)
                  world()->modifyTile({x, y}, PlaceMod{layer, newMod, {}}, true);
              }
            }
            if (mat != EmptyMaterialId)
              break;
          }
        }
      }
    }

  } else if (command == "liquid") {
    if (isSlave())
      return;

    float waterAmount = parameters.getFloat("quantity", 1.0f);
    LiquidId liquid = Root::singleton().liquidsDatabase()->liquidId(parameters.getString("liquid"));
    auto empty = world()->findEmptyTiles(m_lastNonCollidingTile, parameters.getInt("radius", 5), 50);
    for (Vec2I pos : empty) {
      if (world()->lineTileCollision(Vec2F(pos), Vec2F(m_lastNonCollidingTile)))
        continue;

      auto liquidLevel = world()->liquidLevel(pos);
      if (liquidLevel.liquid == EmptyLiquidId || liquidLevel.liquid == liquid) {
        world()->modifyTile(pos, PlaceLiquid{liquid, waterAmount}, true);
        break;
      }
    }

  } else if (command == "projectile") {
    if (isSlave())
      return;

    String type = parameters.getString("type");
    auto projectileParameters = parameters.get("config", JsonObject());
    if (!projectileParameters.contains("damageTeam")) {
      if (m_damageTeam)
        projectileParameters = projectileParameters.set("damageTeam", m_damageTeam);
    }
    if (parameters.contains("inheritDamageFactor") && !projectileParameters.contains("power"))
      projectileParameters = projectileParameters.set("power", m_power * parameters.getFloat("inheritDamageFactor"));
    if (parameters.contains("inheritSpeedFactor"))
      projectileParameters = projectileParameters.set("speed", (m_movementController->velocity() - m_referenceVelocity.value()).magnitude() * parameters.getFloat("inheritSpeedFactor"));

    auto projectile = Root::singleton().projectileDatabase()->createProjectile(type, projectileParameters);
    Vec2F offset;
    if (parameters.contains("offset")) {
      offset = jsonToVec2F(parameters.getArray("offset", {0.0f, 0.0f}));
    } else if (parameters.contains("offsetRange")) {
      auto offsetRange = jsonToVec4F(parameters.get("offsetRange"));
      offset = Vec2F(Random::randf(offsetRange[0], offsetRange[2]), Random::randf(offsetRange[1], offsetRange[3]));
    }
    if (m_referenceVelocity)
      projectile->setReferenceVelocity(m_referenceVelocity);
    projectile->setInitialPosition(position() + offset);
    if (parameters.contains("direction")) {
      projectile->setInitialDirection(jsonToVec2F(parameters.get("direction")));
    } else {
      float angle = m_movementController->rotation();
      float angleAdjust = 0;
      if (parameters.contains("angle"))
        angle = parameters.getFloat("angle") * Constants::pi / 180.0f;
      if (parameters.contains("fuzzAngle"))
        angleAdjust += Random::randf(-1, 1) * parameters.getFloat("fuzzAngle") * Constants::pi / 180.0f;
      if (parameters.contains("angleAdjust"))
        angleAdjust += parameters.getFloat("angleAdjust") * Constants::pi / 180.0f;
      if (parameters.contains("autoFlipAdjust") && parameters.getBool("autoFlipAdjust")) {
        if (Vec2F::withAngle(m_movementController->rotation())[0] < 0)
          angleAdjust = -angleAdjust;
      }
      if (parameters.contains("autoFlipAngle") && parameters.getBool("autoFlipAngle")) {
        if (Vec2F::withAngle(m_movementController->rotation())[0] < 0)
          angle = -angle;
      }
      angle += angleAdjust;
      projectile->setInitialDirection(Vec2F::withAngle(angle, 1.0f));
    }
    projectile->setSourceEntity(m_sourceEntity, false);
    projectile->setPowerMultiplier(m_powerMultiplier);
    
    // if the entity no longer exists and no explicit damage team is set, inherit damage team
    if (!projectile->m_damageTeam && !world()->entity(m_sourceEntity))
      projectile->setTeam(getTeam());

    doWithDelay(parameters.getUInt("delaySteps", 0), [=](World* world) { world->addEntity(projectile); });

  } else if (command == "spark") {
    if (!world()->isClient())
      return;

    auto collisionMaterial = world()->material(m_collisionTile, TileLayer::Foreground);
    if (!m_collision || collisionMaterial == EmptyMaterialId)
      return;

    for (auto& particle : sparkBlock(world(), m_collisionTile, position())) {
      // enable trails and such
      particle.approach += Vec2F(0.0f, 5.0f);
      particle.velocity += Vec2F(Random::randf() - 0.5f, 5.0f + Random::randf());
      particle.velocity += 10.0f * Vec2F(1.0f - 2.0f * Random::randf(), 1.0f - 2.0f * Random::randf());
      particle.finalVelocity = 0.5f * (particle.finalVelocity + Vec2F(Random::randf() - 0.5f, -20.0f + Random::randf()));
      particle.trail = true;

      m_pendingRenderables.append(move(particle));
    }

  } else if (command == "particle") {
    if (!world()->isClient())
      return;

    Particle particle = Root::singleton().particleDatabase()->particle(parameters.get("specification"));
    particle.position = particle.position.rotate(m_movementController->rotation());
    if (parameters.getBool("rotate", false)) {
      particle.rotation = m_movementController->rotation();
      particle.velocity = particle.velocity.rotate(m_movementController->rotation());
    }
    particle.translate(position());
    particle.velocity += m_referenceVelocity.value();
    m_pendingRenderables.append(move(particle));

  } else if (command == "explosion") {
    if (isSlave())
      return;

    float foregroundRadius = parameters.getFloat("foregroundRadius");
    float backgroundRadius = parameters.getFloat("backgroundRadius");
    float explosiveDamageAmount = parameters.getFloat("explosiveDamageAmount");
    auto damageType = TileDamageTypeNames.getLeft(parameters.getString("tileDamageType", "explosive"));
    unsigned harvestLevel = parameters.getUInt("harvestLevel", 0);
    Vec2F explosionPosition = position();

    doWithDelay(parameters.getUInt("delaySteps", 0), [=](World* world) {
        world->damageTiles(tileAreaBrush(foregroundRadius, explosionPosition, false),
            TileLayer::Foreground,
            explosionPosition,
            {damageType, explosiveDamageAmount, harvestLevel},
            sourceEntity());
        world->damageTiles(tileAreaBrush(backgroundRadius, explosionPosition, false),
            TileLayer::Background,
            explosionPosition,
            {damageType, explosiveDamageAmount, harvestLevel},
            sourceEntity());
      });

  } else if (command == "spawnmonster") {
    if (isMaster()) {
      String const type = parameters.getString("type");

      JsonObject arguments = parameters.getObject("arguments", JsonObject{});

      float level = parameters.getFloat("level", m_parameters.getFloat("level", 0.0f));

      auto monsterDatabase = Root::singleton().monsterDatabase();
      auto monster = monsterDatabase->createMonster(monsterDatabase->randomMonster(type, arguments), level);

      auto spawnPosition = position();
      if (parameters.contains("offset"))
        spawnPosition += jsonToVec2F(parameters.get("offset"));
      monster->setPosition(spawnPosition);
      world()->addEntity(monster);
    }

    if (world()->isClient() && parameters.contains("particle")) {
      Particle particle(parameters.getObject("particle"));
      particle.translate(position());
      particle.velocity += m_referenceVelocity.value();
      m_pendingRenderables.append(move(particle));
    }

  } else if (command == "item") {
    if (isSlave())
      return;

    String const name = parameters.getString("name");
    size_t count = parameters.getInt("count", 1);
    JsonObject data = parameters.getObject("data", JsonObject{});

    auto itemDrop = ItemDrop::createRandomizedDrop(ItemDescriptor(name, count, data), position());
    world()->addEntity(itemDrop);

  } else if (command == "sound") {
    if (!world()->isClient())
      return;

    AudioInstancePtr sound = make_shared<AudioInstance>(*Root::singleton().assets()->audio(Random::randValueFrom(parameters.getArray("options")).toString()));
    sound->setPosition(position());
    m_pendingRenderables.append(move(sound));

  } else if (command == "light") {
    if (!world()->isClient())
      return;

    m_pendingRenderables.append(LightSource{
        position(),
        jsonToColor(parameters.get("color")).toRgb(),
        parameters.getBool("pointLight", true),
        0.0f,
        0.0f,
        0.0f
      });

  } else if (command == "option") {
    JsonArray options = parameters.getArray("options");
    if (options.size())
      processAction(Random::randFrom(options));

  } else if (command == "actions") {
    JsonArray list = parameters.getArray("list");
    for (auto const& action : list)
      processAction(action);

  } else if (command == "loop") {
    int count = parameters.getInt("count");
    JsonArray body = parameters.getArray("body");
    while (count > 0) {
      for (auto const& action : body)
        processAction(action);
      count--;
    }

  } else if (command == "config") {
    processAction(Root::singleton().assets()->json(parameters.getString("file")));

  } else {
    throw StarException(strf("Unknown projectile reap command {}", command));
  }
}

void Projectile::tickShared(float dt) {
  if (!m_config->orientationLocked && !m_movementController->stickingDirection()) {
    auto apparentVelocity = m_movementController->velocity() - m_referenceVelocity.value();
    if (apparentVelocity != Vec2F())
      m_movementController->setRotation(apparentVelocity.angle());
  }

  m_animationTimer += dt;
  setFrame(getFrame());

  m_effectEmitter->setSourcePosition("normal", position());
  m_effectEmitter->setDirection(getAngleSide(m_movementController->rotation(), true).second);
  m_effectEmitter->tick(dt, *entityMode());

  if (m_collisionEvent.pullOccurred()) {
    for (auto const& action : m_parameters.getArray("actionOnCollide", m_config->actionOnCollide))
      processAction(action);
  }

  auto periodicActionIt = makeSMutableIterator(m_periodicActions);
  while (periodicActionIt.hasNext()) {
    auto& periodicAction = periodicActionIt.next();
    if (get<1>(periodicAction)) {
      if (get<0>(periodicAction).wrapTick())
        processAction(get<2>(periodicAction));
    } else {
      if (get<0>(periodicAction).tick(dt)) {
        processAction(get<2>(periodicAction));
        periodicActionIt.remove();
      }
    }
  }
}

void Projectile::setup() {
  if (auto uniqueId = m_parameters.optString("uniqueId"))
    setUniqueId(*uniqueId);

  m_acceleration = m_parameters.getFloat("acceleration", m_config->acceleration);
  m_power = m_parameters.getFloat("power", m_config->power);
  m_powerMultiplier = m_parameters.getFloat("powerMultiplier", 1.0f);
  { // it is possible to shove a frame name in processing. I hope nobody actually does this but account for it...
    String processing = m_parameters.getString("processing", "");
    auto begin = processing.utf8().find_first_of('?');
    if (begin == NPos) {
      m_imageDirectives = "";
      m_imageSuffix = move(processing);
    }
    else if (begin == 0) {
      m_imageDirectives = move(processing);
      m_imageSuffix = "";
    }
    else {
      m_imageDirectives = (String)processing.utf8().substr(begin);
      m_imageSuffix = processing.utf8().substr(0, begin);
    }
  }
  m_persistentAudioFile = m_parameters.getString("persistentAudio", m_config->persistentAudio);

  m_damageKind = m_parameters.getString("damageKind", m_config->damageKind);
  m_damageType = DamageTypeNames.getLeft(m_parameters.getString("damageType", m_config->damageType));
  m_rayCheckToSource = m_parameters.getBool("rayCheckToSource", m_config->rayCheckToSource);

  if (auto damageTeam = m_parameters.get("damageTeam", m_config->damageTeam)) {
    m_damageTeam = damageTeam;
    setTeam(EntityDamageTeam(damageTeam));
  }
  m_damageRepeatGroup = m_parameters.optString("damageRepeatGroup").orMaybe(m_config->damageRepeatGroup);
  m_damageRepeatTimeout = m_parameters.optFloat("damageRepeatTimeout").orMaybe(m_config->damageRepeatTimeout);

  m_falldown = m_parameters.getBool("falldown", m_config->falldown);

  m_hydrophobic = m_parameters.getBool("hydrophobic", m_config->hydrophobic);
  m_onlyHitTerrain = m_parameters.getBool("onlyHitTerrain", m_config->onlyHitTerrain);

  auto movementSettings = jsonMerge(m_config->movementSettings, m_parameters.get("movementSettings", Json()));
  if (!movementSettings.contains("physicsEffectCategories"))
    movementSettings = movementSettings.set("physicsEffectCategories", JsonArray{"projectile"});
  m_movementController = make_shared<MovementController>(movementSettings);

  m_effectEmitter = make_shared<EffectEmitter>();

  m_initialSpeed = m_parameters.getFloat("speed", m_config->initialSpeed);
  m_sourceEntity = NullEntityId;
  m_trackSourceEntity = false;
  m_bounces = m_parameters.getInt("bounces", m_config->bounces);

  m_frame = 0;

  m_animationTimer = 0.0f;
  m_animationCycle = m_parameters.getFloat("animationCycle", m_config->animationCycle);
  m_collision = false;

  for (auto const& p : m_config->physicsForces.iterateObject()) {
    auto& forceConfig = m_physicsForces[p.first];

    forceConfig.forceRegion = jsonToPhysicsForceRegion(p.second);
    forceConfig.enabled.set(p.second.getBool("enabled", true));
  }

  for (auto const& p : m_config->physicsCollisions.iterateObject()) {
    auto& forceConfig = m_physicsCollisions[p.first];

    forceConfig.movingCollision = PhysicsMovingCollision::fromJson(p.second);
    forceConfig.enabled.set(p.second.getBool("enabled", true));
  }

  m_physicsForces.sortByKey();
  for (auto& p : m_physicsForces)
    m_netGroup.addNetElement(&p.second.enabled);

  m_physicsCollisions.sortByKey();
  for (auto& p : m_physicsCollisions)
    m_netGroup.addNetElement(&p.second.enabled);

  m_netGroup.addNetElement(&m_collisionEvent);
  m_netGroup.addNetElement(m_movementController.get());
  m_netGroup.addNetElement(m_effectEmitter.get());
}

LuaCallbacks Projectile::makeProjectileCallbacks() {
  LuaCallbacks callbacks;
  callbacks.registerCallback("getParameter", [this](String const& name, Json const& def) {
      return m_parameters.query(name, m_config->config.query(name, def));
    });
  callbacks.registerCallback("die", [this]() { m_timeToLive = 0.0f; });
  callbacks.registerCallback("sourceEntity", [this]() -> Maybe<EntityId> {
      if (m_sourceEntity == NullEntityId)
        return {};
      else
        return m_sourceEntity;
    });
  callbacks.registerCallback("powerMultiplier", [this]() { return powerMultiplier(); });
  callbacks.registerCallback("timeToLive", [this]() { return m_timeToLive; });
  callbacks.registerCallback("setTimeToLive", [this](float const& timeToLive) { return m_timeToLive = timeToLive; });
  callbacks.registerCallback("collision", [this]() { return m_collision; });
  callbacks.registerCallback("processAction", [this](Json const& action) { processAction(action); });
  callbacks.registerCallback("power", [this]() { return m_power; });
  callbacks.registerCallback("setPower", [this](float const& power) { m_power = power; });
  callbacks.registerCallback("setReferenceVelocity", [this](Maybe<Vec2F> const& referenceVelocity) { setReferenceVelocity(referenceVelocity); });
  return callbacks;
}

void Projectile::renderPendingRenderables(RenderCallback* renderCallback) {
  for (auto renderable : m_pendingRenderables) {
    if (renderable.is<AudioInstancePtr>())
      renderCallback->addAudio(renderable.get<AudioInstancePtr>());
    else if (renderable.is<Particle>())
      renderCallback->addParticle(renderable.get<Particle>());
  }
  m_pendingRenderables.clear();
}

}
