#ifndef STAR_REGISTERED_PANE_MANAGER_HPP
#define STAR_REGISTERED_PANE_MANAGER_HPP

#include "StarPaneManager.hpp"

namespace Star {

// This class inherits PaneManager to allow for registered panes that are kept
// internally by the class even when dismissed.  They can be displayed,
// dismissed, and toggled between the two without being lost.
template <typename KeyT>
class RegisteredPaneManager : public PaneManager {
public:
  typedef KeyT Key;

  void registerPane(KeyT paneId, PaneLayer paneLayer, PanePtr pane, DismissCallback onDismiss = {});
  PanePtr deregisterPane(KeyT const& paneId);
  void deregisterAllPanes();

  template <typename T = Pane>
  shared_ptr<T> registeredPane(KeyT const& paneId) const;
  template <typename T = Pane>
  shared_ptr<T> maybeRegisteredPane(KeyT const& paneId) const;

  // Displays a registred pane if it is not already displayed.  Returns true
  // if it is newly displayed.
  bool displayRegisteredPane(KeyT const& paneId);
  bool registeredPaneIsDisplayed(KeyT const& paneId) const;

  // Dismisses a registred pane if it is displayed.  Returns true if it
  // has been dismissed.
  bool dismissRegisteredPane(KeyT const& paneId);

  // Returns whether the pane is now displayed.
  bool toggleRegisteredPane(KeyT const& paneId);

private:
  struct PaneInfo {
    PaneLayer layer;
    PanePtr pane;
    DismissCallback dismissCallback;
  };

  PaneInfo const& getRegisteredPaneInfo(KeyT const& paneId) const;

  // Map of registered panes by name.
  HashMap<KeyT, PaneInfo> m_registeredPanes;
};

template <typename KeyT>
template <typename T>
shared_ptr<T> RegisteredPaneManager<KeyT>::registeredPane(KeyT const& paneId) const {
  if (auto v = m_registeredPanes.ptr(paneId))
    return convert<T>(v->pane);
  throw GuiException(strf("No pane named '{}' found in RegisteredPaneManager", outputAny(paneId)));
}

template <typename KeyT>
template <typename T>
shared_ptr<T> RegisteredPaneManager<KeyT>::maybeRegisteredPane(KeyT const& paneId) const {
  if (auto v = m_registeredPanes.ptr(paneId))
    return convert<T>(v->pane);
  return {};
}

template <typename KeyT>
void RegisteredPaneManager<KeyT>::registerPane(
    KeyT paneId, PaneLayer paneLayer, PanePtr pane, DismissCallback onDismiss) {
  if (!m_registeredPanes.insert(std::move(paneId), {std::move(paneLayer), std::move(pane), std::move(onDismiss)}).second)
    throw GuiException(
        strf("Registered pane with name '{}' registered a second time in RegisteredPaneManager::registerPane",
            outputAny(paneId)));
}

template <typename KeyT>
PanePtr RegisteredPaneManager<KeyT>::deregisterPane(KeyT const& paneId) {
  if (auto v = m_registeredPanes.maybeTake(paneId)) {
    if (isDisplayed(v->pane))
      dismissPane(v->pane);
    return v->pane;
  }
  throw GuiException(strf("No pane named '{}' found in RegisteredPaneManager::deregisterPane", outputAny(paneId)));
}

template <typename KeyT>
void RegisteredPaneManager<KeyT>::deregisterAllPanes() {
  for (auto const& k : m_registeredPanes.keys())
    deregisterPane(k);
}

template <typename KeyT>
bool RegisteredPaneManager<KeyT>::displayRegisteredPane(KeyT const& paneId) {
  auto const& paneInfo = getRegisteredPaneInfo(paneId);
  if (!isDisplayed(paneInfo.pane)) {
    displayPane(paneInfo.layer, paneInfo.pane, paneInfo.dismissCallback);
    return true;
  }
  return false;
}

template <typename KeyT>
bool RegisteredPaneManager<KeyT>::registeredPaneIsDisplayed(KeyT const& paneId) const {
  return isDisplayed(getRegisteredPaneInfo(paneId).pane);
}

template <typename KeyT>
bool RegisteredPaneManager<KeyT>::dismissRegisteredPane(KeyT const& paneId) {
  auto const& paneInfo = getRegisteredPaneInfo(paneId);
  if (isDisplayed(paneInfo.pane)) {
    dismissPane(paneInfo.pane);
    return true;
  }
  return false;
}

template <typename KeyT>
bool RegisteredPaneManager<KeyT>::toggleRegisteredPane(KeyT const& paneId) {
  if (registeredPaneIsDisplayed(paneId)) {
    dismissRegisteredPane(paneId);
    return false;
  } else {
    displayRegisteredPane(paneId);
    return true;
  }
}

template <typename KeyT>
typename RegisteredPaneManager<KeyT>::PaneInfo const& RegisteredPaneManager<KeyT>::getRegisteredPaneInfo(
    KeyT const& paneId) const {
  if (auto p = m_registeredPanes.ptr(paneId))
    return *p;
  throw GuiException(strf("No registered pane with name '{}' found in  RegisteredPaneManager", outputAny(paneId)));
}
}

#endif
