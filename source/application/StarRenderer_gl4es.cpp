// Thanks to https://gitlab.com/OpenMW/openmw!
// EGL does not work reliably for feature detection.
// Instead, we initialize gl4es manually.
#include "StarRenderer_gl4es.hpp"

// For glHint
#include <GL/gl.h>

extern "C"
{

#include <gl4eshint.h>
#include <gl4esinit.h>

    static SDL_Window* gWindow;

    void gl4es_GetMainFBSize(int* width, int* height)
    {
        SDL_GetWindowSize(gWindow, width, height);
    }

    void gl4es_init(SDL_Window* window)
    {
        gWindow = window;
        set_getprocaddress(SDL_GL_GetProcAddress);
        set_getmainfbsize(gl4es_GetMainFBSize);
        initialize_gl4es();

        // merge glBegin/glEnd in beams and console
        glHint(GL_BEGINEND_HINT_GL4ES, 1);
        // dxt unpacked to 16-bit looks ugly
        glHint(GL_AVOID16BITS_HINT_GL4ES, 1);
    }

} // extern "C"
