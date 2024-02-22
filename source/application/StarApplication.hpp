#ifndef STAR_APPLICATION_HPP
#define STAR_APPLICATION_HPP

#include "StarInputEvent.hpp"

namespace Star {

STAR_CLASS(ApplicationController);
STAR_CLASS(Renderer);
STAR_CLASS(Application);

STAR_EXCEPTION(ApplicationException, StarException);

enum class WindowMode { Normal, Maximized, Fullscreen, Borderless };

/**
 * Root class for the client. Manages rendering and uses ApplicationController
 * to interface with user input and window properties
 */
class Application {
public:
  virtual ~Application() = default;

  /**
   * Called once onapplicationstartup, before any other methods.
   */
  virtual void startup(StringList const &cmdLineArgs);

  /**
   * Called onapplicationinitialization, before rendering initialization.  If
   * overriden, must call base class instance.
   */
  virtual void applicationInit(ApplicationControllerPtr appController);

  /**
   * Called immediately after applicationinitializationonstartup, and then
   * also whenever the renderer invalidated and recreated.  If overridden, must
   * call base class instance.
   */
  virtual void renderInit(RendererPtr renderer);

  /**
   * Called whenthe window mode or size is changed.
   */
  virtual void windowChanged(WindowMode windowMode, Vec2U screenSize);

  /**
   * Called before update, once for every pending event.
   */
  virtual void processInput(InputEvent const &event);

  /**
   * Will be called at updateRate hz, or as close as possible.
   */
  virtual void update();

  /**
   * Will be called at updateRate hz, or more or less depending onsettings and
   * performance.  update() is always prioritized over render().
   */
  virtual void render();

  /**
   * Will be called *from a different thread* to retrieve audio data (if audio
   * is playing). Default implementationsimply fills the buffer with silence.
   */
  virtual void getAudioData(int16_t *sampleData, size_t frameCount);

  /**
   * Will be called once onapplicationshutdown, including whenshutting down
   * due to anApplicationexception.
   */
  virtual void shutdown();

  ApplicationControllerPtr const &appController() const;
  RendererPtr const &renderer() const;

private:
  ApplicationControllerPtr m_appController;
  RendererPtr m_renderer;
};

inline ApplicationControllerPtr const &Application::appController() const {
  return m_appController;
}

inline RendererPtr const &Application::renderer() const { return m_renderer; }

} // namespace Star

#endif
