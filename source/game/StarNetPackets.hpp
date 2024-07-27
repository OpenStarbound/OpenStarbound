#pragma once

#include "StarDataStream.hpp"
#include "StarWorldTiles.hpp"
#include "StarItemDescriptor.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarDamageManager.hpp"
#include "StarChatTypes.hpp"
#include "StarUuid.hpp"
#include "StarTileModification.hpp"
#include "StarEntity.hpp"
#include "StarInteractionTypes.hpp"
#include "StarWarping.hpp"
#include "StarWiring.hpp"
#include "StarClientContext.hpp"
#include "StarSystemWorld.hpp"

namespace Star {

STAR_STRUCT(Packet);

STAR_EXCEPTION(StarPacketException, IOException);

extern VersionNumber const StarProtocolVersion;

// Packet types sent between the client and server over a NetSocket.  Does not
// correspond to actual packets, simply logical portions of NetSocket data.
enum class PacketType : uint8_t {
  // Packets used as part of the initial handshake
  ProtocolRequest,
  ProtocolResponse,

  // Packets sent universe server -> universe client
  ServerDisconnect,
  ConnectSuccess,
  ConnectFailure,
  HandshakeChallenge,
  ChatReceive,
  UniverseTimeUpdate,
  CelestialResponse,
  PlayerWarpResult,
  PlanetTypeUpdate,
  Pause,
  ServerInfo,

  // Packets sent universe client -> universe server
  ClientConnect,
  ClientDisconnectRequest,
  HandshakeResponse,
  PlayerWarp,
  FlyShip,
  ChatSend,
  CelestialRequest,

  // Packets sent bidirectionally between the universe client and the universe
  // server
  ClientContextUpdate,

  // Packets sent world server -> world client
  WorldStart,
  WorldStop,
  WorldLayoutUpdate,
  WorldParametersUpdate,
  CentralStructureUpdate,
  TileArrayUpdate,
  TileUpdate,
  TileLiquidUpdate,
  TileDamageUpdate,
  TileModificationFailure,
  GiveItem,
  EnvironmentUpdate,
  UpdateTileProtection,
  SetDungeonGravity,
  SetDungeonBreathable,
  SetPlayerStart,
  FindUniqueEntityResponse,
  Pong,

  // Packets sent world client -> world server
  ModifyTileList,
  DamageTileGroup,
  CollectLiquid,
  RequestDrop,
  SpawnEntity,
  ConnectWire,
  DisconnectAllWires,
  WorldClientStateUpdate,
  FindUniqueEntity,
  WorldStartAcknowledge,
  Ping,

  // Packets sent bidirectionally between world client and world server
  EntityCreate,
  EntityUpdateSet,
  EntityDestroy,
  EntityInteract,
  EntityInteractResult,
  HitRequest,
  DamageRequest,
  DamageNotification,
  EntityMessage,
  EntityMessageResponse,
  UpdateWorldProperties,
  StepUpdate,

  // Packets sent system server -> system client
  SystemWorldStart,
  SystemWorldUpdate,
  SystemObjectCreate,
  SystemObjectDestroy,
  SystemShipCreate,
  SystemShipDestroy,

  // Packets sent system client -> system server
  SystemObjectSpawn
};
extern EnumMap<PacketType> const PacketTypeNames;

enum class NetCompressionMode : uint8_t {
  None,
  Zstd
};
extern EnumMap<NetCompressionMode> const NetCompressionModeNames;

enum class PacketCompressionMode : uint8_t {
  Disabled,
  Automatic,
  Enabled
};

struct Packet {
  virtual ~Packet();

  virtual PacketType type() const = 0;
  virtual String const& typeName() const = 0;

  virtual void readLegacy(DataStream& ds);
  virtual void read(DataStream& ds) = 0;
  virtual void writeLegacy(DataStream& ds) const;
  virtual void write(DataStream& ds) const = 0;

  virtual void readJson(Json const& json);
  virtual Json writeJson() const;

  PacketCompressionMode compressionMode() const;
  void setCompressionMode(PacketCompressionMode compressionMode);

  PacketCompressionMode m_compressionMode = PacketCompressionMode::Automatic;
};

PacketPtr createPacket(PacketType type);
PacketPtr createPacket(PacketType type, Maybe<Json> const& args);

template <PacketType PacketT>
struct PacketBase : public Packet {
  static PacketType const Type = PacketT;

  PacketType type() const override { return Type; }
  String const& typeName() const override { return PacketTypeNames.getRight(Type); }
};

struct ProtocolRequestPacket : PacketBase<PacketType::ProtocolRequest> {
  ProtocolRequestPacket();
  ProtocolRequestPacket(VersionNumber requestProtocolVersion);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  VersionNumber requestProtocolVersion;
};

struct ProtocolResponsePacket : PacketBase<PacketType::ProtocolResponse> {
  ProtocolResponsePacket(bool allowed = false, Json info = {});

  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  bool allowed;
  Json info;
};

struct ServerDisconnectPacket : PacketBase<PacketType::ServerDisconnect> {
  ServerDisconnectPacket();
  ServerDisconnectPacket(String reason);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String reason;
};

struct ConnectSuccessPacket : PacketBase<PacketType::ConnectSuccess> {
  ConnectSuccessPacket();
  ConnectSuccessPacket(ConnectionId clientId, Uuid serverUuid, CelestialBaseInformation celestialInformation);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ConnectionId clientId;
  Uuid serverUuid;
  CelestialBaseInformation celestialInformation;
};

struct ConnectFailurePacket : PacketBase<PacketType::ConnectFailure> {
  ConnectFailurePacket();
  ConnectFailurePacket(String reason);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String reason;
};

struct HandshakeChallengePacket : PacketBase<PacketType::HandshakeChallenge> {
  HandshakeChallengePacket();
  HandshakeChallengePacket(ByteArray const& passwordSalt);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray passwordSalt;
};

struct ChatReceivePacket : PacketBase<PacketType::ChatReceive> {
  ChatReceivePacket();
  ChatReceivePacket(ChatReceivedMessage receivedMessage);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  ChatReceivedMessage receivedMessage;
};

struct UniverseTimeUpdatePacket : PacketBase<PacketType::UniverseTimeUpdate> {
  UniverseTimeUpdatePacket();
  UniverseTimeUpdatePacket(double universeTime);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  double universeTime;
  float timescale;
};

struct CelestialResponsePacket : PacketBase<PacketType::CelestialResponse> {
  CelestialResponsePacket();
  CelestialResponsePacket(List<CelestialResponse> responses);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  List<CelestialResponse> responses;
};

struct PlayerWarpResultPacket : PacketBase<PacketType::PlayerWarpResult> {
  PlayerWarpResultPacket();
  PlayerWarpResultPacket(bool success, WarpAction warpAction, bool warpActionInvalid);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  bool success;
  WarpAction warpAction;
  bool warpActionInvalid;
};

struct PlanetTypeUpdatePacket : PacketBase<PacketType::PlanetTypeUpdate> {
  PlanetTypeUpdatePacket();
  PlanetTypeUpdatePacket(CelestialCoordinate coordinate);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  CelestialCoordinate coordinate;
};

struct PausePacket : PacketBase<PacketType::Pause> {
  PausePacket();
  PausePacket(bool pause, float timescale = 1.0f);

  void readLegacy(DataStream& ds) override;
  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  bool pause = false;
  float timescale = 1.0f;
};

struct ServerInfoPacket : PacketBase<PacketType::ServerInfo> {
  ServerInfoPacket();
  ServerInfoPacket(uint16_t players, uint16_t maxPlayers);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  uint16_t players;
  uint16_t maxPlayers;
};

struct ClientConnectPacket : PacketBase<PacketType::ClientConnect> {
  ClientConnectPacket();
  ClientConnectPacket(ByteArray assetsDigest, bool allowAssetsMismatch, Uuid playerUuid, String playerName,
      String playerSpecies, WorldChunks shipChunks, ShipUpgrades shipUpgrades, bool introComplete,
      String account);

  void readLegacy(DataStream& ds) override;
  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  ByteArray assetsDigest;
  bool allowAssetsMismatch;
  Uuid playerUuid;
  String playerName;
  String playerSpecies;
  WorldChunks shipChunks;
  ShipUpgrades shipUpgrades;
  bool introComplete;
  String account;
  Json info;
};

struct ClientDisconnectRequestPacket : PacketBase<PacketType::ClientDisconnectRequest> {
  ClientDisconnectRequestPacket();

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;
};

struct HandshakeResponsePacket : PacketBase<PacketType::HandshakeResponse> {
  HandshakeResponsePacket();
  HandshakeResponsePacket(ByteArray const& passHash);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray passHash;
};

struct PlayerWarpPacket : PacketBase<PacketType::PlayerWarp> {
  PlayerWarpPacket();
  PlayerWarpPacket(WarpAction action, bool deploy);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  WarpAction action;
  bool deploy;
};

struct FlyShipPacket : PacketBase<PacketType::FlyShip> {
  FlyShipPacket();
  FlyShipPacket(Vec3I system, SystemLocation location);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec3I system;
  SystemLocation location;
};

struct ChatSendPacket : PacketBase<PacketType::ChatSend> {
  ChatSendPacket();
  ChatSendPacket(String text, ChatSendMode sendMode);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String text;
  ChatSendMode sendMode;
};

struct CelestialRequestPacket : PacketBase<PacketType::CelestialRequest> {
  CelestialRequestPacket();
  CelestialRequestPacket(List<CelestialRequest> requests);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  List<CelestialRequest> requests;
};

struct ClientContextUpdatePacket : PacketBase<PacketType::ClientContextUpdate> {
  ClientContextUpdatePacket();
  ClientContextUpdatePacket(ByteArray updateData);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray updateData;
};

// Sent when a client should initialize themselves on a new world
struct WorldStartPacket : PacketBase<PacketType::WorldStart> {
  WorldStartPacket();

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Json templateData;
  ByteArray skyData;
  ByteArray weatherData;
  Vec2F playerStart;
  Vec2F playerRespawn;
  bool respawnInWorld;
  HashMap<DungeonId, float> dungeonIdGravity;
  HashMap<DungeonId, bool> dungeonIdBreathable;
  Set<DungeonId> protectedDungeonIds;
  Json worldProperties;
  ConnectionId clientId;
  bool localInterpolationMode;
};

// Sent when a client is leaving a world
struct WorldStopPacket : PacketBase<PacketType::WorldStop> {
  WorldStopPacket();
  WorldStopPacket(String const& reason);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String reason;
};

// Sent when the region data for the client's current world changes
struct WorldLayoutUpdatePacket : PacketBase<PacketType::WorldLayoutUpdate> {
  WorldLayoutUpdatePacket();
  WorldLayoutUpdatePacket(Json const& layoutData);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Json layoutData;
};

// Sent when the environment status effect list for the client's current world changes
struct WorldParametersUpdatePacket : PacketBase<PacketType::WorldParametersUpdate> {
  WorldParametersUpdatePacket();
  WorldParametersUpdatePacket(ByteArray const& parametersData);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray parametersData;
};

struct CentralStructureUpdatePacket : PacketBase<PacketType::CentralStructureUpdate> {
  CentralStructureUpdatePacket();
  CentralStructureUpdatePacket(Json structureData);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Json structureData;
};

struct TileArrayUpdatePacket : PacketBase<PacketType::TileArrayUpdate> {
  typedef MultiArray<NetTile, 2> TileArray;

  TileArrayUpdatePacket();

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec2I min;
  TileArray array;
};

struct TileUpdatePacket : PacketBase<PacketType::TileUpdate> {
  TileUpdatePacket() {}
  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec2I position;
  NetTile tile;
};

struct TileLiquidUpdatePacket : PacketBase<PacketType::TileLiquidUpdate> {
  TileLiquidUpdatePacket();
  TileLiquidUpdatePacket(Vec2I const& position, LiquidNetUpdate liquidUpdate);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec2I position;
  LiquidNetUpdate liquidUpdate;
};

struct TileDamageUpdatePacket : PacketBase<PacketType::TileDamageUpdate> {
  TileDamageUpdatePacket();
  TileDamageUpdatePacket(Vec2I const& position, TileLayer layer, TileDamageStatus const& tileDamage);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec2I position;
  TileLayer layer;

  TileDamageStatus tileDamage;
};

struct TileModificationFailurePacket : PacketBase<PacketType::TileModificationFailure> {
  TileModificationFailurePacket();
  TileModificationFailurePacket(TileModificationList modifications);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  TileModificationList modifications;
};

struct GiveItemPacket : PacketBase<PacketType::GiveItem> {
  GiveItemPacket();
  GiveItemPacket(ItemDescriptor const& item);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  ItemDescriptor item;
};

struct EnvironmentUpdatePacket : PacketBase<PacketType::EnvironmentUpdate> {
  EnvironmentUpdatePacket();
  EnvironmentUpdatePacket(ByteArray skyDelta, ByteArray weatherDelta);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray skyDelta;
  ByteArray weatherDelta;
};

struct UpdateTileProtectionPacket : PacketBase<PacketType::UpdateTileProtection> {
  UpdateTileProtectionPacket();
  UpdateTileProtectionPacket(DungeonId dungeonId, bool isProtected);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  DungeonId dungeonId;
  bool isProtected;
};

struct SetDungeonGravityPacket : PacketBase<PacketType::SetDungeonGravity> {
  SetDungeonGravityPacket();
  SetDungeonGravityPacket(DungeonId dungeonId, Maybe<float> gravity);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  DungeonId dungeonId;
  Maybe<float> gravity;
};

struct SetDungeonBreathablePacket : PacketBase<PacketType::SetDungeonBreathable> {
  SetDungeonBreathablePacket();
  SetDungeonBreathablePacket(DungeonId dungeonId, Maybe<bool> breathable);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  DungeonId dungeonId;
  Maybe<bool> breathable;
};

struct SetPlayerStartPacket : PacketBase<PacketType::SetPlayerStart> {
  SetPlayerStartPacket();
  SetPlayerStartPacket(Vec2F playerStart, bool respawnInWorld);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  Vec2F playerStart;
  bool respawnInWorld;
};

struct FindUniqueEntityResponsePacket : PacketBase<PacketType::FindUniqueEntityResponse> {
  FindUniqueEntityResponsePacket();
  FindUniqueEntityResponsePacket(String uniqueEntityId, Maybe<Vec2F> entityPosition);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String uniqueEntityId;
  Maybe<Vec2F> entityPosition;
};

struct PongPacket : PacketBase<PacketType::Pong> {
  PongPacket();
  PongPacket(int64_t time);

  void readLegacy(DataStream& ds) override;
  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  int64_t time = 0;
};

struct ModifyTileListPacket : PacketBase<PacketType::ModifyTileList> {
  ModifyTileListPacket();
  ModifyTileListPacket(TileModificationList modifications, bool allowEntityOverlap);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  TileModificationList modifications;
  bool allowEntityOverlap;
};

struct DamageTileGroupPacket : PacketBase<PacketType::DamageTileGroup> {
  DamageTileGroupPacket();
  DamageTileGroupPacket(List<Vec2I> tilePositions, TileLayer layer, Vec2F sourcePosition, TileDamage tileDamage, Maybe<EntityId> sourceEntity);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  List<Vec2I> tilePositions;
  TileLayer layer;
  Vec2F sourcePosition;
  TileDamage tileDamage;
  Maybe<EntityId> sourceEntity;
};

struct CollectLiquidPacket : PacketBase<PacketType::CollectLiquid> {
  CollectLiquidPacket();
  CollectLiquidPacket(List<Vec2I> tilePositions, LiquidId liquidId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  List<Vec2I> tilePositions;
  LiquidId liquidId;
};

struct RequestDropPacket : PacketBase<PacketType::RequestDrop> {
  RequestDropPacket();
  RequestDropPacket(EntityId dropEntityId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  EntityId dropEntityId;
};

struct SpawnEntityPacket : PacketBase<PacketType::SpawnEntity> {
  SpawnEntityPacket();
  SpawnEntityPacket(EntityType entityType, ByteArray storeData, ByteArray firstNetState);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  EntityType entityType;
  ByteArray storeData;
  ByteArray firstNetState;
};

struct ConnectWirePacket : PacketBase<PacketType::ConnectWire> {
  ConnectWirePacket();
  ConnectWirePacket(WireConnection outputConnection, WireConnection inputConnection);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  WireConnection outputConnection;
  WireConnection inputConnection;
};

struct DisconnectAllWiresPacket : PacketBase<PacketType::DisconnectAllWires> {
  DisconnectAllWiresPacket();
  DisconnectAllWiresPacket(Vec2I entityPosition, WireNode wireNode);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec2I entityPosition;
  WireNode wireNode;
};

struct WorldClientStateUpdatePacket : PacketBase<PacketType::WorldClientStateUpdate> {
  WorldClientStateUpdatePacket();
  WorldClientStateUpdatePacket(ByteArray const& worldClientStateDelta);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray worldClientStateDelta;
};

struct FindUniqueEntityPacket : PacketBase<PacketType::FindUniqueEntity> {
  FindUniqueEntityPacket();
  FindUniqueEntityPacket(String uniqueEntityId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String uniqueEntityId;
};

struct WorldStartAcknowledgePacket : PacketBase<PacketType::WorldStartAcknowledge> {
  WorldStartAcknowledgePacket();

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;
};
  
struct PingPacket : PacketBase<PacketType::Ping> {
  PingPacket();
  PingPacket(int64_t time);

  void readLegacy(DataStream& ds) override;
  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  int64_t time = 0;
};

struct EntityCreatePacket : PacketBase<PacketType::EntityCreate> {
  EntityCreatePacket();
  EntityCreatePacket(EntityType entityType, ByteArray storeData, ByteArray firstNetState, EntityId entityId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  EntityType entityType;
  ByteArray storeData;
  ByteArray firstNetState;
  EntityId entityId;
};

// All entity deltas will be sent at the same time for the same connection
// where they are master, any entities whose master is from that connection can
// be assumed to have produced a blank delta.
struct EntityUpdateSetPacket : PacketBase<PacketType::EntityUpdateSet> {
  EntityUpdateSetPacket(ConnectionId forConnection = ServerConnectionId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ConnectionId forConnection;
  HashMap<EntityId, ByteArray> deltas;
};

struct EntityDestroyPacket : PacketBase<PacketType::EntityDestroy> {
  EntityDestroyPacket();
  EntityDestroyPacket(EntityId entityId, ByteArray finalNetState, bool death);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  EntityId entityId;
  ByteArray finalNetState;
  // If true, the entity removal is due to death rather simply for example
  // going out of range of the entity monitoring window.
  bool death;
};

struct EntityInteractPacket : PacketBase<PacketType::EntityInteract> {
  EntityInteractPacket();
  EntityInteractPacket(InteractRequest interactRequest, Uuid requestId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  InteractRequest interactRequest;
  Uuid requestId;
};

struct EntityInteractResultPacket : PacketBase<PacketType::EntityInteractResult> {
  EntityInteractResultPacket();
  EntityInteractResultPacket(InteractAction action, Uuid requestId, EntityId sourceEntityId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  InteractAction action;
  Uuid requestId;
  EntityId sourceEntityId;
};

struct HitRequestPacket : PacketBase<PacketType::HitRequest> {
  HitRequestPacket();
  HitRequestPacket(RemoteHitRequest remoteHitRequest);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  RemoteHitRequest remoteHitRequest;
};

struct DamageRequestPacket : PacketBase<PacketType::DamageRequest> {
  DamageRequestPacket();
  DamageRequestPacket(RemoteDamageRequest remoteDamageRequest);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  RemoteDamageRequest remoteDamageRequest;
};

struct DamageNotificationPacket : PacketBase<PacketType::DamageNotification> {
  DamageNotificationPacket();
  DamageNotificationPacket(RemoteDamageNotification remoteDamageNotification);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  RemoteDamageNotification remoteDamageNotification;
};

struct EntityMessagePacket : PacketBase<PacketType::EntityMessage> {
  EntityMessagePacket();
  EntityMessagePacket(Variant<EntityId, String> entityId, String message, JsonArray args, Uuid uuid, ConnectionId fromConnection = ServerConnectionId);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  Variant<EntityId, String> entityId;
  String message;
  JsonArray args;
  Uuid uuid;
  ConnectionId fromConnection;
};

struct EntityMessageResponsePacket : PacketBase<PacketType::EntityMessageResponse> {
  EntityMessageResponsePacket();
  EntityMessageResponsePacket(Either<String, Json> response, Uuid uuid);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Either<String, Json> response;
  Uuid uuid;
};

struct UpdateWorldPropertiesPacket : PacketBase<PacketType::UpdateWorldProperties> {
  UpdateWorldPropertiesPacket();
  UpdateWorldPropertiesPacket(JsonObject const& updatedProperties);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  void readJson(Json const& json) override;
  Json writeJson() const override;

  JsonObject updatedProperties;
};

struct StepUpdatePacket : PacketBase<PacketType::StepUpdate> {
  StepUpdatePacket();
  StepUpdatePacket(double remoteTime);

  void readLegacy(DataStream& ds) override;
  void read(DataStream& ds) override;
  void writeLegacy(DataStream& ds) const override;
  void write(DataStream& ds) const override;

  double remoteTime;
};

struct SystemWorldStartPacket : PacketBase<PacketType::SystemWorldStart> {
  SystemWorldStartPacket();
  SystemWorldStartPacket(Vec3I location, List<ByteArray> objectStores, List<ByteArray> shipStores, pair<Uuid, SystemLocation> clientShip);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Vec3I location;
  List<ByteArray> objectStores;
  List<ByteArray> shipStores;
  pair<Uuid, SystemLocation> clientShip;
};

struct SystemWorldUpdatePacket : PacketBase<PacketType::SystemWorldUpdate> {
  SystemWorldUpdatePacket();
  SystemWorldUpdatePacket(HashMap<Uuid, ByteArray> objectUpdates, HashMap<Uuid, ByteArray> shipUpdates);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  HashMap<Uuid, ByteArray> objectUpdates;
  HashMap<Uuid, ByteArray> shipUpdates;
};

struct SystemObjectCreatePacket : PacketBase<PacketType::SystemObjectCreate> {
  SystemObjectCreatePacket();
  SystemObjectCreatePacket(ByteArray objectStore);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray objectStore;
};

struct SystemObjectDestroyPacket : PacketBase<PacketType::SystemObjectDestroy> {
  SystemObjectDestroyPacket();
  SystemObjectDestroyPacket(Uuid objectUuid);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Uuid objectUuid;
};

struct SystemShipCreatePacket : PacketBase<PacketType::SystemShipCreate> {
  SystemShipCreatePacket();
  SystemShipCreatePacket(ByteArray shipStore);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  ByteArray shipStore;
};

struct SystemShipDestroyPacket : PacketBase<PacketType::SystemShipDestroy> {
  SystemShipDestroyPacket();
  SystemShipDestroyPacket(Uuid shipUuid);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  Uuid shipUuid;
};

struct SystemObjectSpawnPacket : PacketBase<PacketType::SystemObjectSpawn> {
  SystemObjectSpawnPacket();
  SystemObjectSpawnPacket(String typeName, Uuid uuid, Maybe<Vec2F> position, JsonObject parameters);

  void read(DataStream& ds) override;
  void write(DataStream& ds) const override;

  String typeName;
  Uuid uuid;
  Maybe<Vec2F> position;
  JsonObject parameters;
};
}
