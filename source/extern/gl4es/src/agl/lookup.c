#include "../gl/attributes.h"
#include "../gl/init.h"
#include "../gl/logs.h"
#define STUB_FCT aglStub
#include "../gl/gl_lookup.h"

#include "../glx/glx.h"
#include "../glx/hardext.h"

void aglStub(void *x, ...) {
    return;
}

// Copy/ Paste of agl specifics function, but simplified whithout the Amiga speicifc types
void* aglCreateContext(void * errcode, void * tags);
void* aglCreateContext2(void * errcode, void * tags);
void aglDestroyContext(void* context);
void aglMakeCurrent(void* context);
void aglSwapBuffers();
void aglSetParams2(void * tags);
void aglSetBitmap(void * bitmap);
void* aglGetProcAddress(const char* name);
// TODO: something less error prone, but agl.h cannot be included here, as some Amiga header conflict with some GLES ones

void *gl4es_aglGetProcAddress(const char *name) {

    MAP("glXSwapIntervalMESA", gl4es_glXSwapInterval);
    MAP("glXSwapIntervalSGI", gl4es_glXSwapInterval);
    _EX(aglGetProcAddress);
    MAP("glXGetProcAddress", gl4es_aglGetProcAddress);
    MAP("glXGetProcAddressARB", gl4es_aglGetProcAddress);
    // direct mapping
    MAP("aglCreateContext", aglCreateContext);
    MAP("aglCreateContext2", aglCreateContext2);
    MAP("aglDestroyContext", aglDestroyContext);
    MAP("aglMakeCurrent", aglMakeCurrent);
    MAP("aglSwapBuffers", aglSwapBuffers);
    MAP("aglSetParams2", aglSetParams2);
    MAP("aglSetBitmap", aglSetBitmap);

    return gl4es_GetProcAddress(name);
}
AliasExport(void*,aglGetProcAddress,,(const char* name));
