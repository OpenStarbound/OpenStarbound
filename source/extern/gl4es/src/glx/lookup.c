#include <dlfcn.h>
#include "../gl/attributes.h"
#include "../gl/init.h"
#include "../gl/logs.h"

#define STUB_FCT glXStub
#include "../gl/gl_lookup.h"

#include "glx.h"
#include "hardext.h"

void glXStub(void* x, ...) {
    return;
}
void* gl4es_glXGetProcAddress(const char* name) __attribute__((visibility("default")));
void* gl4es_glXGetProcAddress(const char* name) {
    void* proc = dlsym(RTLD_DEFAULT, (const char*)name);

    if (!proc) {
        DBG(SHUT_LOGD("[WARNING] Failed to get OpenGL function: %s", (const char*)name));
        return NULL;
    }

    return proc;
}
#ifdef AMIGAOS4
// AliasExport(void*,aglGetProcAddress,,(const char* name));
#else
AliasExport(void*, glXGetProcAddress, , (const char* name));
AliasExport(void*, glXGetProcAddress, ARB, (const char* name));
#endif