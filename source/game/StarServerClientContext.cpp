#include "StarServerClientContext.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarWorldServerThread.hpp"
#include "StarScriptedEntity.hpp"
#include "StarContainerEntity.hpp"
#include "StarItemDatabase.hpp"
#include "StarRoot.hpp"
#include "StarUniverseSettings.hpp"

namespace Star {

ServerClientContext::ServerClientContext(ConnectionId clientId, Maybe<HostAddress> remoteAddress, NetCompatibilityRules netRules, Uuid playerUuid,
    String playerName, String shipSpecies, bool canBecomeAdmin, WorldChunks initialShipChunks)
  : m_clientId(clientId),
    m_remoteAddress(remoteAddress),
    m_netRules(netRules),
    m_playerUuid(playerUuid),
    m_playerName(playerName),
    m_shipSpecies(shipSpecies),
    m_canBecomeAdmin(canBecomeAdmin),
    m_shipChunks(std::move(initialShipChunks)) {
  m_rpc.registerHandler("ship.applyShipUpgrades", [this](Json const& args) -> Json {
      RecursiveMutexLocker locker(m_mutex);
      setShipUpgrades(shipUpgrades().apply(args));
      return true;
    });

  m_rpc.registerHandler("ship.setShipSpecies", [this](Json const& species) -> Json {
      RecursiveMutexLocker locker(m_mutex);
      setShipSpecies(species.toString());
      return true;
    });

  m_rpc.registerHandler("world.containerPutItems", [this](Json const& args) -> Json {
      List<ItemDescriptor> overflow = args.getArray("items").transformed(construct<ItemDescriptor>());
      RecursiveMutexLocker locker(m_mutex);
      if (m_worldThread) {
        m_worldThread->executeAction([args, &overflow](WorldServerThread*, WorldServer* server) {
          EntityId entityId = args.getInt("entityId");
          Json items = args.get("items");
          auto itemDatabase = Root::singleton().itemDatabase();
          if (auto containerEntity = as<ContainerEntity>(server->entity(entityId))) {
            overflow.clear();
            for (auto const& itemDescriptor : items.iterateArray()) {
              if (auto left = containerEntity->addItems(itemDatabase->item(ItemDescriptor(itemDescriptor))).result().value())
                overflow.append(left->descriptor());
            }
          }
        });
      }
      return overflow.transformed(mem_fn(&ItemDescriptor::toJson));
    });

  m_rpc.registerHandler("universe.setFlag", [this](Json const& args) -> Json {
      auto flagName = args.toString();
      RecursiveMutexLocker locker(m_mutex);
      if (m_worldThread) {
        m_worldThread->executeAction([flagName](WorldServerThread*, WorldServer* server) {
          server->universeSettings()->setFlag(flagName);
        });
      }
      return Json();
    });

  m_netGroup.addNetElement(&m_orbitWarpActionNetState);
  m_netGroup.addNetElement(&m_playerWorldIdNetState);
  m_netGroup.addNetElement(&m_isAdminNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_shipUpgrades);
  m_netGroup.addNetElement(&m_shipCoordinate);

  m_creationTime = Time::monotonicMilliseconds();
}

ConnectionId ServerClientContext::clientId() const {
  return m_clientId;
}

Maybe<HostAddress> const& ServerClientContext::remoteAddress() const {
  return m_remoteAddress;
}

Uuid const& ServerClientContext::playerUuid() const {
  return m_playerUuid;
}

String const& ServerClientContext::playerName() const {
  return m_playerName;
}

String const& ServerClientContext::shipSpecies() const {
  return m_shipSpecies;
}

bool ServerClientContext::canBecomeAdmin() const {
  return m_canBecomeAdmin;
}

NetCompatibilityRules ServerClientContext::netRules() const {
  return m_netRules;
}

String ServerClientContext::descriptiveName() const {
  RecursiveMutexLocker locker(m_mutex);
  String hostName = m_remoteAddress ? toString(*m_remoteAddress) : "local";
  return strf("'{}' <{}> ({})", m_playerName, m_clientId, hostName);
}

void ServerClientContext::registerRpcHandlers(JsonRpcHandlers const& rpcHandlers) {
  m_rpc.registerHandlers(rpcHandlers);
}

CelestialCoordinate ServerClientContext::shipCoordinate() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_shipCoordinate.get();
}

void ServerClientContext::setShipCoordinate(CelestialCoordinate system) {
  RecursiveMutexLocker locker(m_mutex);
  m_shipCoordinate.set(system);
}

SystemLocation ServerClientContext::shipLocation() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_shipSystemLocation;
}

void ServerClientContext::setShipLocation(SystemLocation location) {
  RecursiveMutexLocker locker(m_mutex);
  m_shipSystemLocation = location;
}

Maybe<pair<WarpAction, WarpMode>> ServerClientContext::orbitWarpAction() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_orbitWarpActionNetState.get();
}

void ServerClientContext::setOrbitWarpAction(Maybe<pair<WarpAction, WarpMode>> warpAction) {
  RecursiveMutexLocker locker(m_mutex);
  m_orbitWarpActionNetState.set(warpAction);
}

bool ServerClientContext::isAdmin() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_isAdminNetState.get();
}

void ServerClientContext::setAdmin(bool admin) {
  RecursiveMutexLocker locker(m_mutex);
  m_isAdminNetState.set(admin);
}

EntityDamageTeam ServerClientContext::team() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_teamNetState.get();
}

void ServerClientContext::setTeam(EntityDamageTeam team) {
  RecursiveMutexLocker locker(m_mutex);
  m_teamNetState.set(team);
}

ShipUpgrades ServerClientContext::shipUpgrades() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_shipUpgrades.get();
}

void ServerClientContext::setShipUpgrades(ShipUpgrades upgrades) {
  RecursiveMutexLocker locker(m_mutex);
  m_shipUpgrades.set(upgrades);
}

void ServerClientContext::setShipSpecies(String shipSpecies) {
  m_shipSpecies = shipSpecies;
}

WorldChunks ServerClientContext::shipChunks() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_shipChunks;
}

void ServerClientContext::updateShipChunks(WorldChunks newShipChunks) {
  RecursiveMutexLocker locker(m_mutex);
  m_shipChunksUpdate.merge(WorldStorage::getWorldChunksUpdate(m_shipChunks, newShipChunks), true);
  m_shipChunks = std::move(newShipChunks);
}

void ServerClientContext::readUpdate(ByteArray data) {
  RecursiveMutexLocker locker(m_mutex);
  m_rpc.receive(data);
}

ByteArray ServerClientContext::writeUpdate() {
  RecursiveMutexLocker locker(m_mutex);

  ByteArray rpcUpdate = m_rpc.send();

  ByteArray shipChunksUpdate;
  if (!m_shipChunksUpdate.empty())
    shipChunksUpdate = DataStreamBuffer::serialize(take(m_shipChunksUpdate));

  ByteArray netGroupUpdate;
  tie(netGroupUpdate, m_netVersion) = m_netGroup.writeNetState(m_netVersion, m_netRules);
  
  StringMap<ByteArray> customWorldChunksUpdate;
  for (auto& p : m_customWorlds) {
    if (!p.second.chunksUpdate.empty())
      customWorldChunksUpdate[p.first] = DataStreamBuffer::serialize(take(p.second.chunksUpdate));
  }

  if (rpcUpdate.empty() && shipChunksUpdate.empty() && netGroupUpdate.empty() && (m_netRules.version() < 15 || customWorldChunksUpdate.empty()))
    return {};

  DataStreamBuffer ds;
  ds.write(rpcUpdate);
  ds.write(shipChunksUpdate);
  if (m_netRules.version() >= 15) {
    ds.write(customWorldChunksUpdate);
  }
  ds.write(netGroupUpdate);

  return ds.takeData();
}

void ServerClientContext::setSystemWorld(SystemWorldServerThreadPtr systemWorldThread) {
  RecursiveMutexLocker locker(m_mutex);
  if (m_systemWorldThread == systemWorldThread)
    return;

  m_systemWorldThread = std::move(systemWorldThread);
}

SystemWorldServerThreadPtr ServerClientContext::systemWorld() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_systemWorldThread;
}

void ServerClientContext::clearSystemWorld() {
  RecursiveMutexLocker locker(m_mutex);
  setSystemWorld({});
}

void ServerClientContext::setPlayerWorld(WorldServerThreadPtr worldThread) {
  RecursiveMutexLocker locker(m_mutex);
  if (m_worldThread == worldThread)
    return;

  m_worldThread = std::move(worldThread);
  if (m_worldThread)
    m_playerWorldIdNetState.set(m_worldThread->worldId());
  else
    m_playerWorldIdNetState.set(WorldId());
}

WorldServerThreadPtr ServerClientContext::playerWorld() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_worldThread;
}

WorldId ServerClientContext::playerWorldId() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_playerWorldIdNetState.get();
}

void ServerClientContext::clearPlayerWorld() {
  setPlayerWorld({});
}

void ServerClientContext::setSubWorld(ClientSubWorldId subWorldId, WorldServerThreadPtr worldThread) {
  RecursiveMutexLocker locker(m_mutex);
  if (m_subWorldThreads[subWorldId] == worldThread)
    return;

  m_subWorldThreads[subWorldId] = std::move(worldThread);
}

WorldServerThreadPtr ServerClientContext::subWorld(ClientSubWorldId subWorldId) const {
  RecursiveMutexLocker locker(m_mutex);
  if (m_subWorldThreads.contains(subWorldId)) {
    return m_subWorldThreads.get(subWorldId);
  } else {
    return {};
  }
}

bool ServerClientContext::hasSubWorld(ClientSubWorldId subWorldId) const {
  RecursiveMutexLocker locker(m_mutex);
  return m_subWorldThreads.contains(subWorldId);
}

void ServerClientContext::clearSubWorld(ClientSubWorldId subWorldId) {
  m_subWorldThreads.remove(subWorldId);
}

List<ClientSubWorldId> ServerClientContext::subWorlds() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_subWorldThreads.keys();
}

WarpToWorld ServerClientContext::playerReturnWarp() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_returnWarp;
}

void ServerClientContext::setPlayerReturnWarp(WarpToWorld warp) {
  RecursiveMutexLocker locker(m_mutex);
  m_returnWarp = std::move(warp);
}

WarpToWorld ServerClientContext::playerReviveWarp() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_reviveWarp;
}

void ServerClientContext::setPlayerReviveWarp(WarpToWorld warp) {
  RecursiveMutexLocker locker(m_mutex);
  m_reviveWarp = std::move(warp);
}

ServerClientContext::CustomWorld::CustomWorld() : chunks(WorldChunks()), chunksUpdate(WorldChunks()), active(false) {}
ServerClientContext::CustomWorld::CustomWorld(WorldChunks initialChunks) : chunks(initialChunks), chunksUpdate(WorldChunks()), active(false) {}

void ServerClientContext::customWorldRequested(String name, RpcThreadPromiseKeeper<WorldChunks> promise) {
  RecursiveMutexLocker locker(m_mutex);
  m_worldRequests.add(name,promise);
}

void ServerClientContext::customWorldReceived(String name, WorldChunks chunks) {
  RecursiveMutexLocker locker(m_mutex);
  if (auto promise = m_worldRequests.maybeTake(name)) {
    (*promise).fulfill(chunks);
  }
  m_customWorlds.add(name,CustomWorld(std::move(chunks)));
}

void ServerClientContext::failWorldRequests() {
  RecursiveMutexLocker locker(m_mutex);
  for (auto& p : m_worldRequests) {
    p.second.fail("Client disconnected");
  }
  m_worldRequests = {};
}

Maybe<WorldChunks> ServerClientContext::customWorldChunks(String name) const {
  RecursiveMutexLocker locker(m_mutex);
  if (m_customWorlds.contains(name)) {
    return m_customWorlds.get(name).chunks;
  } else {
    return {};
  }
}

void ServerClientContext::updateCustomWorldChunks(String name, WorldChunks newWorldChunks) {
  RecursiveMutexLocker locker(m_mutex);
  if (!m_customWorlds.contains(name)) {
    m_customWorlds.add(name,CustomWorld());
  }
  auto &world = m_customWorlds.get(name);
  world.chunksUpdate.merge(WorldStorage::getWorldChunksUpdate(world.chunks, newWorldChunks), true);
  world.chunks = std::move(newWorldChunks);
}

void ServerClientContext::setCustomWorldActive(String name, bool active) {
  RecursiveMutexLocker locker(m_mutex);
  if (m_customWorlds.contains(name)) {
    m_customWorlds.get(name).active = active;
  }
}

List<String> ServerClientContext::customWorlds() const {
  RecursiveMutexLocker locker(m_mutex);
  return m_customWorlds.keys();
}

void ServerClientContext::cleanInactiveCustomWorlds() {
  RecursiveMutexLocker locker(m_mutex);
  for (auto worldName : m_customWorlds.keys()) {
    if (m_customWorlds.contains(worldName)) {
      auto &world = m_customWorlds.get(worldName);
      if (!world.active && world.chunksUpdate.empty()) {
        Logger::info("Removing inactive custom world '{}' for client '{}'",worldName,m_playerUuid.hex());
        m_customWorlds.remove(worldName);
      }
    }
  }
}

void ServerClientContext::loadServerData(Json const& store) {
  RecursiveMutexLocker locker(m_mutex);
  m_shipCoordinate.set(CelestialCoordinate(store.get("shipCoordinate")));
  m_shipSystemLocation = jsonToSystemLocation(store.get("systemLocation"));
  setAdmin(store.getBool("isAdmin"));
  setTeam(EntityDamageTeam(store.get("team")));
  m_reviveWarp = WarpToWorld(store.get("reviveWarp"));
  m_returnWarp = WarpToWorld(store.get("returnWarp"));
}

Json ServerClientContext::storeServerData() {
  RecursiveMutexLocker locker(m_mutex);
  auto store = JsonObject{
    {"shipCoordinate", m_shipCoordinate.get().toJson()},
    {"systemLocation", jsonFromSystemLocation(m_shipSystemLocation)},
    {"isAdmin", m_isAdminNetState.get()},
    {"team", team().toJson()},
    {"reviveWarp", m_reviveWarp.toJson()},
    {"returnWarp", m_returnWarp.toJson()}
  };
  return store;
}

int64_t ServerClientContext::creationTime() const {
  return m_creationTime;
}

}
