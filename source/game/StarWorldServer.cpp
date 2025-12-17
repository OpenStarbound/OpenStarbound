#include "StarWorldServer.hpp"
#include "StarLogging.hpp"
#include "StarIterator.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarBiome.hpp"
#include "StarWireProcessor.hpp"
#include "StarWireEntity.hpp"
#include "StarWorldImpl.hpp"
#include "StarWorldGeneration.hpp"
#include "StarItemDescriptor.hpp"
#include "StarItemDrop.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObject.hpp"
#include "StarItemDatabase.hpp"
#include "StarContainerEntity.hpp"
#include "StarItemBag.hpp"
#include "StarPhysicsEntity.hpp"
#include "StarProjectile.hpp"
#include "StarPlayer.hpp"
#include "StarEntityFactory.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarLiquidTypes.hpp"
#include "StarFallingBlocksAgent.hpp"
#include "StarWarpTargetEntity.hpp"
#include "StarUniverseSettings.hpp"
#include "StarUniverseServerLuaBindings.hpp"

namespace Star {

EnumMap<WorldServerFidelity> const WorldServerFidelityNames{
  {WorldServerFidelity::Minimum, "minimum"},
  {WorldServerFidelity::Low, "low"},
  {WorldServerFidelity::Medium, "medium"},
  {WorldServerFidelity::High, "high"}
};

WorldServer::WorldServer(WorldTemplatePtr const& worldTemplate, IODevicePtr storage) {
  m_worldTemplate = worldTemplate;
  m_worldStorage = make_shared<WorldStorage>(m_worldTemplate->size(), storage, make_shared<WorldGenerator>(this));
  m_adjustPlayerStart = true;
  m_respawnInWorld = false;
  m_tileProtectionEnabled = true;
  m_universeSettings = make_shared<UniverseSettings>();
  m_worldId = worldTemplate->worldName();
  m_expiryTimer = GameTimer(0.0f);

  init(true);
  writeMetadata();
}

WorldServer::WorldServer(Vec2U const& size, IODevicePtr storage)
  : WorldServer(make_shared<WorldTemplate>(size), storage) {}

WorldServer::WorldServer(IODevicePtr const& storage) {
  m_worldStorage = make_shared<WorldStorage>(storage, make_shared<WorldGenerator>(this));
  m_tileProtectionEnabled = true;
  m_universeSettings = make_shared<UniverseSettings>();
  m_worldId = "Nowhere";

  readMetadata();
  init(false);
}

WorldServer::WorldServer(WorldChunks const& chunks) {
  m_worldStorage = make_shared<WorldStorage>(chunks, make_shared<WorldGenerator>(this));
  m_tileProtectionEnabled = true;
  m_universeSettings = make_shared<UniverseSettings>();
  m_worldId = "Nowhere";

  readMetadata();
  init(false);
}

WorldServer::~WorldServer() {
  for (auto& p : m_scriptContexts)
    p.second->uninit();

  m_scriptContexts.clear();
  m_spawner.uninit();
  writeMetadata();
  m_worldStorage->unloadAll(true);
}

void WorldServer::setWorldId(String worldId) {
  m_worldId = std::move(worldId);
}

String const& WorldServer::worldId() const {
  return m_worldId;
}

void WorldServer::setUniverseSettings(UniverseSettingsPtr universeSettings) {
  m_universeSettings = std::move(universeSettings);
}

UniverseSettingsPtr WorldServer::universeSettings() const {
  return m_universeSettings;
}

void WorldServer::setReferenceClock(ClockPtr clock) {
  m_weather.setReferenceClock(clock);
  m_sky->setReferenceClock(clock);
}

void WorldServer::initLua(UniverseServer* universe) {
  m_luaRoot->addCallbacks("universe", LuaBindings::makeUniverseServerCallbacks(universe));
  auto assets = Root::singleton().assets();
  for (auto& p : assets->json("/worldserver.config:scriptContexts").toObject()) {
    auto scriptComponent = make_shared<ScriptComponent>();
    scriptComponent->setScripts(jsonToStringList(p.second.toArray()));

    m_scriptContexts.set(p.first, scriptComponent);
    scriptComponent->init(this);
  }
}

WorldStructure WorldServer::setCentralStructure(WorldStructure centralStructure) {
  removeCentralStructure();

  m_centralStructure = std::move(centralStructure);
  m_centralStructure.setAnchorPosition(Vec2I(m_geometry.size()) / 2);

  m_playerStart = Vec2F(m_centralStructure.flaggedBlocks("playerSpawn").first()) + Vec2F(0, 1);
  m_adjustPlayerStart = false;

  auto materialDatabase = Root::singleton().materialDatabase();
  for (auto const& foregroundBlock : m_centralStructure.foregroundBlocks()) {
    generateRegion(RectI::withSize(foregroundBlock.position, {1, 1}));
    if (auto tile = m_tileArray->modifyTile(foregroundBlock.position)) {
      if (tile->foreground == EmptyMaterialId) {
        tile->foreground = foregroundBlock.materialId;
        tile->foregroundColorVariant = foregroundBlock.materialColor;
        tile->foregroundHueShift = foregroundBlock.materialHue;
        tile->foregroundMod = foregroundBlock.materialMod;
        tile->updateCollision(materialDatabase->materialCollisionKind(foregroundBlock.materialId));
        queueTileUpdates(foregroundBlock.position);
        dirtyCollision(RectI::withSize(foregroundBlock.position, {1, 1}));
      }
    }
  }

  for (auto const& backgroundBlock : m_centralStructure.backgroundBlocks()) {
    generateRegion(RectI::withSize(backgroundBlock.position, {1, 1}));
    if (auto tile = m_tileArray->modifyTile(backgroundBlock.position)) {
      if (tile->background == EmptyMaterialId) {
        tile->background = backgroundBlock.materialId;
        tile->backgroundColorVariant = backgroundBlock.materialColor;
        tile->backgroundHueShift = backgroundBlock.materialHue;
        tile->backgroundMod = backgroundBlock.materialMod;
        queueTileUpdates(backgroundBlock.position);
      }
    }
  }

  auto objectDatabase = Root::singleton().objectDatabase();
  for (auto structureObject : m_centralStructure.objects()) {
    generateRegion(RectI::withSize(structureObject.position, {1, 1}));
    if (auto object = objectDatabase->createForPlacement(this, structureObject.name, structureObject.position, structureObject.direction, structureObject.parameters))
      addEntity(object);
  }

  for (auto const& pair : m_clientInfo)
    pair.second->outgoingPackets.append(make_shared<CentralStructureUpdatePacket>(m_centralStructure.store()));

  return m_centralStructure;
}

WorldStructure const& WorldServer::centralStructure() const {
  return m_centralStructure;
}

void WorldServer::removeCentralStructure() {
  for (auto const& structureObject : m_centralStructure.objects()) {
    if (!structureObject.residual) {
      generateRegion(RectI::withSize(structureObject.position, {1, 1}));
      for (auto const& objectEntity : atTile<Object>(structureObject.position)) {
        if (objectEntity->tilePosition() == structureObject.position && objectEntity->name() == structureObject.name)
          removeEntity(objectEntity->entityId(), false);
      }
    }
  }

  for (auto const& backgroundBlock : m_centralStructure.backgroundBlocks()) {
    if (!backgroundBlock.residual) {
      generateRegion(RectI::withSize(backgroundBlock.position, {1, 1}));
      if (auto tile = m_tileArray->modifyTile(backgroundBlock.position)) {
        if (tile->background == backgroundBlock.materialId) {
          tile->background = EmptyMaterialId;
          tile->backgroundMod = NoModId;
          queueTileUpdates(backgroundBlock.position);
        }
      }
    }
  }

  for (auto const& foregroundBlock : m_centralStructure.foregroundBlocks()) {
    if (!foregroundBlock.residual) {
      generateRegion(RectI::withSize(foregroundBlock.position, {1, 1}));
      if (auto tile = m_tileArray->modifyTile(foregroundBlock.position)) {
        if (tile->foreground == foregroundBlock.materialId) {
          tile->foreground = EmptyMaterialId;
          tile->foregroundMod = NoModId;
          tile->updateCollision(CollisionKind::None);
          dirtyCollision(RectI::withSize(foregroundBlock.position, {1, 1}));
          queueTileUpdates(foregroundBlock.position);
        }
      }
    }
  }
}

bool WorldServer::spawnTargetValid(SpawnTarget const& spawnTarget) const {
  if (spawnTarget.is<SpawnTargetUniqueEntity>())
    return (bool)m_entityMap->get<WarpTargetEntity>(m_worldStorage->loadUniqueEntity(spawnTarget.get<SpawnTargetUniqueEntity>()));
  return true;
}

bool WorldServer::addClient(ConnectionId clientId, SpawnTarget const& spawnTarget, bool isLocal, bool isAdmin, NetCompatibilityRules netRules) {
  if (m_clientInfo.contains(clientId))
    return false;

  Vec2F playerStart;
  if (spawnTarget.is<SpawnTargetPosition>()) {
    playerStart = spawnTarget.get<SpawnTargetPosition>();
  } else if (spawnTarget.is<SpawnTargetX>()) {
    auto targetX = spawnTarget.get<SpawnTargetX>();
    playerStart = findPlayerSpaceStart(targetX);
  } else if (spawnTarget.is<SpawnTargetUniqueEntity>()) {
    if (auto target = m_entityMap->get<WarpTargetEntity>(m_worldStorage->loadUniqueEntity(spawnTarget.get<SpawnTargetUniqueEntity>())))
      playerStart = target->position() + target->footPosition();
    else
      return false;
  } else {
    playerStart = m_playerStart;
    if (m_adjustPlayerStart) {
      m_playerStart = findPlayerStart(m_playerStart);
      playerStart = m_playerStart;
    }
  }
  RectF spawnRegion = RectF(playerStart, playerStart).padded(m_serverConfig.getInt("playerStartInitialGenRadius"));
  generateRegion(RectI::integral(spawnRegion));
  m_spawner.activateEmptyRegion(spawnRegion);

  InterpolationTracker tracker;
  if (isLocal)
    tracker = InterpolationTracker(m_serverConfig.query("interpolationSettings.local"));
  else
    tracker = InterpolationTracker(m_serverConfig.query("interpolationSettings.normal"));

  tracker.update(m_currentTime);

  auto& clientInfo = m_clientInfo.add(clientId, make_shared<ClientInfo>(clientId, tracker));
  clientInfo->local = isLocal;
  clientInfo->admin = isAdmin;
  clientInfo->clientState.setNetCompatibilityRules(netRules);

  auto worldStartPacket = make_shared<WorldStartPacket>();
  auto& templateData = worldStartPacket->templateData = m_worldTemplate->store();
  // this makes it possible to use custom InstanceWorlds without clients having the mod that adds their dungeon:
  if (templateData.optQueryString("worldParameters.primaryDungeon")
    && Root::singletonPtr()->configuration()->getPath("compatibility.customDungeonWorld").optBool().value(false))
    worldStartPacket->templateData = worldStartPacket->templateData.setPath("worldParameters.primaryDungeon", "testarena");

  tie(worldStartPacket->skyData, clientInfo->skyNetVersion) = m_sky->writeUpdate(0, netRules);
  tie(worldStartPacket->weatherData, clientInfo->weatherNetVersion) = m_weather.writeUpdate(0, netRules);
  worldStartPacket->playerStart = playerStart;
  worldStartPacket->playerRespawn = m_playerStart;
  worldStartPacket->respawnInWorld = m_respawnInWorld;
  worldStartPacket->worldProperties = m_worldProperties;
  worldStartPacket->dungeonIdGravity = m_dungeonIdGravity;
  worldStartPacket->dungeonIdBreathable = m_dungeonIdBreathable;
  worldStartPacket->protectedDungeonIds = m_protectedDungeonIds;
  worldStartPacket->clientId = clientId;
  worldStartPacket->localInterpolationMode = isLocal;
  clientInfo->outgoingPackets.append(worldStartPacket);

  clientInfo->outgoingPackets.append(make_shared<CentralStructureUpdatePacket>(m_centralStructure.store()));

  for (auto& p : m_scriptContexts)
    p.second->invoke("addClient", clientId, isLocal);

  return true;
}

List<PacketPtr> WorldServer::removeClient(ConnectionId clientId) {
  auto const& info = m_clientInfo.get(clientId);

  for (auto const& entityId : m_entityMap->entityIds()) {
    if (connectionForEntity(entityId) == clientId)
      removeEntity(entityId, false);
  }

  for (auto const& uuid : m_entityMessageResponses.keys()) {
    if (m_entityMessageResponses[uuid].first == clientId) {
      auto response = m_entityMessageResponses[uuid].second;
      if (response.is<ConnectionId>()) {
        if (auto clientInfo = m_clientInfo.value(response.get<ConnectionId>()))
          clientInfo->outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Client disconnected"), uuid));
      } else {
        response.get<RpcPromiseKeeper<Json>>().fail("Client disconnected");
      }
      m_entityMessageResponses.remove(uuid);
    }
  }

  auto packets = std::move(info->outgoingPackets);
  m_clientInfo.remove(clientId);

  packets.append(make_shared<WorldStopPacket>("Removed"));

  for (auto& p : m_scriptContexts)
    p.second->invoke("removeClient", clientId);

  return packets;
}

List<ConnectionId> WorldServer::clientIds() const {
  return m_clientInfo.keys();
}

bool WorldServer::hasClient(ConnectionId clientId) const {
  return m_clientInfo.contains(clientId);
}

RectF WorldServer::clientWindow(ConnectionId clientId) const {
  auto i = m_clientInfo.find(clientId);
  if (i != m_clientInfo.end())
    return RectF(i->second->clientState.window());
  else
    return RectF::null();
}

PlayerPtr WorldServer::clientPlayer(ConnectionId clientId) const {
  auto i = m_clientInfo.find(clientId);
  if (i != m_clientInfo.end())
    return get<Player>(i->second->clientState.playerId());
  else
    return {};
}

List<EntityId> WorldServer::players() const {
  List<EntityId> playerIds;
  for (auto pair : m_clientInfo)
    playerIds.append(pair.second->clientState.playerId());
  return playerIds;
}

void WorldServer::handleIncomingPackets(ConnectionId clientId, List<PacketPtr> const& packets) {
  shared_ptr<ClientInfo> clientInfo = m_clientInfo.get(clientId);
  auto& root = Root::singleton();
  auto entityFactory = root.entityFactory();
  auto itemDatabase = root.itemDatabase();

  for (auto const& packet : packets) {
    if (auto worldStartAcknowledge = as<WorldStartAcknowledgePacket>(packet)) {
      clientInfo->started = true;

    } else if (!clientInfo->started) {
      // clients which have not sent a world start acknowledge are not sending packets intended for this world
      continue;

    } else if (auto heartbeat = as<StepUpdatePacket>(packet)) {
      clientInfo->interpolationTracker.receiveTimeUpdate(heartbeat->remoteTime);

    } else if (auto wcsPacket = as<WorldClientStateUpdatePacket>(packet)) {
      clientInfo->clientState.readDelta(wcsPacket->worldClientStateDelta);

      // Need to send all sectors that are now in the client window but were not
      // in the old
      HashSet<ServerTileSectorArray::Sector> oldSectors = take(clientInfo->activeSectors);

      for (auto const& monitoredRegion : clientInfo->monitoringRegions(m_entityMap))
        clientInfo->activeSectors.addAll(m_tileArray->validSectorsFor(monitoredRegion));

      clientInfo->pendingSectors.addAll(clientInfo->activeSectors.difference(oldSectors));

    } else if (auto mtpacket = as<ModifyTileListPacket>(packet)) {
      auto unappliedModifications = applyTileModifications(mtpacket->modifications, mtpacket->allowEntityOverlap);
      if (!unappliedModifications.empty())
        clientInfo->outgoingPackets.append(make_shared<TileModificationFailurePacket>(unappliedModifications));

    } else if (auto rtpacket = as<ReplaceTileListPacket>(packet)) {
      auto unappliedModifications = replaceTiles(rtpacket->modifications, rtpacket->tileDamage, rtpacket->applyDamage);
      if (!unappliedModifications.empty())
        clientInfo->outgoingPackets.append(make_shared<TileModificationFailurePacket>(unappliedModifications));

    } else if (auto dtgpacket = as<DamageTileGroupPacket>(packet)) {
      damageTiles(dtgpacket->tilePositions, dtgpacket->layer, dtgpacket->sourcePosition, dtgpacket->tileDamage, dtgpacket->sourceEntity);

    } else if (auto clpacket = as<CollectLiquidPacket>(packet)) {
      if (auto item = collectLiquid(clpacket->tilePositions, clpacket->liquidId))
        clientInfo->outgoingPackets.append(make_shared<GiveItemPacket>(item));

    } else if (auto sepacket = as<SpawnEntityPacket>(packet)) {
      auto netRules = clientInfo->clientState.netCompatibilityRules();
      auto entity = entityFactory->netLoadEntity(sepacket->entityType, std::move(sepacket->storeData), netRules);
      entity->readNetState(std::move(sepacket->firstNetState), 0.0f, netRules);
      addEntity(std::move(entity));

    } else if (auto rdpacket = as<RequestDropPacket>(packet)) {
      auto drop = m_entityMap->get<ItemDrop>(rdpacket->dropEntityId);
      if (drop && drop->isMaster() && drop->canTake()) {
        if (auto taken = drop->takeBy(clientInfo->clientState.playerId()))
          clientInfo->outgoingPackets.append(make_shared<GiveItemPacket>(taken->descriptor()));
      }

    } else if (auto hit = as<HitRequestPacket>(packet)) {
      if (hit->remoteHitRequest.destinationConnection() == ServerConnectionId)
        m_damageManager->pushRemoteHitRequest(hit->remoteHitRequest);
      else
        m_clientInfo.get(hit->remoteHitRequest.destinationConnection())->outgoingPackets.append(make_shared<HitRequestPacket>(hit->remoteHitRequest));

    } else if (auto damage = as<DamageRequestPacket>(packet)) {
      if (damage->remoteDamageRequest.destinationConnection() == ServerConnectionId)
        m_damageManager->pushRemoteDamageRequest(damage->remoteDamageRequest);
      else
        m_clientInfo.get(damage->remoteDamageRequest.destinationConnection())->outgoingPackets.append(make_shared<DamageRequestPacket>(damage->remoteDamageRequest));

    } else if (auto damage = as<DamageNotificationPacket>(packet)) {
      m_damageManager->pushRemoteDamageNotification(damage->remoteDamageNotification);
      for (auto const& pair : m_clientInfo) {
        if (pair.first != clientId && pair.second->needsDamageNotification(damage->remoteDamageNotification))
          pair.second->outgoingPackets.append(make_shared<DamageNotificationPacket>(damage->remoteDamageNotification));
      }

    } else if (auto entityInteract = as<EntityInteractPacket>(packet)) {
      auto targetEntityConnection = connectionForEntity(entityInteract->interactRequest.targetId);
      if (targetEntityConnection == ServerConnectionId) {
        auto interactResult = interact(entityInteract->interactRequest).result();
        clientInfo->outgoingPackets.append(make_shared<EntityInteractResultPacket>(interactResult.take(), entityInteract->requestId, entityInteract->interactRequest.sourceId));
      } else {
        auto const& forwardClientInfo = m_clientInfo.get(targetEntityConnection);
        forwardClientInfo->outgoingPackets.append(entityInteract);
      }

    } else if (auto interactResult = as<EntityInteractResultPacket>(packet)) {
      auto const& forwardClientInfo = m_clientInfo.get(connectionForEntity(interactResult->sourceEntityId));
      forwardClientInfo->outgoingPackets.append(interactResult);

    } else if (auto entityCreate = as<EntityCreatePacket>(packet)) {
      if (!entityIdInSpace(entityCreate->entityId, clientInfo->clientId)) {
        throw WorldServerException::format("WorldServer received entity create packet with illegal entity id {}.", entityCreate->entityId);
      } else {
        if (m_entityMap->entity(entityCreate->entityId)) {
          Logger::error("WorldServer received duplicate entity create packet from client, deleting old entity {}", entityCreate->entityId);
          removeEntity(entityCreate->entityId, false);
        }
        auto netRules = clientInfo->clientState.netCompatibilityRules();
        auto entity = entityFactory->netLoadEntity(entityCreate->entityType, entityCreate->storeData, netRules);
        entity->readNetState(entityCreate->firstNetState, 0.0f, netRules);
        entity->init(this, entityCreate->entityId, EntityMode::Slave);
        m_entityMap->addEntity(entity);

        if (clientInfo->interpolationTracker.interpolationEnabled())
          entity->enableInterpolation(clientInfo->interpolationTracker.extrapolationHint());
      }

    } else if (auto entityUpdateSet = as<EntityUpdateSetPacket>(packet)) {
      float interpolationLeadTime = clientInfo->interpolationTracker.interpolationLeadTime();
      m_entityMap->forAllEntities([&](EntityPtr const& entity) {
          EntityId entityId = entity->entityId();
          if (connectionForEntity(entityId) == clientId) {
            starAssert(entity->isSlave());
            entity->readNetState(entityUpdateSet->deltas.value(entityId), interpolationLeadTime, clientInfo->clientState.netCompatibilityRules());
          }
        });
      clientInfo->pendingForward = true;

    } else if (auto entityDestroy = as<EntityDestroyPacket>(packet)) {
      if (auto entity = m_entityMap->entity(entityDestroy->entityId)) {
        entity->readNetState(entityDestroy->finalNetState, clientInfo->interpolationTracker.interpolationLeadTime(), clientInfo->clientState.netCompatibilityRules());
        // Before destroying the entity, we should make sure that the entity is
        // using the absolute latest data, so we disable interpolation.
        entity->disableInterpolation();
        removeEntity(entityDestroy->entityId, entityDestroy->death);
      }

    } else if (auto disconnectWires = as<DisconnectAllWiresPacket>(packet)) {
      for (auto wireEntity : atTile<WireEntity>(disconnectWires->entityPosition)) {
        for (auto connection : wireEntity->connectionsForNode(disconnectWires->wireNode)) {
          wireEntity->removeNodeConnection(disconnectWires->wireNode, connection);
          for (auto connectedEntity : atTile<WireEntity>(connection.entityLocation))
            connectedEntity->removeNodeConnection({otherWireDirection(disconnectWires->wireNode.direction), connection.nodeIndex}, WireConnection{disconnectWires->entityPosition, disconnectWires->wireNode.nodeIndex});
        }
      }

    } else if (auto connectWire = as<ConnectWirePacket>(packet)) {
      for (auto source : atTile<WireEntity>(connectWire->inputConnection.entityLocation)) {
        for (auto target : atTile<WireEntity>(connectWire->outputConnection.entityLocation)) {
          source->addNodeConnection(WireNode{WireDirection::Input, connectWire->inputConnection.nodeIndex}, connectWire->outputConnection);
          target->addNodeConnection(WireNode{WireDirection::Output, connectWire->outputConnection.nodeIndex}, connectWire->inputConnection);
        }
      }

    } else if (auto findUniqueEntity = as<FindUniqueEntityPacket>(packet)) {
      clientInfo->outgoingPackets.append(make_shared<FindUniqueEntityResponsePacket>(findUniqueEntity->uniqueEntityId,
          m_worldStorage->findUniqueEntity(findUniqueEntity->uniqueEntityId)));

    } else if (auto entityMessagePacket = as<EntityMessagePacket>(packet)) {
      EntityPtr entity;
      if (entityMessagePacket->entityId.is<EntityId>())
        entity = m_entityMap->entity(entityMessagePacket->entityId.get<EntityId>());
      else
        entity = m_entityMap->entity(loadUniqueEntity(entityMessagePacket->entityId.get<String>()));

      if (!entity) {
        clientInfo->outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Unknown entity"), entityMessagePacket->uuid));
      } else {
        if (entity->isMaster()) {
          auto response = entity->receiveMessage(clientId, entityMessagePacket->message, entityMessagePacket->args);
          if (response)
            clientInfo->outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeRight(response.take()), entityMessagePacket->uuid));
          else
            clientInfo->outgoingPackets.append(make_shared<EntityMessageResponsePacket>(makeLeft("Message not handled by entity"), entityMessagePacket->uuid));
        } else if (auto const& clientInfo = m_clientInfo.value(connectionForEntity(entity->entityId()))) {
          m_entityMessageResponses[entityMessagePacket->uuid] = {clientInfo->clientId, clientId};
          entityMessagePacket->fromConnection = clientId;
          clientInfo->outgoingPackets.append(std::move(entityMessagePacket));
        }
      }

    } else if (auto entityMessageResponsePacket = as<EntityMessageResponsePacket>(packet)) {
      if (!m_entityMessageResponses.contains(entityMessageResponsePacket->uuid))
        Logger::warn("EntityMessageResponse received for unknown context [{}]!", entityMessageResponsePacket->uuid.hex());
      else {
        auto response = m_entityMessageResponses.take(entityMessageResponsePacket->uuid).second;
        if (response.is<ConnectionId>()) {
          if (auto clientInfo = m_clientInfo.value(response.get<ConnectionId>()))
            clientInfo->outgoingPackets.append(std::move(entityMessageResponsePacket));
        } else {
          if (entityMessageResponsePacket->response.isRight())
            response.get<RpcPromiseKeeper<Json>>().fulfill(entityMessageResponsePacket->response.right());
          else
            response.get<RpcPromiseKeeper<Json>>().fail(entityMessageResponsePacket->response.left());
        }
      }
    } else if (auto pingPacket = as<PingPacket>(packet)) {
      clientInfo->outgoingPackets.append(make_shared<PongPacket>(pingPacket->time));

    } else if (auto updateWorldProperties = as<UpdateWorldPropertiesPacket>(packet)) {
      // Kae: Properties set to null (nil from Lua) should be erased instead of lingering around
      for (auto& pair : updateWorldProperties->updatedProperties) {
        if (pair.second.isNull())
          m_worldProperties.erase(pair.first);
        else
          m_worldProperties[pair.first] = pair.second;
      }
      for (auto const& pair : m_clientInfo)
        pair.second->outgoingPackets.append(make_shared<UpdateWorldPropertiesPacket>(updateWorldProperties->updatedProperties));

    } else if (auto updateWorldTemplate = as<UpdateWorldTemplatePacket>(packet)) {
      if (!clientInfo->admin)
        continue; // nuh-uh!

      auto newWorldTemplate = make_shared<WorldTemplate>(updateWorldTemplate->templateData);
      setTemplate(newWorldTemplate);
      // setTemplate re-adds all clients currently, update clientInfo
      clientInfo = m_clientInfo.get(clientId);
    } else {
      throw WorldServerException::format("Improper packet type {} received by client", (int)packet->type());
    }
  }
}

List<PacketPtr> WorldServer::getOutgoingPackets(ConnectionId clientId) {
  auto const& clientInfo = m_clientInfo.get(clientId);
  return std::move(clientInfo->outgoingPackets);
}

bool WorldServer::sendPacket(ConnectionId clientId, PacketPtr const& packet) {
  if (auto clientInfo = m_clientInfo.ptr(clientId)) {
    clientInfo->get()->outgoingPackets.append(packet);
    return true;
  }
  return false;
}

Maybe<Json> WorldServer::receiveMessage(ConnectionId fromConnection, String const& message, JsonArray const& args) {
  Maybe<Json> result;
  for (auto& p : m_scriptContexts) {
    result = p.second->handleMessage(message, fromConnection == ServerConnectionId, args);
    if (result)
      break;
  }
  return result;
}

WorldServerFidelity WorldServer::fidelity() const {
  return m_fidelity;
}

void WorldServer::setFidelity(WorldServerFidelity fidelity) {
  m_fidelity = fidelity;
  m_fidelityConfig = m_serverConfig.get("fidelitySettings").get(WorldServerFidelityNames.getRight(m_fidelity));
}

bool WorldServer::shouldExpire() {
  if (!m_clientInfo.empty()) {
    m_expiryTimer.reset();
    return false;
  }

  return m_expiryTimer.ready();
}

void WorldServer::setExpiryTime(float expiryTime) {
  m_expiryTimer = GameTimer(expiryTime);
}

void WorldServer::update(float dt) {
  m_currentTime += dt;
  ++m_currentStep;
  for (auto const& pair : m_clientInfo)
    pair.second->interpolationTracker.update(m_currentTime);

  List<WorldAction> triggeredActions;
  eraseWhere(m_timers, [&triggeredActions, dt](pair<float, WorldAction>& timer) {
      if ((timer.first -= dt) <= 0) {
        triggeredActions.append(timer.second);
        return true;
      }
      return false;
    });
  for (auto const& action : triggeredActions)
    action(this);

  m_spawner.update(dt);

  bool doBreakChecks = m_tileEntityBreakCheckTimer.wrapTick(m_currentTime) && m_needsGlobalBreakCheck;
  if (doBreakChecks)
    m_needsGlobalBreakCheck = false;

  List<EntityId> toRemove;
  m_entityMap->updateAllEntities([&](EntityPtr const& entity) {
      entity->update(dt, m_currentStep);

      if (auto tileEntity = as<TileEntity>(entity)) {
        // Only do break checks on objects if all sectors the object touches
        // *and surrounding sectors* are active.  Objects that this object
        // rests on can be up to an entire sector large in any direction.
        if (doBreakChecks && regionActive(RectI::integral(tileEntity->metaBoundBox().translated(tileEntity->position())).padded(WorldSectorSize)))
          tileEntity->checkBroken();
        updateTileEntityTiles(tileEntity);
      }

      if (entity->shouldDestroy() && entity->entityMode() == EntityMode::Master)
        toRemove.append(entity->entityId());
    }, [](EntityPtr const& a, EntityPtr const& b) {
      return a->entityType() < b->entityType();
    });

  for (auto& pair : m_scriptContexts)
    pair.second->update(pair.second->updateDt(dt));

  updateDamage(dt);
  if (shouldRunThisStep("wiringUpdate"))
    m_wireProcessor->process();

  m_sky->update(dt);

  List<RectI> clientWindows;
  List<RectI> clientMonitoringRegions;
  for (auto const& pair : m_clientInfo) {
    clientWindows.append(pair.second->clientState.window());
    for (auto const& region : pair.second->monitoringRegions(m_entityMap))
      clientMonitoringRegions.appendAll(m_geometry.splitRect(region));
  }

  m_weather.setClientVisibleRegions(clientWindows);
  m_weather.update(dt);
  for (auto projectile : m_weather.pullNewProjectiles())
    addEntity(std::move(projectile));

  if (shouldRunThisStep("liquidUpdate")) {
    m_liquidEngine->setProcessingLimit(m_fidelityConfig.optUInt("liquidEngineBackgroundProcessingLimit"));
    m_liquidEngine->setNoProcessingLimitRegions(clientMonitoringRegions);
    m_liquidEngine->update();
  }

  if (shouldRunThisStep("fallingBlocksUpdate"))
    m_fallingBlocksAgent->update();

  if (auto delta = shouldRunThisStep("blockDamageUpdate"))
    updateDamagedBlocks(*delta * dt);

  if (auto delta = shouldRunThisStep("worldStorageTick"))
    m_worldStorage->tick(*delta * GlobalTimestep, &m_worldId);

  if (auto delta = shouldRunThisStep("worldStorageGenerate")) {
    m_worldStorage->generateQueue(m_fidelityConfig.optUInt("worldStorageGenerationLevelLimit"), [this](WorldStorage::Sector a, WorldStorage::Sector b) {
        auto distanceToClosestPlayer = [this](WorldStorage::Sector sector) {
          Vec2F sectorCenter = RectF(*m_worldStorage->regionForSector(sector)).center();
          float distance = highest<float>();
          for (auto const& pair : m_clientInfo) {
            if (auto player = get<Player>(pair.second->clientState.playerId()))
              distance = min(vmag(sectorCenter - player->position()), distance);
          }
          return distance;
        };

        return distanceToClosestPlayer(a) < distanceToClosestPlayer(b);
      });
  }

  for (EntityId entityId : toRemove)
    removeEntity(entityId, true);

  bool sendRemoteUpdates = m_entityUpdateTimer.wrapTick(dt);
  for (auto const& pair : m_clientInfo) {
    for (auto const& monitoredRegion : pair.second->monitoringRegions(m_entityMap))
      signalRegion(monitoredRegion.padded(jsonToVec2I(m_serverConfig.get("playerActiveRegionPad"))));
    queueUpdatePackets(pair.first, sendRemoteUpdates);
  }
  m_netStateCache.clear();

  for (auto& pair : m_clientInfo)
    pair.second->pendingForward = false;

  m_expiryTimer.tick(dt);

  LogMap::set(strf("server_{}_entities", m_worldId), strf("{} in {} sectors", m_entityMap->size(), m_tileArray->loadedSectorCount()));
  LogMap::set(strf("server_{}_time", m_worldId), strf("age = {:4.2f}, day = {:4.2f}/{:4.2f}s", epochTime(), timeOfDay(), dayLength()));
  LogMap::set(strf("server_{}_active_liquid", m_worldId), m_liquidEngine->activeCells());
  LogMap::set(strf("server_{}_lua_mem", m_worldId), m_luaRoot->luaMemoryUsage());
}

WorldGeometry WorldServer::geometry() const {
  return m_geometry;
}

uint64_t WorldServer::currentStep() const {
  return m_currentStep;
}

MaterialId WorldServer::material(Vec2I const& pos, TileLayer layer) const {
  return m_tileArray->tile(pos).material(layer);
}

MaterialHue WorldServer::materialHueShift(Vec2I const& position, TileLayer layer) const {
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundHueShift : tile.backgroundHueShift;
}

ModId WorldServer::mod(Vec2I const& pos, TileLayer layer) const {
  return m_tileArray->tile(pos).mod(layer);
}

MaterialHue WorldServer::modHueShift(Vec2I const& position, TileLayer layer) const {
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundModHueShift : tile.backgroundModHueShift;
}

MaterialColorVariant WorldServer::colorVariant(Vec2I const& position, TileLayer layer) const {
  auto const& tile = m_tileArray->tile(position);
  return layer == TileLayer::Foreground ? tile.foregroundColorVariant : tile.backgroundColorVariant;
}

EntityPtr WorldServer::entity(EntityId entityId) const {
  return m_entityMap->entity(entityId);
}

void WorldServer::addEntity(EntityPtr const& entity, EntityId entityId) {
  if (!entity)
    return;

  entity->init(this, m_entityMap->reserveEntityId(entityId), EntityMode::Master);
  m_entityMap->addEntity(entity);

  if (auto tileEntity = as<TileEntity>(entity))
    updateTileEntityTiles(tileEntity);
}

EntityPtr WorldServer::closestEntity(Vec2F const& center, float radius, EntityFilter selector) const {
  return m_entityMap->closestEntity(center, radius, selector);
}

void WorldServer::forAllEntities(EntityCallback callback) const {
  m_entityMap->forAllEntities(callback);
}

void WorldServer::forEachEntity(RectF const& boundBox, EntityCallback callback) const {
  m_entityMap->forEachEntity(boundBox, callback);
}

void WorldServer::forEachEntityLine(Vec2F const& begin, Vec2F const& end, EntityCallback callback) const {
  m_entityMap->forEachEntityLine(begin, end, callback);
}

void WorldServer::forEachEntityAtTile(Vec2I const& pos, EntityCallbackOf<TileEntity> callback) const {
  m_entityMap->forEachEntityAtTile(pos, callback);
}

EntityPtr WorldServer::findEntity(RectF const& boundBox, EntityFilter entityFilter) const {
  return m_entityMap->findEntity(boundBox, entityFilter);
}

EntityPtr WorldServer::findEntityLine(Vec2F const& begin, Vec2F const& end, EntityFilter entityFilter) const {
  return m_entityMap->findEntityLine(begin, end, entityFilter);
}

EntityPtr WorldServer::findEntityAtTile(Vec2I const& pos, EntityFilterOf<TileEntity> entityFilter) const {
  return m_entityMap->findEntityAtTile(pos, entityFilter);
}

bool WorldServer::tileIsOccupied(Vec2I const& pos, TileLayer layer, bool includeEphemeral, bool checkCollision) const {
  return WorldImpl::tileIsOccupied(m_tileArray, m_entityMap, pos, layer, includeEphemeral, checkCollision);
}

CollisionKind WorldServer::tileCollisionKind(Vec2I const& pos) const {
  return WorldImpl::tileCollisionKind(m_tileArray, m_entityMap, pos);
}


void WorldServer::forEachCollisionBlock(RectI const& region, function<void(CollisionBlock const&)> const& iterator) const {
  const_cast<WorldServer*>(this)->freshenCollision(region);
  m_tileArray->tileEach(region, [iterator](Vec2I const& pos, ServerTile const& tile) {
      if (tile.getCollision() == CollisionKind::Null) {
        iterator(CollisionBlock::nullBlock(pos));
      } else {
        starAssert(!tile.collisionCacheDirty);
        for (auto const& block : tile.collisionCache)
          iterator(block);
      }
    });
}

bool WorldServer::isTileConnectable(Vec2I const& pos, TileLayer layer, bool tilesOnly) const {
  return m_tileArray->tile(pos).isConnectable(layer, tilesOnly);
}

bool WorldServer::pointTileCollision(Vec2F const& point, CollisionSet const& collisionSet) const {
  return m_tileArray->tile(Vec2I(point.floor())).isColliding(collisionSet);
}

bool WorldServer::lineTileCollision(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) const {
  return WorldImpl::lineTileCollision(m_geometry, m_tileArray, begin, end, collisionSet);
}

Maybe<pair<Vec2F, Vec2I>> WorldServer::lineTileCollisionPoint(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet) const {
  return WorldImpl::lineTileCollisionPoint(m_geometry, m_tileArray, begin, end, collisionSet);
}

List<Vec2I> WorldServer::collidingTilesAlongLine(Vec2F const& begin, Vec2F const& end, CollisionSet const& collisionSet, int maxSize, bool includeEdges) const {
  return WorldImpl::collidingTilesAlongLine(m_geometry, m_tileArray, begin, end, collisionSet, maxSize, includeEdges);
}

bool WorldServer::rectTileCollision(RectI const& region, CollisionSet const& collisionSet) const {
  return WorldImpl::rectTileCollision(m_tileArray, region, collisionSet);
}

LiquidLevel WorldServer::liquidLevel(Vec2I const& pos) const {
  return m_tileArray->tile(pos).liquid;
}

LiquidLevel WorldServer::liquidLevel(RectF const& region) const {
  return WorldImpl::liquidLevel(m_tileArray, region);
}

void WorldServer::activateLiquidRegion(RectI const& region) {
  m_liquidEngine->visitRegion(region);
}

void WorldServer::activateLiquidLocation(Vec2I const& location) {
  m_liquidEngine->visitLocation(location);
}

void WorldServer::requestGlobalBreakCheck() {
  m_needsGlobalBreakCheck = true;
}

void WorldServer::setSpawningEnabled(bool spawningEnabled) {
  m_spawner.setActive(spawningEnabled);
}

void WorldServer::setPropertyListener(String const& propertyName, WorldPropertyListener listener) {
  m_worldPropertyListeners[propertyName] = listener;
}

TileModificationList WorldServer::validTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) const {
  return WorldImpl::splitTileModifications(m_entityMap, modificationList, allowEntityOverlap, m_tileGetterFunction, [this](Vec2I pos, TileModification) {
      return !isTileProtected(pos);
    }).first;
}

TileModificationList WorldServer::applyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) {
  return doApplyTileModifications(modificationList, allowEntityOverlap);
}

bool WorldServer::forceModifyTile(Vec2I const& pos, TileModification const& modification, bool allowEntityOverlap) {
  return forceApplyTileModifications({{pos, modification}}, allowEntityOverlap).empty();
}

TileModificationList WorldServer::forceApplyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap) {
  return doApplyTileModifications(modificationList, allowEntityOverlap, true);
}

bool WorldServer::replaceTile(Vec2I const& pos, TileModification const& modification, TileDamage const& tileDamage) {
  if (isTileProtected(pos))
    return false;

  if (!WorldImpl::validateTileReplacement(modification))
    return false;
  
  if (auto placeMaterial = modification.ptr<PlaceMaterial>()) {
    if (!isTileConnectable(pos, placeMaterial->layer, true))
      return false;

    if (auto tile = m_tileArray->modifyTile(pos)) {
      auto damageParameters = WorldImpl::tileDamageParameters(tile, placeMaterial->layer, tileDamage);
      bool harvested = tileDamage.amount >= 0 && tileDamage.harvestLevel >= damageParameters.requiredHarvestLevel();
      auto damage = placeMaterial->layer == TileLayer::Foreground ? tile->foregroundDamage : tile->backgroundDamage;
      Vec2F dropPosition = centerOfTile(pos);

      for (auto drop : destroyBlock(placeMaterial->layer, pos, harvested, !tileDamageIsPenetrating(damage.damageType()), false))
        addEntity(ItemDrop::createRandomizedDrop(drop, dropPosition));
      
      return true;
    }
  }

  return false;
}

TileModificationList WorldServer::replaceTiles(TileModificationList const& modificationList, TileDamage const& tileDamage, bool applyDamage) {
  TileModificationList success, failures;

  if (applyDamage) {
    List<Vec2I> toDamage;
    TileLayer layer;

    for (auto pair : modificationList) {
      if (auto placeMaterial = pair.second.ptr<PlaceMaterial>()) {
        layer = placeMaterial->layer;

        if (placeMaterial->material == material(pair.first, layer)) {
          failures.append(pair);
          continue;
        }

        if (damageWouldDestroy(pair.first, layer, tileDamage)) {
          if (replaceTile(pair.first, pair.second, tileDamage))
            success.append(pair);
          else
            failures.append(pair);
          continue;
        }

        toDamage.append(pair.first);
        success.append(pair);
        continue;
      }
      
      failures.append(pair);
    }

    if (!toDamage.empty())
      damageTiles(toDamage, layer, Vec2F(), tileDamage, Maybe<EntityId>());
    
  } else {
    for (auto pair : modificationList) {
      if (replaceTile(pair.first, pair.second, tileDamage))
        success.append(pair);
      else
        failures.append(pair);
    }
  }

  failures.appendAll(doApplyTileModifications(success, true, false, false));

  for (auto pair : success) {
    checkEntityBreaks(RectF::withSize(Vec2F(pair.first), Vec2F(1, 1)));
    m_liquidEngine->visitLocation(pair.first);
    m_fallingBlocksAgent->visitLocation(pair.first);
  }

  return failures;
}

bool WorldServer::damageWouldDestroy(Vec2I const& pos, TileLayer layer, TileDamage const& tileDamage) const {
  return WorldImpl::damageWouldDestroy(m_tileArray, pos, layer, tileDamage);
}

TileDamageResult WorldServer::damageTiles(List<Vec2I> const& positions, TileLayer layer, Vec2F const& sourcePosition, TileDamage const& damage, Maybe<EntityId> sourceEntity) {
  Set<Vec2I> positionSet;
  for (auto const& pos : positions)
    positionSet.add(m_geometry.xwrap(pos));

  Set<EntityPtr> damagedEntities;
  auto res = TileDamageResult::None;

  for (auto const& pos : positionSet) {
    if (auto tile = m_tileArray->modifyTile(pos)) {
      auto tileDamage = damage;
      if (isTileProtected(pos))
        tileDamage.type = TileDamageType::Protected;

      auto tileRes = TileDamageResult::None;
      if (layer == TileLayer::Foreground) {
        Vec2I entityDamagePos = pos;
        Set<Vec2I> damagePositionSet = Set<Vec2I>(positionSet);
        if (tile->rootSource) {
          entityDamagePos = tile->rootSource.value();
          damagePositionSet.add(entityDamagePos);
        }

        for (auto entity : m_entityMap->entitiesAtTile(entityDamagePos)) {
          if (!damagedEntities.contains(entity)) {
            Set<Vec2I> entitySpacesSet;
            for (auto const& space : entity->spaces())
              entitySpacesSet.add(m_geometry.xwrap(entity->tilePosition() + space));

            bool broken = entity->damageTiles(entitySpacesSet.intersection(damagePositionSet).values(), sourcePosition, tileDamage);
            if (sourceEntity.isValid() && broken) {
              Maybe<String> name;
              if (auto object = as<Object>(entity))
                name = object->name();
              sendEntityMessage(*sourceEntity, "tileEntityBroken", {
                  jsonFromVec2I(pos),
                  EntityTypeNames.getRight(entity->entityType()),
                  jsonFromMaybe(name),
                });
            }

            if (tileDamage.type == TileDamageType::Protected)
              tileRes = TileDamageResult::Protected;
            else if (broken || entity->canBeDamaged()) {
              tileRes = TileDamageResult::Normal;
              damagedEntities.add(entity);
            }
          }
        }
      }

      // Penetrating damage should carry through to the blocks behind this
      // entity.
      if (tileRes == TileDamageResult::None || tileDamageIsPenetrating(tileDamage.type)) {
        auto damageParameters = WorldImpl::tileDamageParameters(tile, layer, tileDamage);

        if (layer == TileLayer::Foreground && isRealMaterial(tile->foreground)) {
          if (!tile->rootSource || damagedEntities.empty()) {
            tile->foregroundDamage.damage(damageParameters, sourcePosition, tileDamage);

            // if the tile is broken, send a message back to the source entity with position, layer, dungeonId, and whether the tile was harvested
            if (sourceEntity.isValid() && tile->foregroundDamage.dead()) {
              sendEntityMessage(*sourceEntity, "tileBroken", {
                  jsonFromVec2I(pos),
                  TileLayerNames.getRight(TileLayer::Foreground),
                  tile->foreground,
                  tile->dungeonId,
                  tile->foregroundDamage.harvested(),
                });
            }

            queueTileDamageUpdates(pos, TileLayer::Foreground);
            m_damagedBlocks.add(pos);

            if (tileDamage.type == TileDamageType::Protected)
              tileRes = TileDamageResult::Protected;
            else
              tileRes = TileDamageResult::Normal;
          }
        } else if (layer == TileLayer::Background && isRealMaterial(tile->background)) {
          tile->backgroundDamage.damage(damageParameters, sourcePosition, tileDamage);
          
          // if the tile is broken, send a message back to the source entity with position and whether the tile was harvested
            if (sourceEntity.isValid() && tile->backgroundDamage.dead()) {
              sendEntityMessage(*sourceEntity, "tileBroken", {
                  jsonFromVec2I(pos),
                  TileLayerNames.getRight(TileLayer::Background),
                  tile->background,
                  tile->dungeonId,
                  tile->backgroundDamage.harvested(),
                });
            }

          queueTileDamageUpdates(pos, TileLayer::Background);
          m_damagedBlocks.add(pos);

          if (tileDamage.type == TileDamageType::Protected)
            tileRes = TileDamageResult::Protected;
          else
            tileRes = TileDamageResult::Normal;
        }
      }

      res = max<TileDamageResult>(res, tileRes);
    }
  }

  return res;
}

DungeonId WorldServer::dungeonId(Vec2I const& pos) const {
  return m_tileArray->tile(pos).dungeonId;
}

bool WorldServer::isPlayerModified(RectI const& region) const {
  return m_tileArray->tileSatisfies(region, [](Vec2I const&, ServerTile const& tile) {
      return tile.dungeonId == ConstructionDungeonId || tile.dungeonId == DestroyedBlockDungeonId;
    });
}

ItemDescriptor WorldServer::collectLiquid(List<Vec2I> const& tilePositions, LiquidId liquidId) {
  float bucketSize = Root::singleton().assets()->json("/items/defaultParameters.config:liquidItems.bucketSize").toFloat();
  unsigned drainedUnits = 0;
  float nextUnit = bucketSize;
  List<ServerTile*> maybeDrainTiles;

  for (auto pos : tilePositions) {
    ServerTile* tile = m_tileArray->modifyTile(pos);
    if (tile->liquid.liquid == liquidId && !isTileProtected(pos)) {
      if (tile->liquid.level >= nextUnit) {
        tile->liquid.take(nextUnit);
        nextUnit = bucketSize;
        drainedUnits++;

        for (auto previousTile : maybeDrainTiles)
          previousTile->liquid.take(previousTile->liquid.level);

        maybeDrainTiles.clear();
      }

      if (tile->liquid.level > 0) {
        nextUnit -= tile->liquid.level;
        maybeDrainTiles.append(tile);
      }

      for (auto const& pair : m_clientInfo) {
        if (pair.second->activeSectors.contains(m_tileArray->sectorFor(pos)))
          pair.second->pendingLiquidUpdates.add(pos);
      }
      m_liquidEngine->visitLocation(pos);
    }
  }

  if (drainedUnits > 0) {
    auto liquidConfig = Root::singleton().liquidsDatabase()->liquidSettings(liquidId);
    if (liquidConfig && liquidConfig->itemDrop)
      return liquidConfig->itemDrop.multiply(drainedUnits);
  }

  return ItemDescriptor();
}

bool WorldServer::placeDungeon(String const& dungeonName, Vec2I const& position, Maybe<DungeonId> dungeonId, bool forcePlacement) {
  m_generatingDungeon = true;
  m_tileProtectionEnabled = false;

  auto seed = worldTemplate()->seedFor(position[0], position[1]);
  auto facade = make_shared<DungeonGeneratorWorld>(this, true);
  bool placed = false;
  DungeonGenerator dungeonGenerator(dungeonName, seed, m_worldTemplate->threatLevel(), dungeonId);
  if (auto generateResult = dungeonGenerator.generate(facade, position, false, forcePlacement)) {
    auto worldGenerator = make_shared<WorldGenerator>(this);
    for (auto position : generateResult->second) {
      if (ServerTile* tile = modifyServerTile(position))
        worldGenerator->replaceBiomeBlocks(tile);
    }
    placed = true;
  }

  m_tileProtectionEnabled = true;
  m_generatingDungeon = false;

  return placed;
}

void WorldServer::addBiomeRegion(Vec2I const& position, String const& biomeName, String const& subBlockSelector, int width) {
  width = std::min(width, (int)m_worldTemplate->size()[0]);

  auto regions = m_worldTemplate->previewAddBiomeRegion(position, width);

  if (regions.empty()) {
    Logger::info("Aborting addBiomeRegion as it would have no result!");
    return;
  }

  for (auto region : regions) {
    for (auto sector : m_worldStorage->sectorsForRegion(region))
      m_worldStorage->triggerTerraformSector(sector);
  }

  m_worldTemplate->addBiomeRegion(position, biomeName, subBlockSelector, width);
}

void WorldServer::expandBiomeRegion(Vec2I const& position, int newWidth) {
  newWidth = std::min(newWidth, (int)m_worldTemplate->size()[0]);

  auto regions = m_worldTemplate->previewExpandBiomeRegion(position, newWidth);

  if (regions.empty()) {
    Logger::info("Aborting expandBiomeRegion as it would have no result!");
    return;
  }

  for (auto region : regions) {
    for (auto sector : m_worldStorage->sectorsForRegion(region))
      m_worldStorage->triggerTerraformSector(sector);
  }

  m_worldTemplate->expandBiomeRegion(position, newWidth);
}

bool WorldServer::pregenerateAddBiome(Vec2I const& position, int width) {
  auto regions = m_worldTemplate->previewAddBiomeRegion(position, width);

  bool generationComplete = true;
  for (auto region : regions)
    generationComplete = generationComplete && signalRegion(region);

  return generationComplete;
}

bool WorldServer::pregenerateExpandBiome(Vec2I const& position, int newWidth) {
  auto regions = m_worldTemplate->previewExpandBiomeRegion(position, newWidth);

  bool generationComplete = true;
  for (auto region : regions)
    generationComplete = generationComplete && signalRegion(region);

  return generationComplete;
}

void WorldServer::setLayerEnvironmentBiome(Vec2I const& position) {
  auto biomeName = m_worldTemplate->worldLayout()->setLayerEnvironmentBiome(position);

  auto layoutJson = m_worldTemplate->worldLayout()->toJson();
  for (auto const& pair : m_clientInfo)
    pair.second->outgoingPackets.append(make_shared<WorldLayoutUpdatePacket>(layoutJson));
}

void WorldServer::setPlanetType(String const& planetType, String const& primaryBiomeName) {
  if (auto terrestrialParameters = as<TerrestrialWorldParameters>(m_worldTemplate->worldParameters())) {
    if (terrestrialParameters->typeName != planetType) {
      auto newTerrestrialParameters = make_shared<TerrestrialWorldParameters>(*terrestrialParameters);

      newTerrestrialParameters->typeName = planetType;
      newTerrestrialParameters->primaryBiome = primaryBiomeName;

      auto biomeDatabase = Root::singleton().biomeDatabase();
      auto newWeatherPool = biomeDatabase->biomeWeathers(primaryBiomeName, m_worldTemplate->worldSeed(), m_worldTemplate->threatLevel());
      newTerrestrialParameters->weatherPool = newWeatherPool;

      auto newSkyColors = biomeDatabase->biomeSkyColoring(primaryBiomeName, m_worldTemplate->worldSeed());
      newTerrestrialParameters->skyColoring = newSkyColors;

      newTerrestrialParameters->airless = biomeDatabase->biomeIsAirless(primaryBiomeName);
      newTerrestrialParameters->environmentStatusEffects = {};

      newTerrestrialParameters->terraformed = true;

      m_worldTemplate->setWorldParameters(newTerrestrialParameters);

      for (auto const& pair : m_clientInfo)
        pair.second->outgoingPackets.append(make_shared<WorldParametersUpdatePacket>(netStoreVisitableWorldParameters(newTerrestrialParameters)));

      auto newSkyParameters = SkyParameters(m_worldTemplate->skyParameters(), newTerrestrialParameters);
      m_worldTemplate->setSkyParameters(newSkyParameters);

      auto referenceClock = m_sky->referenceClock();
      m_sky = make_shared<Sky>(m_worldTemplate->skyParameters(), false);
      m_sky->setReferenceClock(referenceClock);

      m_weather.setup(m_worldTemplate->weathers(), m_worldTemplate->undergroundLevel(), m_geometry, [this](Vec2I const& pos) {
        auto const& tile = m_tileArray->tile(pos);
        return !isRealMaterial(tile.background);
      });

      m_newPlanetType = pair<String, String>{planetType, primaryBiomeName};
    }
  }
}


void WorldServer::setWeatherIndex(size_t weatherIndex, bool force) {
  m_weather.setWeatherIndex(weatherIndex, force);
}

void WorldServer::setWeather(String const& weatherName, bool force) {
  m_weather.setWeather(weatherName, force);
}

StringList WorldServer::weatherList() const {
  return m_weather.weatherList();
}

Maybe<pair<String, String>> WorldServer::pullNewPlanetType() {
  if (m_newPlanetType)
    return m_newPlanetType.take();
  return {};
}

bool WorldServer::isTileProtected(Vec2I const& pos) const {
  if (!m_tileProtectionEnabled)
    return false;

  auto const& tile = m_tileArray->tile(pos);
  return m_protectedDungeonIds.contains(tile.dungeonId);
}

bool WorldServer::getTileProtection(DungeonId dungeonId) const {
  return m_protectedDungeonIds.contains(dungeonId);
}

void WorldServer::setTileProtection(DungeonId dungeonId, bool isProtected) {
  bool updated = false;
  if (isProtected) {
    updated = m_protectedDungeonIds.add(dungeonId);
  } else {
    updated = m_protectedDungeonIds.remove(dungeonId);
  }

  if (updated) {
    for (auto const& pair : m_clientInfo)
      pair.second->outgoingPackets.append(make_shared<UpdateTileProtectionPacket>(dungeonId, isProtected));
  
    Logger::info("Protected dungeonIds for world set to {}", m_protectedDungeonIds);
  }
}

size_t WorldServer::setTileProtection(List<DungeonId> const& dungeonIds, bool isProtected) {
  List<PacketPtr> updates;
  updates.reserve(dungeonIds.size());
  for (auto const& dungeonId : dungeonIds)
    if (isProtected ? m_protectedDungeonIds.add(dungeonId) : m_protectedDungeonIds.remove(dungeonId))
      updates.append(make_shared<UpdateTileProtectionPacket>(dungeonId, isProtected));

  if (updates.empty())
    return 0;

  for (auto const& pair : m_clientInfo)
    pair.second->outgoingPackets.appendAll(updates);

  auto newDungeonIds = m_protectedDungeonIds.values();
  sort(newDungeonIds);
  Logger::info("Protected dungeonIds for world set to {}", newDungeonIds);
  return updates.size();
}

void WorldServer::setTileProtectionEnabled(bool enabled) {
  m_tileProtectionEnabled = enabled;
}

void WorldServer::setDungeonId(RectI const& tileArea, DungeonId dungeonId) {
  for (int x = tileArea.xMin(); x < tileArea.xMax(); ++x) {
    for (int y = tileArea.yMin(); y < tileArea.yMax(); ++y) {
      auto pos = Vec2I{x, y};
      if (auto tile = m_tileArray->modifyTile(pos)) {
        tile->dungeonId = dungeonId;
        queueTileUpdates(pos);
      }
    }
  }
}

void WorldServer::setDungeonGravity(DungeonId dungeonId, Maybe<float> gravity) {
  Maybe<float> current = m_dungeonIdGravity.maybe(dungeonId);
  if (gravity != current) {
    if (gravity)
      m_dungeonIdGravity[dungeonId] = *gravity;
    else
      m_dungeonIdGravity.remove(dungeonId);

    for (auto const& p : m_clientInfo)
      p.second->outgoingPackets.append(make_shared<SetDungeonGravityPacket>(dungeonId, gravity));
  }
}

float WorldServer::gravity(Vec2F const& pos) const {
  return gravityFromTile(m_tileArray->tile(Vec2I::round(pos)));
}

float WorldServer::gravityFromTile(ServerTile const& tile) const {
  return m_dungeonIdGravity.maybe(tile.dungeonId).value(m_worldTemplate->gravity());
}

bool WorldServer::isFloatingDungeonWorld() const {
  return m_worldTemplate && m_worldTemplate->worldParameters()
      && m_worldTemplate->worldParameters()->type() == WorldParametersType::FloatingDungeonWorldParameters;
}

void WorldServer::init(bool firstTime) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto liquidsDatabase = root.liquidsDatabase();

  m_serverConfig = assets->json("/worldserver.config");
  setFidelity(WorldServerFidelity::Medium);

  m_worldStorage->setFloatingDungeonWorld(isFloatingDungeonWorld());

  m_currentTime = 0;
  m_currentStep = 0;
  m_generatingDungeon = false;
  m_geometry = WorldGeometry(m_worldTemplate->size());
  m_entityMap = m_worldStorage->entityMap();
  m_tileArray = m_worldStorage->tileArray();
  m_tileGetterFunction = [&](Vec2I pos) -> ServerTile const& { return m_tileArray->tile(pos); };
  m_damageManager = make_shared<DamageManager>(this, ServerConnectionId);
  m_wireProcessor = make_shared<WireProcessor>(m_worldStorage);
  m_luaRoot = make_shared<LuaRoot>();
  m_luaRoot->luaEngine().setNullTerminated(false);
  m_luaRoot->tuneAutoGarbageCollection(m_serverConfig.getFloat("luaGcPause"), m_serverConfig.getFloat("luaGcStepMultiplier"));

  m_sky = make_shared<Sky>(m_worldTemplate->skyParameters(), false);

  m_lightIntensityCalculator.setParameters(assets->json("/lighting.config:intensity"));

  m_entityMessageResponses = {};

  m_collisionGenerator.init([=](int x, int y) {
      return m_tileArray->tile({x, y}).getCollision();
    });

  m_entityUpdateTimer = GameTimer(m_serverConfig.query("interpolationSettings.normal").getFloat("entityUpdateDelta") / 60.f);
  m_tileEntityBreakCheckTimer = GameTimer(m_serverConfig.getFloat("tileEntityBreakCheckInterval"));

  m_liquidEngine = make_shared<LiquidCellEngine<LiquidId>>(liquidsDatabase->liquidEngineParameters(), make_shared<LiquidWorld>(this));
  for (auto liquidSettings : liquidsDatabase->allLiquidSettings())
    m_liquidEngine->setLiquidTickDelta(liquidSettings->id, liquidSettings->tickDelta);

  m_fallingBlocksAgent = make_shared<FallingBlocksAgent>(make_shared<FallingBlocksWorld>(this));

  setupForceRegions();

  setTileProtection(ProtectedZeroGDungeonId, true);

  try {
    m_spawner.init(make_shared<SpawnerWorld>(this));

    RandomSource rnd = RandomSource(m_worldTemplate->worldSeed());

    if (firstTime) {
      m_generatingDungeon = true;
      DungeonId currentDungeonId = 0;

      for (auto const& dungeon : m_worldTemplate->dungeons()) {
        Logger::info("Placing dungeon {}", dungeon.dungeon);
        int retryCounter = m_serverConfig.getInt("spawnDungeonRetries");
        while (retryCounter > 0) {
          --retryCounter;
          auto dungeonFacade = make_shared<DungeonGeneratorWorld>(this, true);
          Vec2I position = Vec2I((dungeon.baseX + rnd.randInt(0, dungeon.xVariance)) % m_geometry.width(), dungeon.baseHeight);
          DungeonGenerator dungeonGenerator(dungeon.dungeon, m_worldTemplate->worldSeed(), m_worldTemplate->threatLevel(), currentDungeonId);
          if (auto generateResult = dungeonGenerator.generate(dungeonFacade, position, dungeon.blendWithTerrain, dungeon.force)) {
            if (dungeonGenerator.definition()->isProtected())
              setTileProtection(currentDungeonId, true);

            if (auto gravity = dungeonGenerator.definition()->gravity())
              m_dungeonIdGravity[currentDungeonId] = *gravity;

            if (auto breathable = dungeonGenerator.definition()->breathable())
              m_dungeonIdBreathable[currentDungeonId] = *breathable;

            currentDungeonId++;

            // floating dungeon worlds should force immediate generation (since there won't be terrain) to avoid
            // bottlenecking "generation" of empty generation levels during loading
            if (isFloatingDungeonWorld()) {
              for (auto region : generateResult->first)
                generateRegion(region);
            }

            break;
          }
        }
      }

      m_dungeonIdGravity[ZeroGDungeonId] = 0.0;
      m_dungeonIdGravity[ProtectedZeroGDungeonId] = 0.0;

      m_generatingDungeon = false;
    }

    if (m_adjustPlayerStart)
      m_playerStart = findPlayerStart(firstTime ? Maybe<Vec2F>() : m_playerStart);

    generateRegion(RectI::integral(RectF(m_playerStart, m_playerStart)).padded(m_serverConfig.getInt("playerStartInitialGenRadius")));

    m_weather.setup(m_worldTemplate->weathers(), m_worldTemplate->undergroundLevel(), m_geometry, [this](Vec2I const& pos) {
        auto const& tile = m_tileArray->tile(pos);
        return !isRealMaterial(tile.background);
      });
  } catch (std::exception const& e) {
    m_worldStorage->unloadAll(true);
    throw WorldServerException("Exception encountered initializing world", e);
  }
}

Maybe<unsigned> WorldServer::shouldRunThisStep(String const& timingConfiguration) {
  Vec2U timing = jsonToVec2U(m_fidelityConfig.get(timingConfiguration));
  if ((m_currentStep + timing[1]) % timing[0] == 0)
    return timing[0];
  return {};
}

TileModificationList WorldServer::doApplyTileModifications(TileModificationList const& modificationList, bool allowEntityOverlap, bool ignoreTileProtection, bool updateNeighbors) {
  auto materialDatabase = Root::singleton().materialDatabase();

  TileModificationList unapplied = modificationList;
  size_t unappliedSize = unapplied.size();
  auto it = makeSMutableIterator(unapplied);
  while (it.hasNext()) {
    Vec2I pos;
    TileModification modification;
    std::tie(pos, modification) = it.next();

    if (!ignoreTileProtection && isTileProtected(pos))
      continue;

    if (auto placeMaterial = modification.ptr<PlaceMaterial>()) {
      bool allowTileOverlap = placeMaterial->collisionOverride != TileCollisionOverride::None && collisionKindFromOverride(placeMaterial->collisionOverride) < CollisionKind::Dynamic;
      if (!WorldImpl::canPlaceMaterial(m_entityMap, pos, placeMaterial->layer, placeMaterial->material, allowEntityOverlap, allowTileOverlap, m_tileGetterFunction))
        continue;

      ServerTile* tile = m_tileArray->modifyTile(pos);
      if (!tile)
        continue;

      if (placeMaterial->layer == TileLayer::Background) {
        tile->background = placeMaterial->material;
        if (placeMaterial->materialHueShift)
          tile->backgroundHueShift = *placeMaterial->materialHueShift;
        else
          tile->backgroundHueShift = m_worldTemplate->biomeMaterialHueShift(tile->blockBiomeIndex, placeMaterial->material);

        tile->backgroundColorVariant = DefaultMaterialColorVariant;
        if (tile->background == EmptyMaterialId) {
          // Remove the background mod if removing the background.
          tile->backgroundMod = NoModId;
        } else if (tile->liquid.source) {
          tile->liquid.pressure = 1.0f;
          tile->liquid.source = false;
        }
      } else {
        tile->foreground = placeMaterial->material;
        if (placeMaterial->materialHueShift)
          tile->foregroundHueShift = *placeMaterial->materialHueShift;
        else
          tile->foregroundHueShift = m_worldTemplate->biomeMaterialHueShift(tile->blockBiomeIndex, placeMaterial->material);

        tile->foregroundColorVariant = DefaultMaterialColorVariant;
        if (placeMaterial->collisionOverride != TileCollisionOverride::None)
          tile->updateCollision(collisionKindFromOverride(placeMaterial->collisionOverride));
        else
          tile->updateCollision(materialDatabase->materialCollisionKind(tile->foreground));
        if (tile->foreground == EmptyMaterialId) {
          // Remove the foreground mod if removing the foreground.
          tile->foregroundMod = NoModId;
        }
        if (materialDatabase->blocksLiquidFlow(tile->foreground))
          tile->liquid = LiquidStore();
      }

      tile->dungeonId = ConstructionDungeonId;

      if (updateNeighbors) {
        checkEntityBreaks(RectF::withSize(Vec2F(pos), Vec2F(1, 1)));
        m_liquidEngine->visitLocation(pos);
        m_fallingBlocksAgent->visitLocation(pos);
      }

      if (placeMaterial->layer == TileLayer::Foreground)
        dirtyCollision(RectI::withSize(pos, {1, 1}));
      queueTileUpdates(pos);

    } else if (auto placeMod = modification.ptr<PlaceMod>()) {
      if (!WorldImpl::canPlaceMod(pos, placeMod->layer, placeMod->mod, m_tileGetterFunction))
        continue;

      ServerTile* tile = m_tileArray->modifyTile(pos);
      if (!tile)
        continue;

      if (placeMod->layer == TileLayer::Background) {
        tile->backgroundMod = placeMod->mod;
        if (placeMod->modHueShift)
          tile->backgroundModHueShift = *placeMod->modHueShift;
        else
          tile->backgroundModHueShift = m_worldTemplate->biomeModHueShift(tile->blockBiomeIndex, placeMod->mod);
      } else {
        tile->foregroundMod = placeMod->mod;
        if (placeMod->modHueShift)
          tile->foregroundModHueShift = *placeMod->modHueShift;
        else
          tile->foregroundModHueShift = m_worldTemplate->biomeModHueShift(tile->blockBiomeIndex, placeMod->mod);
      }

      m_liquidEngine->visitLocation(pos);
      queueTileUpdates(pos);

    } else if (auto placeMaterialColor = modification.ptr<PlaceMaterialColor>()) {
      if (!WorldImpl::canPlaceMaterialColorVariant(pos, placeMaterialColor->layer, placeMaterialColor->color, m_tileGetterFunction))
        continue;

      WorldTile* tile = m_tileArray->modifyTile(pos);
      if (!tile)
        continue;

      if (placeMaterialColor->layer == TileLayer::Background) {
        tile->backgroundHueShift = 0;
        if (!materialDatabase->isMultiColor(tile->background))
          continue;
        tile->backgroundColorVariant = placeMaterialColor->color;
      } else {
        tile->foregroundHueShift = 0;
        if (!materialDatabase->isMultiColor(tile->foreground))
          continue;
        tile->foregroundColorVariant = placeMaterialColor->color;
      }

      queueTileUpdates(pos);

    } else if (auto plpacket = modification.ptr<PlaceLiquid>()) {
      modifyLiquid(pos, plpacket->liquid, plpacket->liquidLevel, true);
      m_liquidEngine->visitLocation(pos);
      m_fallingBlocksAgent->visitLocation(pos);
    }

    it.remove();

    if (!it.hasNext()) {
      // If we are at the end, but have made progress by applying at least one
      // modification, then start over at the beginning and keep trying more
      // modifications until we can't make any more progress.
      if (unapplied.size() != unappliedSize) {
        unappliedSize = unapplied.size();
        it.toFront();
      }
    }
  }

  return unapplied;
}

void WorldServer::updateTileEntityTiles(TileEntityPtr const& entity, bool removing, bool checkBreaks) {
  // This method of updating tile entity collision only works if each tile
  // entity's collision spaces are a subset of their normal spaces, and thus no
  // two tile entities can have collision spaces that overlap.
  // NOTE: Some entities may violate this; it's an odd thing to rely on policy
  // for and maybe we shouldn't allow tile entity configurations to specify
  // material spaces outside of their spaces

  auto& spaces = m_tileEntitySpaces[entity->entityId()];

  List<MaterialSpace> newMaterialSpaces = removing ? List<MaterialSpace>() : entity->materialSpaces();
  List<Vec2I> newRoots = removing || entity->ephemeral() ? List<Vec2I>() : entity->roots();

  if (!removing && spaces.materials == newMaterialSpaces && spaces.roots == newRoots)
    return;

  auto materialDatabase = Root::singleton().materialDatabase();

  // remove all old roots
  for (auto const& rootPos : spaces.roots) {
    if (auto tile = m_tileArray->modifyTile(rootPos + entity->tilePosition()))
      tile->rootSource = {};
  }

  // remove all old material spaces
  for (auto const& materialSpace : spaces.materials) {
    Vec2I pos = materialSpace.space + entity->tilePosition();

    ServerTile* tile = m_tileArray->modifyTile(pos);
    if (tile) {
      tile->rootSource = {};
      bool updatedTile = false;
      bool updatedCollision = false;
      if (isBiomeMaterial(materialSpace.material) || tile->foreground == materialSpace.material) {
        // if the world is old, the materialSpace's collision may still be in the tile
        tile->foreground = EmptyMaterialId;
        tile->foregroundMod = NoModId;
        updatedTile = true;
        updatedCollision = tile->updateCollision(CollisionKind::None);
      }
      if (tile->updateObjectCollision(CollisionKind::None))
        updatedTile = updatedCollision = true;
      if (updatedCollision) {
        m_liquidEngine->visitLocation(pos);
        m_fallingBlocksAgent->visitLocation(pos);
        dirtyCollision(RectI::withSize(pos, { 1, 1 }));
      }
      if (updatedTile)
        queueTileUpdates(pos);
    }
  }

  if (removing) {
    m_tileEntitySpaces.remove(entity->entityId());
  } else {
    // add new material spaces and update the known material spaces entry
    List<MaterialSpace> passedSpaces;
    for (auto const& materialSpace : newMaterialSpaces) {
      Vec2I pos = materialSpace.space + entity->tilePosition();

      bool updatedTile = false;
      bool updatedCollision = false;
      ServerTile* tile = m_tileArray->modifyTile(pos);
      if (tile) {
        if (tile->foreground == EmptyMaterialId) {
          tile->foreground = materialSpace.material;
          tile->foregroundMod = NoModId;
          updatedTile = true;
        }
        if (isRealMaterial(materialSpace.material))
          tile->rootSource = entity->tilePosition();
        passedSpaces.emplaceAppend(materialSpace);
        updatedTile |= updatedCollision = tile->updateObjectCollision(materialDatabase->materialCollisionKind(materialSpace.material));
      }
      if (updatedCollision) {
        m_liquidEngine->visitLocation(pos);
        m_fallingBlocksAgent->visitLocation(pos);
        dirtyCollision(RectI::withSize(pos, { 1, 1 }));
      }
      if (updatedTile)
        queueTileUpdates(pos);
    }
    spaces.materials = std::move(passedSpaces);

    // add new roots and update known roots entry
    for (auto const& rootPos : newRoots) {
      if (auto tile = m_tileArray->modifyTile(rootPos + entity->tilePosition()))
        tile->rootSource = entity->tilePosition();
    }
    spaces.roots = std::move(newRoots);
  }

  // check whether we've broken any other nearby entities
  if (checkBreaks)
    checkEntityBreaks(entity->metaBoundBox().translated(entity->position()));
}

ConnectionId WorldServer::connection() const {
  return ServerConnectionId;
}

bool WorldServer::signalRegion(RectI const& region) {
  auto sectors = m_worldStorage->sectorsForRegion(region);
  if (m_generatingDungeon) {
    // When generating a dungeon, all sector activations should immediately
    // load whatever is available and make the sector active for writing, but
    // should trigger no generation (for world generation speed).
    for (auto const& sector : sectors)
      m_worldStorage->loadSector(sector);
  } else {
    for (auto const& sector : sectors)
      m_worldStorage->queueSectorActivation(sector);
  }
  for (auto const& sector : sectors) {
    if (!m_worldStorage->sectorActive(sector))
      return false;
  }
  return true;
}

void WorldServer::generateRegion(RectI const& region) {
  for (auto sector : m_worldStorage->sectorsForRegion(region))
    m_worldStorage->activateSector(sector);
}

bool WorldServer::regionActive(RectI const& region) {
  for (auto const& sector : m_worldStorage->sectorsForRegion(region)) {
    if (!m_worldStorage->sectorActive(sector))
      return false;
  }
  return true;
}

WorldServer::ScriptComponentPtr WorldServer::scriptContext(String const& contextName) {
  if (auto context = m_scriptContexts.ptr(contextName))
    return *context;
  else
    return nullptr;
}

RpcPromise<Vec2I> WorldServer::enqueuePlacement(List<BiomeItemDistribution> distributions, Maybe<DungeonId> id) {
  return m_worldStorage->enqueuePlacement(std::move(distributions), id);
}

ServerTile const& WorldServer::getServerTile(Vec2I const& position, bool withSignal) {
  if (withSignal)
    signalRegion(RectI::withSize(position, {1, 1}));

  return m_tileArray->tile(position);
}

ServerTile* WorldServer::modifyServerTile(Vec2I const& position, bool withSignal) {
  if (withSignal)
    signalRegion(RectI::withSize(position, {1, 1}));

  auto tile = m_tileArray->modifyTile(position);
  if (tile) {
    dirtyCollision(RectI::withSize(position, {1, 1}));
    m_liquidEngine->visitLocation(position);
    queueTileUpdates(position);
  }
  return tile;
}

EntityId WorldServer::loadUniqueEntity(String const& uniqueId) {
  return m_worldStorage->loadUniqueEntity(uniqueId);
}

WorldTemplatePtr WorldServer::worldTemplate() const {
  return m_worldTemplate;
}

SkyPtr WorldServer::sky() const {
  return m_sky;
}

void WorldServer::modifyLiquid(Vec2I const& pos, LiquidId liquid, float quantity, bool additive) {
  if (liquid == EmptyLiquidId)
    quantity = 0;

  if (ServerTile* tile = m_tileArray->modifyTile(pos)) {
    auto materialDatabase = Root::singleton().materialDatabase();
    if (tile->foreground == EmptyMaterialId || !isSolidColliding(materialDatabase->materialCollisionKind(tile->foreground))) {
      if (additive && liquid == tile->liquid.liquid)
        quantity += tile->liquid.level;

      setLiquid(pos, liquid, quantity, tile->liquid.pressure);
      m_liquidEngine->visitLocation(pos);
    }
  }
}

void WorldServer::setLiquid(Vec2I const& pos, LiquidId liquid, float level, float pressure) {
  if (ServerTile* tile = m_tileArray->modifyTile(pos)) {
    if (liquid == EmptyLiquidId)
      level = 0;

    if (auto netUpdate = tile->liquid.update(liquid, level, pressure)) {
      for (auto const& pair : m_clientInfo) {
        if (pair.second->activeSectors.contains(m_tileArray->sectorFor(pos)))
          pair.second->pendingLiquidUpdates.add(pos);
      }
    }
  }
}

List<ItemDescriptor> WorldServer::destroyBlock(TileLayer layer, Vec2I const& pos, bool genItems, bool destroyModFirst, bool updateNeighbors) {
  auto materialDatabase = Root::singleton().materialDatabase();

  auto* tile = m_tileArray->modifyTile(pos);
  if (!tile)
    return {};

  List<ItemDescriptor> drops;

  if (layer == TileLayer::Background) {
    if (isRealMod(tile->backgroundMod) && destroyModFirst
        && !materialDatabase->modBreaksWithTile(tile->backgroundMod)) {
      if (genItems) {
        if (auto drop = materialDatabase->modItemDrop(tile->backgroundMod))
          drops.append(drop);
      }
      tile->backgroundMod = NoModId;
    } else {
      if (genItems) {
        if (auto drop = materialDatabase->materialItemDrop(tile->background))
          drops.append(drop);
        if (auto drop = materialDatabase->modItemDrop(tile->backgroundMod))
          drops.append(drop);
      }
      tile->background = EmptyMaterialId;
      tile->backgroundColorVariant = DefaultMaterialColorVariant;
      tile->backgroundHueShift = 0;
      tile->backgroundMod = NoModId;
    }

    tile->backgroundDamage.reset();

  } else {
    if (isRealMod(tile->foregroundMod) && destroyModFirst
        && !materialDatabase->modBreaksWithTile(tile->foregroundMod)) {
      if (genItems) {
        if (auto drop = materialDatabase->modItemDrop(tile->foregroundMod))
          drops.append(drop);
      }
      tile->foregroundMod = NoModId;
    } else {
      if (genItems) {
        if (auto drop = materialDatabase->materialItemDrop(tile->foreground))
          drops.append(drop);
        if (auto drop = materialDatabase->modItemDrop(tile->foregroundMod))
          drops.append(drop);
      }
      tile->foreground = EmptyMaterialId;
      tile->foregroundColorVariant = DefaultMaterialColorVariant;
      tile->foregroundHueShift = 0;
      tile->foregroundMod = NoModId;
      tile->updateCollision(CollisionKind::None);
      dirtyCollision(RectI::withSize(pos, {1, 1}));
    }

    tile->foregroundDamage.reset();
  }

  if (tile->background == EmptyMaterialId && tile->foreground == EmptyMaterialId) {
    auto blockInfo = m_worldTemplate->blockInfo(pos[0], pos[1]);
    if (blockInfo.oceanLiquid != EmptyLiquidId && !blockInfo.encloseLiquids && pos[1] < blockInfo.oceanLiquidLevel)
      tile->liquid = LiquidStore::endless(blockInfo.oceanLiquid, blockInfo.oceanLiquidLevel - pos[1]);
  }

  tile->dungeonId = DestroyedBlockDungeonId;

  if (updateNeighbors) {
    checkEntityBreaks(RectF::withSize(Vec2F(pos), Vec2F(1, 1)));
    m_liquidEngine->visitLocation(pos);
    m_fallingBlocksAgent->visitLocation(pos);
  }
  queueTileUpdates(pos);
  queueTileDamageUpdates(pos, layer);

  return drops;
}

void WorldServer::queueUpdatePackets(ConnectionId clientId, bool sendRemoteUpdates) {
  auto const& clientInfo = m_clientInfo.get(clientId);
  clientInfo->outgoingPackets.append(make_shared<StepUpdatePacket>(m_currentTime));

  if (shouldRunThisStep("environmentUpdate")) {
    ByteArray skyDelta;
    tie(skyDelta, clientInfo->skyNetVersion) = m_sky->writeUpdate(clientInfo->skyNetVersion, clientInfo->clientState.netCompatibilityRules());

    ByteArray weatherDelta;
    tie(weatherDelta, clientInfo->weatherNetVersion) = m_weather.writeUpdate(clientInfo->weatherNetVersion, clientInfo->clientState.netCompatibilityRules());

    if (!skyDelta.empty() || !weatherDelta.empty())
      clientInfo->outgoingPackets.append(make_shared<EnvironmentUpdatePacket>(std::move(skyDelta), std::move(weatherDelta)));
  }

  for (auto sector : clientInfo->pendingSectors.values()) {
    if (!m_worldStorage->sectorActive(sector))
      continue;

    auto tileArrayUpdate = make_shared<TileArrayUpdatePacket>();
    auto sectorTiles = m_tileArray->sectorRegion(sector);
    tileArrayUpdate->min = sectorTiles.min();
    tileArrayUpdate->array.resize(Vec2S(sectorTiles.width(), sectorTiles.height()));
    for (int x = sectorTiles.xMin(); x < sectorTiles.xMax(); ++x) {
      for (int y = sectorTiles.yMin(); y < sectorTiles.yMax(); ++y)
        writeNetTile({x, y}, tileArrayUpdate->array(x - sectorTiles.xMin(), y - sectorTiles.yMin()));
    }

    clientInfo->outgoingPackets.append(tileArrayUpdate);
    clientInfo->pendingSectors.remove(sector);
  }

  for (auto pos : clientInfo->pendingTileUpdates) {
    auto tileUpdate = make_shared<TileUpdatePacket>();
    tileUpdate->position = pos;
    writeNetTile(pos, tileUpdate->tile);

    clientInfo->outgoingPackets.append(tileUpdate);
  }
  clientInfo->pendingTileUpdates.clear();

  for (auto pair : clientInfo->pendingTileDamageUpdates) {
    auto tile = m_tileArray->tile(pair.first);
    if (pair.second == TileLayer::Foreground)
      clientInfo->outgoingPackets.append(
          make_shared<TileDamageUpdatePacket>(pair.first, TileLayer::Foreground, tile.foregroundDamage));
    else
      clientInfo->outgoingPackets.append(
          make_shared<TileDamageUpdatePacket>(pair.first, TileLayer::Background, tile.backgroundDamage));
  }
  clientInfo->pendingTileDamageUpdates.clear();

  for (auto pos : clientInfo->pendingLiquidUpdates) {
    auto tile = m_tileArray->tile(pos);
    clientInfo->outgoingPackets.append(make_shared<TileLiquidUpdatePacket>(pos, tile.liquid.netUpdate()));
  }
  clientInfo->pendingLiquidUpdates.clear();

  HashSet<EntityPtr> monitoredEntities;
  for (auto const& monitoredRegion : clientInfo->monitoringRegions(m_entityMap))
    monitoredEntities.addAll(m_entityMap->entityQuery(RectF(monitoredRegion)));

  auto entityFactory = Root::singleton().entityFactory();
  auto outOfMonitoredRegionsEntities = HashSet<EntityId>::from(clientInfo->clientSlavesNetVersion.keys());
  for (auto const& monitoredEntity : monitoredEntities)
    outOfMonitoredRegionsEntities.remove(monitoredEntity->entityId());
  for (auto entityId : outOfMonitoredRegionsEntities) {
    clientInfo->outgoingPackets.append(make_shared<EntityDestroyPacket>(entityId, ByteArray(), false));
    clientInfo->clientSlavesNetVersion.remove(entityId);
  }

  HashMap<ConnectionId, shared_ptr<EntityUpdateSetPacket>> updateSetPackets;
  if (sendRemoteUpdates || clientInfo->local)
    updateSetPackets.add(ServerConnectionId, make_shared<EntityUpdateSetPacket>(ServerConnectionId));
  for (auto const& p : m_clientInfo) {
    if (p.first != clientId && p.second->pendingForward)
      updateSetPackets.add(p.first, make_shared<EntityUpdateSetPacket>(p.first));
  }

  for (auto const& monitoredEntity : monitoredEntities) {
    EntityId entityId = monitoredEntity->entityId();
    ConnectionId connectionId = connectionForEntity(entityId);
    if (connectionId != clientId) {
      auto netRules = clientInfo->clientState.netCompatibilityRules();
      if (auto version = clientInfo->clientSlavesNetVersion.ptr(entityId)) {
        if (auto updateSetPacket = updateSetPackets.value(connectionId)) {
          auto pair = make_pair(entityId, *version);
          auto& cache = m_netStateCache[netRules];
          auto i = cache.find(pair);
          if (i == cache.end())
            i = cache.insert(pair, monitoredEntity->writeNetState(*version, netRules)).first;
          const auto& netState = i->second;
          if (!netState.first.empty())
            updateSetPacket->deltas[entityId] = netState.first;
          *version = netState.second;
        }
      } else if (!monitoredEntity->masterOnly()) {
        // Client was unaware of this entity until now
        auto firstUpdate = monitoredEntity->writeNetState(0, netRules);
        clientInfo->clientSlavesNetVersion.add(entityId, firstUpdate.second);
        clientInfo->outgoingPackets.append(make_shared<EntityCreatePacket>(monitoredEntity->entityType(),
              entityFactory->netStoreEntity(monitoredEntity, netRules), std::move(firstUpdate.first), entityId));
      }
    }
  }

  for (auto& p : updateSetPackets)
    clientInfo->outgoingPackets.append(std::move(p.second));
}

void WorldServer::updateDamage(float dt) {
  m_damageManager->update(dt);

  // Do nothing with damage notifications at the moment.
  m_damageManager->pullPendingNotifications();

  for (auto const& remoteHitRequest : m_damageManager->pullRemoteHitRequests())
    m_clientInfo.get(remoteHitRequest.destinationConnection())
        ->outgoingPackets.append(make_shared<HitRequestPacket>(remoteHitRequest));

  for (auto const& remoteDamageRequest : m_damageManager->pullRemoteDamageRequests())
    m_clientInfo.get(remoteDamageRequest.destinationConnection())
        ->outgoingPackets.append(make_shared<DamageRequestPacket>(remoteDamageRequest));

  for (auto const& remoteDamageNotification : m_damageManager->pullRemoteDamageNotifications()) {
    for (auto const& pair : m_clientInfo) {
      if (pair.second->needsDamageNotification(remoteDamageNotification))
        pair.second->outgoingPackets.append(make_shared<DamageNotificationPacket>(remoteDamageNotification));
    }
  }
}

void WorldServer::sync() {
  writeMetadata();
  m_worldStorage->sync();
}

void WorldServer::unloadAll(bool force) {
  m_worldStorage->unloadAll(force);
}

WorldChunks WorldServer::readChunks() {
  writeMetadata();
  return m_worldStorage->readChunks();
}

void WorldServer::updateDamagedBlocks(float dt) {
  auto materialDatabase = Root::singleton().materialDatabase();

  for (auto pos : m_damagedBlocks.values()) {
    auto tile = m_tileArray->modifyTile(pos);
    if (!tile) {
      m_damagedBlocks.remove(pos);
      continue;
    }

    Vec2F dropPosition = centerOfTile(pos);
    if (tile->foregroundDamage.dead()) {
      bool harvested = tile->foregroundDamage.harvested();
      for (auto drop : destroyBlock(TileLayer::Foreground, pos, harvested, !tileDamageIsPenetrating(tile->foregroundDamage.damageType())))
        addEntity(ItemDrop::createRandomizedDrop(drop, dropPosition));

    } else if (tile->foregroundDamage.damaged()) {
      if (isRealMaterial(tile->foreground)) {
        if (isRealMod(tile->foregroundMod)) {
          if (tileDamageIsPenetrating(tile->foregroundDamage.damageType()))
            tile->foregroundDamage.recover(materialDatabase->materialDamageParameters(tile->foreground), dt);
          else if (materialDatabase->modBreaksWithTile(tile->foregroundMod))
            tile->foregroundDamage.recover(materialDatabase->modDamageParameters(tile->foregroundMod).sum(materialDatabase->materialDamageParameters(tile->foreground)), dt);
          else
            tile->foregroundDamage.recover(materialDatabase->modDamageParameters(tile->foregroundMod), dt);
        } else
          tile->foregroundDamage.recover(materialDatabase->materialDamageParameters(tile->foreground), dt);
      } else
        tile->foregroundDamage.reset();

      queueTileDamageUpdates(pos, TileLayer::Foreground);
    }

    if (tile->backgroundDamage.dead()) {
      bool harvested = tile->backgroundDamage.harvested();
      for (auto drop : destroyBlock(TileLayer::Background, pos, harvested, !tileDamageIsPenetrating(tile->backgroundDamage.damageType())))
        addEntity(ItemDrop::createRandomizedDrop(drop, dropPosition));

    } else if (tile->backgroundDamage.damaged()) {
      if (isRealMaterial(tile->background)) {
        if (isRealMod(tile->backgroundMod)) {
          if (tileDamageIsPenetrating(tile->backgroundDamage.damageType()))
            tile->backgroundDamage.recover(materialDatabase->materialDamageParameters(tile->background), dt);
          else if (materialDatabase->modBreaksWithTile(tile->backgroundMod))
            tile->backgroundDamage.recover(materialDatabase->modDamageParameters(tile->backgroundMod).sum(materialDatabase->materialDamageParameters(tile->background)), dt);
          else
            tile->backgroundDamage.recover(materialDatabase->modDamageParameters(tile->backgroundMod), dt);
        } else {
          tile->backgroundDamage.recover(materialDatabase->materialDamageParameters(tile->background), dt);
        }
      } else {
        tile->backgroundDamage.reset();
      }

      queueTileDamageUpdates(pos, TileLayer::Background);
    }

    if (tile->backgroundDamage.healthy() && tile->foregroundDamage.healthy())
      m_damagedBlocks.remove(pos);
  }
}

void WorldServer::checkEntityBreaks(RectF const& rect) {
  for (auto tileEntity : m_entityMap->query<TileEntity>(rect))
    tileEntity->checkBroken();
}

void WorldServer::queueTileUpdates(Vec2I const& pos) {
  for (auto const& pair : m_clientInfo) {
    if (pair.second->activeSectors.contains(m_tileArray->sectorFor(pos)))
      pair.second->pendingTileUpdates.add(pos);
  }
}

void WorldServer::queueTileDamageUpdates(Vec2I const& pos, TileLayer layer) {
  for (auto const& pair : m_clientInfo) {
    if (pair.second->activeSectors.contains(m_tileArray->sectorFor(pos)))
      pair.second->pendingTileDamageUpdates.add({pos, layer});
  }
}

void WorldServer::writeNetTile(Vec2I const& pos, NetTile& netTile) const {
  auto const& tile = m_tileArray->tile(pos);
  netTile.foreground = tile.foreground;
  netTile.foregroundHueShift = tile.foregroundHueShift;
  netTile.foregroundColorVariant = tile.foregroundColorVariant;
  netTile.foregroundMod = tile.foregroundMod;
  netTile.foregroundModHueShift = tile.foregroundModHueShift;
  netTile.background = tile.background;
  netTile.backgroundHueShift = tile.backgroundHueShift;
  netTile.backgroundColorVariant = tile.backgroundColorVariant;
  netTile.backgroundMod = tile.backgroundMod;
  netTile.backgroundModHueShift = tile.backgroundModHueShift;
  netTile.liquid = tile.liquid.netUpdate();
  netTile.collision = tile.getCollision();
  netTile.blockBiomeIndex = tile.blockBiomeIndex;
  netTile.environmentBiomeIndex = tile.environmentBiomeIndex;
  netTile.dungeonId = tile.dungeonId;
}

void WorldServer::dirtyCollision(RectI const& region) {
  auto dirtyRegion = region.padded(CollisionGenerator::BlockInfluenceRadius);
  for (int x = dirtyRegion.xMin(); x < dirtyRegion.xMax(); ++x) {
    for (int y = dirtyRegion.yMin(); y < dirtyRegion.yMax(); ++y) {
      if (auto tile = m_tileArray->modifyTile({x, y}))
        tile->collisionCacheDirty = true;
    }
  }
}

void WorldServer::freshenCollision(RectI const& region) {
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
        tile->collisionCache.append(std::move(collisionBlock));
    }
  }
}

void WorldServer::removeEntity(EntityId entityId, bool andDie) {
  auto entity = m_entityMap->entity(entityId);
  if (!entity)
    return;

  if (auto tileEntity = as<TileEntity>(entity))
    updateTileEntityTiles(tileEntity, true);

  if (andDie)
    entity->destroy(nullptr);

  for (auto const& pair : m_clientInfo) {
    auto& clientInfo = pair.second;
    if (auto version = clientInfo->clientSlavesNetVersion.maybeTake(entity->entityId())) {
      auto netRules = clientInfo->clientState.netCompatibilityRules();
      ByteArray finalDelta = entity->writeNetState(*version, netRules).first;
      clientInfo->outgoingPackets.append(make_shared<EntityDestroyPacket>(entity->entityId(), std::move(finalDelta), andDie));
    }
  }

  m_entityMap->removeEntity(entityId);
  entity->uninit();
}

float WorldServer::windLevel(Vec2F const& pos) const {
  return WorldImpl::windLevel(m_tileArray, pos, m_weather.wind());
}

float WorldServer::lightLevel(Vec2F const& pos) const {
  return WorldImpl::lightLevel(m_tileArray, m_entityMap, m_geometry, m_worldTemplate, m_sky, m_lightIntensityCalculator, pos);
}

void WorldServer::setDungeonBreathable(DungeonId dungeonId, Maybe<bool> breathable) {
  Maybe<bool> current = m_dungeonIdBreathable.maybe(dungeonId);
  if (breathable != current) {
    if (breathable)
      m_dungeonIdBreathable[dungeonId] = *breathable;
    else
      m_dungeonIdBreathable.remove(dungeonId);

    for (auto const& p : m_clientInfo)
      p.second->outgoingPackets.append(make_shared<SetDungeonBreathablePacket>(dungeonId, breathable));
  }
}



bool WorldServer::breathable(Vec2F const& pos) const {
  return WorldImpl::breathable(this, m_tileArray, m_dungeonIdBreathable, m_worldTemplate, pos);
}

float WorldServer::threatLevel() const {
  return m_worldTemplate->threatLevel();
}

StringList WorldServer::environmentStatusEffects(Vec2F const& pos) const {
  return m_worldTemplate->environmentStatusEffects(floor(pos[0]), floor(pos[1]));
}

StringList WorldServer::weatherStatusEffects(Vec2F const& pos) const {
  if (!m_weather.statusEffects().empty()) {
     if (exposedToWeather(pos))
      return m_weather.statusEffects();
  }

  return {};
}

bool WorldServer::exposedToWeather(Vec2F const& pos) const {
  if (!isUnderground(pos) && liquidLevel(Vec2I::floor(pos)).liquid == EmptyLiquidId) {
    auto assets = Root::singleton().assets();
    float weatherRayCheckDistance = assets->json("/weather.config:weatherRayCheckDistance").toFloat();
    float weatherRayCheckWindInfluence = assets->json("/weather.config:weatherRayCheckWindInfluence").toFloat();

    auto offset = Vec2F(-m_weather.wind() * weatherRayCheckWindInfluence, weatherRayCheckDistance).normalized() * weatherRayCheckDistance;

    return !lineCollision({pos, pos + offset});
  }

  return false;
}

bool WorldServer::isUnderground(Vec2F const& pos) const {
  return m_worldTemplate->undergroundLevel() >= pos[1];
}

bool WorldServer::disableDeathDrops() const {
  if (m_worldTemplate->worldParameters())
    return m_worldTemplate->worldParameters()->disableDeathDrops;
  return false;
}

List<PhysicsForceRegion> WorldServer::forceRegions() const {
  return m_forceRegions;
}

Json WorldServer::getProperty(String const& propertyName, Json const& def) const {
  return m_worldProperties.value(propertyName, def);
}

void WorldServer::setProperty(String const& propertyName, Json const& property) {
  // Kae: Properties set to null (nil from Lua) should be erased instead of lingering around
  auto entry = m_worldProperties.find(propertyName);
  bool missing = entry == m_worldProperties.end();
  if (missing ? !property.isNull() : property != entry->second) {
    if (missing) // property can't be null if we're doing this when missing is true
      m_worldProperties.emplace(propertyName, property);
    else if (property.isNull())
      m_worldProperties.erase(entry);
    else
      entry->second = property;
    for (auto const& pair : m_clientInfo)
      pair.second->outgoingPackets.append(make_shared<UpdateWorldPropertiesPacket>(JsonObject{ {propertyName, property} }));
  }
  auto listener = m_worldPropertyListeners.find(propertyName);
  if (listener != m_worldPropertyListeners.end())
    listener->second(property);
}

void WorldServer::timer(float delay, WorldAction worldAction) {
  m_timers.append({delay, worldAction});
}

void WorldServer::startFlyingSky(bool enterHyperspace, bool startInWarp, Json settings) {
  m_sky->startFlying(enterHyperspace, startInWarp, settings);
}

void WorldServer::stopFlyingSkyAt(SkyParameters const& destination) {
  m_sky->stopFlyingAt(destination);
  m_sky->setType(SkyType::Orbital);
}

void WorldServer::setOrbitalSky(SkyParameters const& destination) {
  m_sky->jumpTo(destination);
  m_sky->setType(SkyType::Orbital);
}

double WorldServer::epochTime() const {
  return m_sky->epochTime();
}

uint32_t WorldServer::day() const {
  return m_sky->day();
}

float WorldServer::dayLength() const {
  return m_sky->dayLength();
}

float WorldServer::timeOfDay() const {
  return m_sky->timeOfDay();
}

LuaRootPtr WorldServer::luaRoot() {
  return m_luaRoot;
}

RpcPromise<Vec2F> WorldServer::findUniqueEntity(String const& uniqueId) {
  if (auto pos = m_worldStorage->findUniqueEntity(uniqueId))
    return RpcPromise<Vec2F>::createFulfilled(*pos);
  else
    return RpcPromise<Vec2F>::createFailed("Unknown entity");
}

RpcPromise<Json> WorldServer::sendEntityMessage(Variant<EntityId, String> const& entityId, String const& message, JsonArray const& args) {
  EntityPtr entity;
  if (entityId.is<EntityId>())
    entity = m_entityMap->entity(entityId.get<EntityId>());
  else
    entity = m_entityMap->entity(loadUniqueEntity(entityId.get<String>()));

  if (!entity) {
    return RpcPromise<Json>::createFailed("Unknown entity");
  } else if (entity->isMaster()) {
    if (auto resp = entity->receiveMessage(ServerConnectionId, message, args))
      return RpcPromise<Json>::createFulfilled(resp.take());
    else
      return RpcPromise<Json>::createFailed("Message not handled by entity");
  } else {
    auto pair = RpcPromise<Json>::createPair();
    auto clientInfo = m_clientInfo.get(connectionForEntity(entity->entityId()));
    Uuid uuid;
    m_entityMessageResponses[uuid] = {clientInfo->clientId, pair.second};
    clientInfo->outgoingPackets.append(make_shared<EntityMessagePacket>(entity->entityId(), message, args, uuid));
    return pair.first;
  }
}

void WorldServer::setPlayerStart(Vec2F const& startPosition, bool respawnInWorld) {
  m_playerStart = startPosition;
  m_respawnInWorld = respawnInWorld;
  m_adjustPlayerStart = false;
  for (auto pair : m_clientInfo)
    pair.second->outgoingPackets.append(make_shared<SetPlayerStartPacket>(m_playerStart, m_respawnInWorld));
}

Vec2F WorldServer::findPlayerStart(Maybe<Vec2F> firstTry) {
  Vec2F spawnRectSize = jsonToVec2F(m_serverConfig.get("playerStartRegionSize"));
  auto maximumVerticalSearch = m_serverConfig.getInt("playerStartRegionMaximumVerticalSearch");
  auto maximumTries = m_serverConfig.getInt("playerStartRegionMaximumTries");

  static const Set<DungeonId> allowedSpawnDungeonIds = {NoDungeonId, SpawnDungeonId, ConstructionDungeonId, DestroyedBlockDungeonId};

  Vec2F pos;
  if (firstTry)
    pos = *firstTry;
  else
    pos = Vec2F(m_worldTemplate->findSensiblePlayerStart().value(Vec2I(0, m_worldTemplate->surfaceLevel())));

  CollisionSet collideWithAnything{CollisionKind::Null, CollisionKind::Block, CollisionKind::Dynamic, CollisionKind::Platform, CollisionKind::Slippery};
  for (int t = 0; t < maximumTries; ++t) {
    bool foundGround = false;
    // First go downward until we collide with terrain
    for (int i = 0; i < maximumVerticalSearch; ++i) {
      RectF spawnRect = RectF(pos[0] - spawnRectSize[0] / 2, pos[1], pos[0] + spawnRectSize[0] / 2, pos[1] + spawnRectSize[1]);
      generateRegion(RectI::integral(spawnRect));
      if (rectTileCollision(RectI::integral(spawnRect), collideWithAnything)) {
        foundGround = true;
        break;
      }
      --pos[1];
    }

    if (foundGround) {
      // Then go up until our spawn region is no longer in the terrain, but bail
      // out and try again if we can't signal the region or we are stuck in a
      // dungeon.
      for (int i = 0; i < maximumVerticalSearch; ++i) {
        if (m_tileArray->tile(Vec2I::floor(pos)).liquid.liquid != EmptyLiquidId)
          break;

        RectF spawnRect = RectF(pos[0] - spawnRectSize[0] / 2, pos[1], pos[0] + spawnRectSize[0] / 2, pos[1] + spawnRectSize[1]);

        generateRegion(RectI::integral(spawnRect));

        auto tileDungeonId = getServerTile(Vec2I::floor(pos)).dungeonId;

        if (!allowedSpawnDungeonIds.contains(tileDungeonId))
          break;

        if (!rectTileCollision(RectI::integral(spawnRect), collideWithAnything) && spawnRect.yMax() < m_geometry.height())
          return pos;

        ++pos[1];
      }
    }

    pos = Vec2F(m_worldTemplate->findSensiblePlayerStart().value(Vec2I(0, m_worldTemplate->surfaceLevel())));
  }

  return pos;
}

Vec2F WorldServer::findPlayerSpaceStart(float targetX) {
  Vec2F testRectSize = jsonToVec2F(m_serverConfig.get("playerSpaceStartRegionSize"));
  auto distanceIncrement = m_serverConfig.getFloat("playerSpaceStartDistanceIncrement");
  auto maximumTries = m_serverConfig.getInt("playerSpaceStartMaximumTries");

  Vec2F basePos = Vec2F(targetX, m_geometry.height() * 0.5);

  CollisionSet collideWithAnything{CollisionKind::Null, CollisionKind::Block, CollisionKind::Dynamic, CollisionKind::Platform, CollisionKind::Slippery};
  for (int t = 0; t < maximumTries; ++t) {
    Vec2F testPos = m_geometry.limit(basePos + Vec2F::withAngle(Random::randf() * 2 * Constants::pi, t * distanceIncrement));
    RectF testRect = RectF::withCenter(testPos, testRectSize);
    generateRegion(RectI::integral(testRect));
    if (!rectTileCollision(RectI::integral(testRect), collideWithAnything))
      return testPos;
  }

  return basePos;
}

void WorldServer::readMetadata() {
  auto dungeonDefinitions = Root::singleton().dungeonDefinitions();
  auto versioningDatabase = Root::singleton().versioningDatabase();

  auto metadata = versioningDatabase->loadVersionedJson(m_worldStorage->worldMetadata(), "WorldMetadata");

  m_playerStart = jsonToVec2F(metadata.get("playerStart"));
  m_respawnInWorld = metadata.getBool("respawnInWorld");
  m_adjustPlayerStart = metadata.getBool("adjustPlayerStart");
  m_worldTemplate = make_shared<WorldTemplate>(metadata.get("worldTemplate"));
  m_centralStructure = WorldStructure(metadata.get("centralStructure"));
  m_protectedDungeonIds = jsonToSet<StableHashSet<DungeonId>>(metadata.get("protectedDungeonIds"), mem_fn(&Json::toUInt));
  m_worldProperties = metadata.getObject("worldProperties");
  m_spawner.setActive(metadata.getBool("spawningEnabled"));

  m_dungeonIdGravity = transform<HashMap<DungeonId, float>>(metadata.getArray("dungeonIdGravity"), [](Json const& p) {
      return make_pair(p.getInt(0), p.getFloat(1));
    });

  m_dungeonIdBreathable = transform<HashMap<DungeonId, bool>>(metadata.getArray("dungeonIdBreathable"), [](Json const& p) {
      return make_pair(p.getInt(0), p.getBool(1));
    });
}

void WorldServer::writeMetadata() {
  auto versioningDatabase = Root::singleton().versioningDatabase();

  Json metadata = JsonObject{
    {"playerStart", jsonFromVec2F(m_playerStart)},
    {"respawnInWorld", m_respawnInWorld},
    {"adjustPlayerStart", m_adjustPlayerStart},
    {"worldTemplate", m_worldTemplate->store()},
    {"centralStructure", m_centralStructure.store()},
    {"protectedDungeonIds", jsonFromSet(m_protectedDungeonIds)},
    {"worldProperties", m_worldProperties},
    {"spawningEnabled", m_spawner.active()},
    {"dungeonIdGravity", m_dungeonIdGravity.pairs().transformed([](auto const& p) -> Json {
        return JsonArray{p.first, p.second};
      })},
    {"dungeonIdBreathable", m_dungeonIdBreathable.pairs().transformed([](auto const& p) -> Json {
        return JsonArray{p.first, p.second};
      })}
  };

  m_worldStorage->setWorldMetadata(versioningDatabase->makeCurrentVersionedJson("WorldMetadata", metadata));
}

bool WorldServer::isVisibleToPlayer(RectF const& region) const {
  for (auto const& p : m_clientInfo) {
    for (auto playerRegion : p.second->monitoringRegions(m_entityMap)) {
      if (m_geometry.rectIntersectsRect(RectF(playerRegion), region))
        return true;
    }
  }
  return false;
}

WorldServer::ClientInfo::ClientInfo(ConnectionId clientId, InterpolationTracker const trackerInit)
  : clientId(clientId), skyNetVersion(0), weatherNetVersion(0), pendingForward(false), started(false), local(false), admin(false), interpolationTracker(trackerInit) {}

List<RectI> WorldServer::ClientInfo::monitoringRegions(EntityMapPtr const& entityMap) const {
  return clientState.monitoringRegions([entityMap](EntityId entityId) -> Maybe<RectI> {
    if (auto entity = entityMap->entity(entityId))
      return RectI::integral(entity->metaBoundBox().translated(entity->position()));
    return {};
  });
}

bool WorldServer::ClientInfo::needsDamageNotification(RemoteDamageNotification const& rdn) const {
  if (clientId == connectionForEntity(rdn.sourceEntityId) || clientId == connectionForEntity(rdn.damageNotification.targetEntityId))
    return true;

  if (clientSlavesNetVersion.contains(rdn.damageNotification.targetEntityId))
    return true;

  if (clientState.window().contains(Vec2I::floor(rdn.damageNotification.position)))
    return true;

  return false;
}

InteractiveEntityPtr WorldServer::getInteractiveInRange(Vec2F const& targetPosition, Vec2F const& sourcePosition, float maxRange) const {
  return WorldImpl::getInteractiveInRange(m_geometry, m_entityMap, targetPosition, sourcePosition, maxRange);
}

bool WorldServer::canReachEntity(Vec2F const& position, float radius, EntityId targetEntity, bool preferInteractive) const {
  return WorldImpl::canReachEntity(m_geometry, m_tileArray, m_entityMap, position, radius, targetEntity, preferInteractive);
}

RpcPromise<InteractAction> WorldServer::interact(InteractRequest const& request) {
  if (auto entity = as<InteractiveEntity>(m_entityMap->entity(request.targetId)))
    return RpcPromise<InteractAction>::createFulfilled(entity->interact(request));
  else
    return RpcPromise<InteractAction>::createFulfilled(InteractAction());
}

void WorldServer::setupForceRegions() {
  m_forceRegions.clear();

  if (!worldTemplate() || !worldTemplate()->worldParameters())
    return;

  auto forceRegionType = worldTemplate()->worldParameters()->worldEdgeForceRegions;

  if (forceRegionType == WorldEdgeForceRegionType::None)
    return;

  bool addTopRegion = forceRegionType == WorldEdgeForceRegionType::Top || forceRegionType == WorldEdgeForceRegionType::TopAndBottom;
  bool addBottomRegion = forceRegionType == WorldEdgeForceRegionType::Bottom || forceRegionType == WorldEdgeForceRegionType::TopAndBottom;

  auto regionHeight = m_serverConfig.getFloat("worldEdgeForceRegionHeight");
  auto regionForce = m_serverConfig.getFloat("worldEdgeForceRegionForce");
  auto regionVelocity = m_serverConfig.getFloat("worldEdgeForceRegionVelocity");
  auto regionCategoryFilter = PhysicsCategoryFilter::whitelist({"player", "monster", "npc", "vehicle", "itemdrop"});
  auto worldSize = Vec2F(worldTemplate()->size());

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

void WorldServer::setTemplate(WorldTemplatePtr newTemplate) {
  m_worldTemplate = std::move(newTemplate);
  for (auto& client : clientIds()) {
    auto& info = m_clientInfo.get(client);
    bool local = info->local;
    bool isAdmin = info->admin;
    auto netRules = info->clientState.netCompatibilityRules();
    SpawnTarget spawnTarget;
    if (auto player = clientPlayer(client))
      spawnTarget = SpawnTargetPosition(player->position() + player->feetOffset());
    removeClient(client);
    addClient(client, spawnTarget, local, isAdmin, netRules);
  }
}

void WorldServer::wire(Vec2I const& outputPosition, size_t outputIndex, Vec2I const& inputPosition, size_t inputIndex) {
  WireConnection output = {outputPosition, outputIndex};
  WireConnection input = {inputPosition, inputIndex};
  for (auto source : atTile<WireEntity>(input.entityLocation)) {
    for (auto target : atTile<WireEntity>(output.entityLocation)) {
      source->addNodeConnection(WireNode{WireDirection::Input, input.nodeIndex}, output);
      target->addNodeConnection(WireNode{WireDirection::Output, output.nodeIndex}, input);
    }
  }
}

}
