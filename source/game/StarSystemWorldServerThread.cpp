#include "StarSystemWorldServerThread.hpp"
#include "StarTickRateMonitor.hpp"
#include "StarNetPackets.hpp"

namespace Star {

SystemWorldServerThread::SystemWorldServerThread(Vec3I const& location, SystemWorldServerPtr systemWorld, String storageFile)
  : Thread(strf("SystemWorldServer: {}", location)), m_stop(false), m_storageFile(storageFile) {
  m_systemLocation = location;
  m_systemWorld = move(systemWorld);
}

SystemWorldServerThread::~SystemWorldServerThread() {
  m_stop = true;
  join();
}

Vec3I SystemWorldServerThread::location() const {
  return m_systemLocation;
}

List<ConnectionId> SystemWorldServerThread::clients() {
  return m_clients.values();
}

void SystemWorldServerThread::addClient(ConnectionId clientId, Uuid const& uuid, float shipSpeed, SystemLocation const& location) {
  WriteLocker locker(m_mutex);
  m_clients.add(clientId);
  m_outgoingPacketQueue.set(clientId, List<PacketPtr>());

  m_systemWorld->addClientShip(clientId, uuid, shipSpeed, location);

  m_clientShipLocations.set(clientId, {m_systemWorld->clientShipLocation(clientId), m_systemWorld->clientSkyParameters(clientId)});
  if (auto warpAction = m_systemWorld->clientWarpAction(clientId))
    m_clientWarpActions.set(clientId, *warpAction);
}

void SystemWorldServerThread::removeClient(ConnectionId clientId) {
  WriteLocker locker(m_mutex);
  m_systemWorld->removeClientShip(clientId);
  m_clients.remove(clientId);
  m_clientShipDestinations.remove(clientId);
  m_clientShipLocations.remove(clientId);
  m_outgoingPacketQueue.remove(clientId);
}

void SystemWorldServerThread::setPause(shared_ptr<const atomic<bool>> pause) {
  m_pause = move(pause);
}

void SystemWorldServerThread::run() {
  TickRateApproacher tickApproacher(1.0 / SystemWorldTimestep, 0.5);

  while (!m_stop) {
    LogMap::set(strf("system_{}_update_rate", m_systemLocation), strf("{:4.2f}Hz", tickApproacher.rate()));

    update();

    m_periodicStorage -= 1.0 / tickApproacher.rate();
    if (m_triggerStorage || m_periodicStorage <= 0.0) {
      m_triggerStorage = false;
      m_periodicStorage = 300.0; // store every 5 minutes
      store();
    }

    tickApproacher.tick();

    double spareTime = tickApproacher.spareTime();
    uint64_t millis = floor(spareTime * 1000);
    if (spareTime > 0)
      sleepPrecise(millis);
  }

  store();
}

void SystemWorldServerThread::stop() {
  m_stop = true;
}

void SystemWorldServerThread::update() {
  WriteLocker queueLocker(m_queueMutex);
  WriteLocker locker(m_mutex);

  for (auto p : take(m_incomingPacketQueue))
    m_systemWorld->handleIncomingPacket(p.first, p.second);

  for (auto p : take(m_clientShipActions))
    p.second(m_systemWorld->clientShip(p.first).get());

  if (!m_pause || *m_pause == false)
    m_systemWorld->update();
  m_triggerStorage = m_systemWorld->triggeredStorage();

  // important to set destinations before getting locations
  // setting a destination nullifies the current location
  for (auto p : take(m_clientShipDestinations))
    m_systemWorld->setClientDestination(p.first, p.second);

  m_activeInstanceWorlds = m_systemWorld->activeInstanceWorlds();

  for (auto clientId : m_clients) {
    m_outgoingPacketQueue[clientId].appendAll(m_systemWorld->pullOutgoingPackets(clientId));
    auto shipSystemLocation = m_systemWorld->clientShipLocation(clientId);
    auto& shipLocation = m_clientShipLocations[clientId];
    if (shipLocation.first != shipSystemLocation) {
      shipLocation.first = shipSystemLocation;
      shipLocation.second = m_systemWorld->clientSkyParameters(clientId);
    }
    if (auto warpAction = m_systemWorld->clientWarpAction(clientId))
      m_clientWarpActions.set(clientId, *warpAction);
    else if (m_clientWarpActions.contains(clientId))
      m_clientWarpActions.remove(clientId);
  }
  queueLocker.unlock();

  if (m_updateAction)
    m_updateAction(this);
}

void SystemWorldServerThread::setClientDestination(ConnectionId clientId, SystemLocation const& destination) {
  WriteLocker locker(m_queueMutex);
  m_clientShipDestinations.set(clientId, destination);
}

void SystemWorldServerThread::executeClientShipAction(ConnectionId clientId, ClientShipAction action) {
  WriteLocker locker(m_queueMutex);
  m_clientShipActions.append({clientId, move(action)});
}

SystemLocation SystemWorldServerThread::clientShipLocation(ConnectionId clientId) {
  ReadLocker locker(m_queueMutex);
  // while a ship destination is pending the ship is assumed to be flying
  if (m_clientShipDestinations.contains(clientId))
    return {};
  return m_clientShipLocations.get(clientId).first;
}

Maybe<pair<WarpAction, WarpMode>> SystemWorldServerThread::clientWarpAction(ConnectionId clientId) {
  ReadLocker locker(m_queueMutex);
  if (m_clientShipDestinations.contains(clientId))
    return {};
  return m_clientWarpActions.maybe(clientId);
}

SkyParameters SystemWorldServerThread::clientSkyParameters(ConnectionId clientId) {
  ReadLocker locker(m_queueMutex);
  return m_clientShipLocations.get(clientId).second;
}

List<InstanceWorldId> SystemWorldServerThread::activeInstanceWorlds() const {
  return m_activeInstanceWorlds;
}

void SystemWorldServerThread::setUpdateAction(function<void(SystemWorldServerThread*)> updateAction) {
  m_updateAction = updateAction;
}

void SystemWorldServerThread::pushIncomingPacket(ConnectionId clientId, PacketPtr packet) {
  WriteLocker locker(m_queueMutex);
  m_incomingPacketQueue.append({move(clientId), move(packet)});
}

List<PacketPtr> SystemWorldServerThread::pullOutgoingPackets(ConnectionId clientId) {
  WriteLocker locker(m_queueMutex);
  return take(m_outgoingPacketQueue[clientId]);
}

void SystemWorldServerThread::store() {
  ReadLocker locker(m_mutex);
  Json store = m_systemWorld->diskStore();
  locker.unlock();

  Logger::debug("Trigger disk storage for system world {}:{}:{}", m_systemLocation.x(), m_systemLocation.y(), m_systemLocation.z());
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto versionedStore = versioningDatabase->makeCurrentVersionedJson("System", store);
  VersionedJson::writeFile(versionedStore, m_storageFile);
}

}
