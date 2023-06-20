#include "StarClientContext.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

DataStream& operator>>(DataStream& ds, ShipUpgrades& upgrades) {
  ds.read(upgrades.shipLevel);
  ds.read(upgrades.maxFuel);
  ds.read(upgrades.crewSize);
  ds.read(upgrades.fuelEfficiency);
  ds.read(upgrades.shipSpeed);
  ds.read(upgrades.capabilities);
  return ds;
}

DataStream& operator<<(DataStream& ds, ShipUpgrades const& upgrades) {
  ds.write(upgrades.shipLevel);
  ds.write(upgrades.maxFuel);
  ds.write(upgrades.crewSize);
  ds.write(upgrades.fuelEfficiency);
  ds.write(upgrades.shipSpeed);
  ds.write(upgrades.capabilities);
  return ds;
}

ClientContext::ClientContext(Uuid serverUuid) {
  m_serverUuid = move(serverUuid);
  m_rpc = make_shared<JsonRpc>();

  m_netGroup.addNetElement(&m_orbitWarpActionNetState);
  m_netGroup.addNetElement(&m_playerWorldIdNetState);
  m_netGroup.addNetElement(&m_isAdminNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_shipUpgrades);
  m_netGroup.addNetElement(&m_shipCoordinate);
}

Uuid ClientContext::serverUuid() const {
  return m_serverUuid;
}

CelestialCoordinate ClientContext::shipCoordinate() const {
  return m_shipCoordinate.get();
}

Maybe<pair<WarpAction, WarpMode>> ClientContext::orbitWarpAction() const {
  return m_orbitWarpActionNetState.get();
}

WorldId ClientContext::playerWorldId() const {
  return m_playerWorldIdNetState.get();
}

bool ClientContext::isAdmin() const {
  return m_isAdminNetState.get();
}

EntityDamageTeam ClientContext::team() const {
  return m_teamNetState.get();
}

JsonRpcInterfacePtr ClientContext::rpcInterface() const {
  return m_rpc;
}

WorldChunks ClientContext::newShipUpdates() {
  return take(m_newShipUpdates);
}

ShipUpgrades ClientContext::shipUpgrades() const {
  return m_shipUpgrades.get();
}

void ClientContext::readUpdate(ByteArray data) {
  if (data.empty())
    return;

  DataStreamBuffer ds(move(data));

  m_rpc->receive(ds.read<ByteArray>());

  auto shipUpdates = ds.read<ByteArray>();
  if (!shipUpdates.empty())
    m_newShipUpdates.merge(DataStreamBuffer::deserialize<WorldChunks>(move(shipUpdates)), true);

  m_netGroup.readNetState(ds.read<ByteArray>());
}

ByteArray ClientContext::writeUpdate() {
  return m_rpc->send();
}

}
