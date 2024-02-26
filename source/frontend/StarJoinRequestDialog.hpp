#pragma once

#include "StarPane.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(JoinRequestDialog);

class JoinRequestDialog : public Pane {
public:
  JoinRequestDialog();

  virtual ~JoinRequestDialog() {}

  void displayRequest(String const& userName, function<void(P2PJoinRequestReply)> callback);

  void dismissed() override;

private:
  void reply(P2PJoinRequestReply reply);

  function<void(P2PJoinRequestReply)> m_callback;
  bool m_confirmed;
};

}
