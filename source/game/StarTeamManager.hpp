#ifndef STAR_TEAMMANAGER_HPP
#define STAR_TEAMMANAGER_HPP

#include "StarDrawable.hpp"
#include "StarUuid.hpp"
#include "StarJsonRpc.hpp"
#include "StarWarping.hpp"
#include "StarThread.hpp"
#include "StarDamageTypes.hpp"

namespace Star {

STAR_CLASS(TeamManager);

class TeamManager {
public:
  TeamManager();

  JsonRpcHandlers rpcHandlers();

  void setConnectedPlayers(StringMap<List<Uuid>> const& connectedPlayers);
  void playerDisconnected(Uuid const& playerUuid);

  TeamNumber getPvpTeam(Uuid const& playerUuid);
  HashMap<Uuid, TeamNumber> getPvpTeams();
  Maybe<Uuid> getTeam(Uuid const& playerUuid) const;

private:
  struct TeamMember {
    String name;
    int entity;
    float healthPercentage;
    float energyPercentage;
    WorldId world;
    Vec2F position;
    WarpMode warpMode;

    List<Drawable> portrait;
  };

  struct Team {
    Uuid leaderUuid;
    TeamNumber pvpTeamNumber;

    Map<Uuid, TeamMember> members;
  };

  struct Invitation {
    Uuid inviterUuid;
    String inviterName;
  };

  void purgeInvitationsFor(Uuid const& playerUuid);
  void purgeInvitationsFrom(Uuid const& playerUuid);

  bool playerWithUuidExists(Uuid const& playerUuid) const;

  Uuid createTeam(Uuid const& leaderUuid);
  bool addToTeam(Uuid const& playerUuid, Uuid const& teamUuid);
  bool removeFromTeam(Uuid const& playerUuid, Uuid const& teamUuid);

  RecursiveMutex m_mutex;
  Map<Uuid, Team> m_teams;
  StringMap<List<Uuid>> m_connectedPlayers;
  Map<Uuid, Invitation> m_invitations;

  unsigned m_maxTeamSize;

  TeamNumber m_pvpTeamCounter;

  Json fetchTeamStatus(Json const& args);
  Json updateStatus(Json const& args);
  Json invite(Json const& args);
  Json pollInvitation(Json const& args);
  Json acceptInvitation(Json const& args);
  Json removeFromTeam(Json const& args);
  Json makeLeader(Json const& args);
};

}

#endif
