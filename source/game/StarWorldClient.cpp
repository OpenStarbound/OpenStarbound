#include "StarWorldClient.hpp"
#include "StarIterator.hpp"
#include "StarLogging.hpp"
#include "StarBiome.hpp"
#include "StarMaterialRenderProfile.hpp"
#include "StarLiquidTypes.hpp"
#include "StarDamageDatabase.hpp"
#include "StarParticleDatabase.hpp"
#include "StarParticleManager.hpp"
#include "StarWorldImpl.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLog.hpp"
#include "StarAggressiveEntity.hpp"
#include "StarPhysicsEntity.hpp"
#include "StarItemDrop.hpp"
#include "StarItemDatabase.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObject.hpp"
#include "StarEntityFactory.hpp"
#include "StarWorldTemplate.hpp"
#include "StarStoredFunctions.hpp"
#include "StarInspectableEntity.hpp"
#include "StarCurve25519.hpp"

namespace Star {

const std::string SECRET_BROADCAST_PUBLIC_KEY = "SecretBroadcastPublicKey";
const std::string SECRET_BROADCAST_PREFIX = "\0Broadcast\0"s;

const float WorldClient::DropDist = 6.0f;
WorldClient::WorldClient(PlayerPtr mainPlayer) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  m_clientConfig = assets->json("/client.config");

  m_currentStep = 0;
  m_currentServerStep = 0.0;
  m_fullBright = false;
  m_asyncLighting = true;
  m_worldDimTimer = GameTimer(m_clientConfig.getFloat("worldDimTime"));
  m_worldDimTimer.setDone();
  m_worldDimLevel = 0.0f;

  m_parallaxFadeTimer = GameTimer(m_clientConfig.getFloat("parallaxFadeTime"));
  m_parallaxFadeTimer.setDone();

  m_collisionDebug = false;
  m_inWorld = false;

  m_luaRoot = make_shared<LuaRoot>();

  m_mainPlayer = mainPlayer;

  centerClientWindowOnPlayer(Vec2U(100, 100));

  m_collisionGenerator.init([this](int x, int y) {
      return m_tileArray->tile({x, y}).collision;
    });

  m_modifiedTilePredictionTimeout = (int)round(m_clientConfig.getFloat("modifiedTilePredictionTimeout") / WorldTimestep);

  m_latency = 0.0;

  m_blockDamageParticle = Particle(m_clientConfig.getObject("blockDamageParticle"));
  m_blockDamageParticleVariance = Particle(m_clientConfig.getObject("blockDamageParticleVariance"));
  m_blockDamageParticleProbability = m_clientConfig.getFloat("blockDamageParticleProbability");

  m_blockDingParticle = Particle(m_clientConfig.getObject("blockDingParticle"));
  m_blockDingParticleVariance = Particle(m_clientConfig.getObject("blockDingParticleVariance"));
  m_blockDingParticleProbability = m_clientConfig.getFloat("blockDingParticleProbability");

  m_damageNotificationBatchDuration = m_clientConfig.getFloat("damageNotificationBatchDuration");

  m_ambientSounds.setTrackFadeInTime(assets->json("/interface.config:ambientTrackFadeInTime").toFloat());
  m_ambientSounds.setTrackSwitchGrace(assets->json("/interface.config:ambientTrackSwitchGrace").toFloat());

  m_musicTrack.setTrackSwitchGrace(assets->json("/interface.config:musicTrackSwitchGrace").toFloat());
  m_musicTrack.setTrackFadeInTime(assets->json("/interface.config:musicTrackFadeInTime").toFloat());

  m_altMusicTrack.setTrackFadeInTime(assets->json("/interface.config:musicTrackFadeInTime").toFloat());
  m_altMusicTrack.setTrackSwitchGrace(assets->json("/interface.config:musicTrackFadeInTime").toFloat());
  m_altMusicTrack.setVolume(0, 0, 0);
  m_altMusicActive = false;

  m_stopLightingThread = false;
  m_lightingThread = Thread::invoke("WorldClient::lightingMain", mem_fn(&WorldClient::lightingMain), this);
  m_renderData = nullptr;

  clearWorld();
}

WorldClient::~WorldClient() {
  m_stopLightingThread = true;
  {
    MutexLocker locker(m_lightingMutex);
    m_lightingCond.broadcast();
  }

  m_lightingThread.finish();
  clearWorld();
}

bool WorldClient::inWorld() const {
  return m_inWorld;
}

bool WorldClient::inSpace() const {
  if (!m_sky)
    return false;
  return m_sky->inSpace();
}

bool WorldClient::flying() const {
  if (!m_sky)
    return false;
  return m_sky->flying();
}

bool WorldClient::mainPlayerDead() const {
  if (inWorld())
    return !m_entityMap->get<Player>(m_mainPlayer->entityId());
  else
    return false;
}

void WorldClient::reviveMainPlayer() {
  if (inWorld() && mainPlayerDead()) {
    m_mainPlayer->revive(m_playerStart);
    m_mainPlayer->init(this, m_entityMap->reserveEntityId(), EntityMode::Master);
    m_entityMap->addEntity(m_mainPlayer);
  }
}

bool WorldClient::respawnInWorld() const {
  return m_respawnInWorld;
}

void WorldClient::removeEntity(EntityId entityId, bool andDie) {
  auto entity = m_entityMap->entity(entityId);
  if (!entity)
    return;

  if (andDie) {
    ClientRenderCallback renderCallback;
    entity->destroy(&renderCallback);

    const List<Directives>* directives = nullptr;
    if (auto& worldTemplate = m_worldTemplate) {
      if (const auto& parameters = worldTemplate->worldParameters())
        if (auto& globalDirectives = m_worldTemplate->worldParameters()->globalDirectives)
          directives = &globalDirectives.get();
    }
    if (directives) {
      int directiveIndex = unsigned(entity->entityId()) % directives->size();
      for (auto& p : renderCallback.particles)
        p.directives.append(directives->get(directiveIndex));
    }

    m_particles->addParticles(move(renderCallback.particles));
    m_samples.appendAll(move(renderCallback.audios));
  }

  if (auto version = m_masterEntitiesNetVersion.maybeTake(entity->entityId())) {
    ByteArray finalNetState = entity->writeNetState(*version).first;
    m_outgoingPackets.append(make_shared<EntityDestroyPacket>(entity->entityId(), move(finalNetState), andDie));
  }

  m_entityMap->removeEntity(entityId);
  entity->uninit();
}

WorldTemplateConstPtr WorldClient::currentTemplate() const {
  return m_worldTemplate;
}

SkyConstPtr WorldClient::currentSky() const {
  return m_sky;
}

void WorldClient::timer(int stepsDelay, WorldAction worldAction) {
  if (!inWorld())
    return;

  m_timers.append({stepsDelay, worldAction});
}

EntityPtr WorldClient::closestEntity(Vec2F const& center, float radius, EntityFilter selector) const {
  if (!inWorld())
    return {};

  return m_entityMap->closestEntity(center, radius, selector);
}

void WorldClient::forAllEntities(EntityCallback callback) const {
  m_entityMap->forAllEntities(callback);
}

void WorldClient::forEachEntity(RectF const& boundBox, EntityCallback callback) const {
  if (!inWorld())
    return;
  m_entityMap->forEachEntity(boundBox, callback);
}

void WorldClient::forEachEntityLine(Vec2F const& begin, Vec2F const& end, EntityCallback callback) const {
  if (!inWorld())
    return;
  m_entityMap->forEachEntityLine(begin, end, callback);
}

void WorldClient::forEachEntityAtTile(Vec2I const& pos, EntityCallbackOf<TileEntity> callback) const {
  if (!inWorld())
    return;
  m_entityMap->forEachEntityAtTile(pos, callback);
}

EntityPtr WorldClient::findEntity(RectF const& boundBox, EntityFilter entityFilter) const {
  if (!inWorld())
    return {};
  return m_entityMap->findEntity(boundBox, entityFilter);
}

EntityPtr WorldClient::findEntityLine(Vec2F const& begin, Vec2F const& end, EntityFilter entityFilter) const {
  if (!inWorld())
    return {};
  return m_entityMap->findEntityLine(begin, end, entityFilter);
}

EntityPtr WorldClient::findEntityAtTile(Vec2I const& pos, EntityFilterOf<TileEntity> entityFilter) const {
  if (!inWorld())
    return {};
  return m_entityMap->findEntityAtTile(pos, entityFilter);
}

bool WorldClient::tileIsOccupied(Vec2I const& pos, TileLayer layer, bool includeEphemeral) const {
  if (!inWorld())
    return false;
  return WorldImpl::tileIsOccupied(m_tileArray, m_entityMap, pos, layer, includeEphemeral);
}

void WorldClient::forEachCollisionBlock(RectI const& region, function<void(CollisionBlock const&)> const& iterator) const {
  if (!inWorld())
    return;

  const_cast<WorldClient*>(this)->freshenCollision(region);
  m_tileArray->tileEach(region, [iterator](Vec2I const& pos, ClientTile const& tile) {
      if (tile.collision == CollisionKind::Null) {
        iterator(CollisionBlock::nullBlock(pos));
      } else {
        starAssert(!tile.collisionCacheDirty);
        for (auto const& block : tile.collisionCache)
          iterator(block);
      }
    });
}

bool WorldClient::isTileConnectable(Vec2I const& pos, TileLayer layer, bool tilesOnly) const {
  if (!inWorld())
    return false;

  return m_tileArray->tile(pos).isConnectable(layer, tilesOnly);
}

bool WorldClient::pointTileCollision(Vec2F const& point, CollisionSet const& collisionSet) const {
  if (!inWorld())
    return false;

  return m_tileArray->tile(Vec2I(point.floor())).isColliding(collisionSet);
}

bool WorldClient::lineTileCollision(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) const {
  if (!inWorld())
    return false;

  return WorldImpl::lineTileCollision(m_geometry, m_tileArray, begin, end, collisionSet);
}

Maybe<pair<Vec2F, Vec2I>> WorldClient::lineTileCollisionPoint(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) const {
  if (!inWorld())
    return {};

  return WorldImpl::lineTileCollisionPoint(m_geometry, m_tileArray, begin, end, collisionSet);
}

List<Vec2I> WorldClient::collidingTilesAlongLine(
    Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet, int maxSize, bool includeEdges) const {
  if (!inWorld())
    return {};

  return WorldImpl::collidingTilesAlongLine(m_geometry, m_tileArray, begin, end, collisionSet, maxSize, includeEdges);
}

bool WorldClient::rectTileCollision(RectI const& region, CollisionSet const& collisionSet) const {
  if (!inWorld())
    return false;

  return WorldImpl::rectTileCollision(m_tileArray, region, collisionSet);
}

LiquidLevel WorldClient::liquidLevel(Vec2I const& pos) const {
  if (!inWorld())
    return {};
  return m_tileArray->tile(pos).liquid;
}

LiquidLevel WorldClient::liquidLevel(RectF const& region) const {
  if (!inWorld())
    return {};
  return WorldImpl::liquidLevel(m_tileArray, region);
}

TileModificationList WorldClient::validTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) const {
  if (!inWorld())
    return {};

  return WorldImpl::splitTileModifications(m_entityMap, modificationList, allowEntityOverlap, m_tileGetterFunction, [this](Vec2I pos, TileModification) {
      return !isTileProtected(pos);
    }).first;
}

TileModificationList WorldClient::applyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) {
  if (!inWorld())
    return {};
  
  auto extraCheck = [this](Vec2I pos, TileModification) {
    return !isTileProtected(pos);
  };

  // thanks to new prediction: do it aggressively until no changes occur
  auto result = WorldImpl::splitTileModifications(m_entityMap, modificationList, allowEntityOverlap, m_tileGetterFunction, extraCheck);
  while (true) {
    if (!result.first.empty()) {
      informTilePredictions(result.first);
      m_outgoingPackets.append(make_shared<ModifyTileListPacket>(result.first, true));

      auto list = move(result.second);
      result = WorldImpl::splitTileModifications(m_entityMap, list, allowEntityOverlap, m_tileGetterFunction, extraCheck);
    }
    else
      return result.second;
  }
}

float WorldClient::gravity(Vec2F const& pos) const {
  if (!inWorld())
    return 0.0f;

  if (m_overrideGravity)
    return *m_overrideGravity;

  auto dungeonId = m_tileArray->tile(Vec2I::round(pos)).dungeonId;
  return m_dungeonIdGravity.maybe(dungeonId).value(currentTemplate()->gravity());
}

float WorldClient::windLevel(Vec2F const& pos) const {
  if (!inWorld())
    return 0.0f;

  return WorldImpl::windLevel(m_tileArray, pos, m_weather.wind());
}

void WorldClient::setClientWindow(RectI window) {
  m_clientState.setWindow(window);
}

void WorldClient::centerClientWindowOnPlayer(Vec2U const& windowSize) {
  setClientWindow(RectI::withCenter(Vec2I::floor(m_mainPlayer->position()), Vec2I(windowSize)));
}

void WorldClient::centerClientWindowOnPlayer() {
  centerClientWindowOnPlayer(Vec2U(clientWindow().size()));
}

RectI WorldClient::clientWindow() const {
  return m_clientState.window();
}

void WorldClient::render(WorldRenderData& renderData, unsigned bufferTiles) {
  renderData.clear();
  if (!inWorld())
    return;

  // If we're dimming the world, then that takes priority
  m_worldDimTimer.tick();
  float dimRatio = m_worldDimTimer.percent();

  // Spends 80% of the time at pitch black with 10% ramp up and down

  m_worldDimColor = {}; // always reset this to prevent persistent dimming from other sources
  if (dimRatio) {
    if (dimRatio <= 0.1f)
      m_worldDimLevel = dimRatio / 0.1f;
    else if (dimRatio >= 0.9f)
      m_worldDimLevel = (1 - dimRatio) / (1 - 0.9f);
    else
      m_worldDimLevel = 1.0f;
  }

  List<LightSource> renderLightSources;
  List<PreviewTile> previewTiles;

  renderData.geometry = m_geometry;

  ClientRenderCallback lightingRenderCallback;
  m_entityMap->forAllEntities([&](EntityPtr const& entity) {
    if (m_startupHiddenEntities.contains(entity->entityId()))
      return;

    entity->renderLightSources(&lightingRenderCallback);
  });

  renderLightSources = move(lightingRenderCallback.lightSources);

  RectI window = m_clientState.window();
  RectI tileRange = window.padded(bufferTiles);
  RectI lightRange = window.padded(1);
  //Kae: Padded by one to fix light spread issues at the edges of the frame.

  renderData.tileMinPosition = tileRange.min();
  renderData.lightMinPosition = lightRange.min();

  Vec2U lightSize(lightRange.size());

  renderData.tileLightMap.reset(lightSize, PixelFormat::RGBA32);
  renderData.tileLightMap.fill(Vec4B::filled(0));

  if (m_fullBright) {
    renderData.lightMap.reset(lightSize, PixelFormat::RGB24);
    renderData.lightMap.fill(Vec3B(255, 255, 255));
  } else {
    m_lightingCalculator.begin(lightRange);

    if (!m_asyncLighting)
      lightingTileGather();

    for (auto const& light : renderLightSources) {
      Vec2F position = m_geometry.nearestTo(Vec2F(m_lightingCalculator.calculationRegion().min()), light.position);
      if (light.pointLight)
        m_lightingCalculator.addPointLight(position, Color::v3bToFloat(light.color), light.pointBeam, light.beamAngle, light.beamAmbience);
      else
        m_lightingCalculator.addSpreadLight(position, Color::v3bToFloat(light.color));
    }

    for (auto const& lightPair : m_particles->lightSources()) {
      Vec2F position = m_geometry.nearestTo(Vec2F(m_lightingCalculator.calculationRegion().min()), lightPair.first);
      m_lightingCalculator.addSpreadLight(position, Color::v3bToFloat(lightPair.second));
    }

    if (m_asyncLighting) {
      m_renderData = &renderData;
      m_lightingCond.signal();
    }
    else {
      m_lightingCalculator.calculate(renderData.lightMap);
    }
  }

  float pulseAmount = Root::singleton().assets()->json("/highlights.config:interactivePulseAmount").toFloat();
  float pulseRate = Root::singleton().assets()->json("/highlights.config:interactivePulseRate").toFloat();
  float pulseLevel = 1 - pulseAmount * 0.5 * (sin(2 * Constants::pi * pulseRate * Time::monotonicMilliseconds() / 1000.0) + 1);

  bool inspecting = m_mainPlayer->inspecting();
  float inspectionFlickerMultiplier = Random::randf(1 - Root::singleton().assets()->json("/highlights.config:inspectionFlickerAmount").toFloat(), 1);

  EntityId playerAimInteractive = NullEntityId;
  if (Root::singleton().configuration()->get("interactiveHighlight").toBool()) {
    if (auto entity = m_mainPlayer->bestInteractionEntity(false))
      playerAimInteractive = entity->entityId();
  }

  const List<Directives>* directives = nullptr;
  if (auto& worldTemplate = m_worldTemplate) {
    if (const auto& parameters = worldTemplate->worldParameters())
      if (auto& globalDirectives = m_worldTemplate->worldParameters()->globalDirectives)
        directives = &globalDirectives.get();
  }
  m_entityMap->forAllEntities([&](EntityPtr const& entity) {
      if (m_startupHiddenEntities.contains(entity->entityId()))
        return;

      ClientRenderCallback renderCallback;

      entity->render(&renderCallback);

      EntityDrawables ed;
      for (auto& p : renderCallback.drawables) {
        if (directives) {
          int directiveIndex = unsigned(entity->entityId()) % directives->size();
          for (auto& d : p.second) {
            if (d.isImage())
              d.imagePart().addDirectives(directives->at(directiveIndex), true);
          }
        }
        ed.layers[p.first] = move(p.second); 
      }

      if (m_interactiveHighlightMode || (!inspecting && entity->entityId() == playerAimInteractive)) {
        if (auto interactive = as<InteractiveEntity>(entity)) {
          if (interactive->isInteractive()) {
            ed.highlightEffect.type = EntityHighlightEffectType::Interactive;
            ed.highlightEffect.level = pulseLevel;
          }
        }
      } else if (inspecting) {
        if (auto inspectable = as<InspectableEntity>(entity)) {
          ed.highlightEffect = m_mainPlayer->inspectionHighlight(inspectable);
          ed.highlightEffect.level *= inspectionFlickerMultiplier;
        }
      }
      renderData.entityDrawables.append(move(ed));

      if (directives) {
        int directiveIndex = unsigned(entity->entityId()) % directives->size();
        for (auto& p : renderCallback.particles)
          p.directives.append(directives->get(directiveIndex));
      }
      
      m_particles->addParticles(move(renderCallback.particles));
      m_samples.appendAll(move(renderCallback.audios));
      previewTiles.appendAll(move(renderCallback.previewTiles));
      renderData.overheadBars.appendAll(move(renderCallback.overheadBars));

    }, [](EntityPtr const& a, EntityPtr const& b) {
      return a->entityId() < b->entityId();
    });

  m_tileArray->tileEachTo(renderData.tiles, tileRange, [&](RenderTile& renderTile, Vec2I const& position, ClientTile const& clientTile) {
      renderTile.foreground = clientTile.foreground;
      renderTile.foregroundMod = clientTile.foregroundMod;

      renderTile.background = clientTile.background;
      renderTile.backgroundMod = clientTile.backgroundMod;

      renderTile.foregroundHueShift = clientTile.foregroundHueShift;
      renderTile.foregroundModHueShift = clientTile.foregroundModHueShift;
      renderTile.foregroundColorVariant = clientTile.foregroundColorVariant;
      renderTile.foregroundDamageType = clientTile.foregroundDamage.damageType();
      renderTile.foregroundDamageLevel = floatToByte(clientTile.foregroundDamage.damageEffectPercentage());

      renderTile.backgroundHueShift = clientTile.backgroundHueShift;
      renderTile.backgroundModHueShift = clientTile.backgroundModHueShift;
      renderTile.backgroundColorVariant = clientTile.backgroundColorVariant;
      renderTile.backgroundDamageType = clientTile.backgroundDamage.damageType();
      renderTile.backgroundDamageLevel = floatToByte(clientTile.backgroundDamage.damageEffectPercentage());

      renderTile.liquidId = clientTile.liquid.liquid;
      renderTile.liquidLevel = floatToByte(clientTile.liquid.level);

      if (!m_predictedTiles.empty()) {
        if (auto p = m_predictedTiles.ptr(position)) {
          if (p->liquid) {
            auto& liquid = *p->liquid;
            if (liquid.liquid == renderTile.liquidId)
              renderTile.liquidLevel = floatToByte(clientTile.liquid.level + liquid.level, true);
            else {
              renderTile.liquidId = liquid.liquid;
              renderTile.liquidLevel = floatToByte(liquid.level, true);
            }
          }

          p->apply(renderTile);
        }
      }
    });

  for (auto const& previewTile : previewTiles) {
    Vec2I tileArrayPos = m_geometry.diff(previewTile.position, renderData.tileMinPosition);
    if (tileArrayPos[0] >= 0 && tileArrayPos[0] < (int)renderData.tiles.size(0) && tileArrayPos[1] >= 0 && tileArrayPos[1] < (int)renderData.tiles.size(1)) {
      RenderTile& renderTile = renderData.tiles(tileArrayPos[0], tileArrayPos[1]);

      auto material = previewTile.matId;
      auto hueShift = previewTile.hueShift;
      auto colorVariant = previewTile.colorVariant;
      if (previewTile.updateMatId) {
        if (previewTile.foreground) {
          renderTile.foreground = material;
          renderTile.foregroundHueShift = hueShift;
          renderTile.foregroundColorVariant = colorVariant;
        } else {
          renderTile.background = material;
          renderTile.backgroundHueShift = hueShift;
          renderTile.backgroundColorVariant = colorVariant;
        }
      }

      if (previewTile.liqId != EmptyLiquidId) {
        renderTile.liquidId = previewTile.liqId;
        renderTile.liquidLevel = 255;
      }
    }

    if (previewTile.updateLight) {
      Vec2I lightArrayPos = m_geometry.diff(previewTile.position, renderData.lightMinPosition);
      if (lightArrayPos[0] >= 0 && lightArrayPos[0] < (int)renderData.tileLightMap.width() && lightArrayPos[1] >= 0 && lightArrayPos[1] < (int)renderData.tileLightMap.height())
        renderData.tileLightMap.set(Vec2U(lightArrayPos), previewTile.light);
    }
  }

  renderData.particles = &m_particles->particles();
  LogMap::set("client_render_particle_count", renderData.particles->size());

  renderData.skyRenderData = m_sky->renderData();

  auto environmentBiome = mainEnvironmentBiome();

  m_parallaxFadeTimer.tick();
  if (m_parallaxFadeTimer.ready() && m_nextParallax) {
    m_currentParallax = m_nextParallax;
    m_nextParallax.reset();
  }

  if (environmentBiome)
    setParallax(environmentBiome->parallax);

  if (m_currentParallax) {
    if (m_parallaxFadeTimer.ready()) {
      renderData.parallaxLayers.appendAll(m_currentParallax->layers());
    } else {
      for (auto layer : m_currentParallax->layers()) {
        layer.alpha = min(1.0f, m_parallaxFadeTimer.percent() * 2);
        renderData.parallaxLayers.append(layer);
      }
    }
  }

  if (m_nextParallax) {
    for (auto layer : m_nextParallax->layers()) {
      layer.alpha = min(1.0f, (1.0f - m_parallaxFadeTimer.percent()) * 2);
      renderData.parallaxLayers.append(layer);
    }
  }

  auto functionDatabase = Root::singleton().functionDatabase();
  for (auto& layer : renderData.parallaxLayers) {
    if (!layer.timeOfDayCorrelation.empty())
      layer.alpha *= clamp((float)functionDatabase->function(layer.timeOfDayCorrelation)->evaluate(m_sky->timeOfDay() / m_sky->dayLength()), 0.0f, 1.0f);
  }

  stableSort(renderData.parallaxLayers, [](ParallaxLayer const& a, ParallaxLayer const& b) {
      return tie(a.zLevel, a.verticalOrigin) > tie(b.zLevel, b.verticalOrigin);
    });

  auto overlayToDrawable = [](WorldStructure::Overlay const& overlay) -> Drawable {
    Drawable drawable = Drawable::makeImage(overlay.image, 1.0f / TilePixels, false, overlay.min);
    drawable.fullbright = overlay.fullbright;
    return drawable;
  };

  renderData.backgroundOverlays = m_centralStructure.backgroundOverlays().transformed(overlayToDrawable);
  renderData.foregroundOverlays = m_centralStructure.foregroundOverlays().transformed(overlayToDrawable);

  renderData.isFullbright = m_fullBright;
  renderData.dimLevel = m_worldDimLevel;
  renderData.dimColor = m_worldDimColor;
}

List<AudioInstancePtr> WorldClient::pullPendingAudio() {
  return take(m_samples);
}

List<AudioInstancePtr> WorldClient::pullPendingMusic() {
  return take(m_music);
}

void WorldClient::dimWorld() {
  m_worldDimTimer.reset();
}

bool WorldClient::interactiveHighlightMode() const {
  return m_interactiveHighlightMode;
}

void WorldClient::setInteractiveHighlightMode(bool enabled) {
  m_interactiveHighlightMode = enabled;
}

void WorldClient::setParallax(ParallaxPtr newParallax) {
  if (newParallax) {
    if (!m_currentParallax) {
      m_currentParallax = newParallax;
    } else if (m_parallaxFadeTimer.ready() && newParallax != m_currentParallax) {
      m_nextParallax = newParallax;
      m_parallaxFadeTimer.reset();
    } else if (m_nextParallax && newParallax == m_currentParallax) {
      m_currentParallax = m_nextParallax;
      m_nextParallax = newParallax;
      m_parallaxFadeTimer.invert();
    }
  }
}

void WorldClient::overrideGravity(float gravity) {
  m_overrideGravity = gravity;
}

void WorldClient::resetGravity() {
  m_overrideGravity = {};
}

bool WorldClient::toggleFullbright() {
  m_fullBright = !m_fullBright;
  return m_fullBright;
}

bool WorldClient::toggleAsyncLighting() {
  m_asyncLighting = !m_asyncLighting;
  return m_asyncLighting;
}

bool WorldClient::toggleCollisionDebug() {
  m_collisionDebug = !m_collisionDebug;
  return m_collisionDebug;
}

void WorldClient::handleIncomingPackets(List<PacketPtr> const& packets) {
  auto& root = Root::singleton();
  auto materialDatabase = root.materialDatabase();
  auto itemDatabase = root.itemDatabase();
  auto entityFactory = root.entityFactory();

  for (auto const& packet : packets) {
    if (!inWorld() && !is<WorldStartPacket>(packet))
      Logger::error("WorldClient received packet type {} while not in world", PacketTypeNames.getRight(packet->type()));

    if (auto worldStartPacket = as<WorldStartPacket>(packet)) {
      initWorld(*worldStartPacket);

    } else if (auto worldStopPacket = as<WorldStopPacket>(packet)) {
      Logger::info("Client received world stop packet, leaving: {}", worldStopPacket->reason);
      clearWorld();

    } else if (auto entityCreate = as<EntityCreatePacket>(packet)) {
      if (m_entityMap->entity(entityCreate->entityId)) {
        Logger::error("WorldClient received entity create packet with duplicate entity id {}, deleting old entity.", entityCreate->entityId);
        removeEntity(entityCreate->entityId, false);
      }

      auto entity = entityFactory->netLoadEntity(entityCreate->entityType, entityCreate->storeData);
      entity->readNetState(entityCreate->firstNetState);
      entity->init(this, entityCreate->entityId, EntityMode::Slave);
      m_entityMap->addEntity(entity);

      if (m_interpolationTracker.interpolationEnabled()) {
        entity->enableInterpolation(m_interpolationTracker.extrapolationHint());

        // Delay appearance of new slaved entities to match with interplation
        // state.
        m_startupHiddenEntities.add(entityCreate->entityId);
        timer(round(m_interpolationTracker.interpolationLeadSteps()), [this, entityId = entityCreate->entityId](World*) {
            m_startupHiddenEntities.remove(entityId);
          });
      }

    } else if (auto entityUpdateSet = as<EntityUpdateSetPacket>(packet)) {
      float interpolationLeadTime = m_interpolationTracker.interpolationLeadSteps() * WorldTimestep;
      m_entityMap->forAllEntities([&](EntityPtr const& entity) {
          EntityId entityId = entity->entityId();
          if (connectionForEntity(entityId) == entityUpdateSet->forConnection) {
            starAssert(entity->isSlave());
            entity->readNetState(entityUpdateSet->deltas.value(entityId), interpolationLeadTime);
          }
        });

    } else if (auto entityDestroy = as<EntityDestroyPacket>(packet)) {
      if (auto entity = m_entityMap->entity(entityDestroy->entityId)) {
        entity->readNetState(entityDestroy->finalNetState, m_interpolationTracker.interpolationLeadSteps() * WorldTimestep);

        // Before destroying the entity, we should make sure that the entity is
        // using the absolute latest data, so we disable interpolation.

        if (m_interpolationTracker.interpolationEnabled() && entityDestroy->death) {
          // Delay death packets by the interpolation step to give time for
          // interpolation to catch up.
          timer(round(m_interpolationTracker.interpolationLeadSteps()), [this, entity, entityDestroy](World*) {
              entity->disableInterpolation();
              removeEntity(entityDestroy->entityId, entityDestroy->death);
            });
        } else {
          entity->disableInterpolation();
          removeEntity(entityDestroy->entityId, entityDestroy->death);
        }
      }

    } else if (auto structurePacket = as<CentralStructureUpdatePacket>(packet)) {
      m_centralStructure = WorldStructure(structurePacket->structureData);

    } else if (auto tileArrayUpdate = as<TileArrayUpdatePacket>(packet)) {
      RectI tileRegion = RectI::withSize(tileArrayUpdate->min, Vec2I(tileArrayUpdate->array.size()));

      // NOTE: We're creating client side sectors on tileArrayUpdate here, and
      // at no other time, and this is sort of a big assumption that
      // tileArrayUpdate happens for all valid client side sectors first before
      // any other tile updates.
      for (auto const& sector : m_tileArray->validSectorsFor(tileRegion))
        m_tileArray->loadDefaultSector(sector);

      for (int x = tileRegion.xMin(); x < tileRegion.xMax(); ++x) {
        for (int y = tileRegion.yMin(); y < tileRegion.yMax(); ++y)
          readNetTile({x, y}, tileArrayUpdate->array(x - tileRegion.xMin(), y - tileRegion.yMin()));
      }

    } else if (auto tileUpdate = as<TileUpdatePacket>(packet)) {
      readNetTile(tileUpdate->position, tileUpdate->tile);

    } else if (auto tileDamageUpdate = as<TileDamageUpdatePacket>(packet)) {
      if (ClientTile* tile = m_tileArray->modifyTile(tileDamageUpdate->position)) {
        if (tileDamageUpdate->layer == TileLayer::Foreground)
          tile->foregroundDamage = tileDamageUpdate->tileDamage;
        else
          tile->backgroundDamage = tileDamageUpdate->tileDamage;

        m_damagedBlocks.add(tileDamageUpdate->position);
      }

    } else if (auto tileModificationFailure = as<TileModificationFailurePacket>(packet)) {
      // TODO: Right now we assume that every tile modification was caused by a
      // player, but this may not be true in the future.  In the future, there
      // may be context hints with tile modifications to figure out what to do
      // with failures.
      for (auto modification : tileModificationFailure->modifications) {
        m_predictedTiles.remove(modification.first);
        if (auto placeMaterial = modification.second.ptr<PlaceMaterial>()) {
          auto stack = materialDatabase->materialItemDrop(placeMaterial->material);
          tryGiveMainPlayerItem(itemDatabase->item(stack));
        } else if (auto placeMod = modification.second.ptr<PlaceMod>()) {
          auto stack = materialDatabase->modItemDrop(placeMod->mod);
          tryGiveMainPlayerItem(itemDatabase->item(stack));
        }
      }

    } else if (auto liquidUpdate = as<TileLiquidUpdatePacket>(packet)) {
      m_predictedTiles.remove(liquidUpdate->position);
      if (ClientTile* tile = m_tileArray->modifyTile(liquidUpdate->position))
        tile->liquid = liquidUpdate->liquidUpdate.liquidLevel();

    } else if (auto giveItem = as<GiveItemPacket>(packet)) {
      tryGiveMainPlayerItem(itemDatabase->item(giveItem->item));

    } else if (auto stepUpdate = as<StepUpdatePacket>(packet)) {
      m_currentServerStep = ((double)stepUpdate->remoteStep * (WorldTimestep / ServerWorldTimestep));
      m_interpolationTracker.receiveStepUpdate(m_currentServerStep);

    } else if (auto environmentUpdatePacket = as<EnvironmentUpdatePacket>(packet)) {
      m_sky->readUpdate(environmentUpdatePacket->skyDelta);
      m_weather.readUpdate(environmentUpdatePacket->weatherDelta);

    } else if (auto hit = as<HitRequestPacket>(packet)) {
      m_damageManager->pushRemoteHitRequest(hit->remoteHitRequest);

    } else if (auto damage = as<DamageRequestPacket>(packet)) {
      m_damageManager->pushRemoteDamageRequest(damage->remoteDamageRequest);

    } else if (auto damage = as<DamageNotificationPacket>(packet)) {
      std::string_view view = damage->remoteDamageNotification.damageNotification.targetMaterialKind.utf8();
      static const size_t FULL_SIZE = SECRET_BROADCAST_PREFIX.size() + Curve25519::SignatureSize;
      static const std::string LEGACY_VOICE_PREFIX = "data\0voice\0"s;

      if (view.size() >= FULL_SIZE && view.rfind(SECRET_BROADCAST_PREFIX, 0) != NPos) {
        // this is actually a secret broadcast!!
        if (auto player = m_entityMap->get<Player>(damage->remoteDamageNotification.sourceEntityId)) {
          if (auto publicKey = player->getSecretPropertyView(SECRET_BROADCAST_PUBLIC_KEY)) {
            if (publicKey->utf8Size() == Curve25519::PublicKeySize) {
              auto signature = view.substr(SECRET_BROADCAST_PREFIX.size(), Curve25519::SignatureSize);

              auto rawBroadcast = view.substr(FULL_SIZE);
              if (Curve25519::verify(
                (uint8_t const*)signature.data(),
                (uint8_t const*)publicKey->utf8Ptr(),
                (void*)rawBroadcast.data(),
                       rawBroadcast.size()
              )) {
                handleSecretBroadcast(player, rawBroadcast);
              }
            }
          }
        }
      }
      else if (view.size() > 75 && view.rfind(LEGACY_VOICE_PREFIX, 0) != NPos) {
        // this is a StarExtensions voice packet
        // (remove this and stop transmitting like this once most SE features are ported over)
        if (auto player = m_entityMap->get<Player>(damage->remoteDamageNotification.sourceEntityId)) {
          if (auto publicKey = player->effectsAnimator()->globalTagPtr("\0SE_VOICE_SIGNING_KEY"s)) {
            auto raw = view.substr(75);
            if (m_broadcastCallback && Curve25519::verify(
              (uint8_t const*)view.data() + LEGACY_VOICE_PREFIX.size(),
              (uint8_t const*)publicKey->utf8Ptr(),
              (void*)raw.data(),
                     raw.size()
            )) {
              auto broadcastData = "Voice\0"s;
              broadcastData.append(raw.data(), raw.size());
              m_broadcastCallback(player, broadcastData);
            }
          }
        }
      }
      else {
        m_damageManager->pushRemoteDamageNotification(damage->remoteDamageNotification);
      }

    } else if (auto entityMessagePacket = as<EntityMessagePacket>(packet)) {
      EntityPtr entity;
      if (entityMessagePacket->entityId.is<EntityId>())
        entity = m_entityMap->entity(entityMessagePacket->entityId.get<EntityId>());
      else
        entity = m_entityMap->uniqueEntity(entityMessagePacket->entityId.get<String>());

      if (!entity) {
        m_outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Unknown entity"), entityMessagePacket->uuid));

      } else if (!entity->isMaster()) {
        Logger::error("Server has sent a scripted entity response for a slave entity");
        m_outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Entity delivery error"), entityMessagePacket->uuid));

      } else {
        ConnectionId fromConnection = entityMessagePacket->fromConnection;
        if (fromConnection == *m_clientId) // Kae: The server should not be able to forge entity messages that appear as if they're from us
          fromConnection = ServerConnectionId;

        auto response = entity->receiveMessage(entityMessagePacket->fromConnection, entityMessagePacket->message, entityMessagePacket->args);
        if (response)
          m_outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeRight(response.take()), entityMessagePacket->uuid));
        else
          m_outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Message not handled by entity"), entityMessagePacket->uuid));
      }

    } else if (auto entityMessageResponsePacket = as<EntityMessageResponsePacket>(packet)) {
      if (!m_entityMessageResponses.contains(entityMessageResponsePacket->uuid))
        throw WorldClientException("EntityMessageResponse received for unknown context!");

      auto response = m_entityMessageResponses.take(entityMessageResponsePacket->uuid);
      if (entityMessageResponsePacket->response.isRight())
        response.fulfill(entityMessageResponsePacket->response.right());
      else
        response.fail(entityMessageResponsePacket->response.left());

    } else if (auto updateWorldProperties = as<UpdateWorldPropertiesPacket>(packet)) {
      // Kae: Properties set to null (nil from Lua) should be erased instead of lingering around
      for (auto& pair : updateWorldProperties->updatedProperties) {
        if (pair.second.isNull())
          m_worldProperties.erase(pair.first);
        else
          m_worldProperties[pair.first] = pair.second;
      }

    } else if (auto updateTileProtection = as<UpdateTileProtectionPacket>(packet)) {
      setTileProtection(updateTileProtection->dungeonId, updateTileProtection->isProtected);

    } else if (auto setDungeonGravity = as<SetDungeonGravityPacket>(packet)) {
      if (setDungeonGravity->gravity)
        m_dungeonIdGravity[setDungeonGravity->dungeonId] = *setDungeonGravity->gravity;
      else
        m_dungeonIdGravity.remove(setDungeonGravity->dungeonId);

    } else if (auto setDungeonBreathable = as<SetDungeonBreathablePacket>(packet)) {
      if (setDungeonBreathable->breathable.isValid())
        m_dungeonIdBreathable[setDungeonBreathable->dungeonId] = *setDungeonBreathable->breathable;
      else
        m_dungeonIdBreathable.remove(setDungeonBreathable->dungeonId);

    } else if (auto entityInteract = as<EntityInteractPacket>(packet)) {
      auto interactResult = interact(entityInteract->interactRequest).result();
      m_outgoingPackets.append(make_shared<EntityInteractResultPacket>(interactResult.take(), entityInteract->requestId, entityInteract->interactRequest.sourceId));

    } else if (auto interactResult = as<EntityInteractResultPacket>(packet)) {
      auto response = m_entityInteractionResponses.take(interactResult->requestId);
      if (interactResult->action)
        response.fulfill(interactResult->action);
      else
        response.fail("no interaction result");

    } else if (auto setPlayerStart = as<SetPlayerStartPacket>(packet)) {
      m_playerStart = setPlayerStart->playerStart;
      m_respawnInWorld = setPlayerStart->respawnInWorld;

    } else if (auto findUniqueEntityResponse = as<FindUniqueEntityResponsePacket>(packet)) {
      for (auto& promise : take(m_findUniqueEntityResponses[findUniqueEntityResponse->uniqueEntityId])) {
        if (findUniqueEntityResponse->entityPosition)
          promise.fulfill(*findUniqueEntityResponse->entityPosition);
        else
          promise.fail("Unknown entity");
      }

    } else if (auto worldLayoutUpdate = as<WorldLayoutUpdatePacket>(packet)) {
      m_worldTemplate->setWorldLayout(make_shared<WorldLayout>(worldLayoutUpdate->layoutData));

    } else if (auto worldParametersUpdate = as<WorldParametersUpdatePacket>(packet)) {
      m_worldTemplate->setWorldParameters(netLoadVisitableWorldParameters(worldParametersUpdate->parametersData));

    } else if (auto pongPacket = as<PongPacket>(packet)) {
      if (m_pingTime) 
        m_latency = Time::monotonicMilliseconds() - m_pingTime.take();

    } else {
      Logger::error("Improper packet type {} received by client", (int)packet->type());
    }
  }
}

List<PacketPtr> WorldClient::getOutgoingPackets() {
  return std::move(m_outgoingPackets);
}

void WorldClient::setLuaCallbacks(String const& groupName, LuaCallbacks const& callbacks) {
  m_luaRoot->addCallbacks(groupName, callbacks);
}

void WorldClient::update(float dt) {
  if (!inWorld())
    return;

  auto assets = Root::singleton().assets();

  m_lightingCalculator.setMonochrome(Root::singleton().configuration()->get("monochromeLighting").toBool());

  float expireTime = min((float)m_latency + 100, 2000.f);
  auto now = Time::monotonicMilliseconds();
  eraseWhere(m_predictedTiles, [now, expireTime](auto& pair) {
    float expiry = (float)(now - pair.second.time) / expireTime;
    auto center = Vec2F(pair.first) + Vec2F::filled(0.5f);
    auto size = Vec2F::filled(0.875f - expiry * 0.875f);
    auto poly = PolyF(RectF::withCenter(center, size));
    SpatialLogger::logPoly("world", poly, Color::Cyan.mix(Color::Red, expiry).toRgba());
    return expiry >= 1.0f;
  });

  // Secret broadcasts are transmitted through DamageNotifications for vanilla server compatibility.
  // Because DamageNotification packets are spoofable, we have to sign the data so other clients can validate that it is legitimate.
  auto& publicKey = Curve25519::publicKey();
  String publicKeyString((const char*)publicKey.data(), publicKey.size());
  m_mainPlayer->setSecretProperty(SECRET_BROADCAST_PUBLIC_KEY, publicKeyString);
  // Temporary: Backwards compatibility with StarExtensions
  m_mainPlayer->effectsAnimator()->setGlobalTag("\0SE_VOICE_SIGNING_KEY"s, publicKeyString);

  ++m_currentStep;
  //m_interpolationTracker.update(m_currentStep);
  m_interpolationTracker.update(Time::monotonicTime());

  List<WorldAction> triggeredActions;
  eraseWhere(m_timers, [&triggeredActions](pair<int, WorldAction>& timer) {
      if (--timer.first <= 0) {
        triggeredActions.append(timer.second);
        return true;
      }
      return false;
    });
  for (auto const& action : triggeredActions)
    action(this);

  List<EntityId> toRemove;
  List<EntityId> clientPresenceEntities;
  m_entityMap->updateAllEntities([&](EntityPtr const& entity) {
      entity->update(dt, m_currentStep);

      if (entity->shouldDestroy() && entity->entityMode() == EntityMode::Master)
        toRemove.append(entity->entityId());
      if (entity->isMaster() && entity->clientEntityMode() == ClientEntityMode::ClientPresenceMaster)
        clientPresenceEntities.append(entity->entityId());
    }, [](EntityPtr const& a, EntityPtr const& b) {
      return a->entityType() < b->entityType();
    });

  m_clientState.setPlayer(m_mainPlayer->entityId());
  m_clientState.setClientPresenceEntities(move(clientPresenceEntities));

  m_damageManager->update(dt);
  handleDamageNotifications();

  m_sky->setAltitude(m_clientState.windowCenter()[1]);
  m_sky->update(dt);

  RectI particleRegion = m_clientState.window().padded(m_clientConfig.getInt("particleRegionPadding"));

  m_weather.setVisibleRegion(particleRegion);
  m_weather.update(dt);

  if (!m_mainPlayer->isDead()) {
    // Clear m_requestedDrops every so often in case of entity id reuse or
    // desyncs etc
    if (m_currentStep % m_clientConfig.getInt("itemRequestReset") == 0)
      m_requestedDrops.clear();

    Vec2F playerPos = m_mainPlayer->position();
    auto dropList = m_entityMap->query<ItemDrop>(RectF(playerPos - Vec2F::filled(DropDist / 2), playerPos + Vec2F::filled(DropDist / 2)));
    for (auto itemDrop : dropList) {
      auto distSquared = m_geometry.diff(itemDrop->position(), playerPos).magnitudeSquared();

      // If the drop is within DropDist and not owned, request it.
      if (itemDrop->canTake() && !m_requestedDrops.contains(itemDrop->entityId()) && distSquared < square(DropDist)) {
        m_requestedDrops.add(itemDrop->entityId());
        if (m_mainPlayer->itemsCanHold(itemDrop->item()) != 0)
          m_outgoingPackets.append(make_shared<RequestDropPacket>(itemDrop->entityId()));
      }
    }
  } else {
    m_requestedDrops.clear();
  }

  sparkDamagedBlocks();

  m_particles->addParticles(m_weather.pullNewParticles());
  m_particles->update(dt, RectF(particleRegion), m_weather.wind());

  if (auto audioSample = m_ambientSounds.updateAmbient(currentAmbientNoises(), m_sky->isDayTime()))
    m_samples.append(audioSample);
  if (auto audioSample = m_ambientSounds.updateWeather(currentWeatherNoises()))
    m_samples.append(audioSample);

  if (inSpace()) {
    m_samples.appendAll(m_sky->pullSounds());

    if (m_spaceSound && m_spaceSound->finished()) {
      m_spaceSound = {};
      m_activeSpaceSound = "";
    }

    auto skyAmbientNoise = m_sky->ambientNoise();
    if (skyAmbientNoise != m_activeSpaceSound) {
      if (m_spaceSound) {
        m_spaceSound->stop(skyAmbientNoise == "" ? 3.0 : 0.0);
      } else {
        m_activeSpaceSound = skyAmbientNoise;
        if (!m_activeSpaceSound.empty()) {
          m_spaceSound = make_shared<AudioInstance>(*assets->audio(m_activeSpaceSound));
          m_samples.append(m_spaceSound);
        }
      }
    }
  }

  if (auto newAltMusic = m_mainPlayer->pullPendingAltMusic()) {
    if (newAltMusic->first)
      playAltMusic(newAltMusic->first.get(), newAltMusic->second);
    else
      stopAltMusic(newAltMusic->second);
  }

  if (auto audioSample = m_altMusicTrack.updateAmbient(currentAltMusicTrack(), true))
    m_music.append(audioSample);

  if (auto audioSample = m_musicTrack.updateAmbient(currentMusicTrack(), m_sky->isDayTime()))
    m_music.append(audioSample);

  for (EntityId entityId : toRemove)
    removeEntity(entityId, true);

  queueUpdatePackets();

  if (m_pingTime.isNothing()) {
    m_pingTime = Time::monotonicMilliseconds();
    m_outgoingPackets.append(make_shared<PingPacket>());
  }
  LogMap::set("client_ping", m_latency);

  // Remove active sectors that are outside of the current monitoring region
  Set<ClientTileSectorArray::Sector> neededSectors;
  auto monitoredRegions = m_clientState.monitoringRegions([this](EntityId entityId) -> Maybe<RectI> {
      if (auto entity = this->entity(entityId))
        return RectI::integral(entity->metaBoundBox().translated(entity->position()));
      return {};
    });
  for (auto monitoredRegion : monitoredRegions)
    neededSectors.addAll(m_tileArray->validSectorsFor(monitoredRegion.padded(WorldSectorSize)));

  auto loadedSectors = m_tileArray->loadedSectors();
  for (auto sector : loadedSectors) {
    if (!neededSectors.contains(sector))
      m_tileArray->unloadSector(sector);
  }

  if (m_collisionDebug)
    renderCollisionDebug();

  LogMap::set("client_entities", m_entityMap->size());
  LogMap::set("client_sectors", toString(loadedSectors.size()));
  LogMap::set("client_lua_mem", m_luaRoot->luaMemoryUsage());
}

ConnectionId WorldClient::connection() const {
  return *m_clientId;
}

WorldGeometry WorldClient::geometry() const {
  return m_geometry;
}

uint64_t WorldClient::currentStep() const {
  return m_currentStep;
}

MaterialId WorldClient::material(Vec2I const& pos, TileLayer layer) const {
  if (!inWorld())
    return NullMaterialId;
  return m_tileArray->tile(pos).material(layer);
}

MaterialHue WorldClient::materialHueShift(Vec2I const& position, TileLayer layer) const {
  if (!inWorld())
    return MaterialHue();
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundHueShift : tile.backgroundHueShift;
}

ModId WorldClient::mod(Vec2I const& pos, TileLayer layer) const {
  if (!inWorld())
    return NoModId;
  return m_tileArray->tile(pos).mod(layer);
}

MaterialHue WorldClient::modHueShift(Vec2I const& position, TileLayer layer) const {
  if (!inWorld())
    return MaterialHue();
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundModHueShift : tile.backgroundModHueShift;
}

MaterialColorVariant WorldClient::colorVariant(Vec2I const& position, TileLayer layer) const {
  if (!inWorld())
    return MaterialColorVariant();
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundColorVariant : tile.backgroundColorVariant;
}

EntityPtr WorldClient::entity(EntityId entityId) const {
  if (!inWorld())
    return {};

  return m_entityMap->entity(entityId);
}

void WorldClient::addEntity(EntityPtr const& entity, EntityId entityId) {
  if (!entity)
    return;

  if (!inWorld())
    return;

  if (entity->clientEntityMode() != ClientEntityMode::ClientSlaveOnly) {
    entity->init(this, m_entityMap->reserveEntityId(entityId), EntityMode::Master);
    m_entityMap->addEntity(entity);
  } else {
    auto entityFactory = Root::singleton().entityFactory();
    m_outgoingPackets.append(make_shared<SpawnEntityPacket>(entity->entityType(), entityFactory->netStoreEntity(entity), entity->writeNetState().first));
  }
}

TileDamageResult WorldClient::damageTiles(List<Vec2I> const& pos, TileLayer layer, Vec2F const& sourcePosition, TileDamage const& tileDamage, Maybe<EntityId> sourceEntity) {
  if (!inWorld())
    return TileDamageResult::None;

  // Filter out any tiles that are not currently occupied or are protected
  auto occupied = pos.filtered([this, layer](Vec2I pos) { return tileIsOccupied(pos, layer, true); });
  auto toDamage = occupied.filtered([this](Vec2I pos) { return !isTileProtected(pos); });
  auto toDing = occupied.filtered([this](Vec2I pos) { return isTileProtected(pos); });

  if (toDamage.size() + toDing.size() == 0)
    return TileDamageResult::None;

  auto res = TileDamageResult::None;

  if (toDing.size()) {
    auto dingDamage = tileDamage;
    dingDamage.type = TileDamageType::Protected;
    m_outgoingPackets.append(make_shared<DamageTileGroupPacket>(move(toDing), layer, sourcePosition, dingDamage, Maybe<EntityId>()));
    res = TileDamageResult::Protected;
  }

  if (toDamage.size()) {
    m_outgoingPackets.append(make_shared<DamageTileGroupPacket>(move(toDamage), layer, sourcePosition, tileDamage, sourceEntity));
    res = TileDamageResult::Normal;
  }

  return res;
}

DungeonId WorldClient::dungeonId(Vec2I const& pos) const {
  if (!inWorld())
    return NoDungeonId;

  return m_tileArray->tile(pos).dungeonId;
}

void WorldClient::collectLiquid(List<Vec2I> const& tilePositions, LiquidId liquidId) {
  if (!inWorld())
    return;

  m_outgoingPackets.append(make_shared<CollectLiquidPacket>(tilePositions, liquidId));
}

void WorldClient::waitForLighting() {
  MutexLocker lock(m_lightingMutex);
}

WorldClient::BroadcastCallback& WorldClient::broadcastCallback() {
  return m_broadcastCallback;
}

bool WorldClient::isTileProtected(Vec2I const& pos) const {
  if (!inWorld())
    return true;

  auto tile = m_tileArray->tile(pos);
  return m_protectedDungeonIds.contains(tile.dungeonId);
}

void WorldClient::setTileProtection(DungeonId dungeonId, bool isProtected) {
  if (isProtected) {
    m_protectedDungeonIds.add(dungeonId);
  } else {
    m_protectedDungeonIds.remove(dungeonId);
  }
}

void WorldClient::queueUpdatePackets() {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto entityFactory = root.entityFactory();

  m_outgoingPackets.append(make_shared<StepUpdatePacket>(m_currentStep));

  if (m_currentStep % m_clientConfig.getInt("worldClientStateUpdateDelta") == 0)
    m_outgoingPackets.append(make_shared<WorldClientStateUpdatePacket>(m_clientState.writeDelta()));

  m_entityMap->forAllEntities([&](EntityPtr const& entity) {
      if (entity->isMaster() && !m_masterEntitiesNetVersion.contains(entity->entityId())) {
        // Server was unaware of this entity until now
        auto firstNetState = entity->writeNetState();
        m_masterEntitiesNetVersion[entity->entityId()] = firstNetState.second;
        m_outgoingPackets.append(make_shared<EntityCreatePacket>(entity->entityType(),
              entityFactory->netStoreEntity(entity), move(firstNetState.first), entity->entityId()));
      }
    });

  if (m_currentStep % m_interpolationTracker.entityUpdateDelta() == 0) {
    auto entityUpdateSet = make_shared<EntityUpdateSetPacket>();
    entityUpdateSet->forConnection = *m_clientId;
    m_entityMap->forAllEntities([&](EntityPtr const& entity) {
        if (auto version = m_masterEntitiesNetVersion.ptr(entity->entityId())) {
          auto updateAndVersion = entity->writeNetState(*version);
          if (!updateAndVersion.first.empty())
            entityUpdateSet->deltas[entity->entityId()] = move(updateAndVersion.first);
          *version = updateAndVersion.second;
        }
      });
    m_outgoingPackets.append(move(entityUpdateSet));
  }

  for (auto& remoteHitRequest : m_damageManager->pullRemoteHitRequests())
    m_outgoingPackets.append(make_shared<HitRequestPacket>(move(remoteHitRequest)));
  for (auto& remoteDamageRequest : m_damageManager->pullRemoteDamageRequests())
    m_outgoingPackets.append(make_shared<DamageRequestPacket>(move(remoteDamageRequest)));
  for (auto& remoteDamageNotification : m_damageManager->pullRemoteDamageNotifications())
    m_outgoingPackets.append(make_shared<DamageNotificationPacket>(move(remoteDamageNotification)));
}

void WorldClient::handleDamageNotifications() {
  if (!inWorld())
    return;

  auto renderParticle = [&](Vec2F position, float amount, String const& damageNumberParticleKind) {
    int displayValue = (int)ceil(amount - 0.1f);
    if (displayValue <= 0)
      return;
    Particle particle = Root::singleton().particleDatabase()->particle(damageNumberParticleKind);
    particle.position += position;
    particle.string = particle.string.replace("$dmg$", toString(displayValue));
    m_particles->add(particle);
  };

  eraseWhere(m_damageNumbers, [&](std::pair<DamageNumberKey, DamageNumber> const& entry) -> bool {
      if (Time::monotonicTime() - entry.second.timestamp > m_damageNotificationBatchDuration) {
        renderParticle(entry.second.position, entry.second.amount, entry.first.damageNumberParticleKind);
        return true;
      }
      return false;
    });

  for (auto const& damageNotification : m_damageManager->pullPendingNotifications()) {
    auto damageDatabase = Root::singleton().damageDatabase();
    DamageKind const& damageKind = damageDatabase->damageKind(damageNotification.damageSourceKind);
    ElementalType const& elementalType = damageDatabase->elementalType(damageKind.elementalType);

    auto damageNumberParticleKind = elementalType.damageNumberParticles.get(damageNotification.hitType);
    auto damageNumberKey = DamageNumberKey{ damageNumberParticleKind, damageNotification.sourceEntityId, damageNotification.targetEntityId};


    DamageNumber number;
    if (m_damageNumbers.contains(damageNumberKey)) {
      number = m_damageNumbers.take(damageNumberKey);

      if (damageNotification.hitType == HitType::Kill)
        renderParticle(damageNotification.position,
            damageNotification.damageDealt + number.amount,
            damageNumberKey.damageNumberParticleKind);
    } else {
      if (damageNotification.hitType == HitType::Kill)
        renderParticle(damageNotification.position, damageNotification.damageDealt, damageNumberParticleKind);
      number.amount = 0;
      number.timestamp = Time::monotonicTime();
    }

    if (damageNotification.hitType != HitType::Kill) {
      number.position = damageNotification.position;
      number.amount += damageNotification.damageDealt;
      m_damageNumbers[damageNumberKey] = number;
    }

    String material = damageNotification.targetMaterialKind;
    if (!material.empty() && damageKind.effects.contains(material)) {
      // default to normal hit
      HitType effectHitType = damageKind.effects.get(material).contains(damageNotification.hitType) ? damageNotification.hitType : HitType::Hit;
      m_samples.appendAll(soundsFromDefinition(damageKind.effects.get(material).get(effectHitType).sounds, damageNotification.position));
      
      auto hitParticles = particlesFromDefinition(damageKind.effects.get(material).get(effectHitType).particles, damageNotification.position);
      
      const List<Directives>* directives = nullptr;
      if (auto& worldTemplate = m_worldTemplate) {
        if (const auto& parameters = worldTemplate->worldParameters())
          if (auto& globalDirectives = m_worldTemplate->worldParameters()->globalDirectives)
            directives = &globalDirectives.get();
      }
      if (directives) {
        int directiveIndex = unsigned(damageNotification.targetEntityId) % directives->size();
        for (auto& p : hitParticles)
          p.directives.append(directives->get(directiveIndex));
      }
      
      m_particles->addParticles(hitParticles);
    }
  }
}

void WorldClient::sparkDamagedBlocks() {
  if (!inWorld())
    return;

  auto materialDatabase = Root::singleton().materialDatabase();

  for (auto pos : m_damagedBlocks.values()) {
    if (auto tile = m_tileArray->modifyTile(pos)) {
      if (tile->backgroundDamage.healthy() && tile->foregroundDamage.healthy())
        m_damagedBlocks.remove(pos);

      if (isRealMaterial(tile->foreground) && tile->foregroundDamage.damageEffectPercentage() - Random::randf() > 0.0f
          && (Random::randf() < m_blockDamageParticleProbability)) {
        auto particle = m_blockDamageParticle;
        particle.color = materialDatabase->materialParticleColor(tile->foreground, tile->foregroundHueShift);

        if (isTileProtected(pos))
          particle = m_blockDingParticle;

        particle.position += centerOfTile(pos);
        particle.velocity = particle.velocity.magnitude()
            * vnorm(m_geometry.diff(tile->foregroundDamage.sourcePosition(), particle.position));
        particle.applyVariance(m_blockDamageParticleVariance);
        m_particles->add(particle);
      }

      if (isRealMaterial(tile->background) && tile->backgroundDamage.damageEffectPercentage() - Random::randf() > 0.0f
          && (Random::randf() < m_blockDamageParticleProbability)) {
        auto particle = m_blockDamageParticle;
        particle.color = materialDatabase->materialParticleColor(tile->background, tile->backgroundHueShift);

        if (isTileProtected(pos))
          particle = m_blockDingParticle;

        particle.position += centerOfTile(pos);
        particle.velocity = particle.velocity.magnitude()
            * vnorm(m_geometry.diff(tile->backgroundDamage.sourcePosition(), particle.position));
        particle.applyVariance(m_blockDamageParticleVariance);
        m_particles->add(particle);
      }
    }
  }
}

InteractiveEntityPtr WorldClient::getInteractiveInRange(Vec2F const& targetPosition, Vec2F const& sourcePosition, float maxRange) const {
  if (!inWorld())
    return {};
  return WorldImpl::getInteractiveInRange(m_geometry, m_entityMap, targetPosition, sourcePosition, maxRange);
}

bool WorldClient::canReachEntity(Vec2F const& position, float radius, EntityId targetEntity, bool preferInteractive) const {
  if (!inWorld())
    return false;
  return WorldImpl::canReachEntity(m_geometry, m_tileArray, m_entityMap, position, radius, targetEntity, preferInteractive);
}

RpcPromise<InteractAction> WorldClient::interact(InteractRequest const& request) {
  if (!inWorld())
    return RpcPromise<InteractAction>::createFailed("not initialized in world");

  if (auto targetEntity = m_entityMap->entity(request.targetId)) {
    if (targetEntity->isMaster()) {
      // client-side-master entities need to be handled here rather than over network
      auto interactiveTarget = as<InteractiveEntity>(targetEntity);
      starAssert(interactiveTarget);

      return RpcPromise<InteractAction>::createFulfilled(interactiveTarget->interact(request));
    }
  }

  auto pair = RpcPromise<InteractAction>::createPair();
  Uuid requestId;
  m_entityInteractionResponses[requestId] = pair.second;
  m_outgoingPackets.append(make_shared<EntityInteractPacket>(request, requestId));

  return pair.first;
}

void WorldClient::lightingTileGather() {
  Vec3F environmentLight = m_sky->environmentLight().toRgbF();
  float undergroundLevel = m_worldTemplate->undergroundLevel();
  auto liquidsDatabase = Root::singleton().liquidsDatabase();
  auto materialDatabase = Root::singleton().materialDatabase();

  // Each column in tileEvalColumns is guaranteed to be no larger than the sector size.

  m_tileArray->tileEvalColumns(m_lightingCalculator.calculationRegion(), [&](Vec2I const& pos, ClientTile const* column, size_t ySize) {
    size_t baseIndex = m_lightingCalculator.baseIndexFor(pos);
    for (size_t y = 0; y < ySize; ++y) {
      auto& tile = column[y];

      Vec3F light;
      if (tile.foreground != EmptyMaterialId || tile.foregroundMod != NoModId)
        light += materialDatabase->radiantLight(tile.foreground, tile.foregroundMod);

      if (tile.liquid.liquid != EmptyLiquidId && tile.liquid.level != 0.0f)
        light += liquidsDatabase->radiantLight(tile.liquid);
      if (tile.foregroundLightTransparent) {
        if (tile.background != EmptyMaterialId || tile.backgroundMod != NoModId)
          light += materialDatabase->radiantLight(tile.background, tile.backgroundMod);
        if (tile.backgroundLightTransparent && pos[1] + y > undergroundLevel)
          light += environmentLight;
      }
      m_lightingCalculator.setCellIndex(baseIndex + y, move(light), !tile.foregroundLightTransparent);
    }
  });
}

void WorldClient::lightingMain() {
  while (true) {
    MutexLocker locker(m_lightingMutex);

    m_lightingCond.wait(m_lightingMutex);
    if (m_stopLightingThread)
      return;

    if (WorldRenderData* renderData = m_renderData) {
      int64_t start = Time::monotonicMicroseconds();

      lightingTileGather();

      m_lightingCalculator.calculate(renderData->lightMap);
      m_renderData = nullptr;
      LogMap::set("client_render_world_async_light_calc", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - start));
    }

    continue;

    locker.unlock();
    Thread::yield();
  }
}

void WorldClient::initWorld(WorldStartPacket const& startPacket) {
  clearWorld();
  m_outgoingPackets.append(make_shared<WorldStartAcknowledgePacket>());

  auto assets = Root::singleton().assets();
  if (startPacket.localInterpolationMode)
    m_interpolationTracker = InterpolationTracker(m_clientConfig.query("interpolationSettings.local"));
  else
    m_interpolationTracker = InterpolationTracker(m_clientConfig.query("interpolationSettings.normal"));

  m_clientId = startPacket.clientId;
  auto entitySpace = connectionEntitySpace(startPacket.clientId);
  m_worldTemplate = make_shared<WorldTemplate>(startPacket.templateData);
  m_entityMap = make_shared<EntityMap>(m_worldTemplate->size(), entitySpace.first, entitySpace.second);
  m_tileArray = make_shared<ClientTileSectorArray>(m_worldTemplate->size());
  m_tileGetterFunction = [&, tile = ClientTile()](Vec2I pos) mutable -> ClientTile const& {
    if (!m_predictedTiles.empty()) {
      if (auto p = m_predictedTiles.ptr(pos)) {
        p->apply(tile = m_tileArray->tile(pos));
        if (p->liquid) {
          if (p->liquid->liquid == tile.liquid.liquid)
            tile.liquid.level += p->liquid->level;
          else {
            tile.liquid.liquid = p->liquid->liquid;
            tile.liquid.level = p->liquid->level;
          }
        }
        return tile;
      }
    }
    return m_tileArray->tile(pos);
  };
  m_damageManager = make_shared<DamageManager>(this, startPacket.clientId);
  m_luaRoot->restart();
  m_luaRoot->tuneAutoGarbageCollection(m_clientConfig.getFloat("luaGcPause"), m_clientConfig.getFloat("luaGcStepMultiplier"));
  m_playerStart = startPacket.playerRespawn;
  m_respawnInWorld = startPacket.respawnInWorld;
  m_worldProperties = startPacket.worldProperties.optObject().value();
  m_dungeonIdGravity = startPacket.dungeonIdGravity;
  m_dungeonIdBreathable = startPacket.dungeonIdBreathable;
  m_protectedDungeonIds = startPacket.protectedDungeonIds;

  m_geometry = WorldGeometry(m_worldTemplate->size());

  m_particles = make_shared<ParticleManager>(m_geometry, m_tileArray);
  m_particles->setUndergroundLevel(m_worldTemplate->undergroundLevel());

  setupForceRegions();

  if (!m_mainPlayer->isDead()) {
    m_mainPlayer->init(this, m_entityMap->reserveEntityId(), EntityMode::Master);
    m_entityMap->addEntity(m_mainPlayer);
  }
  m_mainPlayer->moveTo(startPacket.playerStart);
  if (m_worldTemplate->worldParameters())
    m_mainPlayer->overrideTech(m_worldTemplate->worldParameters()->overrideTech);
  else
    m_mainPlayer->overrideTech({});

  // Auto reposition the client window on the player when the main player
  // changes position.
  centerClientWindowOnPlayer();

  m_sky = make_shared<Sky>();
  m_sky->readUpdate(startPacket.skyData);

  m_weather.setup(m_geometry, [this](Vec2I const& pos) {
      auto const& tile = m_tileArray->tile(pos);
      return !isRealMaterial(tile.background) && !isSolidColliding(tile.collision);
    });
  m_weather.readUpdate(startPacket.weatherData);

  m_lightingCalculator.setMonochrome(Root::singleton().configuration()->get("monochromeLighting").toBool());
  m_lightingCalculator.setParameters(assets->json("/lighting.config:lighting"));
  m_lightIntensityCalculator.setParameters(assets->json("/lighting.config:intensity"));

  m_inWorld = true;
}

void WorldClient::clearWorld() {
  if (m_entityMap) {
    while (m_entityMap->size() > 0) {
      for (auto entityId : m_entityMap->entityIds())
        removeEntity(entityId, false);
    }
  }

  waitForLighting();

  m_currentStep = 0;
  m_currentServerStep = 0.0;
  m_inWorld = false;
  m_clientId.reset();

  m_interpolationTracker = InterpolationTracker();

  m_masterEntitiesNetVersion.clear();
  m_outgoingPackets.clear();

  m_pingTime.reset();

  m_entityMap.reset();
  m_worldTemplate.reset();
  m_worldProperties.clear();

  m_tileArray.reset();

  m_damageManager.reset();

  m_luaRoot->shutdown();

  m_particles.reset();

  m_sky.reset();

  m_currentParallax.reset();
  m_nextParallax.reset();
  m_parallaxFadeTimer.setDone();

  m_clientState.reset();
  m_ambientSounds.cancelAll();
  m_musicTrack.cancelAll();
  m_musicTrack.setVolume(1, 0, 0);
  m_altMusicTrack.cancelAll();
  m_altMusicTrack.setVolume(0, 0, 0);
  m_altMusicActive = false;

  if (m_spaceSound) {
    m_spaceSound->stop();
    m_spaceSound = {};
  }

  m_entityMessageResponses = {};

  m_forceRegions.clear();
}

void WorldClient::tryGiveMainPlayerItem(ItemPtr item) {
  if (auto spill = m_mainPlayer->pickupItems(item))
    addEntity(ItemDrop::createRandomizedDrop(spill->descriptor(), m_mainPlayer->position()));
}

Vec2I WorldClient::environmentBiomeTrackPosition() const {
  if (!inWorld())
    return {};

  auto pos = Vec2I::floor(m_clientState.windowCenter());
  return {m_geometry.xwrap(pos[0]), pos[1]};
}

AmbientNoisesDescriptionPtr WorldClient::currentAmbientNoises() const {
  if (!inWorld())
    return {};

  Vec2I pos = environmentBiomeTrackPosition();
  return m_worldTemplate->ambientNoises(pos[0], pos[1]);
}

WeatherNoisesDescriptionPtr WorldClient::currentWeatherNoises() const {
  if (!inWorld())
    return {};

  auto trackOptions = m_weather.weatherTrackOptions();
  if (trackOptions.empty())
    return {};
  else
    return make_shared<WeatherNoisesDescription>(move(trackOptions));
}

AmbientNoisesDescriptionPtr WorldClient::currentMusicTrack() const {
  if (!inWorld())
    return {};

  Vec2I pos = environmentBiomeTrackPosition();
  return m_worldTemplate->musicTrack(pos[0], pos[1]);
}

AmbientNoisesDescriptionPtr WorldClient::currentAltMusicTrack() const {
  if (!inWorld())
    return {};

  return m_altMusicTrackDescription;
}

void WorldClient::playAltMusic(StringList const& newTracks, float fadeTime) {
  auto newTrackGroup = AmbientTrackGroup(newTracks);
  m_altMusicTrackDescription = make_shared<AmbientNoisesDescription>(AmbientTrackGroup(newTracks), AmbientTrackGroup());
  if (!m_altMusicActive) {
    m_musicTrack.setVolume(0.0, 0.0, fadeTime);
    m_altMusicTrack.setVolume(1.0, 0.0, fadeTime);
    m_altMusicActive = true;
  }
}

void WorldClient::stopAltMusic(float fadeTime) {
  if (m_altMusicActive) {
    m_musicTrack.setVolume(1.0, 0.0, fadeTime);
    m_altMusicTrack.setVolume(0.0, 0.0, fadeTime);
    m_altMusicActive = false;
  }
}

BiomeConstPtr WorldClient::mainEnvironmentBiome() const {
  if (!inWorld())
    return {};

  Vec2I pos = environmentBiomeTrackPosition();
  return m_worldTemplate->environmentBiome(pos[0], pos[1]);
}

bool WorldClient::readNetTile(Vec2I const& pos, NetTile const& netTile) {
  ClientTile* tile = m_tileArray->modifyTile(pos);
  if (!tile)
    return false;

  if (!m_predictedTiles.empty()) {
    auto findPrediction = m_predictedTiles.find(pos);
    if (findPrediction != m_predictedTiles.end()) {
      auto& p = findPrediction->second;

      if (p.foreground && *p.foreground == netTile.foreground)
        p.foreground.reset();
      if (p.foregroundMod && *p.foregroundMod == netTile.foregroundMod)
        p.foregroundMod.reset();
      if (p.foregroundHueShift && *p.foregroundHueShift == netTile.foregroundHueShift)
        p.foregroundHueShift.reset();
      if (p.foregroundModHueShift && *p.foregroundModHueShift == netTile.foregroundModHueShift)
        p.foregroundModHueShift.reset();

      if (p.background && *p.background == netTile.background)
        p.background.reset();
      if (p.backgroundMod && *p.backgroundMod == netTile.backgroundMod)
        p.backgroundMod.reset();
      if (p.backgroundHueShift && *p.backgroundHueShift == netTile.backgroundHueShift)
        p.backgroundHueShift.reset();
      if (p.backgroundModHueShift && *p.backgroundModHueShift == netTile.backgroundModHueShift)
        p.backgroundModHueShift.reset();

      if (!p)
        m_predictedTiles.erase(findPrediction);
    }
  }

  tile->background = netTile.background;
  tile->backgroundHueShift = netTile.backgroundHueShift;
  tile->backgroundColorVariant = netTile.backgroundColorVariant;
  tile->backgroundMod = netTile.backgroundMod;
  tile->backgroundModHueShift = netTile.backgroundModHueShift;
  tile->foreground = netTile.foreground;
  tile->foregroundHueShift = netTile.foregroundHueShift;
  tile->foregroundColorVariant = netTile.foregroundColorVariant;
  tile->foregroundMod = netTile.foregroundMod;
  tile->foregroundModHueShift = netTile.foregroundModHueShift;
  tile->collision = netTile.collision;
  tile->blockBiomeIndex = netTile.blockBiomeIndex;
  tile->environmentBiomeIndex = netTile.environmentBiomeIndex;
  tile->liquid = netTile.liquid.liquidLevel();
  tile->dungeonId = netTile.dungeonId;

  auto materialDatabase = Root::singleton().materialDatabase();
  tile->backgroundLightTransparent = materialDatabase->backgroundLightTransparent(tile->background);
  tile->foregroundLightTransparent =
      materialDatabase->foregroundLightTransparent(tile->foreground) && tile->collision != CollisionKind::Dynamic;

  dirtyCollision(RectI::withSize(pos, {1, 1}));

  return true;
}

void WorldClient::dirtyCollision(RectI const& region) {
  if (!inWorld())
    return;

  auto dirtyRegion = region.padded(CollisionGenerator::BlockInfluenceRadius);
  for (int x = dirtyRegion.xMin(); x < dirtyRegion.xMax(); ++x) {
    for (int y = dirtyRegion.yMin(); y < dirtyRegion.yMax(); ++y) {
      if (auto tile = m_tileArray->modifyTile({x, y}))
        tile->collisionCacheDirty = true;
    }
  }
}

void WorldClient::freshenCollision(RectI const& region) {
  if (!inWorld())
    return;

  RectI freshenRegion = RectI::null();
  for (int x = region.xMin(); x < region.xMax(); ++x) {
    for (int y = region.yMin(); y < region.yMax(); ++y) {
      if (auto tile = m_tileArray->modifyTile({x, y})) {
        if (tile->collisionCacheDirty)
          freshenRegion.combine(RectI(x, y, x + 1, y + 1));
      }
    }
  }

  if (!freshenRegion.isNull()) {
    for (int x = freshenRegion.xMin(); x < freshenRegion.xMax(); ++x) {
      for (int y = freshenRegion.yMin(); y < freshenRegion.yMax(); ++y) {
        if (auto tile = m_tileArray->modifyTile({x, y})) {
          tile->collisionCacheDirty = false;
          tile->collisionCache.clear();
        }
      }
    }

    for (auto collisionBlock : m_collisionGenerator.getBlocks(freshenRegion)) {
      if (auto tile = m_tileArray->modifyTile(collisionBlock.space))
        tile->collisionCache.append(move(collisionBlock));
    }
  }
}

float WorldClient::lightLevel(Vec2F const& pos) const {
  if (!inWorld())
    return 0.0f;
  return WorldImpl::lightLevel(m_tileArray, m_entityMap, m_geometry, m_worldTemplate, m_sky, m_lightIntensityCalculator, pos);
}

bool WorldClient::breathable(Vec2F const& pos) const {
  if (!inWorld())
    return true;

  return WorldImpl::breathable(this, m_tileArray, m_dungeonIdBreathable, m_worldTemplate, pos);
}

float WorldClient::threatLevel() const {
  if (!inWorld())
    return 0.0f;
  return m_worldTemplate->threatLevel();
}

StringList WorldClient::environmentStatusEffects(Vec2F const& pos) const {
  if (!inWorld())
    return {};

  return m_worldTemplate->environmentStatusEffects(floor(pos[0]), floor(pos[1]));
}

StringList WorldClient::weatherStatusEffects(Vec2F const& pos) const {
  if (!inWorld())
    return {};

  if (!m_weather.statusEffects().empty()) {
     if (exposedToWeather(pos))
      return m_weather.statusEffects();
  }

  return {};
}

bool WorldClient::exposedToWeather(Vec2F const& pos) const {
  if (!inWorld())
    return false;

  if (!isUnderground(pos) && liquidLevel(Vec2I::floor(pos)).liquid == EmptyLiquidId) {
    auto assets = Root::singleton().assets();
    float weatherRayCheckDistance = assets->json("/weather.config:weatherRayCheckDistance").toFloat();
    float weatherRayCheckWindInfluence = assets->json("/weather.config:weatherRayCheckWindInfluence").toFloat();

    auto offset = Vec2F(-m_weather.wind() * weatherRayCheckWindInfluence, weatherRayCheckDistance).normalized() * weatherRayCheckDistance;

    return !lineCollision({pos, pos + offset});
  }

  return false;
}

bool WorldClient::isUnderground(Vec2F const& pos) const {
  if (!inWorld())
    return true;
  return m_worldTemplate->undergroundLevel() >= pos[1];
}

bool WorldClient::disableDeathDrops() const {
  if (m_worldTemplate->worldParameters())
    return m_worldTemplate->worldParameters()->disableDeathDrops;
  return false;
}

List<PhysicsForceRegion> WorldClient::forceRegions() const {
  return m_forceRegions;
}

Json WorldClient::getProperty(String const& propertyName, Json const& def) const {
  if (!inWorld())
    return {};

  return m_worldProperties.value(propertyName, def);
}

void WorldClient::setProperty(String const& propertyName, Json const& property) {
  if (!inWorld())
    return;

  if (m_worldProperties[propertyName] == property)
    return;

  m_outgoingPackets.append(make_shared<UpdateWorldPropertiesPacket>(JsonObject{{propertyName, property}}));
}

bool WorldClient::playerCanReachEntity(EntityId entityId, bool preferInteractive) const {
  return canReachEntity(m_mainPlayer->position(), m_mainPlayer->interactRadius(), entityId, preferInteractive);
}

void WorldClient::disconnectAllWires(Vec2I wireEntityPosition, WireNode const& node) {
  m_outgoingPackets.append(make_shared<DisconnectAllWiresPacket>(wireEntityPosition, node));
}

void WorldClient::connectWire(WireConnection const& output, WireConnection const& input) {
  m_outgoingPackets.append(make_shared<ConnectWirePacket>(output, input));
}

bool WorldClient::sendSecretBroadcast(StringView broadcast, bool raw) {
  if (!inWorld() || !m_mainPlayer || !m_mainPlayer->getSecretPropertyView(SECRET_BROADCAST_PUBLIC_KEY))
    return false;

  auto signature = Curve25519::sign((void*)broadcast.utf8Ptr(), broadcast.utf8Size());

  auto damageNotification = make_shared<DamageNotificationPacket>();
  auto& remDmg = damageNotification->remoteDamageNotification;
  auto& dmg = remDmg.damageNotification;

  dmg.targetEntityId = dmg.sourceEntityId = remDmg.sourceEntityId = m_mainPlayer->entityId();
  dmg.damageDealt = dmg.healthLost = 0.0f;
  dmg.hitType = HitType::Hit;
  dmg.damageSourceKind = "nodamage";
  dmg.targetMaterialKind = raw ? broadcast : strf("{}{}{}", SECRET_BROADCAST_PREFIX, StringView((char*)&signature, sizeof(signature)), broadcast);
  dmg.position = m_mainPlayer->position();

  m_outgoingPackets.emplace_back(move(damageNotification));
  return true;
}

bool WorldClient::handleSecretBroadcast(PlayerPtr player, StringView broadcast) {
  if (m_broadcastCallback)
    return m_broadcastCallback(player, broadcast);
  else
    return false;
}


void WorldClient::ClientRenderCallback::addDrawable(Drawable drawable, EntityRenderLayer renderLayer) {
  drawables[renderLayer].append(move(drawable));
}

void WorldClient::ClientRenderCallback::addLightSource(LightSource lightSource) {
  lightSources.append(move(lightSource));
}

void WorldClient::ClientRenderCallback::addParticle(Particle particle) {
  particles.append(move(particle));
}

void WorldClient::ClientRenderCallback::addAudio(AudioInstancePtr audio) {
  audios.append(move(audio));
}

void WorldClient::ClientRenderCallback::addTilePreview(PreviewTile preview) {
  previewTiles.append(move(preview));
}

void WorldClient::ClientRenderCallback::addOverheadBar(OverheadBar bar) {
  overheadBars.append(move(bar));
}

double WorldClient::epochTime() const {
  if (!inWorld())
    return 0;
  return m_sky->epochTime();
}

uint32_t WorldClient::day() const {
  if (!inWorld())
    return 0;
  return m_sky->day();
}

float WorldClient::dayLength() const {
  if (!inWorld())
    return 0;
  return m_sky->dayLength();
}

float WorldClient::timeOfDay() const {
  if (!inWorld())
    return 0;
  return m_sky->timeOfDay();
}

LuaRootPtr WorldClient::luaRoot() {
  return m_luaRoot;
}

RpcPromise<Vec2F> WorldClient::findUniqueEntity(String const& uniqueId) {
  if (auto entity = m_entityMap->uniqueEntity(uniqueId))
    return RpcPromise<Vec2F>::createFulfilled(entity->position());

  auto pair = RpcPromise<Vec2F>::createPair();
  auto& rpcPromises = m_findUniqueEntityResponses[uniqueId];
  if (rpcPromises.empty())
    m_outgoingPackets.append(make_shared<FindUniqueEntityPacket>(uniqueId));
  rpcPromises.append(pair.second);

  return pair.first;
}

RpcPromise<Json> WorldClient::sendEntityMessage(Variant<EntityId, String> const& entityId, String const& message, JsonArray const& args) {
  EntityPtr entity;
  if (entityId.is<EntityId>())
    entity = m_entityMap->entity(entityId.get<EntityId>());
  else
    entity = m_entityMap->uniqueEntity(entityId.get<String>());

  // Only fail with "unknown entity" if we know this entity should exist on the
  // client, because it's entity id indicates it is master here.
  if (entityId.is<EntityId>() && !entity && m_clientId == connectionForEntity(entityId.get<EntityId>())) {
    return RpcPromise<Json>::createFailed("Unknown entity");
  } else if (entity && entity->isMaster()) {
    if (auto resp = entity->receiveMessage(*m_clientId, message, args))
      return RpcPromise<Json>::createFulfilled(resp.take());
    else
      return RpcPromise<Json>::createFailed("Message not handled by entity");
  } else {
    auto pair = RpcPromise<Json>::createPair();
    Uuid uuid;
    m_entityMessageResponses[uuid] = pair.second;
    m_outgoingPackets.append(make_shared<EntityMessagePacket>(entityId, message, args, uuid));
    return pair.first;
  }
}

List<ChatAction> WorldClient::pullPendingChatActions() {
  List<ChatAction> result;
  if (m_entityMap) {
    for (auto const& entity : m_entityMap->all<ChattyEntity>())
      result.appendAll(entity->pullPendingChatActions());
  }
  return result;
}

WorldStructure const& WorldClient::centralStructure() const {
  return m_centralStructure;
}

bool WorldClient::DamageNumberKey::operator<(DamageNumberKey const& other) const {
  return tie(sourceEntityId, targetEntityId, damageNumberParticleKind)
      < tie(other.sourceEntityId, other.targetEntityId, other.damageNumberParticleKind);
}

void WorldClient::renderCollisionDebug() {
  RectI clientWindow = m_clientState.window();
  if (clientWindow.isEmpty())
    return;

  auto logPoly = [](PolyF poly, Vec2F position, float r, float g, float b) {
    poly.translate(position);
    SpatialLogger::logPoly("world", poly, {floatToByte(r, true), floatToByte(g, true), floatToByte(b, true), 255});
  };

  forEachCollisionBlock(clientWindow, [&](auto const& block) {
      logPoly(block.poly, Vec2F{}, 1.0f, 0.0f, 0.0f);
    });

  for (auto const& object : query<TileEntity>(RectF(clientWindow))) {
    for (auto const& space : object->spaces())
      logPoly(PolyF(RectF(Vec2F(space), Vec2F(space) + Vec2F(1, 1))), Vec2F(object->tilePosition()), 0., 1., 0.);
  }

  for (auto const& physics : query<PhysicsEntity>(RectF(clientWindow))) {
    for (auto const& forceRegion : physics->forceRegions()) {
      if (auto dfr = forceRegion.ptr<DirectionalForceRegion>())
        logPoly(dfr->region, {}, 1.0f, 1.0f, 0.0f);
      else if (auto rfr = forceRegion.ptr<RadialForceRegion>())
        logPoly(PolyF(rfr->boundBox()), {}, 0.0f, 1.0f, 1.0f);
    }

    for (size_t i = 0; i < physics->movingCollisionCount(); ++i) {
      if (auto pmc = physics->movingCollision(i)) {
        logPoly(pmc->collision, pmc->position, 1.0f, 1.0f, 1.0f);
      }
    }
  }
}

void WorldClient::informTilePredictions(TileModificationList const& modifications) {
  auto now = Time::monotonicMilliseconds();
  for (auto& pair : modifications) {
    auto& p = m_predictedTiles[pair.first];
    p.time = now;
    if (auto placeMaterial = pair.second.ptr<PlaceMaterial>()) {
      if (placeMaterial->layer == TileLayer::Foreground) {
        p.foreground = placeMaterial->material;
        p.foregroundHueShift = placeMaterial->materialHueShift;
      } else {
        p.background = placeMaterial->material;
        p.backgroundHueShift = placeMaterial->materialHueShift;
      }
    }
    else if (auto placeMod = pair.second.ptr<PlaceMod>()) {
      if (placeMod->layer == TileLayer::Foreground)
        p.foregroundMod = placeMod->mod;
      else
        p.backgroundMod = placeMod->mod;
    }
    else if (auto placeColor = pair.second.ptr<PlaceMaterialColor>()) {
      if (placeColor->layer == TileLayer::Foreground)
        p.foregroundColorVariant = placeColor->color;
      else
        p.backgroundColorVariant = placeColor->color;
    }
    else if (auto placeLiquid = pair.second.ptr<PlaceLiquid>()) {
      if (!p.liquid || p.liquid->liquid != placeLiquid->liquid)
        p.liquid = LiquidLevel(placeLiquid->liquid, placeLiquid->liquidLevel);
      else
        p.liquid->level += placeLiquid->liquidLevel;
    }
  }
}

void WorldClient::setupForceRegions() {
  m_forceRegions.clear();

  if (!currentTemplate() || !currentTemplate()->worldParameters())
    return;

  auto forceRegionType = currentTemplate()->worldParameters()->worldEdgeForceRegions;

  if (forceRegionType == WorldEdgeForceRegionType::None)
    return;

  bool addTopRegion = forceRegionType == WorldEdgeForceRegionType::Top || forceRegionType == WorldEdgeForceRegionType::TopAndBottom;
  bool addBottomRegion = forceRegionType == WorldEdgeForceRegionType::Bottom || forceRegionType == WorldEdgeForceRegionType::TopAndBottom;

  auto worldServerConfig = Root::singleton().assets()->json("/worldserver.config");

  auto regionHeight = worldServerConfig.getFloat("worldEdgeForceRegionHeight");
  auto regionForce = worldServerConfig.getFloat("worldEdgeForceRegionForce");
  auto regionVelocity = worldServerConfig.getFloat("worldEdgeForceRegionVelocity");
  auto regionCategoryFilter = PhysicsCategoryFilter::whitelist({"player", "monster", "npc", "vehicle"});
  auto worldSize = Vec2F(currentTemplate()->size());

  if (addTopRegion) {
    auto topForceRegion = GradientForceRegion();
    topForceRegion.region = PolyF({
        {0, worldSize[1] - regionHeight},
        {worldSize[0], worldSize[1] - regionHeight},
        (worldSize),
        {0, worldSize[1]}});
    topForceRegion.gradient = Line2F({0, worldSize[1]}, {0, worldSize[1] - regionHeight});
    topForceRegion.baseTargetVelocity = regionVelocity;
    topForceRegion.baseControlForce = regionForce;
    topForceRegion.categoryFilter = regionCategoryFilter;
    m_forceRegions.append(topForceRegion);
  }

  if (addBottomRegion) {
    auto bottomForceRegion = GradientForceRegion();
    bottomForceRegion.region = PolyF({
        {0, 0},
        {worldSize[0], 0},
        {worldSize[0], regionHeight},
        {0, regionHeight}});
    bottomForceRegion.gradient = Line2F({0, 0}, {0, regionHeight});
    bottomForceRegion.baseTargetVelocity = regionVelocity;
    bottomForceRegion.baseControlForce = regionForce;
    bottomForceRegion.categoryFilter = regionCategoryFilter;
    m_forceRegions.append(bottomForceRegion);
  }
}

}
