#pragma once

#include "StarBaseScriptPane.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(UniverseClient);
STAR_CLASS(JoinRequestDialog);

class JoinRequestDialog : public BaseScriptPane {
public:
  JoinRequestDialog(UniverseClientPtr client);

  virtual ~JoinRequestDialog() {}

  // A script gets "request(userName, message)" after this and can change anything on the pane.
  void displayRequest(String const& userName, function<void(P2PJoinRequestReply)> callback);

  void dismissed() override;

private:
  void reply(P2PJoinRequestReply reply);

  function<void(P2PJoinRequestReply)> m_callback;
  bool m_confirmed;
};

}
