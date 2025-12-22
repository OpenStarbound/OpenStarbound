#include "StarApplication.hpp"
#include "StarTime.hpp"
#include "StarLogging.hpp"

namespace Star {

#ifdef STAR_ENABLE_STEAM_INTEGRATION
#ifdef STAR_SYSTEM_LINUX
// Hacky, but I need to use this from StarClientApplication
// and StarPlatformServies_pc...
bool g_steamIsFlatpak = false;
#endif
#endif

void Application::startup(StringList const&) {}

void Application::applicationInit(ApplicationControllerPtr appController) {
  m_appController = std::move(appController);
}

void Application::renderInit(RendererPtr renderer) {
  m_renderer = std::move(renderer);
}

void Application::windowChanged(WindowMode, Vec2U) {}

void Application::processInput(InputEvent const&) {}

void Application::update() {}

unsigned Application::framesSkipped() const { return 0; }

void Application::render() {}

void Application::getAudioData(int16_t* samples, size_t sampleCount) {
  for (size_t i = 0; i < sampleCount; ++i)
    samples[i] = 0;
}

void Application::shutdown() {}
}
