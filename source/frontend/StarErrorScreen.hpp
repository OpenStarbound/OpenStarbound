#pragma once

#include "StarVector.hpp"
#include "StarString.hpp"
#include "StarInterfaceCursor.hpp"
#include "StarInputEvent.hpp"

namespace Star {

STAR_CLASS(Pane);
STAR_CLASS(PaneManager);
STAR_CLASS(GuiContext);

STAR_CLASS(ErrorScreen);

class ErrorScreen {
public:
  ErrorScreen();

  // Resets accepted
  void setMessage(String const& message);

  bool accepted();

  void render();

  bool handleInputEvent(InputEvent const& event);
  void update(float dt);

private:
  void renderCursor();

  float interfaceScale() const;
  unsigned windowHeight() const;
  unsigned windowWidth() const;

  GuiContext* m_guiContext;
  PaneManagerPtr m_paneManager;
  PanePtr m_errorPane;

  bool m_accepted;
  Vec2I m_cursorScreenPos;
  InterfaceCursor m_cursor;
};

}
