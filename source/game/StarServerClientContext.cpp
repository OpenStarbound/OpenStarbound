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

ServerClientContext::ServerClientContext(ConnectionId clientId, Maybe<HostAddress> remoteAddress, Uuid playerUuid,
    String playerName, String playerSpecies, bool canBecomeAdmin, WorldChunks initialShipChunks)
  : m_clientId(clientId),
    m_remoteAddress(remoteAddress),
    m_playerUuid(playerUuid),
    m_playerName(playerName),
    m_playerSpecies(playerSpecies),
    m_canBecomeAdmin(canBecomeAdmin),
    m_shipChunks(std::move(initialShipChunks)) {
  m_rpc.registerHandler("ship.applyShipUpgrades", [this](Json const& args) -> Json {
      RecursiveMutexLocker locker(m_mutex);
      setShipUpgrades(shipUpgrades().apply(args));
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

String const& ServerClientContext::playerSpecies() const {
  return m_playerSpecies;
}

bool ServerClientContext::canBecomeAdmin() const {
  return m_canBecomeAdmin;
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
  tie(netGroupUpdate, m_netVersion) = m_netGroup.writeNetState(m_netVersion);

  if (rpcUpdate.empty() && shipChunksUpdate.empty() && netGroupUpdate.empty())
    return {};

  DataStreamBuffer ds;
  ds.write(rpcUpdate);
  ds.write(shipChunksUpdate);
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

}
