#ifndef GLSL_FOR_ES
#define GLSL_FOR_ES
#include <stdio.h>
#include <GL/gl.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char* GLSLtoGLSLES_c(const char* glsl_code, GLenum glsl_type, unsigned int essl_version, unsigned int glsl_version, int* return_code);

extern int getGLSLVersion(const char* glsl_code);
#ifdef __cplusplus
}
#endif
#endif
