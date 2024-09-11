#include "StarPaneManager.hpp"
#include "StarGameTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

EnumMap<PaneLayer> const PaneLayerNames{
  {PaneLayer::Tooltip, "Tooltip"},
  {PaneLayer::ModalWindow, "ModalWindow"},
  {PaneLayer::Window, "Window"},
  {PaneLayer::Hud, "Hud"},
  {PaneLayer::World, "World"}
};

PaneManager::PaneManager()
  : m_context(GuiContext::singletonPtr()), m_prevInterfaceScale(1) {
  auto assets = Root::singleton().assets();
  m_tooltipMouseoverTime = assets->json("/panes.config:tooltipMouseoverTime").toFloat();
  m_tooltipMouseoverRadius = assets->json("/panes.config:tooltipMouseoverRadius").toFloat();
  m_tooltipMouseOffset = jsonToVec2I(assets->json("/panes.config:tooltipMouseoverOffset"));

  m_tooltipShowTimer = m_tooltipMouseoverTime;
}

void PaneManager::displayPane(PaneLayer paneLayer, PanePtr const& pane, DismissCallback onDismiss) {
  if (!m_displayedPanes[paneLayer].insertFront(pane, std::move(onDismiss)).second)
    throw GuiException("Pane displayed twice in PaneManager::displayPane");

  if (!pane->hasDisplayed() && pane->anchor() == PaneAnchor::None)
    pane->setPosition(Vec2I((windowSize() - pane->size()) / 2) + pane->centerOffset()); // center it

  pane->displayed();
}

bool PaneManager::isDisplayed(PanePtr const& pane) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (layerPair.second.contains(pane))
      return true;
  }

  return false;
}

void PaneManager::dismissPane(PanePtr const& pane) {
  if (!dismiss(pane))
    throw GuiException("No such pane in PaneManager::dismissPane");
}

void PaneManager::dismissAllPanes(Set<PaneLayer> const& paneLayers) {
  for (auto const& paneLayer : paneLayers) {
    for (auto const& panePair : copy(m_displayedPanes[paneLayer]))
      dismiss(panePair.first);
  }
}

void PaneManager::dismissAllPanes() {
  for (auto layerPair : copy(m_displayedPanes)) {
    for (auto const& panePair : layerPair.second)
      dismiss(panePair.first);
  }
}

PanePtr PaneManager::topPane(Set<PaneLayer> const& paneLayers) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (paneLayers.contains(layerPair.first) && !layerPair.second.empty())
      return layerPair.second.firstKey();
  }
  return {};
}

PanePtr PaneManager::topPane() const {
  for (auto const& layerPair : m_displayedPanes) {
    if (!layerPair.second.empty())
      return layerPair.second.firstKey();
  }
  return {};
}

void PaneManager::bringToTop(PanePtr const& pane) {
  for (auto& layerPair : m_displayedPanes) {
    if (layerPair.second.contains(pane)) {
      layerPair.second.toFront(pane);
      return;
    }
  }

  throw GuiException("Pane was not displayed in PaneManager::bringToTop");
}

void PaneManager::bringPaneAdjacent(PanePtr const& anchor, PanePtr const& adjacent, int gap) {
  Vec2I centerAdjacent = anchor->position() + (anchor->size() / 2) - (adjacent->size() / 2);
  centerAdjacent = centerAdjacent.piecewiseClamp(Vec2I(), windowSize() - adjacent->size()); // keeps pane inside window

  if (anchor->position()[0] + anchor->size()[0] + gap + adjacent->size()[0] <= windowSize()[0])
    adjacent->setPosition(Vec2I(anchor->position()[0] + anchor->size()[0] + gap, centerAdjacent[1])); // place to the right
  else if (anchor->position()[0] - gap - adjacent->size()[0] >= 0)
    adjacent->setPosition(Vec2I(anchor->position()[0] - gap - adjacent->size()[0], centerAdjacent[1])); // place to the left
  else if (anchor->position()[1] + anchor->size()[1] + gap + adjacent->size()[1] <= windowSize()[1])
    adjacent->setPosition(Vec2I(centerAdjacent[0], anchor->position()[1] + anchor->size()[1] + gap)); // place above
  else if (anchor->position()[1] - gap - adjacent->size()[1] >= 0)
    adjacent->setPosition(Vec2I(centerAdjacent[0], anchor->position()[1] - gap - adjacent->size()[1])); // place below
  else
    adjacent->setPosition(centerAdjacent);

  bringToTop(adjacent);
}

PanePtr PaneManager::getPaneAt(Set<PaneLayer> const& paneLayers, Vec2I const& position) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (!paneLayers.contains(layerPair.first))
      continue;

    for (auto const& panePair : layerPair.second) {
      if (panePair.first->inWindow(position) && panePair.first->active())
        return panePair.first;
    }
  }

  return {};
}

PanePtr PaneManager::getPaneAt(Vec2I const& position) const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (panePair.first->inWindow(position) && panePair.first->active())
        return panePair.first;
    }
  }

  return {};
}

void PaneManager::setBackgroundWidget(WidgetPtr bg) {
  m_backgroundWidget = bg;
}

void PaneManager::dismissWhere(function<bool(PanePtr const&)> func) {
  if (!func)
    return;

  for (auto& layerPair : m_displayedPanes) {
    eraseWhere(layerPair.second, [&](auto& panePair) {
      if (func(panePair.first)) {
        panePair.first->dismissed();
        if (panePair.second)
          panePair.second(panePair.first);
        return true;
      }
      return false;
    });
  }
}

PanePtr PaneManager::keyboardCapturedPane() const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (panePair.first->active() && panePair.first->keyboardCaptured() != KeyboardCaptureMode::None)
        return panePair.first;
    }
  }

  return {};
}

bool PaneManager::keyboardCapturedForTextInput() const {
  if (auto pane = keyboardCapturedPane())
    return pane->keyboardCaptured() == KeyboardCaptureMode::TextInput;
  return false;
}

bool PaneManager::sendInputEvent(InputEvent const& event) {
  if (event.is<MouseMoveEvent>()) {
    m_tooltipLastMousePos = *m_context->mousePosition(event);

    for (auto const& layerPair : m_displayedPanes) {
      for (auto const& panePair : layerPair.second) {
        if (panePair.first->dragActive()) {
          panePair.first->drag(*m_context->mousePosition(event));
          return true;
        }
      }
    }
  }

  if (event.is<MouseButtonDownEvent>()) {
    m_tooltipShowTimer = m_tooltipMouseoverTime;
    if (m_activeTooltip) {
      dismiss(m_activeTooltip);
      m_activeTooltip.reset();
      m_tooltipParentPane.reset();
      m_tooltipShowTimer = m_tooltipMouseoverTime;
    }
  }

  if (event.is<MouseButtonUpEvent>()) {
    for (auto const& layerPair : m_displayedPanes) {
      for (auto const& panePair : layerPair.second) {
        if (panePair.first->dragActive()) {
          panePair.first->setDragActive(false, {});
          return true;
        }
      }
    }
  }

  // The gui close event can only be intercepted by a pane that has captured
  // the keyboard otherwise it will always be used to close first before being
  // a normal event. This is so a window can control its own closing if it
  // really needs to (like the keybindings window).
  if (event.is<KeyDownEvent>() && m_context->actions(event).contains(InterfaceAction::GuiClose)) {
    if (auto top = topPane({PaneLayer::ModalWindow, PaneLayer::Window})) {
      dismiss(top);
      return true;
    }
  }

  // If there is a pane that has captured the keyboard, keyboard events will
  // ONLY be sent to it.
  auto keyCapturePane = keyboardCapturedPane();
  if (keyCapturePane && (event.is<KeyDownEvent>() || event.is<KeyUpEvent>() || event.is<TextInputEvent>()))
    return keyCapturePane->sendEvent(event);

  bool foundModal = false;
  for (auto& layerPair : m_displayedPanes) {
    for (auto const& panePair : copy(layerPair.second)) {
      if (panePair.first->sendEvent(event)) {
        if (event.is<MouseButtonDownEvent>())
          layerPair.second.toFront(panePair.first);
        return true;
      }
      // If any modal windows are shown, Only the first modal window should
      // have a chance to consume the input event and all other panes below it
      // including different layers should ignore it.
      if (layerPair.first == PaneLayer::ModalWindow) {
        foundModal = true;
        break;
      }
    }
    if (foundModal)
      break;
  }

  return false;
}

void PaneManager::render() {
  if (m_backgroundWidget) {
    auto size = m_backgroundWidget->size();
    m_backgroundWidget->setPosition(Vec2I((windowSize()[0] - size[0]) / 2, (windowSize()[1] - size[1]) / 2));
    m_backgroundWidget->render(RectI(Vec2I(), windowSize()));
  }

  for (auto const& layerPair : reverseIterate(m_displayedPanes)) {
    for (auto const& panePair : reverseIterate(layerPair.second)) {
      if (panePair.first->active()) {
        if (m_prevInterfaceScale != m_context->interfaceScale())
          panePair.first->setPosition(
              calculateNewInterfacePosition(panePair.first, (float)m_context->interfaceScale() / m_prevInterfaceScale));

        panePair.first->setDrawingOffset(calculatePaneOffset(panePair.first));
        panePair.first->render(RectI(Vec2I(), windowSize()));
      }
    }
  }

  m_context->resetInterfaceScissorRect();
  m_prevInterfaceScale = m_context->interfaceScale();
}

void PaneManager::update(float dt) {
  m_tooltipShowTimer -= GlobalTimestep;
  if (m_tooltipParentPane.expired())
    m_tooltipParentPane.reset();

  bool removeTooltip = vmag(m_tooltipInitialPosition - m_tooltipLastMousePos) > m_tooltipMouseoverRadius
    || m_tooltipParentPane.expired() || m_tooltipParentPane.lock()->inWindow(m_tooltipLastMousePos);

  if (removeTooltip) {
    dismiss(m_activeTooltip);
    m_activeTooltip.reset();
    m_tooltipParentPane.reset();
  }

  // Scan for a new tooltip if we just removed the old one, or the show timer has expired
  if (removeTooltip || (m_tooltipShowTimer < 0 && !m_activeTooltip)) {
    if (auto parentPane = getPaneAt(m_tooltipLastMousePos)) {
      if (auto tooltip = parentPane->createTooltip(m_tooltipLastMousePos)) {
        m_activeTooltip = std::move(tooltip);
        m_tooltipParentPane = std::move(parentPane);
        m_tooltipInitialPosition = m_tooltipLastMousePos;
        displayPane(PaneLayer::Tooltip, m_activeTooltip);
      } else {
        m_tooltipShowTimer = m_tooltipMouseoverTime;
      }
    }
  }

  if (m_activeTooltip) {
    Vec2I offsetDirection = Vec2I::filled(1);
    Vec2I offsetAdjust = Vec2I();

    if (m_tooltipLastMousePos[0] + m_tooltipMouseOffset[0] + m_activeTooltip->size()[0] > (int)m_context->windowWidth() / m_context->interfaceScale()) {
      offsetDirection[0] = -1;
      offsetAdjust[0] = -m_activeTooltip->size()[0];
    }

    if (m_tooltipLastMousePos[1] + m_tooltipMouseOffset[1] - m_activeTooltip->size()[1] < 0)
      offsetDirection[1] = -1;
    else
      offsetAdjust[1] = -m_activeTooltip->size()[1];

    m_activeTooltip->setPosition(m_tooltipLastMousePos + (offsetAdjust + m_tooltipMouseOffset.piecewiseMultiply(offsetDirection)));
  }

  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : copy(layerPair.second)) {
      if (panePair.first->isDismissed())
        dismiss(panePair.first);
    }
  }

  for (auto const& layerPair : reverseIterate(m_displayedPanes)) {
    for (auto const& panePair : reverseIterate(layerPair.second)) {
      panePair.first->tick(dt);
      if (panePair.first->active())
        panePair.first->update(dt);
    }
  }
}

Vec2I PaneManager::windowSize() const {
  return Vec2I(m_context->windowInterfaceSize());
}

Vec2I PaneManager::calculatePaneOffset(PanePtr const& pane) const {
  Vec2I size = pane->size();
  switch (pane->anchor()) {
    case PaneAnchor::None:
      return pane->anchorOffset();
    case PaneAnchor::BottomLeft:
      return pane->anchorOffset();
    case PaneAnchor::BottomRight:
      return pane->anchorOffset() + Vec2I{windowSize()[0] - size[0], 0};
    case PaneAnchor::TopLeft:
      return pane->anchorOffset() + Vec2I{0, windowSize()[1] - size[1]};
    case PaneAnchor::TopRight:
      return pane->anchorOffset() + (windowSize() - size);
    case PaneAnchor::CenterTop:
      return pane->anchorOffset() + Vec2I{(windowSize()[0] - size[0]) / 2, windowSize()[1] - size[1]};
    case PaneAnchor::CenterBottom:
      return pane->anchorOffset() + Vec2I{(windowSize()[0] - size[0]) / 2, 0};
    case PaneAnchor::CenterLeft:
      return pane->anchorOffset() + Vec2I{0, (windowSize()[1] - size[1]) / 2};
    case PaneAnchor::CenterRight:
      return pane->anchorOffset() + Vec2I{windowSize()[0] - size[0], (windowSize()[1] - size[1]) / 2};
    case PaneAnchor::Center:
      return pane->anchorOffset() + ((windowSize() - size) / 2);
    default:
      return pane->anchorOffset();
  }
}

Vec2I PaneManager::calculateNewInterfacePosition(PanePtr const& pane, float interfaceScaleRatio) const {
  Vec2F position(pane->relativePosition());
  Vec2F size(pane->size());
  Mat3F scale;
  switch (pane->anchor()) {
    case PaneAnchor::None:
      scale = Mat3F::scaling(interfaceScaleRatio, Vec2F(windowSize()) / 2);
      break;
    case PaneAnchor::BottomLeft:
      scale = Mat3F::scaling(interfaceScaleRatio);
      break;
    case PaneAnchor::BottomRight:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0], 0});
      break;
    case PaneAnchor::TopLeft:
      scale = Mat3F::scaling(interfaceScaleRatio, {0, size[1]});
      break;
    case PaneAnchor::TopRight:
      scale = Mat3F::scaling(interfaceScaleRatio, size);
      break;
    case PaneAnchor::CenterTop:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0] / 2, size[1]});
      break;
    case PaneAnchor::CenterBottom:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0] / 2, 0});
      break;
    case PaneAnchor::CenterLeft:
      scale = Mat3F::scaling(interfaceScaleRatio, {0, size[1] / 2});
      break;
    case PaneAnchor::CenterRight:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0], size[1] / 2});
      break;
    case PaneAnchor::Center:
      scale = Mat3F::scaling(interfaceScaleRatio, size / 2);
      break;
    default:
      scale = Mat3F::scaling(interfaceScaleRatio, Vec2F(windowSize()) / 2);
  }
  return Vec2I::round((scale * Vec3F(position, 0)).vec2());
}

bool PaneManager::dismiss(PanePtr const& pane) {
  bool dismissed = false;
  for (auto& layerPair : m_displayedPanes) {
    if (auto panePair = layerPair.second.maybeTake(pane)) {
      dismissed = true;
      panePair->first->dismissed();
      if (panePair->second)
        panePair->second(pane);
    }
  }

  return dismissed;
}

}
