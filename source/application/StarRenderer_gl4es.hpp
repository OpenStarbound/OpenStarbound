// Thanks to https://gitlab.com/OpenMW/openmw!
#ifndef GL4ESINIT_HPP
#define GL4ESINIT_HPP
#define SDL_FUNCTION_POINTER_IS_VOID_POINTER
#include <SDL3/SDL_video.h>

// Must be called once SDL video mode has been set,
// which creates a context.
//
// GL4ES can then query the context for features and extensions.
extern "C" void gl4es_init(SDL_Window* window);

#endif // GL4ESINIT_HPP
