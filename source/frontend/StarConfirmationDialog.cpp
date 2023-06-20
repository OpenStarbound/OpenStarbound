#include "StarConfirmationDialog.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLabelWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"

namespace Star {

ConfirmationDialog::ConfirmationDialog() {}

void ConfirmationDialog::displayConfirmation(Json const& dialogConfig, RpcPromiseKeeper<Json> resultPromise) {
  m_resultPromise = resultPromise;
  displayConfirmation(dialogConfig, [this] (Widget*) { m_resultPromise->fulfill(true); }, [this] (Widget*) { m_resultPromise->fulfill(false); } );
}

void ConfirmationDialog::displayConfirmation(Json const& dialogConfig, WidgetCallbackFunc okCallback, WidgetCallbackFunc cancelCallback) {
  Json config;
  if (dialogConfig.isType(Json::Type::String))
    config = Root::singleton().assets()->json(dialogConfig.toString());
  else
    config = dialogConfig;

  auto assets = Root::singleton().assets();

  removeAllChildren();

  GuiReader reader;

  m_okCallback = move(okCallback);
  m_cancelCallback = move(cancelCallback);

  reader.registerCallback("close", bind(&ConfirmationDialog::dismiss, this));
  reader.registerCallback("cancel", bind(&ConfirmationDialog::dismiss, this));
  reader.registerCallback("ok", bind(&ConfirmationDialog::ok, this));

  m_confirmed = false;

  String paneLayoutPath =
      config.optString("paneLayout").value("/interface/windowconfig/confirmation.config:paneLayout");
  reader.construct(assets->json(paneLayoutPath), this);

  ImageWidgetPtr titleIcon = {};
  if (config.contains("icon"))
    titleIcon = make_shared<ImageWidget>(config.getString("icon"));

  setTitle(titleIcon, config.getString("title", ""), config.getString("subtitle", ""));
  fetchChild<LabelWidget>("message")->setText(config.getString("message"));

  if (config.contains("okCaption"))
    fetchChild<ButtonWidget>("ok")->setText(config.getString("okCaption"));
  if (config.contains("cancelCaption"))
    fetchChild<ButtonWidget>("cancel")->setText(config.getString("cancelCaption"));

  m_sourceEntityId = config.optInt("sourceEntityId");

  for (auto image : config.optObject("images").value({})) {
    auto widget = fetchChild<ImageWidget>(image.first);
    if (image.second.isType(Json::Type::String))
      widget->setImage(image.second.toString());
    else
      widget->setDrawables(image.second.toArray().transformed(construct<Drawable>()));
  }

  for (auto label : config.optObject("labels").value({})) {
    auto widget = fetchChild<LabelWidget>(label.first);
    widget->setText(label.second.toString());
  }

  show();
  auto sound = Random::randValueFrom(Root::singleton().assets()->json("/interface/windowconfig/confirmation.config:onShowSound").toArray(), "").toString();

  if (!sound.empty())
    context()->playAudio(sound);
}

Maybe<EntityId> ConfirmationDialog::sourceEntityId() {
  return m_sourceEntityId;
}

void ConfirmationDialog::dismissed() {
  if (!m_confirmed)
    m_cancelCallback(this);

  Pane::dismissed();
}

void ConfirmationDialog::ok() {
  m_okCallback(this);
  m_confirmed = true;
  dismiss();
}

}
