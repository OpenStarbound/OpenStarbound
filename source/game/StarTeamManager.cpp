#include "StarTeamManager.hpp"
#include "StarRandom.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

TeamManager::TeamManager() {
  m_pvpTeamCounter = 1;
  m_maxTeamSize = Root::singleton().configuration()->get("maxTeamSize").toUInt();
}

JsonRpcHandlers TeamManager::rpcHandlers() {
  return JsonRpcHandlers{
    {"team.fetchTeamStatus", bind(&TeamManager::fetchTeamStatus, this, _1)},
    {"team.updateStatus", bind(&TeamManager::updateStatus, this, _1)},
    {"team.invite", bind(&TeamManager::invite, this, _1)},
    {"team.pollInvitation", bind(&TeamManager::pollInvitation, this, _1)},
    {"team.acceptInvitation", bind(&TeamManager::acceptInvitation, this, _1)},
    {"team.makeLeader", bind(&TeamManager::makeLeader, this, _1)},
    {"team.removeFromTeam", [&](Json request) -> Json { return removeFromTeam(request); }}
  };
}

void TeamManager::setConnectedPlayers(StringMap<List<Uuid>> const& connectedPlayers) {
  m_connectedPlayers = std::move(connectedPlayers);
}

void TeamManager::playerDisconnected(Uuid const& playerUuid) {
  RecursiveMutexLocker lock(m_mutex);

  purgeInvitationsFor(playerUuid);
  purgeInvitationsFrom(playerUuid);

  for (auto teamUuid : m_teams.keys()) {
    auto& team = m_teams[teamUuid];
    if (team.members.contains(playerUuid))
      removeFromTeam(playerUuid, teamUuid);
  }
}

TeamNumber TeamManager::getPvpTeam(Uuid const& playerUuid) {
  RecursiveMutexLocker lock(m_mutex);
  for (auto const& teamPair : m_teams) {
    if (teamPair.second.members.contains(playerUuid))
      return teamPair.second.pvpTeamNumber;
  }
  return 0;
}

HashMap<Uuid, TeamNumber> TeamManager::getPvpTeams() {
  HashMap<Uuid, TeamNumber> result;
  for (auto const& teamPair : m_teams) {
    for (auto const& memberPair : teamPair.second.members)
      result[memberPair.first] = teamPair.second.pvpTeamNumber;
  }
  return result;
}

Maybe<Uuid> TeamManager::getTeam(Uuid const& playerUuid) const {
  for (auto const& teamPair : m_teams) {
    if (teamPair.second.members.contains(playerUuid))
      return teamPair.first;
  }
  return {};
}

void TeamManager::purgeInvitationsFor(Uuid const& playerUuid) {
  m_invitations.remove(playerUuid);
}

void TeamManager::purgeInvitationsFrom(Uuid const& playerUuid) {
  eraseWhere(m_invitations, [playerUuid](auto const& invitation) {
      return invitation.second.inviterUuid == playerUuid;
    });
}

bool TeamManager::playerWithUuidExists(Uuid const& playerUuid) const {
  for (auto const& p : m_connectedPlayers) {
    if (p.second.contains(playerUuid))
      return true;
  }
  return false;
}

Uuid TeamManager::createTeam(Uuid const& leaderUuid) {
  RecursiveMutexLocker lock(m_mutex);

  Uuid teamUuid;
  auto& team = m_teams[teamUuid]; // create

  int limiter = 256;
  while (true) {
    team.pvpTeamNumber = m_pvpTeamCounter++;
    if (m_pvpTeamCounter == 0)
      m_pvpTeamCounter = 1;
    bool done = true;
    for (auto k : m_teams.keys()) {
      auto t = m_teams[k];
      if (t.pvpTeamNumber == team.pvpTeamNumber) {
        if (k != teamUuid)
          done = false;
      }
    }
    if (done)
      break;
    if (limiter-- == 0) {
      team.pvpTeamNumber = 0;
      break;
    }
  }

  addToTeam(leaderUuid, teamUuid);
  team.leaderUuid = leaderUuid;

  return teamUuid;
}

bool TeamManager::addToTeam(Uuid const& playerUuid, Uuid const& teamUuid) {
  RecursiveMutexLocker lock(m_mutex);

  if (!m_teams.contains(teamUuid))
    return false;

  auto& team = m_teams.get(teamUuid);

  if (team.members.contains(playerUuid))
    return false;

  if (team.members.size() >= m_maxTeamSize)
    return false;

  purgeInvitationsFor(playerUuid);

  for (auto otherTeam : m_teams) {
    List<Uuid> alreadyMemberOf;
    if (otherTeam.second.members.contains(playerUuid))
      alreadyMemberOf.append(otherTeam.first);
    for (auto leaveTeamUuid : alreadyMemberOf)
      removeFromTeam(playerUuid, leaveTeamUuid);
  }

  team.members.insert(playerUuid, TeamMember());

  return true;
}

bool TeamManager::removeFromTeam(Uuid const& playerUuid, Uuid const& teamUuid) {
  RecursiveMutexLocker lock(m_mutex);

  if (!m_teams.contains(teamUuid))
    return false;

  auto& team = m_teams.get(teamUuid);

  if (!team.members.contains(playerUuid))
    return false;

  purgeInvitationsFrom(playerUuid);

  team.members.remove(playerUuid);
  if (team.members.size() <= 1)
    m_teams.remove(teamUuid);
  else if (team.leaderUuid == playerUuid)
    team.leaderUuid = Random::randFrom(team.members).first;

  return true;
}

Json TeamManager::fetchTeamStatus(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto playerUuid = Uuid(arguments.getString("playerUuid"));
  JsonObject result;
  if (auto teamUuid = getTeam(playerUuid)) {
    auto& team = m_teams.get(*teamUuid);

    result["teamUuid"] = teamUuid->hex();
    result["leader"] = team.leaderUuid.hex();
    JsonArray members;
    for (auto const& m : team.members) {
      JsonObject member;
      auto const& mem = m.second;
      member["name"] = mem.name;
      member["uuid"] = m.first.hex();
      member["leader"] = m.first == team.leaderUuid;
      member["entity"] = mem.entity;
      member["health"] = mem.healthPercentage;
      member["energy"] = mem.energyPercentage;
      member["x"] = mem.position[0];
      member["y"] = mem.position[1];
      member["world"] = printWorldId(mem.world);
      member["warpMode"] = WarpModeNames.getRight(mem.warpMode);
      member["portrait"] = jsonFromList(mem.portrait, mem_fn(&Drawable::toJson));
      members.push_back(member);
    }
    result["members"] = members;
  }
  return result;
}

Json TeamManager::updateStatus(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto playerUuid = Uuid(arguments.getString("playerUuid"));
  if (auto teamUuid = getTeam(playerUuid)) {
    auto& team = m_teams.get(*teamUuid);
    auto& entry = team.members.get(playerUuid);
    if (arguments.contains("name"))
      entry.name = arguments.getString("name");
    if (arguments.contains("entity"))
      entry.entity = arguments.getInt("entity");
    entry.healthPercentage = arguments.getFloat("health");
    entry.energyPercentage = arguments.getFloat("energy");
    entry.position[0] = arguments.getFloat("x");
    entry.position[1] = arguments.getFloat("y");
    entry.warpMode = WarpModeNames.getLeft(arguments.getString("warpMode"));
    if (arguments.contains("world"))
      entry.world = parseWorldId(arguments.getString("world"));
    if (arguments.contains("portrait"))
      entry.portrait = jsonToList<Drawable>(arguments.get("portrait"));
    return {};
  } else {
    return "notAMemberOfTeam";
  }
}

Json TeamManager::invite(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto inviteeName = arguments.getString("inviteeName").toLower();

  if (!m_connectedPlayers.contains(inviteeName))
    return "inviteeNotFound";

  auto inviterUuid = Uuid(arguments.getString("inviterUuid"));

  for (auto inviteeUuid : m_connectedPlayers[inviteeName]) {
    if (inviteeUuid == inviterUuid)
      continue;

    Invitation invitation;
    invitation.inviterUuid = inviterUuid;
    invitation.inviterName = arguments.getString("inviterName");
    m_invitations[inviteeUuid] = invitation;
  }

  return {};
}

Json TeamManager::pollInvitation(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto playerUuid = Uuid(arguments.getString("playerUuid"));
  if (m_invitations.contains(playerUuid)) {
    auto invite = m_invitations.take(playerUuid);
    JsonObject result;
    result["inviterUuid"] = invite.inviterUuid.hex();
    result["inviterName"] = invite.inviterName;
    return result;
  }
  return {};
}

Json TeamManager::acceptInvitation(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto inviterUuid = Uuid(arguments.getString("inviterUuid"));
  auto inviteeUuid = Uuid(arguments.getString("inviteeUuid"));

  if (!playerWithUuidExists(inviterUuid) || !playerWithUuidExists(inviteeUuid))
    return "acceptInvitationFailed";

  purgeInvitationsFrom(inviteeUuid);

  Uuid teamUuid;
  if (auto existingTeamUuid = getTeam(inviterUuid))
    teamUuid = *existingTeamUuid;
  else
    teamUuid = createTeam(inviterUuid);

  auto success = addToTeam(inviteeUuid, teamUuid);
  return success ? Json() : "acceptInvitationFailed";
}

Json TeamManager::removeFromTeam(Json const& arguments) {
  auto playerUuid = Uuid(arguments.getString("playerUuid"));
  auto teamUuid = Uuid(arguments.getString("teamUuid"));

  auto success = removeFromTeam(playerUuid, teamUuid);
  return success ? Json() : "removeFromTeamFailed";
}

Json TeamManager::makeLeader(Json const& arguments) {
  RecursiveMutexLocker lock(m_mutex);
  auto playerUuid = Uuid(arguments.getString("playerUuid"));
  auto teamUuid = Uuid(arguments.getString("teamUuid"));

  if (!m_teams.contains(teamUuid))
    return "noSuchTeam";

  auto& team = m_teams.get(teamUuid);

  if (!team.members.contains(playerUuid))
    return "notAMemberOfTeam";

  team.leaderUuid = playerUuid;

  return {};
}

}
