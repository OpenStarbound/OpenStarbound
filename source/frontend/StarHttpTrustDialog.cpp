#include "StarHttpTrustDialog.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarAssets.hpp"
#include "StarConfiguration.hpp"

namespace Star {

HttpTrustDialog::HttpTrustDialog() : m_confirmed(false) {}

void HttpTrustDialog::displayRequest(String const& domain, function<void(HttpTrustReply, bool)> callback) {
  const auto assets = Root::singleton().assets();

  removeAllChildren();

  GuiReader reader;

  m_domain = domain;
  m_callback = std::move(callback);

  reader.registerCallback("yes", [this](Widget*) { reply(HttpTrustReply::Allow); });
  reader.registerCallback("no", [this](Widget*) { reply(HttpTrustReply::Deny); });
  // reader.registerCallback("rememberCheckbox", [this](Widget*) {
  //   // just to capture it
  // });

  m_confirmed = false;

  const Json config = assets->json("/interface/warning/warning.config");

  reader.construct(config.get("paneLayout"), this);

  // Update message with domain
  const String message = strf("^green;{}^reset;", domain);
  fetchChild<LabelWidget>("domain")->setText(message);
  // fetchChild<ButtonWidget>("yes")->setText("Allow");
  // fetchChild<ButtonWidget>("no")->setText("Deny"); // I did it cuz of: if some smart guy will swap yes/no buttons texts in the config file
  fetchChild<ButtonWidget>("yes")->setText("✅");
  fetchChild<ButtonWidget>("no")->setText("❌"); // Emoji buttons dont need to be translated

  show();
}

void HttpTrustDialog::reply(const HttpTrustReply replyType) {
  m_confirmed = true;

  // Check if "remember" checkbox is checked
  const bool remember = fetchChild<ButtonWidget>("rememberCheckbox")->isChecked();

  // If allowing and remember is checked, add to trusted list
  if (replyType == HttpTrustReply::Allow && remember) {
    auto& root = Root::singleton();
    const auto config = root.configuration();

    JsonArray trustedSites;
    if (auto existing = config->getPath("safe.luaHttp.trustedSites").optArray())
      trustedSites = *existing;

    // Check if already exists
    bool exists = false;
    for (auto const& site : trustedSites) {
      if (site.toString() == m_domain) {
        exists = true;
        break;
      }
    }

    if (!exists) {
      trustedSites.append(m_domain);
      config->setPath("safe.luaHttp.trustedSites", trustedSites);
    }
  }

  m_callback(replyType, remember);
  dismiss();
}

void HttpTrustDialog::dismissed() {
  if (!m_confirmed)
    m_callback(HttpTrustReply::Deny, false);

  Pane::dismissed();
}

}





