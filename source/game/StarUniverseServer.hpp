#pragma once

#include "StarLockFile.hpp"
#include "StarIdMap.hpp"
#include "StarWorkerPool.hpp"
#include "StarGameTypes.hpp"
#include "StarCelestialCoordinate.hpp"
#include "StarServerClientContext.hpp"
#include "StarWorldServerThread.hpp"
#include "StarSystemWorldServerThread.hpp"
#include "StarUniverseConnection.hpp"
#include "StarUniverseSettings.hpp"

namespace Star {

STAR_CLASS(Clock);
STAR_CLASS(File);
STAR_CLASS(Player);
STAR_CLASS(ChatProcessor);
STAR_CLASS(CommandProcessor);
STAR_CLASS(TeamManager);
STAR_CLASS(UniverseServer);
STAR_CLASS(WorldTemplate);
STAR_CLASS(WorldServer);
STAR_CLASS(UniverseSettings);

STAR_EXCEPTION(UniverseServerException, StarException);

// Manages all running worlds, listens for new client connections and marshalls
// between all the different worlds and all the different client connections
// and routes packets between them.
class UniverseServer : public Thread {
public:
  UniverseServer(String const& storageDir);
  ~UniverseServer();

  // If enabled, will listen on the configured server port for incoming
  // connections.
  void setListeningTcp(bool listenTcp);

  // Connects an arbitrary UniverseConnection to this server
  void addClient(UniverseConnection remoteConnection);
  // Constructs an in-process connection to a UniverseServer for a
  // UniverseClient, and returns the other side of the connection.
  UniverseConnection addLocalClient();

  // Signals the UniverseServer to stop and then joins the thread.
  void stop();

  void setPause(bool pause);
  void setTimescale(float timescale);
  void setTickRate(float tickRate);

  List<WorldId> activeWorlds() const;
  bool isWorldActive(WorldId const& worldId) const;

  List<ConnectionId> clientIds() const;
  List<pair<ConnectionId, int64_t>> clientIdsAndCreationTime() const;
  size_t numberOfClients() const;
  uint32_t maxClients() const;
  bool isConnectedClient(ConnectionId clientId) const;

  String clientDescriptor(ConnectionId clientId) const;

  String clientNick(ConnectionId clientId) const;
  Maybe<ConnectionId> findNick(String const& nick) const;

  Maybe<Uuid> uuidForClient(ConnectionId clientId) const;
  Maybe<ConnectionId> clientForUuid(Uuid const& uuid) const;

  void adminBroadcast(String const& text);
  void adminWhisper(ConnectionId clientId, String const& text);
  String adminCommand(String text);

  bool isAdmin(ConnectionId clientId) const;
  bool canBecomeAdmin(ConnectionId clientId) const;
  void setAdmin(ConnectionId clientId, bool admin);

  bool isLocal(ConnectionId clientId) const;

  bool isPvp(ConnectionId clientId) const;
  void setPvp(ConnectionId clientId, bool pvp);

  RpcThreadPromise<Json> sendWorldMessage(WorldId const& worldId, String const& message, JsonArray const& args = {});

  void clientWarpPlayer(ConnectionId clientId, WarpAction action, bool deploy = false);
  void clientFlyShip(ConnectionId clientId, Vec3I const& system, SystemLocation const& location, Json const& settings = {});
  WorldId clientWorld(ConnectionId clientId) const;
  CelestialCoordinate clientShipCoordinate(ConnectionId clientId) const;

  ClockPtr universeClock() const;
  UniverseSettingsPtr universeSettings() const;

	CelestialDatabase& celestialDatabase();

  // If the client exists and is in a valid connection state, executes the
  // given function on the client world and player object in a thread safe way.
  // Returns true if function was called, false if client was not found or in
  // an invalid connection state.
  bool executeForClient(ConnectionId clientId, function<void(WorldServer*, PlayerPtr)> action);
  void disconnectClient(ConnectionId clientId, String const& reason);
  void banUser(ConnectionId clientId, String const& reason, pair<bool, bool> banType, Maybe<int> timeout);
  bool unbanIp(String const& addressString);
  bool unbanUuid(String const& uuidString);

  bool updatePlanetType(CelestialCoordinate const& coordinate, String const& newType, String const& weatherBiome);

  bool setWeather(CelestialCoordinate const& coordinate, String const& weatherName, bool force = false);

  StringList weatherList(CelestialCoordinate const& coordinate);

  bool sendPacket(ConnectionId clientId, PacketPtr packet);

protected:
  virtual void run();

private:
  struct TimeoutBan {
    int64_t banExpiry;
    String reason;
    Maybe<HostAddress> ip;
    Maybe<Uuid> uuid;
  };

  enum class TcpState : uint8_t { No, Yes, Fuck };

  void processUniverseFlags();
  void sendPendingChat();
  void updateTeams();
  void updateShips();
  void sendClockUpdates();
  void sendClientContextUpdate(ServerClientContextPtr clientContext);
  void sendClientContextUpdates();
  void kickErroredPlayers();
  void reapConnections();
  void processPlanetTypeChanges();
  void warpPlayers();
  void flyShips();
  void arriveShips();
  void respondToCelestialRequests();
  void processChat();
  void clearBrokenWorlds();
  void handleWorldMessages();
  void shutdownInactiveWorlds();
  void doTriggeredStorage();

  void saveSettings();
  void loadSettings();

  void startLuaScripts();
  void updateLua();
  void stopLua();

  // Either returns the default configured starter world, or a new randomized
  // starter world, or if a randomized world is not yet available, starts a job
  // to find a randomized starter world and returns nothing until it is ready.
  Maybe<CelestialCoordinate> nextStarterWorld();

  void loadTempWorldIndex();
  void saveTempWorldIndex();
  String tempWorldFile(InstanceWorldId const& worldId) const;

  Maybe<String> isBannedUser(Maybe<HostAddress> hostAddress, Uuid playerUuid) const;
  void doTempBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType, int timeout);
  void doPermBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType);
  void removeTimedBan();

  void addCelestialRequests(ConnectionId clientId, List<CelestialRequest> requests);

  void worldUpdated(WorldServerThread* worldServer);
  void systemWorldUpdated(SystemWorldServerThread* systemWorldServer);
  void packetsReceived(UniverseConnectionServer* connectionServer, ConnectionId clientId, List<PacketPtr> packets);

  void acceptConnection(UniverseConnection connection, Maybe<HostAddress> remoteAddress);

  // Main lock and clients read lock must be held when calling
  WarpToWorld resolveWarpAction(WarpAction warpAction, ConnectionId clientId, bool deploy) const;

  void doDisconnection(ConnectionId clientId, String const& reason);

  // Clients read lock must be held when calling
  Maybe<ConnectionId> getClientForUuid(Uuid const& uuid) const;

  // Get the world only if it is already loaded, Main lock must be held when
  // calling.
  WorldServerThreadPtr getWorld(WorldId const& worldId);

  // If the world is not created, block and load it, otherwise just return the
  // loaded world.  Main lock and Clients read lock must be held when calling.
  WorldServerThreadPtr createWorld(WorldId const& worldId);

  // Trigger off-thread world creation, returns a value when the creation is
  // finished, either successfully or with an error.  Main lock and Clients
  // read lock must be held when calling.
  Maybe<WorldServerThreadPtr> triggerWorldCreation(WorldId const& worldId);

  // Main lock and clients read lock must be held when calling world promise
  // generators
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> makeWorldPromise(WorldId const& worldId);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> shipWorldPromise(ClientShipWorldId const& uuid);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> celestialWorldPromise(CelestialWorldId const& coordinate);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> instanceWorldPromise(InstanceWorldId const& instanceWorld);

  // If the system world is not created, initialize it, otherwise return the
  // already initialized one
  SystemWorldServerThreadPtr createSystemWorld(Vec3I const& location);

  bool instanceWorldStoredOrActive(InstanceWorldId const& worldId) const;

  // Signal that a world either failed to load, or died due to an exception,
  // kicks clients if that world is a ship world.  Main lock and clients read
  // lock must be held when calling.
  void worldDiedWithError(WorldId world);

  // Get SkyParameters if the coordinate is a valid world, and empty
  // SkyParameters otherwise.
  SkyParameters celestialSkyParameters(CelestialCoordinate const& coordinate) const;

  mutable RecursiveMutex m_mainLock;

  String m_storageDirectory;
  ByteArray m_assetsDigest;
  Maybe<LockFile> m_storageDirectoryLock;
  StringMap<StringList> m_speciesShips;
  CelestialMasterDatabasePtr m_celestialDatabase;
  ClockPtr m_universeClock;
  UniverseSettingsPtr m_universeSettings;
  WorkerPool m_workerPool;

  int64_t m_storageTriggerDeadline;
  int64_t m_clearBrokenWorldsDeadline;
  int64_t m_lastClockUpdateSent;
  atomic<bool> m_stop;
  atomic<TcpState> m_tcpState;

  mutable ReadersWriterMutex m_clientsLock;
  unsigned m_maxPlayers;
  IdMap<ConnectionId, ServerClientContextPtr> m_clients;

  shared_ptr<atomic<bool>> m_pause;
  Map<WorldId, Maybe<WorkerPoolPromise<WorldServerThreadPtr>>> m_worlds;
  Map<InstanceWorldId, pair<int64_t, int64_t>> m_tempWorldIndex;
  Map<Vec3I, SystemWorldServerThreadPtr> m_systemWorlds;
  UniverseConnectionServerPtr m_connectionServer;

  RecursiveMutex m_connectionAcceptThreadsMutex;
  List<ThreadFunction<void>> m_connectionAcceptThreads;
  LinkedList<pair<UniverseConnection, int64_t>> m_deadConnections;

  ChatProcessorPtr m_chatProcessor;
  CommandProcessorPtr m_commandProcessor;
  TeamManagerPtr m_teamManager;

  HashMap<ConnectionId, pair<WarpAction, bool>> m_pendingPlayerWarps;
  HashMap<ConnectionId, pair<tuple<Vec3I, SystemLocation, Json>, Maybe<double>>> m_queuedFlights;
  HashMap<ConnectionId, tuple<Vec3I, SystemLocation, Json>> m_pendingFlights;
  HashMap<ConnectionId, CelestialCoordinate> m_pendingArrivals;
  HashMap<ConnectionId, String> m_pendingDisconnections;
  HashMap<ConnectionId, List<WorkerPoolPromise<CelestialResponse>>> m_pendingCelestialRequests;
  List<pair<WorldId, UniverseFlagAction>> m_pendingFlagActions;
  HashMap<ConnectionId, List<tuple<String, ChatSendMode, JsonObject>>> m_pendingChat;
  Maybe<WorkerPoolPromise<CelestialCoordinate>> m_nextRandomizedStarterWorld;
  Map<WorldId, List<WorldServerThread::Message>> m_pendingWorldMessages;

  List<TimeoutBan> m_tempBans;

  LuaRootPtr m_luaRoot;

  typedef LuaUpdatableComponent<LuaBaseComponent> ScriptComponent;
  typedef shared_ptr<ScriptComponent> ScriptComponentPtr;
  StringMap<ScriptComponentPtr> m_scriptContexts;
};

}
