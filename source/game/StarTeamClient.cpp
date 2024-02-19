#include "StarTeamClient.hpp"
#include "StarJsonExtra.hpp"
#include "StarWorldTemplate.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLog.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarClientContext.hpp"
#include "StarWorldClient.hpp"
#include "StarJsonRpc.hpp"

namespace Star {

TeamClient::TeamClient(PlayerPtr mainPlayer, ClientContextPtr clientContext) {
  m_mainPlayer = mainPlayer;
  m_clientContext = clientContext;

  m_hasPendingInvitation = false;
  m_pollInvitationsTimer = 0;

  m_fullUpdateRunning = false;
  m_fullUpdateTimer = 0;

  m_statusUpdateRunning = false;
  m_statusUpdateTimer = 0;
}

bool TeamClient::isTeamLeader() {
  if (!m_teamUuid)
    return false;
  return m_teamLeader == m_clientContext->playerUuid();
}

bool TeamClient::isTeamLeader(Uuid const& playerUuid) {
  if (!m_teamUuid)
    return false;
  return m_teamLeader == playerUuid;
}

bool TeamClient::isMemberOfTeam() {
  return (bool)m_teamUuid;
}

void TeamClient::invitePlayer(String const& playerName) {
  if (playerName.empty())
    return;

  JsonObject request;
  request["inviteeName"] = playerName;
  request["inviterUuid"] = m_clientContext->playerUuid().hex();
  request["inviterName"] = m_mainPlayer->name();
  invokeRemote("team.invite", request, [](Json) {});
}

void TeamClient::acceptInvitation(Uuid const& inviterUuid) {
  JsonObject request;
  request["inviterUuid"] = inviterUuid.hex();
  request["inviteeUuid"] = m_clientContext->playerUuid().hex();
  invokeRemote("team.acceptInvitation", request, [this](Json) { forceUpdate(); });
}

Maybe<Uuid> TeamClient::currentTeam() const {
  return m_teamUuid;
}

void TeamClient::makeLeader(Uuid const& playerUuid) {
  if (!m_teamUuid)
    return;
  if (!isTeamLeader())
    return;
  JsonObject request;
  request["teamUuid"] = m_teamUuid->hex();
  request["playerUuid"] = playerUuid.hex();
  invokeRemote("team.makeLeader", request, [this](Json) { forceUpdate(); });
}

void TeamClient::removeFromTeam(Uuid const& playerUuid) {
  if (!m_teamUuid)
    return;
  if (!isTeamLeader() && playerUuid != m_clientContext->playerUuid())
    return;
  JsonObject request;
  request["teamUuid"] = m_teamUuid->hex();
  request["playerUuid"] = playerUuid.hex();
  invokeRemote("team.removeFromTeam", request, [this](Json) { forceUpdate(); });
}

bool TeamClient::hasInvitationPending() {
  return m_hasPendingInvitation;
}

std::pair<Uuid, String> TeamClient::pullInvitation() {
  m_hasPendingInvitation = false;
  return m_pendingInvitation;
}

void TeamClient::update() {
  handleRpcResponses();

  if (!m_hasPendingInvitation) {
    if (Time::monotonicTime() - m_pollInvitationsTimer > Root::singleton().assets()->json("/interface.config:invitationPollInterval").toFloat()) {
      m_pollInvitationsTimer = Time::monotonicTime();
      JsonObject request;
      request["playerUuid"] = m_clientContext->playerUuid().hex();
      invokeRemote("team.pollInvitation", request, [this](Json response) {
          if (response.isNull())
            return;
          if (m_hasPendingInvitation)
            return;
          m_pendingInvitation = {Uuid(response.getString("inviterUuid")), response.getString("inviterName")};
          m_hasPendingInvitation = true;
        });
    }
  }
  if (!m_fullUpdateRunning) {
    if (Time::monotonicTime() - m_fullUpdateTimer > Root::singleton().assets()->json("/interface.config:fullUpdateInterval").toFloat()) {
      m_fullUpdateTimer = Time::monotonicTime();
      pullFullUpdate();
    }
  }
  if (!m_statusUpdateRunning) {
    if (Time::monotonicTime() - m_statusUpdateTimer > Root::singleton().assets()->json("/interface.config:statusUpdateInterval").toFloat()) {
      m_statusUpdateTimer = Time::monotonicTime();
      statusUpdate();
    }
  }
}

void TeamClient::pullFullUpdate() {
  if (m_fullUpdateRunning)
    return;
  m_fullUpdateRunning = true;
  JsonObject request;
  request["playerUuid"] = m_clientContext->playerUuid().hex();

  invokeRemote("team.fetchTeamStatus", request, [this](Json response) {
      m_fullUpdateRunning = false;

      m_teamUuid = response.optString("teamUuid").apply(construct<Uuid>());

      if (m_teamUuid) {
        m_teamLeader = Uuid(response.getString("leader"));
        m_members.clear();

        for (auto m : response.getArray("members")) {
          Member member;
          member.name = m.getString("name");
          member.uuid = Uuid(m.getString("uuid"));
          member.entity = m.getInt("entity");
          member.healthPercentage = m.getFloat("health");
          member.energyPercentage = m.getFloat("energy");
          member.position[0] = m.getFloat("x");
          member.position[1] = m.getFloat("y");
          member.world = parseWorldId(m.getString("world"));
          member.warpMode = WarpModeNames.getLeft(m.getString("warpMode"));
          member.portrait = jsonToList<Drawable>(m.get("portrait"));
          m_members.push_back(member);
        }
        std::sort(m_members.begin(), m_members.end(), [](Member const& a, Member const& b) { return a.name < b.name; });
      } else {
        clearTeam();
      }
    });
}

void TeamClient::statusUpdate() {
  if (m_statusUpdateRunning)
    return;
  if (!m_teamUuid)
    return;
  m_statusUpdateRunning = true;
  JsonObject request;
  auto player = m_mainPlayer;

  // TODO: write full player data less often?
  writePlayerData(request, player, true);

  invokeRemote("team.updateStatus", request, [this](Json) {
      m_statusUpdateRunning = false;
    });
}

List<TeamClient::Member> TeamClient::members() {
  return m_members;
}

void TeamClient::forceUpdate() {
  m_statusUpdateTimer = 0;
  m_fullUpdateTimer = 0;
  m_pollInvitationsTimer = 0;
}

void TeamClient::invokeRemote(String const& method, Json const& args, function<void(Json const&)> responseFunction) {
  auto promise = m_clientContext->rpcInterface()->invokeRemote(method, args);
  m_pendingResponses.append({std::move(promise), std::move(responseFunction)});
}

void TeamClient::handleRpcResponses() {
  List<RpcResponseHandler> stillPendingResponses;
  while (m_pendingResponses.size() > 0) {
    auto handler = m_pendingResponses.takeLast();
    if (handler.first.finished()) {
      if (auto const& res = handler.first.result()) {
        if (handler.second)
          handler.second(*res);
      }
    } else {
      stillPendingResponses.append(std::move(handler));
    }
  }
  m_pendingResponses = stillPendingResponses;
}

void TeamClient::writePlayerData(JsonObject& request, PlayerPtr player, bool fullWrite) const {
  request["playerUuid"] = m_clientContext->playerUuid().hex();
  request["entity"] = player->entityId();
  request["health"] = player->health() / player->maxHealth();
  request["energy"] = player->energy() / player->maxEnergy();
  request["x"] = player->position()[0];
  request["y"] = player->position()[1];
  request["world"] = printWorldId(m_clientContext->playerWorldId());

  WarpMode mode = WarpMode::None;
  if (player->log()->introComplete()) {
    if (m_clientContext->playerWorldId().is<CelestialWorldId>())
      mode = WarpMode::BeamOnly;
    else
      mode = player->isDeployed() ? WarpMode::DeployOnly : WarpMode::BeamOnly;
  }
  request["warpMode"] = WarpModeNames.getRight(mode);

  if (fullWrite) {
    request["name"] = player->name();
    request["portrait"] = jsonFromList(player->portrait(PortraitMode::Head), mem_fn(&Drawable::toJson));
  }
}

void TeamClient::clearTeam() {
  m_teamLeader = Uuid();
  m_teamUuid = {};
  m_members.clear();
  forceUpdate();
}

}
