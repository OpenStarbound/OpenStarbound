#pragma once

#include "StarInputEvent.hpp"

namespace Star {

STAR_CLASS(ApplicationController);
STAR_CLASS(Renderer);
STAR_CLASS(Application);

STAR_EXCEPTION(ApplicationException, StarException);

enum class WindowMode {
  Normal,
  Maximized,
  Fullscreen,
  Borderless
};

class Application {
public:
  virtual ~Application() = default;

  // Called once on application startup, before any other methods.
  virtual void startup(StringList const& cmdLineArgs);

  // Called on application initialization, before rendering initialization.  If
  // overriden, must call base class instance.
  virtual void applicationInit(ApplicationControllerPtr appController);

  // Called immediately after application initialization on startup, and then
  // also whenever the renderer invalidated and recreated.  If overridden, must
  // call base class instance.
  virtual void renderInit(RendererPtr renderer);

  // Called when the window mode or size is changed.
  virtual void windowChanged(WindowMode windowMode, Vec2U screenSize);

  // Called before update, once for every pending event.
  virtual void processInput(InputEvent const& event);

  // Will be called at updateRate hz, or as close as possible.
  virtual void update();

  // Will be called at updateRate hz, or more or less depending on settings and
  // performance.  update() is always prioritized over render().
  virtual void render();

  // Will be called *from a different thread* to retrieve audio data (if audio
  // is playing). Default implementation simply fills the buffer with silence.
  virtual void getAudioData(int16_t* sampleData, size_t frameCount);

  // Will be called once on application shutdown, including when shutting down
  // due to an Application exception.
  virtual void shutdown();

  ApplicationControllerPtr const& appController() const;
  RendererPtr const& renderer() const;

private:
  ApplicationControllerPtr m_appController;
  RendererPtr m_renderer;
};

inline ApplicationControllerPtr const& Application::appController() const {
  return m_appController;
}

inline RendererPtr const& Application::renderer() const {
  return m_renderer;
}

}
