#ifndef STAR_CONFIRMATION_DIALOG_HPP
#define STAR_CONFIRMATION_DIALOG_HPP

#include "StarPane.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(ConfirmationDialog);

class ConfirmationDialog : public Pane {
public:
  ConfirmationDialog();

  virtual ~ConfirmationDialog() {}

  void displayConfirmation(Json const& dialogConfig, RpcPromiseKeeper<Json> resultPromise);
  void displayConfirmation(Json const& dialogConfig, WidgetCallbackFunc okCallback, WidgetCallbackFunc cancelCallback);
  
  Maybe<EntityId> sourceEntityId();

  void dismissed() override;

private:
  void ok();

  WidgetCallbackFunc m_okCallback;
  WidgetCallbackFunc m_cancelCallback;
  bool m_confirmed;

  Maybe<EntityId> m_sourceEntityId;

  Maybe<RpcPromiseKeeper<Json>> m_resultPromise;
};

}

#endif
