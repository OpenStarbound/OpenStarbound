#ifndef STAR_SYSTEM_WORLD_SERVER_THREAD_HPP
#define STAR_SYSTEM_WORLD_SERVER_THREAD_HPP

#include "StarSystemWorldServer.hpp"
#include "StarThread.hpp"
#include "StarNetPackets.hpp"

namespace Star {

STAR_CLASS(SystemWorldServerThread);

typedef function<void(SystemClientShip*)> ClientShipAction;

class SystemWorldServerThread : public Thread {
public:
  SystemWorldServerThread(Vec3I const& location, SystemWorldServerPtr systemWorld, String storageFile);
  ~SystemWorldServerThread();

  Vec3I location() const;

  List<ConnectionId> clients();
  void addClient(ConnectionId clientId, Uuid const& uuid, float shipSpeed, SystemLocation const& location);
  void removeClient(ConnectionId clientId);

  void setPause(shared_ptr<const atomic<bool>> pause);
  void run() override;
  void stop();

  void update();

  void setClientDestination(ConnectionId clientId, SystemLocation const& location);
  void executeClientShipAction(ConnectionId clientId, ClientShipAction action);

  SystemLocation clientShipLocation(ConnectionId clientId);
  Maybe<pair<WarpAction, WarpMode>> clientWarpAction(ConnectionId clientId);
  SkyParameters clientSkyParameters(ConnectionId clientId);

  List<InstanceWorldId> activeInstanceWorlds() const;

  // callback to be run after update in the server thread
  void setUpdateAction(function<void(SystemWorldServerThread*)> updateAction);
  void pushIncomingPacket(ConnectionId clientId, PacketPtr packet);
  List<PacketPtr> pullOutgoingPackets(ConnectionId clientId);

  void store();

private:
  Vec3I m_systemLocation;
  SystemWorldServerPtr m_systemWorld;

  atomic<bool> m_stop{false};
  float m_periodicStorage{300.0f};
  bool m_triggerStorage{ false};
  String m_storageFile;

  shared_ptr<const atomic<bool>> m_pause;
  function<void(SystemWorldServerThread*)> m_updateAction;

  ReadersWriterMutex m_mutex;
  ReadersWriterMutex m_queueMutex;

  HashSet<ConnectionId> m_clients;
  HashMap<ConnectionId, SystemLocation> m_clientShipDestinations;
  HashMap<ConnectionId, pair<SystemLocation, SkyParameters>> m_clientShipLocations;
  HashMap<ConnectionId, pair<WarpAction, WarpMode>> m_clientWarpActions;
  List<pair<ConnectionId, ClientShipAction>> m_clientShipActions;
  List<InstanceWorldId> m_activeInstanceWorlds;
  Map<ConnectionId, List<PacketPtr>> m_outgoingPacketQueue;
  List<pair<ConnectionId, PacketPtr>> m_incomingPacketQueue;
};


}

#endif
