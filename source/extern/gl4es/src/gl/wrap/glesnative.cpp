#include <GL/gl.h>
#include "../gl4es.h"
#include "../loader.h"
#include "gles.h"

// #define DEBUG

#ifndef __APPLE__
#define NATIVE_FUNCTION_HEAD(type, name, ...)                                                                          \
    extern "C" GLAPI GLAPIENTRY type name##ARB(__VA_ARGS__) __attribute__((alias(#name)));                             \
    extern "C" GLAPI GLAPIENTRY type name(__VA_ARGS__) {
#else
#define NATIVE_FUNCTION_HEAD(type, name, ...) extern "C" GLAPI GLAPIENTRY type name(__VA_ARGS__) {
#endif

#define NATIVE_FUNCTION_END(type, name, ...)                                                                           \
    DBG(SHUT_LOGD("Native function: %s(...)", __FUNCTION__);)                                                          \
    LOAD_GLES3(name);                                                                                                  \
    type ret = gles_##name(__VA_ARGS__);                                                                               \
    return ret;                                                                                                        \
    }

#define NATIVE_FUNCTION_END_NO_RETURN(type, name, ...)                                                                 \
    DBG(SHUT_LOGD("Native function: %s(...)", __FUNCTION__);)                                                          \
    LOAD_GLES3(name);                                                                                                  \
    gles_##name(__VA_ARGS__);                                                                                          \
    }

NATIVE_FUNCTION_HEAD(GLsync, glFenceSync, GLenum condition, GLbitfield flags)
NATIVE_FUNCTION_END(GLsync, glFenceSync, condition, flags)

NATIVE_FUNCTION_HEAD(GLboolean, glIsSync, GLsync sync)
NATIVE_FUNCTION_END(GLboolean, glIsSync, sync)

NATIVE_FUNCTION_HEAD(void, glDeleteSync, GLsync sync)
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteSync, sync)

NATIVE_FUNCTION_HEAD(GLenum, glClientWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)
NATIVE_FUNCTION_END(GLenum, glClientWaitSync, sync, flags, timeout)

NATIVE_FUNCTION_HEAD(void, glWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)
NATIVE_FUNCTION_END_NO_RETURN(void, glWaitSync, sync, flags, timeout)

NATIVE_FUNCTION_HEAD(void, glGetSynciv, GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)
NATIVE_FUNCTION_END_NO_RETURN(void, glGetSynciv, sync, pname, bufSize, length, values)

NATIVE_FUNCTION_HEAD(void, glDrawArraysIndirect, GLenum mode, const void* indirect)
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawArraysIndirect, mode, indirect)

VISIBLE void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const char* name) {
    LOAD_GLES3(glBindFragDataLocationIndexedEXT);
    gles_glBindFragDataLocationIndexedEXT(program, colorNumber, index, name);
}

VISIBLE GLint glGetFragDataIndex(GLuint program, const char* name) {
    LOAD_GLES3(glGetFragDataIndexEXT);
    return gles_glGetFragDataIndexEXT(program, name);
}

// VISIBLE void glBindFragDataLocation(GLuint program, GLuint colorNumber, const char *name) {
//     LOAD_GLES3(glBindFragDataLocationEXT);
//     gles_glBindFragDataLocationEXT(program, colorNumber, name);
// }

VISIBLE GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const char* name) {
    LOAD_GLES3(glGetProgramResourceLocationIndexEXT);
    return gles_glGetProgramResourceLocationIndexEXT(program, programInterface, name);
}
