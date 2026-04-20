#ifndef _GL4ES_SHADER_H_
#define _GL4ES_SHADER_H_

#include "khash.h"
#include "gles.h"

#define MAX_LINE_LENGTH 2048
#define MAX_VARIABLE_LENGTH 1024
#define MAX_INITIAL_VALUE_LENGTH 1024
#define MAX_UNIFORM_VARIABLE_NUMBER 1024

typedef struct {
    char variable[MAX_VARIABLE_LENGTH];
    char initial_value[MAX_INITIAL_VALUE_LENGTH];
} uniform_declaration_s;

typedef uniform_declaration_s uniforms_declarations[MAX_UNIFORM_VARIABLE_NUMBER];

#define shaderconv_need_t struct shaderconv_need_s
#include "oldprogram.h"
#undef shaderconv_need_t

#include <stdint.h>

typedef struct shaderconv_need_s {
    int need_color;     // front and back
    int need_secondary; //  same
    int need_fogcoord;
    int need_texcoord;     // max texcoord needed (-1 for none)
    int need_notexarray;   // need to not use tex array
    int need_normalmatrix; // if normal matrix is needed (for highp / mediump choosing)
    int need_mvmatrix;
    int need_mvpmatrix;
    int need_clean; // this shader needs to stay "clean", no hack in here
    int need_clipvertex;
    uint32_t need_texs; // flags of what tex is needed
} shaderconv_need_t;

struct shader_s {
    GLuint id;                // internal id of the shader
    GLenum type;              // type of the shader (GL_VERTEX or GL_FRAGMENT)
    int attached;             // number of time the shader is attached
    int deleted;              // flagged for deletion
    int compiled;             // flag if compiled
    struct oldprogram_s* old; // in case the shader is an old ARB ASM-like program
    char* source;             // original source of the shader (or converted if coming from "old")
    char* converted;          // converted source (or null if nothing)
    // shaderconv
    shaderconv_need_t need; // the varying need / provide of the shader
    uniforms_declarations uniforms_declarations;
    int uniforms_declarations_count;
    int is_converted_essl_320;
    char* before_patch;
}; // shader_t defined in oldprogram.h

KHASH_MAP_DECLARE_INT(shaderlist, struct shader_s*);

GLuint APIENTRY_GL4ES gl4es_glCreateShader(GLenum shaderType);
void APIENTRY_GL4ES gl4es_glDeleteShader(GLuint shader);
void APIENTRY_GL4ES gl4es_glCompileShader(GLuint shader);
void APIENTRY_GL4ES gl4es_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string,
                                         const GLint* length);
void APIENTRY_GL4ES gl4es_glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source);
GLboolean APIENTRY_GL4ES gl4es_glIsShader(GLuint shader);
void APIENTRY_GL4ES gl4es_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void APIENTRY_GL4ES gl4es_glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
void APIENTRY_GL4ES gl4es_glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint* range,
                                                     GLint* precision);
void APIENTRY_GL4ES gl4es_glShaderBinary(GLsizei count, const GLuint* shaders, GLenum binaryFormat, const void* binary,
                                         GLsizei length);
void APIENTRY_GL4ES gl4es_glReleaseShaderCompiler(void);

void accumShaderNeeds(GLuint shader, shaderconv_need_t* need);
int isShaderCompatible(GLuint shader, shaderconv_need_t* need);
void redoShader(GLuint shader, shaderconv_need_t* need);
struct shader_s* getShader(GLuint shader);

#define CHECK_SHADER(type, shader)                                                                                     \
    if (!shader) {                                                                                                     \
        noerrorShim();                                                                                                 \
        return (type)0;                                                                                                \
    }                                                                                                                  \
    struct shader_s* glshader = NULL;                                                                                  \
    khint_t k_##shader;                                                                                                \
    {                                                                                                                  \
        khash_t(shaderlist)* shaders = glstate->glsl->shaders;                                                         \
        k_##shader = kh_get(shaderlist, shaders, shader);                                                              \
        if (k_##shader != kh_end(shaders)) glshader = kh_value(shaders, k_##shader);                                   \
    }                                                                                                                  \
    if (!glshader) {                                                                                                   \
        errorShim(GL_INVALID_OPERATION);                                                                               \
        return (type)0;                                                                                                \
    }

// ========== GL_ARB_shader_objects ==============

GLhandleARB APIENTRY_GL4ES gl4es_glCreateShaderObject(GLenum shaderType);

#endif // _GL4ES_SHADER_H_
