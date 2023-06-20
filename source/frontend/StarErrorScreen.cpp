#include "StarErrorScreen.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarPaneManager.hpp"
#include "StarLabelWidget.hpp"
#include "StarAssets.hpp"
#include "StarGameTypes.hpp"

namespace Star {

ErrorScreen::ErrorScreen() {
  m_paneManager = make_shared<PaneManager>();

  m_accepted = false;

  auto assets = Root::singleton().assets();

  m_guiContext = GuiContext::singletonPtr();

  m_errorPane = make_shared<Pane>();
  GuiReader reader;
  reader.registerCallback("btnOk", [this](Widget*) {
      m_accepted = true;
    });
  reader.construct(assets->json("/interface/windowconfig/error.config:paneLayout"), m_errorPane.get());

  m_paneManager->displayPane(PaneLayer::Window, m_errorPane, [this](PanePtr) {
      m_accepted = true;
    });
}

void ErrorScreen::setMessage(String const& errorMessage) {
  m_errorPane->fetchChild<LabelWidget>("labelError")->setText(errorMessage);
  m_accepted = false;
}

bool ErrorScreen::accepted() {
  return m_accepted;
}

void ErrorScreen::render() {
  auto assets = Root::singleton().assets();

  for (auto backdropImage : assets->json("/interface/windowconfig/title.config:backdropImages").toArray()) {
    Vec2F offset = jsonToVec2F(backdropImage.get(0)) * interfaceScale();
    String image = backdropImage.getString(1);
    float scale = backdropImage.getFloat(2);
    Vec2F imageSize = Vec2F(m_guiContext->textureSize(image)) * interfaceScale() * scale;

    Vec2F lowerLeft = Vec2F(windowWidth() / 2.0f, windowHeight());
    lowerLeft[0] -= imageSize[0] / 2;
    lowerLeft[1] -= imageSize[1];
    lowerLeft += offset;
    RectF screenCoords(lowerLeft, lowerLeft + imageSize);
    m_guiContext->drawQuad(image, screenCoords);
  }

  m_paneManager->render();
  renderCursor();
}

bool ErrorScreen::handleInputEvent(InputEvent const& event) {
  if (auto mouseMove = event.ptr<MouseMoveEvent>())
    m_cursorScreenPos = mouseMove->mousePosition;

  return m_paneManager->sendInputEvent(event);
}

void ErrorScreen::update() {
  m_paneManager->update();
}

void ErrorScreen::renderCursor() {
  m_cursor.update(WorldTimestep);
  Vec2I cursorPos = m_cursorScreenPos;
  Vec2I cursorSize = m_cursor.size();
  Vec2I cursorOffset = m_cursor.offset();

  cursorPos[0] -= cursorOffset[0] * interfaceScale();
  cursorPos[1] -= (cursorSize[1] - cursorOffset[1]) * interfaceScale();
  m_guiContext->drawDrawable(m_cursor.drawable(), Vec2F(cursorPos), interfaceScale());
}

float ErrorScreen::interfaceScale() const {
  return m_guiContext->interfaceScale();
}

unsigned ErrorScreen::windowHeight() const {
  return m_guiContext->windowHeight();
}

unsigned ErrorScreen::windowWidth() const {
  return m_guiContext->windowWidth();
}

}
