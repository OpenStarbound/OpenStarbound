#include "StarNetPackets.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

VersionNumber const StarProtocolVersion = 747;

EnumMap<PacketType> const PacketTypeNames{
  {PacketType::ProtocolRequest, "ProtocolRequest"},
  {PacketType::ProtocolResponse, "ProtocolResponse"},
  {PacketType::ServerDisconnect, "ServerDisconnect"},
  {PacketType::ConnectSuccess, "ConnectSuccess"},
  {PacketType::ConnectFailure, "ConnectFailure"},
  {PacketType::HandshakeChallenge, "HandshakeChallenge"},
  {PacketType::ChatReceive, "ChatReceive"},
  {PacketType::UniverseTimeUpdate, "UniverseTimeUpdate"},
  {PacketType::CelestialResponse, "CelestialResponse"},
  {PacketType::PlayerWarpResult, "PlayerWarpResult"},
  {PacketType::PlanetTypeUpdate, "PlanetTypeUpdate"},
  {PacketType::Pause, "Pause"},
  {PacketType::ServerInfo, "ServerInfo"},
  {PacketType::ClientConnect, "ClientConnect"},
  {PacketType::ClientDisconnectRequest, "ClientDisconnectRequest"},
  {PacketType::HandshakeResponse, "HandshakeResponse"},
  {PacketType::PlayerWarp, "PlayerWarp"},
  {PacketType::FlyShip, "FlyShip"},
  {PacketType::ChatSend, "ChatSend"},
  {PacketType::CelestialRequest, "CelestialRequest"},
  {PacketType::ClientContextUpdate, "ClientContextUpdate"},
  {PacketType::WorldStart, "WorldStart"},
  {PacketType::WorldStop, "WorldStop"},
  {PacketType::WorldLayoutUpdate, "WorldLayoutUpdate"},
  {PacketType::WorldParametersUpdate, "WorldParametersUpdate"},
  {PacketType::CentralStructureUpdate, "CentralStructureUpdate"},
  {PacketType::TileArrayUpdate, "TileArrayUpdate"},
  {PacketType::TileUpdate, "TileUpdate"},
  {PacketType::TileLiquidUpdate, "TileLiquidUpdate"},
  {PacketType::TileDamageUpdate, "TileDamageUpdate"},
  {PacketType::TileModificationFailure, "TileModificationFailure"},
  {PacketType::GiveItem, "GiveItem"},
  {PacketType::EnvironmentUpdate, "EnvironmentUpdate"},
  {PacketType::UpdateTileProtection, "UpdateTileProtection"},
  {PacketType::SetDungeonGravity, "SetDungeonGravity"},
  {PacketType::SetDungeonBreathable, "SetDungeonBreathable"},
  {PacketType::SetPlayerStart, "SetPlayerStart"},
  {PacketType::FindUniqueEntityResponse, "FindUniqueEntityResponse"},
  {PacketType::Pong, "Pong"},
  {PacketType::ModifyTileList, "ModifyTileList"},
  {PacketType::DamageTileGroup, "DamageTileGroup"},
  {PacketType::CollectLiquid, "CollectLiquid"},
  {PacketType::RequestDrop, "RequestDrop"},
  {PacketType::SpawnEntity, "SpawnEntity"},
  {PacketType::ConnectWire, "ConnectWire"},
  {PacketType::DisconnectAllWires, "DisconnectAllWires"},
  {PacketType::WorldClientStateUpdate, "WorldClientStateUpdate"},
  {PacketType::FindUniqueEntity, "FindUniqueEntity"},
  {PacketType::WorldStartAcknowledge, "WorldStartAcknowledge"},
  {PacketType::Ping, "Ping"},
  {PacketType::EntityCreate, "EntityCreate"},
  {PacketType::EntityUpdateSet, "EntityUpdate"},
  {PacketType::EntityDestroy, "EntityDestroy"},
  {PacketType::EntityInteract, "EntityInteract"},
  {PacketType::EntityInteractResult, "EntityInteractResult"},
  {PacketType::HitRequest, "HitRequest"},
  {PacketType::DamageRequest, "DamageRequest"},
  {PacketType::DamageNotification, "DamageNotification"},
  {PacketType::EntityMessage, "EntityMessage"},
  {PacketType::EntityMessageResponse, "EntityMessageResponse"},
  {PacketType::UpdateWorldProperties, "UpdateWorldProperties"},
  {PacketType::StepUpdate, "StepUpdate"},
  {PacketType::SystemWorldStart, "SystemWorldStart"},
  {PacketType::SystemWorldUpdate, "SystemWorldUpdate"},
  {PacketType::SystemObjectCreate, "SystemObjectCreate"},
  {PacketType::SystemObjectDestroy, "SystemObjectDestroy"},
  {PacketType::SystemShipCreate, "SystemShipCreate"},
  {PacketType::SystemShipDestroy, "SystemShipDestroy"},
  {PacketType::SystemObjectSpawn, "SystemObjectSpawn"}
};

Packet::~Packet() {}

void Packet::readLegacy(DataStream& ds) {
  read(ds);
}
void Packet::writeLegacy(DataStream& ds) const {
  write(ds);
}

PacketCompressionMode Packet::compressionMode() const
  { return m_compressionMode; }
void Packet::setCompressionMode(PacketCompressionMode compressionMode)
  { m_compressionMode = compressionMode; }

PacketPtr createPacket(PacketType type) {
  switch (type) {
    case PacketType::ProtocolRequest: return make_shared<ProtocolRequestPacket>();
    case PacketType::ProtocolResponse: return make_shared<ProtocolResponsePacket>();
    case PacketType::ServerDisconnect: return make_shared<ServerDisconnectPacket>();
    case PacketType::ConnectSuccess: return make_shared<ConnectSuccessPacket>();
    case PacketType::ConnectFailure: return make_shared<ConnectFailurePacket>();
    case PacketType::HandshakeChallenge: return make_shared<HandshakeChallengePacket>();
    case PacketType::ChatReceive: return make_shared<ChatReceivePacket>();
    case PacketType::UniverseTimeUpdate: return make_shared<UniverseTimeUpdatePacket>();
    case PacketType::CelestialResponse: return make_shared<CelestialResponsePacket>();
    case PacketType::PlayerWarpResult: return make_shared<PlayerWarpResultPacket>();
    case PacketType::PlanetTypeUpdate: return make_shared<PlanetTypeUpdatePacket>();
    case PacketType::Pause: return make_shared<PausePacket>();
    case PacketType::ServerInfo: return make_shared<ServerInfoPacket>();
    case PacketType::ClientConnect: return make_shared<ClientConnectPacket>();
    case PacketType::ClientDisconnectRequest: return make_shared<ClientDisconnectRequestPacket>();
    case PacketType::HandshakeResponse: return make_shared<HandshakeResponsePacket>();
    case PacketType::PlayerWarp: return make_shared<PlayerWarpPacket>();
    case PacketType::FlyShip: return make_shared<FlyShipPacket>();
    case PacketType::ChatSend: return make_shared<ChatSendPacket>();
    case PacketType::CelestialRequest: return make_shared<CelestialRequestPacket>();
    case PacketType::ClientContextUpdate: return make_shared<ClientContextUpdatePacket>();
    case PacketType::WorldStart: return make_shared<WorldStartPacket>();
    case PacketType::WorldStop: return make_shared<WorldStopPacket>();
    case PacketType::WorldLayoutUpdate: return make_shared<WorldLayoutUpdatePacket>();
    case PacketType::WorldParametersUpdate: return make_shared<WorldParametersUpdatePacket>();
    case PacketType::CentralStructureUpdate: return make_shared<CentralStructureUpdatePacket>();
    case PacketType::TileArrayUpdate: return make_shared<TileArrayUpdatePacket>();
    case PacketType::TileUpdate: return make_shared<TileUpdatePacket>();
    case PacketType::TileLiquidUpdate: return make_shared<TileLiquidUpdatePacket>();
    case PacketType::TileDamageUpdate: return make_shared<TileDamageUpdatePacket>();
    case PacketType::TileModificationFailure: return make_shared<TileModificationFailurePacket>();
    case PacketType::GiveItem: return make_shared<GiveItemPacket>();
    case PacketType::EnvironmentUpdate: return make_shared<EnvironmentUpdatePacket>();
    case PacketType::UpdateTileProtection: return make_shared<UpdateTileProtectionPacket>();
    case PacketType::SetDungeonGravity: return make_shared<SetDungeonGravityPacket>();
    case PacketType::SetDungeonBreathable: return make_shared<SetDungeonBreathablePacket>();
    case PacketType::SetPlayerStart: return make_shared<SetPlayerStartPacket>();
    case PacketType::FindUniqueEntityResponse: return make_shared<FindUniqueEntityResponsePacket>();
    case PacketType::Pong: return make_shared<PongPacket>();
    case PacketType::ModifyTileList: return make_shared<ModifyTileListPacket>();
    case PacketType::DamageTileGroup: return make_shared<DamageTileGroupPacket>();
    case PacketType::CollectLiquid: return make_shared<CollectLiquidPacket>();
    case PacketType::RequestDrop: return make_shared<RequestDropPacket>();
    case PacketType::SpawnEntity: return make_shared<SpawnEntityPacket>();
    case PacketType::ConnectWire: return make_shared<ConnectWirePacket>();
    case PacketType::DisconnectAllWires: return make_shared<DisconnectAllWiresPacket>();
    case PacketType::WorldClientStateUpdate: return make_shared<WorldClientStateUpdatePacket>();
    case PacketType::FindUniqueEntity: return make_shared<FindUniqueEntityPacket>();
    case PacketType::WorldStartAcknowledge: return make_shared<WorldStartAcknowledgePacket>();
    case PacketType::Ping: return make_shared<PingPacket>();
    case PacketType::EntityCreate: return make_shared<EntityCreatePacket>();
    case PacketType::EntityUpdateSet: return make_shared<EntityUpdateSetPacket>();
    case PacketType::EntityDestroy: return make_shared<EntityDestroyPacket>();
    case PacketType::EntityInteract: return make_shared<EntityInteractPacket>();
    case PacketType::EntityInteractResult: return make_shared<EntityInteractResultPacket>();
    case PacketType::HitRequest: return make_shared<HitRequestPacket>();
    case PacketType::DamageRequest: return make_shared<DamageRequestPacket>();
    case PacketType::DamageNotification: return make_shared<DamageNotificationPacket>();
    case PacketType::EntityMessage: return make_shared<EntityMessagePacket>();
    case PacketType::EntityMessageResponse: return make_shared<EntityMessageResponsePacket>();
    case PacketType::UpdateWorldProperties: return make_shared<UpdateWorldPropertiesPacket>();
    case PacketType::StepUpdate: return make_shared<StepUpdatePacket>();
    case PacketType::SystemWorldStart: return make_shared<SystemWorldStartPacket>();
    case PacketType::SystemWorldUpdate: return make_shared<SystemWorldUpdatePacket>();
    case PacketType::SystemObjectCreate: return make_shared<SystemObjectCreatePacket>();
    case PacketType::SystemObjectDestroy: return make_shared<SystemObjectDestroyPacket>();
    case PacketType::SystemShipCreate: return make_shared<SystemShipCreatePacket>();
    case PacketType::SystemShipDestroy: return make_shared<SystemShipDestroyPacket>();
    case PacketType::SystemObjectSpawn: return make_shared<SystemObjectSpawnPacket>();
    default:
      throw StarPacketException(strf("Unrecognized packet type {}", (unsigned int)type));
  }
}

PacketPtr createPacket(PacketType type, JsonArray const& args) {
  switch (type) {
    case PacketType::Pause: return make_shared<PausePacket>(args[0].toBool());
    case PacketType::ServerInfo: return make_shared<ServerInfoPacket>(args[0].toUInt(), args[1].toUInt());
    case PacketType::GiveItem: return make_shared<GiveItemPacket>(ItemDescriptor(args[0]));
    case PacketType::UpdateTileProtection: return make_shared<UpdateTileProtectionPacket>(args[0].toUInt(), args[1].toBool());
    case PacketType::SetDungeonGravity: return make_shared<SetDungeonGravityPacket>(args[0].toUInt(), args[0].toFloat());
    case PacketType::SetDungeonBreathable: return make_shared<SetDungeonBreathablePacket>(args[0].toUInt(), args[0].toBool());
    case PacketType::SetPlayerStart: return make_shared<SetPlayerStartPacket>(Vec2F{args[0].toArray()[0].toFloat(), args[0].toArray()[0].toFloat()}, args[1].toBool());
    case PacketType::EntityMessage: return make_shared<EntityMessagePacket>(EntityId(args[0].toUInt()), args[1].toString(), args[2].toArray(), Uuid(args[3].toString()));
    case PacketType::UpdateWorldProperties: return make_shared<UpdateWorldPropertiesPacket>(args[0].toObject());
    default:
      throw StarPacketException(strf("Unrecognized packet type {}", (unsigned int)type));
  }
}

ProtocolRequestPacket::ProtocolRequestPacket()
  : requestProtocolVersion(0) {}

ProtocolRequestPacket::ProtocolRequestPacket(VersionNumber requestProtocolVersion)
  : requestProtocolVersion(requestProtocolVersion) {}

void ProtocolRequestPacket::read(DataStream& ds) {
  ds.read(requestProtocolVersion);
}

void ProtocolRequestPacket::write(DataStream& ds) const {
  ds.write(requestProtocolVersion);
}

ProtocolResponsePacket::ProtocolResponsePacket(bool allowed)
  : allowed(allowed) {}

void ProtocolResponsePacket::read(DataStream& ds) {
  ds.read(allowed);
}

void ProtocolResponsePacket::write(DataStream& ds) const {
  ds.write(allowed);
}

ConnectSuccessPacket::ConnectSuccessPacket() {}

ConnectSuccessPacket::ConnectSuccessPacket(
    ConnectionId clientId, Uuid serverUuid, CelestialBaseInformation celestialInformation)
  : clientId(clientId), serverUuid(std::move(serverUuid)), celestialInformation(std::move(celestialInformation)) {}

void ConnectSuccessPacket::read(DataStream& ds) {
  ds.vuread(clientId);
  ds.read(serverUuid);
  ds.read(celestialInformation);
}

void ConnectSuccessPacket::write(DataStream& ds) const {
  ds.vuwrite(clientId);
  ds.write(serverUuid);
  ds.write(celestialInformation);
}

ConnectFailurePacket::ConnectFailurePacket() {}

ConnectFailurePacket::ConnectFailurePacket(String reason) : reason(std::move(reason)) {}

void ConnectFailurePacket::read(DataStream& ds) {
  ds.read(reason);
}

void ConnectFailurePacket::write(DataStream& ds) const {
  ds.write(reason);
}

HandshakeChallengePacket::HandshakeChallengePacket() {}

HandshakeChallengePacket::HandshakeChallengePacket(ByteArray const& passwordSalt) : passwordSalt(passwordSalt) {}

void HandshakeChallengePacket::read(DataStream& ds) {
  ds.read(passwordSalt);
}

void HandshakeChallengePacket::write(DataStream& ds) const {
  ds.write(passwordSalt);
}

ChatReceivePacket::ChatReceivePacket() {}

ChatReceivePacket::ChatReceivePacket(ChatReceivedMessage receivedMessage) : receivedMessage(receivedMessage) {}

void ChatReceivePacket::read(DataStream& ds) {
  ds.read(receivedMessage);
}

void ChatReceivePacket::write(DataStream& ds) const {
  ds.write(receivedMessage);
}

UniverseTimeUpdatePacket::UniverseTimeUpdatePacket() {
  universeTime = 0;
}

UniverseTimeUpdatePacket::UniverseTimeUpdatePacket(double universeTime) : universeTime(universeTime) {}

void UniverseTimeUpdatePacket::read(DataStream& ds) {
  ds.vfread(universeTime, 0.05);
}

void UniverseTimeUpdatePacket::write(DataStream& ds) const {
  ds.vfwrite(universeTime, 0.05);
}

CelestialResponsePacket::CelestialResponsePacket() {}

CelestialResponsePacket::CelestialResponsePacket(List<CelestialResponse> responses) : responses(std::move(responses)) {}

void CelestialResponsePacket::read(DataStream& ds) {
  ds.read(responses);
}

void CelestialResponsePacket::write(DataStream& ds) const {
  ds.write(responses);
}

PlayerWarpResultPacket::PlayerWarpResultPacket() : warpActionInvalid(false) {}

PlayerWarpResultPacket::PlayerWarpResultPacket(bool success, WarpAction warpAction, bool warpActionInvalid)
  : success(success), warpAction(std::move(warpAction)), warpActionInvalid(warpActionInvalid) {}

void PlayerWarpResultPacket::read(DataStream& ds) {
  ds.read(success);
  ds.read(warpAction);
  ds.read(warpActionInvalid);
}

void PlayerWarpResultPacket::write(DataStream& ds) const {
  ds.write(success);
  ds.write(warpAction);
  ds.write(warpActionInvalid);
}

PlanetTypeUpdatePacket::PlanetTypeUpdatePacket() {}

PlanetTypeUpdatePacket::PlanetTypeUpdatePacket(CelestialCoordinate coordinate)
  : coordinate(coordinate) {}

void PlanetTypeUpdatePacket::read(DataStream& ds) {
  ds.read(coordinate);
}

void PlanetTypeUpdatePacket::write(DataStream& ds) const {
  ds.write(coordinate);
}

PausePacket::PausePacket() {}

PausePacket::PausePacket(bool pause, float timescale) : pause(pause), timescale(timescale) {}

void PausePacket::readLegacy(DataStream& ds) {
  ds.read(pause);
  timescale = 1.0f;
}

void PausePacket::read(DataStream& ds) {
  readLegacy(ds);
  ds.read(timescale);
}

void PausePacket::writeLegacy(DataStream& ds) const {
  ds.write(pause);
}

void PausePacket::write(DataStream& ds) const {
  writeLegacy(ds);
  ds.write(timescale);
}

ServerInfoPacket::ServerInfoPacket() {}

ServerInfoPacket::ServerInfoPacket(uint16_t players, uint16_t maxPlayers) :
  players(players),
  maxPlayers(maxPlayers) {}

void ServerInfoPacket::read(DataStream& ds)
{
  ds.read(players);
  ds.read(maxPlayers);
}

void ServerInfoPacket::write(DataStream& ds) const
{
  ds.write(players);
  ds.write(maxPlayers);
}

ClientConnectPacket::ClientConnectPacket() {}

ClientConnectPacket::ClientConnectPacket(ByteArray assetsDigest, bool allowAssetsMismatch, Uuid playerUuid,
    String playerName, String playerSpecies, WorldChunks shipChunks, ShipUpgrades shipUpgrades,
    bool introComplete, String account)
  : assetsDigest(std::move(assetsDigest)), allowAssetsMismatch(allowAssetsMismatch), playerUuid(std::move(playerUuid)),
    playerName(std::move(playerName)), playerSpecies(std::move(playerSpecies)), shipChunks(std::move(shipChunks)),
    shipUpgrades(std::move(shipUpgrades)), introComplete(std::move(introComplete)), account(std::move(account)) {}

void ClientConnectPacket::readLegacy(DataStream& ds) {
  ds.read(assetsDigest);
  ds.read(allowAssetsMismatch);
  ds.read(playerUuid);
  ds.read(playerName);
  ds.read(playerSpecies);
  ds.read(shipChunks);
  ds.read(shipUpgrades);
  ds.read(introComplete);
  ds.read(account);
}

void ClientConnectPacket::read(DataStream& ds) {
  readLegacy(ds);
  ds.read(info);
}

void ClientConnectPacket::writeLegacy(DataStream& ds) const {
  ds.write(assetsDigest);
  ds.write(allowAssetsMismatch);
  ds.write(playerUuid);
  ds.write(playerName);
  ds.write(playerSpecies);
  ds.write(shipChunks);
  ds.write(shipUpgrades);
  ds.write(introComplete);
  ds.write(account);
}

void ClientConnectPacket::write(DataStream& ds) const {
  writeLegacy(ds);
  ds.write(info);
}

ClientDisconnectRequestPacket::ClientDisconnectRequestPacket() {}

void ClientDisconnectRequestPacket::read(DataStream& ds) {
  // Packets cannot be empty due to the way packet serialization is handled.
  ds.read<uint8_t>();
}

void ClientDisconnectRequestPacket::write(DataStream& ds) const {
  // Packets cannot be empty due to the way packet serialization is handled.
  ds.write<uint8_t>(0);
}

HandshakeResponsePacket::HandshakeResponsePacket() {}

HandshakeResponsePacket::HandshakeResponsePacket(ByteArray const& passHash) : passHash(passHash) {}

void HandshakeResponsePacket::read(DataStream& ds) {
  ds.read(passHash);
}

void HandshakeResponsePacket::write(DataStream& ds) const {
  ds.write(passHash);
}

PlayerWarpPacket::PlayerWarpPacket() {}

PlayerWarpPacket::PlayerWarpPacket(WarpAction action, bool deploy) : action(std::move(action)), deploy(std::move(deploy)) {}

void PlayerWarpPacket::read(DataStream& ds) {
  ds.read(action);
  ds.read(deploy);
}

void PlayerWarpPacket::write(DataStream& ds) const {
  ds.write(action);
  ds.write(deploy);
}

FlyShipPacket::FlyShipPacket() {}

FlyShipPacket::FlyShipPacket(Vec3I system, SystemLocation location) : system(std::move(system)), location(std::move(location)) {}

void FlyShipPacket::read(DataStream& ds) {
  ds.read(system);
  ds.read(location);
}

void FlyShipPacket::write(DataStream& ds) const {
  ds.write(system);
  ds.write(location);
}

ChatSendPacket::ChatSendPacket() : sendMode(ChatSendMode::Broadcast) {}

ChatSendPacket::ChatSendPacket(String text, ChatSendMode sendMode) : text(std::move(text)), sendMode(sendMode) {}

void ChatSendPacket::read(DataStream& ds) {
  ds.read(text);
  ds.read(sendMode);
}

void ChatSendPacket::write(DataStream& ds) const {
  ds.write(text);
  ds.write(sendMode);
}

CelestialRequestPacket::CelestialRequestPacket() {}

CelestialRequestPacket::CelestialRequestPacket(List<CelestialRequest> requests) : requests(std::move(requests)) {}

void CelestialRequestPacket::read(DataStream& ds) {
  ds.read(requests);
}

void CelestialRequestPacket::write(DataStream& ds) const {
  ds.write(requests);
}

ClientContextUpdatePacket::ClientContextUpdatePacket() {}

ClientContextUpdatePacket::ClientContextUpdatePacket(ByteArray updateData) : updateData(std::move(updateData)) {}

void ClientContextUpdatePacket::read(DataStream& ds) {
  ds.read(updateData);
}

void ClientContextUpdatePacket::write(DataStream& ds) const {
  ds.write(updateData);
}

WorldStartPacket::WorldStartPacket() : clientId(), localInterpolationMode() {}

void WorldStartPacket::read(DataStream& ds) {
  ds.read(templateData);
  ds.read(skyData);
  ds.read(weatherData);
  ds.read(playerStart);
  ds.read(playerRespawn);
  ds.read(respawnInWorld);
  ds.read(worldProperties);
  ds.read(dungeonIdGravity);
  ds.read(dungeonIdBreathable);
  ds.read(protectedDungeonIds);
  ds.read(clientId);
  ds.read(localInterpolationMode);
}

void WorldStartPacket::write(DataStream& ds) const {
  ds.write(templateData);
  ds.write(skyData);
  ds.write(weatherData);
  ds.write(playerStart);
  ds.write(playerRespawn);
  ds.write(respawnInWorld);
  ds.write(worldProperties);
  ds.write(dungeonIdGravity);
  ds.write(dungeonIdBreathable);
  ds.write(protectedDungeonIds);
  ds.write(clientId);
  ds.write(localInterpolationMode);
}

WorldStopPacket::WorldStopPacket() {}

WorldStopPacket::WorldStopPacket(String const& reason) : reason(reason) {}

void WorldStopPacket::read(DataStream& ds) {
  ds.read(reason);
}

void WorldStopPacket::write(DataStream& ds) const {
  ds.write(reason);
}

WorldLayoutUpdatePacket::WorldLayoutUpdatePacket() {}

WorldLayoutUpdatePacket::WorldLayoutUpdatePacket(Json const& layoutData) : layoutData(layoutData) {}

void WorldLayoutUpdatePacket::read(DataStream& ds) {
  ds.read(layoutData);
}

void WorldLayoutUpdatePacket::write(DataStream& ds) const {
  ds.write(layoutData);
}

WorldParametersUpdatePacket::WorldParametersUpdatePacket() {}

WorldParametersUpdatePacket::WorldParametersUpdatePacket(ByteArray const& parametersData) : parametersData(parametersData) {}

void WorldParametersUpdatePacket::read(DataStream& ds) {
  ds.read(parametersData);
}

void WorldParametersUpdatePacket::write(DataStream& ds) const {
  ds.write(parametersData);
}

CentralStructureUpdatePacket::CentralStructureUpdatePacket() {}

CentralStructureUpdatePacket::CentralStructureUpdatePacket(Json structureData) : structureData(std::move(structureData)) {}

void CentralStructureUpdatePacket::read(DataStream& ds) {
  ds.read(structureData);
}

void CentralStructureUpdatePacket::write(DataStream& ds) const {
  ds.write(structureData);
}

TileArrayUpdatePacket::TileArrayUpdatePacket() {}

void TileArrayUpdatePacket::read(DataStream& ds) {
  ds.viread(min[0]);
  ds.viread(min[1]);

  size_t width, height;
  ds.vuread(width);
  ds.vuread(height);
  array.resize(width, height);
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x)
      ds.read(array(x, y));
  }
}

void TileArrayUpdatePacket::write(DataStream& ds) const {
  ds.viwrite(min[0]);
  ds.viwrite(min[1]);
  ds.vuwrite(array.size(0));
  ds.vuwrite(array.size(1));
  for (size_t y = 0; y < array.size(1); ++y) {
    for (size_t x = 0; x < array.size(0); ++x)
      ds.write(array(x, y));
  }
}

void TileUpdatePacket::read(DataStream& ds) {
  ds.viread(position[0]);
  ds.viread(position[1]);
  ds.read(tile);
}

void TileUpdatePacket::write(DataStream& ds) const {
  ds.viwrite(position[0]);
  ds.viwrite(position[1]);
  ds.write(tile);
}

TileLiquidUpdatePacket::TileLiquidUpdatePacket() {}

TileLiquidUpdatePacket::TileLiquidUpdatePacket(Vec2I const& position, LiquidNetUpdate liquidUpdate)
  : position(position), liquidUpdate(liquidUpdate) {}

void TileLiquidUpdatePacket::read(DataStream& ds) {
  ds.viread(position[0]);
  ds.viread(position[1]);
  ds.read(liquidUpdate.liquid);
  ds.read(liquidUpdate.level);
}

void TileLiquidUpdatePacket::write(DataStream& ds) const {
  ds.viwrite(position[0]);
  ds.viwrite(position[1]);
  ds.write(liquidUpdate.liquid);
  ds.write(liquidUpdate.level);
}

TileDamageUpdatePacket::TileDamageUpdatePacket() : layer(TileLayer::Foreground) {}

TileDamageUpdatePacket::TileDamageUpdatePacket(
    Vec2I const& position, TileLayer layer, TileDamageStatus const& tileDamage)
  : position(position), layer(layer), tileDamage(tileDamage) {}

void TileDamageUpdatePacket::read(DataStream& ds) {
  ds.read(position);
  ds.read(layer);
  ds.read(tileDamage);
}

void TileDamageUpdatePacket::write(DataStream& ds) const {
  ds.write(position);
  ds.write(layer);
  ds.write(tileDamage);
}

TileModificationFailurePacket::TileModificationFailurePacket() {}

TileModificationFailurePacket::TileModificationFailurePacket(TileModificationList modifications)
  : modifications(modifications) {}

void TileModificationFailurePacket::read(DataStream& ds) {
  ds.readContainer(modifications);
}

void TileModificationFailurePacket::write(DataStream& ds) const {
  ds.writeContainer(modifications);
}

GiveItemPacket::GiveItemPacket() {}

GiveItemPacket::GiveItemPacket(ItemDescriptor const& item) : item(item) {}

void GiveItemPacket::read(DataStream& ds) {
  ds.read(item);
}

void GiveItemPacket::write(DataStream& ds) const {
  ds.write(item);
}

EnvironmentUpdatePacket::EnvironmentUpdatePacket() {}

EnvironmentUpdatePacket::EnvironmentUpdatePacket(ByteArray skyDelta, ByteArray weatherDelta)
  : skyDelta(std::move(skyDelta)), weatherDelta(std::move(weatherDelta)) {}

void EnvironmentUpdatePacket::read(DataStream& ds) {
  ds.read(skyDelta);
  ds.read(weatherDelta);
}

void EnvironmentUpdatePacket::write(DataStream& ds) const {
  ds.write(skyDelta);
  ds.write(weatherDelta);
}

ModifyTileListPacket::ModifyTileListPacket() : allowEntityOverlap() {}

ModifyTileListPacket::ModifyTileListPacket(TileModificationList modifications, bool allowEntityOverlap)
  : modifications(modifications), allowEntityOverlap(allowEntityOverlap) {}

void ModifyTileListPacket::read(DataStream& ds) {
  ds.readContainer(modifications);
  ds.read(allowEntityOverlap);
}

void ModifyTileListPacket::write(DataStream& ds) const {
  ds.writeContainer(modifications);
  ds.write(allowEntityOverlap);
}

DamageTileGroupPacket::DamageTileGroupPacket() : layer(TileLayer::Foreground) {}

DamageTileGroupPacket::DamageTileGroupPacket(
    List<Vec2I> tilePositions, TileLayer layer, Vec2F sourcePosition, TileDamage tileDamage, Maybe<EntityId> sourceEntity)
  : tilePositions(std::move(tilePositions)), layer(layer), sourcePosition(sourcePosition), tileDamage(std::move(tileDamage)), sourceEntity(std::move(sourceEntity)) {}

void DamageTileGroupPacket::read(DataStream& ds) {
  ds.readContainer(tilePositions);
  ds.read(layer);
  ds.read(sourcePosition);
  ds.read(tileDamage);
  ds.read(sourceEntity);
}

void DamageTileGroupPacket::write(DataStream& ds) const {
  ds.writeContainer(tilePositions);
  ds.write(layer);
  ds.write(sourcePosition);
  ds.write(tileDamage);
  ds.write(sourceEntity);
}

CollectLiquidPacket::CollectLiquidPacket() {}

CollectLiquidPacket::CollectLiquidPacket(List<Vec2I> tilePositions, LiquidId liquidId)
  : tilePositions(std::move(tilePositions)), liquidId(liquidId) {}

void CollectLiquidPacket::read(DataStream& ds) {
  ds.readContainer(tilePositions);
  ds.read(liquidId);
}

void CollectLiquidPacket::write(DataStream& ds) const {
  ds.writeContainer(tilePositions);
  ds.write(liquidId);
}

RequestDropPacket::RequestDropPacket() {
  dropEntityId = NullEntityId;
}

RequestDropPacket::RequestDropPacket(EntityId dropEntityId) : dropEntityId(dropEntityId) {}

void RequestDropPacket::read(DataStream& ds) {
  ds.viread(dropEntityId);
}

void RequestDropPacket::write(DataStream& ds) const {
  ds.viwrite(dropEntityId);
}

SpawnEntityPacket::SpawnEntityPacket() {}

SpawnEntityPacket::SpawnEntityPacket(EntityType entityType, ByteArray storeData, ByteArray firstNetState)
  : entityType(entityType), storeData(std::move(storeData)), firstNetState(std::move(firstNetState)) {}

void SpawnEntityPacket::read(DataStream& ds) {
  ds.read(entityType);
  ds.read(storeData);
  ds.read(firstNetState);
}

void SpawnEntityPacket::write(DataStream& ds) const {
  ds.write(entityType);
  ds.write(storeData);
  ds.write(firstNetState);
}

EntityInteractPacket::EntityInteractPacket() {}

EntityInteractPacket::EntityInteractPacket(InteractRequest interactRequest, Uuid requestId)
  : interactRequest(interactRequest), requestId(requestId) {}

void EntityInteractPacket::read(DataStream& ds) {
  ds.read(interactRequest);
  ds.read(requestId);
}

void EntityInteractPacket::write(DataStream& ds) const {
  ds.write(interactRequest);
  ds.write(requestId);
}

EntityInteractResultPacket::EntityInteractResultPacket() {}

EntityInteractResultPacket::EntityInteractResultPacket(InteractAction action, Uuid requestId, EntityId sourceEntityId)
  : action(action), requestId(requestId), sourceEntityId(sourceEntityId) {}

void EntityInteractResultPacket::read(DataStream& ds) {
  ds.read(action);
  ds.read(requestId);
  ds.read(sourceEntityId);
}

void EntityInteractResultPacket::write(DataStream& ds) const {
  ds.write(action);
  ds.write(requestId);
  ds.write(sourceEntityId);
}

EntityCreatePacket::EntityCreatePacket() {
  entityId = NullEntityId;
}

ServerDisconnectPacket::ServerDisconnectPacket() {}

ServerDisconnectPacket::ServerDisconnectPacket(String reason) : reason(std::move(reason)) {}

void ServerDisconnectPacket::read(DataStream& ds) {
  ds.read(reason);
}

void ServerDisconnectPacket::write(DataStream& ds) const {
  ds.write(reason);
}

ConnectWirePacket::ConnectWirePacket() {}

ConnectWirePacket::ConnectWirePacket(WireConnection outputConnection, WireConnection inputConnection)
  : outputConnection(outputConnection), inputConnection(inputConnection) {}

void ConnectWirePacket::read(DataStream& ds) {
  ds.read(outputConnection);
  ds.read(inputConnection);
}

void ConnectWirePacket::write(DataStream& ds) const {
  ds.write(outputConnection);
  ds.write(inputConnection);
}

DisconnectAllWiresPacket::DisconnectAllWiresPacket() {}

DisconnectAllWiresPacket::DisconnectAllWiresPacket(Vec2I entityPosition, WireNode wireNode)
  : entityPosition(entityPosition), wireNode(wireNode) {}

void DisconnectAllWiresPacket::read(DataStream& ds) {
  ds.viread(entityPosition[0]);
  ds.viread(entityPosition[1]);
  ds.read(wireNode);
}

void DisconnectAllWiresPacket::write(DataStream& ds) const {
  ds.viwrite(entityPosition[0]);
  ds.viwrite(entityPosition[1]);
  ds.write(wireNode);
}

WorldClientStateUpdatePacket::WorldClientStateUpdatePacket() {}

WorldClientStateUpdatePacket::WorldClientStateUpdatePacket(ByteArray const& worldClientStateDelta)
  : worldClientStateDelta(worldClientStateDelta) {}

void WorldClientStateUpdatePacket::read(DataStream& ds) {
  ds.read(worldClientStateDelta);
}

void WorldClientStateUpdatePacket::write(DataStream& ds) const {
  ds.write(worldClientStateDelta);
}

FindUniqueEntityPacket::FindUniqueEntityPacket() {}

FindUniqueEntityPacket::FindUniqueEntityPacket(String uniqueEntityId)
  : uniqueEntityId(std::move(uniqueEntityId)) {}

void FindUniqueEntityPacket::read(DataStream& ds) {
  ds.read(uniqueEntityId);
}

void FindUniqueEntityPacket::write(DataStream& ds) const {
  ds.write(uniqueEntityId);
}

WorldStartAcknowledgePacket::WorldStartAcknowledgePacket() {}

void WorldStartAcknowledgePacket::read(DataStream& ds) {
  // Packets can't be empty, read the trash data
  ds.read<bool>();
}

void WorldStartAcknowledgePacket::write(DataStream& ds) const {
  // Packets can't be empty, write some trash data
  ds.write<bool>(false);
}

PingPacket::PingPacket() {}

void PingPacket::read(DataStream& ds) {
  // Packets can't be empty, read the trash data
  ds.read<bool>();
}

void PingPacket::write(DataStream& ds) const {
  // Packets can't be empty, write some trash data
  ds.write(false);
}

EntityCreatePacket::EntityCreatePacket(EntityType entityType, ByteArray storeData, ByteArray firstNetState, EntityId entityId)
  : entityType(entityType), storeData(std::move(storeData)), firstNetState(std::move(firstNetState)), entityId(entityId) {}

void EntityCreatePacket::read(DataStream& ds) {
  ds.read(entityType);
  ds.read(storeData);
  ds.read(firstNetState);
  ds.viread(entityId);
}

void EntityCreatePacket::write(DataStream& ds) const {
  ds.write(entityType);
  ds.write(storeData);
  ds.write(firstNetState);
  ds.viwrite(entityId);
}

EntityUpdateSetPacket::EntityUpdateSetPacket(ConnectionId forConnection) : forConnection(forConnection) {}

void EntityUpdateSetPacket::read(DataStream& ds) {
  ds.vuread(forConnection);
  ds.readMapContainer(deltas,
      [](DataStream& ds, EntityId& entityId, ByteArray& delta) {
        ds.viread(entityId);
        ds.read(delta);
      });
}

void EntityUpdateSetPacket::write(DataStream& ds) const {
  ds.vuwrite(forConnection);
  ds.writeMapContainer(deltas, [](DataStream& ds, EntityId const& entityId, ByteArray const& delta) {
      ds.viwrite(entityId);
      ds.write(delta);
    });
}

EntityDestroyPacket::EntityDestroyPacket() {
  entityId = NullEntityId;
  death = false;
}

EntityDestroyPacket::EntityDestroyPacket(EntityId entityId, ByteArray finalNetState, bool death)
  : entityId(entityId), finalNetState(std::move(finalNetState)), death(death) {}

void EntityDestroyPacket::read(DataStream& ds) {
  ds.viread(entityId);
  ds.read(finalNetState);
  ds.read(death);
}

void EntityDestroyPacket::write(DataStream& ds) const {
  ds.viwrite(entityId);
  ds.write(finalNetState);
  ds.write(death);
}

HitRequestPacket::HitRequestPacket() {}

HitRequestPacket::HitRequestPacket(RemoteHitRequest remoteHitRequest) : remoteHitRequest(std::move(remoteHitRequest)) {}

void HitRequestPacket::read(DataStream& ds) {
  ds.read(remoteHitRequest);
}

void HitRequestPacket::write(DataStream& ds) const {
  ds.write(remoteHitRequest);
}

DamageRequestPacket::DamageRequestPacket() {}

DamageRequestPacket::DamageRequestPacket(RemoteDamageRequest remoteDamageRequest)
  : remoteDamageRequest(std::move(remoteDamageRequest)) {}

void DamageRequestPacket::read(DataStream& ds) {
  ds.read(remoteDamageRequest);
}

void DamageRequestPacket::write(DataStream& ds) const {
  ds.write(remoteDamageRequest);
}

DamageNotificationPacket::DamageNotificationPacket() {}

DamageNotificationPacket::DamageNotificationPacket(RemoteDamageNotification remoteDamageNotification)
  : remoteDamageNotification(std::move(remoteDamageNotification)) {}

void DamageNotificationPacket::read(DataStream& ds) {
  ds.read(remoteDamageNotification);
}

void DamageNotificationPacket::write(DataStream& ds) const {
  ds.write(remoteDamageNotification);
}

EntityMessagePacket::EntityMessagePacket() {}

EntityMessagePacket::EntityMessagePacket(Variant<EntityId, String> entityId, String message, JsonArray args, Uuid uuid, ConnectionId fromConnection)
  : entityId(entityId), message(std::move(message)), args(std::move(args)), uuid(uuid), fromConnection(fromConnection) {}

void EntityMessagePacket::read(DataStream& ds) {
  ds.read(entityId);
  ds.read(message);
  ds.read(args);
  ds.read(uuid);
  ds.read(fromConnection);
}

void EntityMessagePacket::write(DataStream& ds) const {
  ds.write(entityId);
  ds.write(message);
  ds.write(args);
  ds.write(uuid);
  ds.write(fromConnection);
}

EntityMessageResponsePacket::EntityMessageResponsePacket() {}

EntityMessageResponsePacket::EntityMessageResponsePacket(Either<String, Json> response, Uuid uuid)
  : response(std::move(response)), uuid(uuid) {}

void EntityMessageResponsePacket::read(DataStream& ds) {
  ds.read(response);
  ds.read(uuid);
}

void EntityMessageResponsePacket::write(DataStream& ds) const {
  ds.write(response);
  ds.write(uuid);
}

UpdateWorldPropertiesPacket::UpdateWorldPropertiesPacket() {}

UpdateWorldPropertiesPacket::UpdateWorldPropertiesPacket(JsonObject const& updatedProperties)
  : updatedProperties(updatedProperties) {}

void UpdateWorldPropertiesPacket::read(DataStream& ds) {
  ds.readMapContainer(updatedProperties);
}

void UpdateWorldPropertiesPacket::write(DataStream& ds) const {
  ds.writeMapContainer(updatedProperties);
}

UpdateTileProtectionPacket::UpdateTileProtectionPacket() {}

UpdateTileProtectionPacket::UpdateTileProtectionPacket(DungeonId dungeonId, bool isProtected)
  : dungeonId(dungeonId), isProtected(isProtected) {}

void UpdateTileProtectionPacket::read(DataStream& ds) {
  ds.read(dungeonId);
  ds.read(isProtected);
}

void UpdateTileProtectionPacket::write(DataStream& ds) const {
  ds.write(dungeonId);
  ds.write(isProtected);
}

SetDungeonGravityPacket::SetDungeonGravityPacket() {}

SetDungeonGravityPacket::SetDungeonGravityPacket(DungeonId dungeonId, Maybe<float> gravity)
  : dungeonId(std::move(dungeonId)), gravity(std::move(gravity)) {}

void SetDungeonGravityPacket::read(DataStream& ds) {
  ds.read(dungeonId);
  ds.read(gravity);
}

void SetDungeonGravityPacket::write(DataStream& ds) const {
  ds.write(dungeonId);
  ds.write(gravity);
}

SetDungeonBreathablePacket::SetDungeonBreathablePacket() {}

SetDungeonBreathablePacket::SetDungeonBreathablePacket(DungeonId dungeonId, Maybe<bool> breathable)
  : dungeonId(std::move(dungeonId)), breathable(std::move(breathable)) {}

void SetDungeonBreathablePacket::read(DataStream& ds) {
  ds.read(dungeonId);
  ds.read(breathable);
}

void SetDungeonBreathablePacket::write(DataStream& ds) const {
  ds.write(dungeonId);
  ds.write(breathable);
}

SetPlayerStartPacket::SetPlayerStartPacket() {}

SetPlayerStartPacket::SetPlayerStartPacket(Vec2F playerStart, bool respawnInWorld) : playerStart(playerStart), respawnInWorld(respawnInWorld) {}

void SetPlayerStartPacket::read(DataStream& ds) {
  ds.read(playerStart);
  ds.read(respawnInWorld);
}

void SetPlayerStartPacket::write(DataStream& ds) const {
  ds.write(playerStart);
  ds.write(respawnInWorld);
}

FindUniqueEntityResponsePacket::FindUniqueEntityResponsePacket() {}

FindUniqueEntityResponsePacket::FindUniqueEntityResponsePacket(String uniqueEntityId, Maybe<Vec2F> entityPosition)
  : uniqueEntityId(std::move(uniqueEntityId)), entityPosition(std::move(entityPosition)) {}

void FindUniqueEntityResponsePacket::read(DataStream& ds) {
  ds.read(uniqueEntityId);
  ds.read(entityPosition);
}

void FindUniqueEntityResponsePacket::write(DataStream& ds) const {
  ds.write(uniqueEntityId);
  ds.write(entityPosition);
}

PongPacket::PongPacket() {}

void PongPacket::read(DataStream& ds) {
  // Packets can't be empty, read the trash data
  ds.read<bool>();
}

void PongPacket::write(DataStream& ds) const {
  // Packets can't be empty, write some trash data
  ds.write<bool>(false);
}

StepUpdatePacket::StepUpdatePacket() : remoteTime(0.0) {}

StepUpdatePacket::StepUpdatePacket(double remoteTime) : remoteTime(remoteTime) {}

void StepUpdatePacket::readLegacy(DataStream& ds) {
  auto steps = ds.readVlqU();
  remoteTime = double(steps) / 60.0;
}

void StepUpdatePacket::read(DataStream& ds) {
  ds.read(remoteTime);
}

void StepUpdatePacket::writeLegacy(DataStream& ds) const {
  ds.writeVlqU((uint64_t)round(remoteTime * 60.0));
}

void StepUpdatePacket::write(DataStream& ds) const {
  ds.write(remoteTime);
}

SystemWorldStartPacket::SystemWorldStartPacket() {}

SystemWorldStartPacket::SystemWorldStartPacket(Vec3I location, List<ByteArray> objectStores, List<ByteArray> shipStores, pair<Uuid, SystemLocation> clientShip)
  : location(std::move(location)), objectStores(std::move(objectStores)), shipStores(std::move(shipStores)), clientShip(std::move(clientShip)) {}

void SystemWorldStartPacket::read(DataStream& ds) {
  ds.read(location);
  ds.read(objectStores);
  ds.read(shipStores);
  ds.read(clientShip);
}

void SystemWorldStartPacket::write(DataStream& ds) const {
  ds.write(location);
  ds.write(objectStores);
  ds.write(shipStores);
  ds.write(clientShip);
}

SystemWorldUpdatePacket::SystemWorldUpdatePacket() {}

SystemWorldUpdatePacket::SystemWorldUpdatePacket(HashMap<Uuid, ByteArray> objectUpdates, HashMap<Uuid, ByteArray> shipUpdates)
  : objectUpdates(std::move(objectUpdates)), shipUpdates(std::move(shipUpdates)) {}

void SystemWorldUpdatePacket::read(DataStream& ds) {
  ds.read(objectUpdates);
  ds.read(shipUpdates);
}

void SystemWorldUpdatePacket::write(DataStream& ds) const {
  ds.write(objectUpdates);
  ds.write(shipUpdates);
}

SystemObjectCreatePacket::SystemObjectCreatePacket() {}

SystemObjectCreatePacket::SystemObjectCreatePacket(ByteArray objectStore) : objectStore(std::move(objectStore)) {}

void SystemObjectCreatePacket::read(DataStream& ds) {
  ds.read(objectStore);
}

void SystemObjectCreatePacket::write(DataStream& ds) const {
  ds.write(objectStore);
}

SystemObjectDestroyPacket::SystemObjectDestroyPacket() {}

SystemObjectDestroyPacket::SystemObjectDestroyPacket(Uuid objectUuid) : objectUuid(std::move(objectUuid)) {}

void SystemObjectDestroyPacket::read(DataStream& ds) {
  ds.read(objectUuid);
}

void SystemObjectDestroyPacket::write(DataStream& ds) const {
  ds.write(objectUuid);
}

SystemShipCreatePacket::SystemShipCreatePacket() {}

SystemShipCreatePacket::SystemShipCreatePacket(ByteArray shipStore) : shipStore(std::move(shipStore)) {}

void SystemShipCreatePacket::read(DataStream& ds) {
  ds.read(shipStore);
}

void SystemShipCreatePacket::write(DataStream& ds) const {
  ds.write(shipStore);
}

SystemShipDestroyPacket::SystemShipDestroyPacket() {}

SystemShipDestroyPacket::SystemShipDestroyPacket(Uuid shipUuid) : shipUuid(std::move(shipUuid)) {}

void SystemShipDestroyPacket::read(DataStream& ds) {
  ds.read(shipUuid);
}

void SystemShipDestroyPacket::write(DataStream& ds) const {
  ds.write(shipUuid);
}

SystemObjectSpawnPacket::SystemObjectSpawnPacket() {}

SystemObjectSpawnPacket::SystemObjectSpawnPacket(String typeName, Uuid uuid, Maybe<Vec2F> position, JsonObject parameters)
  : typeName(std::move(typeName)), uuid(std::move(uuid)), position(std::move(position)), parameters(std::move(parameters)) {}

void SystemObjectSpawnPacket::read(DataStream& ds) {
  ds.read(typeName);
  ds.read(uuid);
  ds.read(position);
  ds.read(parameters);
}

void SystemObjectSpawnPacket::write(DataStream& ds) const {
  ds.write(typeName);
  ds.write(uuid);
  ds.write(position);
  ds.write(parameters);
}

}
