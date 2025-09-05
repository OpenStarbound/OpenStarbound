#pragma once

#include "StarUuid.hpp"
#include "StarDrawable.hpp"
#include "StarWarping.hpp"
#include "StarJsonRpc.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(ClientContext);
STAR_CLASS(TeamClient);

class TeamClient {
public:
  struct Member {
    String name;
    Uuid uuid;
    int entity;
    float healthPercentage;
    float energyPercentage;
    WorldId world;
    Vec2F position;
    WarpMode warpMode;
    List<Drawable> portrait;
  };

  TeamClient(PlayerPtr mainPlayer, ClientContextPtr clientContext);

  void invitePlayer(String const& playerName);
  void acceptInvitation(Uuid const& inviterUuid);

  Maybe<Uuid> currentTeam() const;

  void makeLeader(Uuid const& playerUuid);
  void removeFromTeam(Uuid const& playerUuid);

  bool isTeamLeader();
  bool isTeamLeader(Uuid const& playerUuid);
  bool isMemberOfTeam();

  bool hasInvitationPending();
  pair<Uuid, String> pullInvitation();
  List<Variant<pair<String, bool>, StringList>> pullInviteResults();

  void update();

  void pullFullUpdate();
  void statusUpdate();

  void forceUpdate();

  List<Member> members();

private:
  typedef pair<RpcPromise<Json>, function<void(Json const&)>> RpcResponseHandler;

  void invokeRemote(String const& method, Json const& args, function<void(Json const&)> responseFunction = {});
  void handleRpcResponses();

  void writePlayerData(JsonObject& request, PlayerPtr player, bool fullWrite = false) const;

  void clearTeam();

  PlayerPtr m_mainPlayer;
  ClientContextPtr m_clientContext;
  Maybe<Uuid> m_teamUuid;

  Uuid m_teamLeader;

  List<Member> m_members;

  bool m_hasPendingInvitation;
  pair<Uuid, String> m_pendingInvitation;
  double m_pollInvitationsTimer;
  List<Variant<pair<String, bool>, StringList>> m_pendingInviteResults;

  bool m_fullUpdateRunning;
  double m_fullUpdateTimer;

  bool m_statusUpdateRunning;
  double m_statusUpdateTimer;

  List<RpcResponseHandler> m_pendingResponses;
};

}
