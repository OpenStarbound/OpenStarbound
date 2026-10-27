#include "StarJoinRequestDialog.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"
#include "StarUniverseClient.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

JoinRequestDialog::JoinRequestDialog(UniverseClientPtr client)
  : BaseScriptPane(BaseScriptPane::windowConfig("/interface/windowconfig/joinrequest.config"), false), m_confirmed(false) {
  m_script.setLuaRoot(client->luaRoot());
  m_script.addCallbacks("player", LuaBindings::makePlayerCallbacks(client->mainPlayer().get()));
  m_script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(client->mainPlayer()->statusController()));
  m_script.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(client.get()));
  setupScript();

  m_reader->registerCallback("yes", [this](Widget*){ reply(P2PJoinRequestReply::Yes); });
  m_reader->registerCallback("no", [this](Widget*){ reply(P2PJoinRequestReply::No); });
  m_reader->registerCallback("ignore", [this](Widget*){ reply(P2PJoinRequestReply::Ignore); });
}

void JoinRequestDialog::displayRequest(String const& userName, function<void(P2PJoinRequestReply)> callback) {
  removeAllChildren();

  m_callback = std::move(callback);
  m_confirmed = false;

  m_reader->construct(m_config.get("gui"), this);

  String message = m_config.getString("joinMessage").replaceTags(StringMap<String>{{"username", userName}});
  fetchChild<LabelWidget>("message")->setText(message);

  show();

  // the pane is displayed (and its script initialised) before this is called, so a script can adjust it now
  m_script.invoke("request", userName, message);
}

void JoinRequestDialog::reply(P2PJoinRequestReply reply) {
  m_confirmed = true;
  m_callback(reply);
  dismiss();
}

void JoinRequestDialog::dismissed() {
  if (!m_confirmed)
    m_callback(P2PJoinRequestReply::No);

  BaseScriptPane::dismissed();
}

}
