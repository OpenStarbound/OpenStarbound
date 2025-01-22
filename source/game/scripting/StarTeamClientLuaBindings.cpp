#include "StarTeamClientLuaBindings.hpp"
#include "StarTeamClient.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeTeamClientCallbacks(TeamClient* teamClient) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<void>("isMemberOfTeam", bind(mem_fn(&TeamClient::isMemberOfTeam), teamClient));
  callbacks.registerCallbackWithSignature<void, String>("invitePlayer", bind(mem_fn(&TeamClient::invitePlayer), teamClient, _1));

  callbacks.registerCallback("isTeamLeader", [teamClient](Maybe<String>  const& playerUuid) -> bool {
      if (playerUuid)
        return teamClient->isTeamLeader(Uuid(*playerUuid));
      return teamClient->isTeamLeader();
    });
  callbacks.registerCallback("currentTeam", [teamClient]() -> Maybe<String> {
      auto teamUuid = teamClient->currentTeam();
      if (teamUuid)
        return teamUuid->hex();
      return {};
    });
  callbacks.registerCallback("makeLeader", [teamClient](String const& playerUuid) {
      teamClient->makeLeader(Uuid(playerUuid));
    });
  callbacks.registerCallback("removeFromTeam", [teamClient](String const& playerUuid) {
      teamClient->removeFromTeam(Uuid(playerUuid));
    });

  return callbacks;
}

}
