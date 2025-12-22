#pragma once

#include "StarPane.hpp"

namespace Star {

STAR_CLASS(HttpTrustDialog);

enum class HttpTrustReply {
  Allow,
  Deny
};

class HttpTrustDialog final : public Pane {
public:
  HttpTrustDialog();

  ~HttpTrustDialog() override = default;

  void displayRequest(String const& domain, function<void(HttpTrustReply, bool)> callback);

  void dismissed() override;

private:
  void reply(HttpTrustReply replyType);

  String m_domain;
  bool m_confirmed;
  function<void(HttpTrustReply, bool)> m_callback;
};

}

