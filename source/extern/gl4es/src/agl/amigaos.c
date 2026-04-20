#include "amigaos.h"

#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
// make sure we don't use inline version here
#undef __USE_INLINE__
#include <proto/ogles2.h>
#include <proto/Warp3DNova.h>

#include "../glx/hardext.h"
#include "../gl/logs.h"

#define MIN_W3DNOVA_LIB_VERSION 1
#define MIN_W3DNOVA_LIB_REVISION 62
#define MIN_OGLES2_LIB_VERSION 1
#define MIN_OGLES2_LIB_REVISION 22

struct Library *LOGLES2 = NULL; 
struct OGLES2IFace *IOGLES2 = NULL;

struct so_bases
{
	struct ExecIFace *IExec;
};

struct so_bases so_exec;

// Open OGLES2 library and interface
void os4OpenLib(void** lib)
{	
	so_exec.IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;
	if(!IOGLES2) {
	  // first check version for Warp3DNova lib
	  struct Library *Warp3DNovaBase = NULL;
		Warp3DNovaBase = so_exec.IExec->OpenLibrary("Warp3DNova.library", MIN_W3DNOVA_LIB_VERSION);
		if(!Warp3DNovaBase) {
	        SHUT_LOGE("Error, cannot open Warp3DNova.library!\n");
	        return;
	    }
	    SHUT_LOGD("Using Warp3DNova.library v%d revision %d\n", Warp3DNovaBase->lib_Version, Warp3DNovaBase->lib_Revision);
		if (!(Warp3DNovaBase->lib_Version > MIN_W3DNOVA_LIB_VERSION || (Warp3DNovaBase->lib_Version == MIN_W3DNOVA_LIB_VERSION && Warp3DNovaBase->lib_Revision >= MIN_W3DNOVA_LIB_REVISION)))  {
	        SHUT_LOGE("Warning, your Warp3DNovaBase.library is too old, minimum is v%d.%d, please update!\n", MIN_W3DNOVA_LIB_VERSION,MIN_W3DNOVA_LIB_REVISION);
		}	
		//close warp3dnova.library, we open it just for version check
		so_exec.IExec->CloseLibrary(Warp3DNovaBase);
		Warp3DNovaBase = NULL;

	  LOGLES2 = so_exec.IExec->OpenLibrary("ogles2.library", MIN_OGLES2_LIB_VERSION);
	  if(!LOGLES2) {
	      SHUT_LOGE("Error, cannot open ogles2 Library!\n");
	      return;
	  }
	  SHUT_LOGD("Using OGLES2.library v%d revision %d\n", LOGLES2->lib_Version, LOGLES2->lib_Revision);
		if (!(LOGLES2->lib_Version > MIN_OGLES2_LIB_VERSION || (LOGLES2->lib_Version == MIN_OGLES2_LIB_VERSION && LOGLES2->lib_Revision >= MIN_OGLES2_LIB_REVISION)))  {
	        SHUT_LOGE("Warning, your OGLES2.library is too old, minimum is v%d.%d, please update!\n", MIN_OGLES2_LIB_VERSION,MIN_OGLES2_LIB_REVISION);
		}	
	  IOGLES2 = (struct OGLES2IFace *)so_exec.IExec->GetInterface(LOGLES2, "main", 1, NULL); 
	  if(!IOGLES2) {
	      SHUT_LOGE("Error, cannot open ogles2 Interface!\n");
	      so_exec.IExec->CloseLibrary(LOGLES2);
	      LOGLES2 = NULL;
	      return;
	  }
	  if (LOGLES2->lib_Version > 2 || (LOGLES2->lib_Version == 2 && LOGLES2->lib_Revision >= 9))  {
	      hardext.prgbin_n = 1;
	  }
	}
  *lib = LOGLES2;
  // small debug message, always helpful at beginning
  SHUT_LOGD("OGLES2 Library and Interface opened successfully!\n");
}

// Close OGLES2 lib and interface
void os4CloseLib()
{
    if(IOGLES2) {
        so_exec.IExec->DropInterface((struct Interface*)IOGLES2);
        IOGLES2 = NULL;
    }
    if(LOGLES2) {
        so_exec.IExec->CloseLibrary(LOGLES2);
        LOGLES2 = NULL;
    }
    SHUT_LOGD("OGLES2 Library and Interface closed\n");
}


static void AmiglActiveTexture (GLenum texture) {
    return IOGLES2->glActiveTexture(texture);
}

static void AmiglAttachShader (GLuint program, GLuint shader) {
    return IOGLES2->glAttachShader(program, shader);
}

static void AmiglBindAttribLocation (GLuint program, GLuint index, const GLchar *name) {
    return IOGLES2->glBindAttribLocation(program, index, name);
}

static void AmiglBindBuffer (GLenum target, GLuint buffer) {
    return IOGLES2->glBindBuffer(target, buffer);
}

static void AmiglBindFramebuffer (GLenum target, GLuint framebuffer) {
    return IOGLES2->glBindFramebuffer(target, framebuffer);
}

static void AmiglBindRenderbuffer (GLenum target, GLuint renderbuffer) {
    return IOGLES2->glBindRenderbuffer(target, renderbuffer);
}

static void AmiglBindTexture (GLenum target, GLuint texture) {
    return IOGLES2->glBindTexture(target, texture);
}

static void AmiglBlendColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    return IOGLES2->glBlendColor(red, green, blue, alpha);
}

static void AmiglBlendEquation (GLenum mode) {
    return IOGLES2->glBlendEquation(mode);
}

static void AmiglBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha) {
    return IOGLES2->glBlendEquationSeparate(modeRGB, modeAlpha);
}

static void AmiglBlendFunc (GLenum sfactor, GLenum dfactor) {
    return IOGLES2->glBlendFunc(sfactor, dfactor);
}

static void AmiglBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    return IOGLES2->glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static void AmiglBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    return IOGLES2->glBufferData(target, size, data, usage);
}

static void AmiglBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    return IOGLES2->glBufferSubData(target, offset, size, data);
}

static GLenum AmiglCheckFramebufferStatus (GLenum target) {
    return IOGLES2->glCheckFramebufferStatus(target);
}

static void AmiglClear (GLbitfield mask) {
    return IOGLES2->glClear(mask);
}

static void AmiglClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    return IOGLES2->glClearColor(red, green, blue, alpha);
}

static void AmiglClearDepthf (GLfloat d) {
    return IOGLES2->glClearDepthf(d);
}

static void AmiglClearStencil (GLint s) {
    return IOGLES2->glClearStencil(s);
}

static void AmiglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    return IOGLES2->glColorMask(red, green, blue, alpha);
}

static void AmiglCompileShader (GLuint shader) {
    return IOGLES2->glCompileShader(shader);
}

static void AmiglCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    return IOGLES2->glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

static void AmiglCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    return IOGLES2->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static void AmiglCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    return IOGLES2->glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

static void AmiglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    return IOGLES2->glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

static GLuint AmiglCreateProgram (void) {
    return IOGLES2->glCreateProgram();
}

static GLuint AmiglCreateShader (GLenum type) {
    return IOGLES2->glCreateShader(type);
}

static void AmiglCullFace (GLenum mode) {
    return IOGLES2->glCullFace(mode);
}

static void AmiglDeleteBuffers (GLsizei n, const GLuint *buffers) {
    return IOGLES2->glDeleteBuffers(n, buffers);
}

static void AmiglDeleteFramebuffers (GLsizei n, const GLuint *framebuffers) {
    return IOGLES2->glDeleteFramebuffers(n, framebuffers);
}

static void AmiglDeleteProgram (GLuint program) {
    return IOGLES2->glDeleteProgram(program);
}

static void AmiglDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers) {
    return IOGLES2->glDeleteRenderbuffers(n, renderbuffers);
}

static void AmiglDeleteShader (GLuint shader) {
    return IOGLES2->glDeleteShader(shader);
}

static void AmiglDeleteTextures (GLsizei n, const GLuint *textures) {
    return IOGLES2->glDeleteTextures(n, textures);
}

static void AmiglDepthFunc (GLenum func) {
    return IOGLES2->glDepthFunc(func);
}

static void AmiglDepthMask (GLboolean flag) {
    return IOGLES2->glDepthMask(flag);
}

static void AmiglDepthRangef (GLfloat n, GLfloat f) {
    return IOGLES2->glDepthRangef(n, f);
}

static void AmiglDetachShader (GLuint program, GLuint shader) {
    return IOGLES2->glDetachShader(program, shader);
}

static void AmiglDisable (GLenum cap) {
    return IOGLES2->glDisable(cap);
}

static void AmiglDisableVertexAttribArray (GLuint index) {
    return IOGLES2->glDisableVertexAttribArray(index);
}

static void AmiglDrawArrays (GLenum mode, GLint first, GLsizei count) {
    return IOGLES2->glDrawArrays(mode, first, count);
}

static void AmiglDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {
    return IOGLES2->glDrawElements(mode, count, type, indices);
}

static void AmiglEnable (GLenum cap) {
    return IOGLES2->glEnable(cap);
}

static void AmiglEnableVertexAttribArray (GLuint index) {
    return IOGLES2->glEnableVertexAttribArray(index);
}

static void AmiglFinish (void) {
    return IOGLES2->glFinish();
}

static void AmiglFlush (void) {
    return IOGLES2->glFlush();
}

static void AmiglFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    return IOGLES2->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

static void AmiglFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    return IOGLES2->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

static void AmiglFrontFace (GLenum mode) {
    return IOGLES2->glFrontFace(mode);
}

static void AmiglGenBuffers (GLsizei n, GLuint *buffers) {
    return IOGLES2->glGenBuffers(n, buffers);
}

static void AmiglGenerateMipmap (GLenum target) {
    return IOGLES2->glGenerateMipmap(target);
}

static void AmiglGenFramebuffers (GLsizei n, GLuint *framebuffers) {
    return IOGLES2->glGenFramebuffers(n, framebuffers);
}

static void AmiglGenRenderbuffers (GLsizei n, GLuint *renderbuffers) {
    return IOGLES2->glGenRenderbuffers(n, renderbuffers);
}

static void AmiglGenTextures (GLsizei n, GLuint *textures) {
    return IOGLES2->glGenTextures(n, textures);
}

static void AmiglGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    return IOGLES2->glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}

static void AmiglGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    return IOGLES2->glGetActiveUniform(program,  index, bufSize, length, size, type, name);
}

static void AmiglGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    return IOGLES2->glGetAttachedShaders(program, maxCount, count, shaders);
}

static GLint AmiglGetAttribLocation (GLuint program, const GLchar *name) {
    return IOGLES2->glGetAttribLocation(program, name);
}

static void AmiglGetBooleanv (GLenum pname, GLboolean *data) {
    return IOGLES2->glGetBooleanv(pname, data);
}

static void AmiglGetBufferParameteriv (GLenum target, GLenum pname, GLint *params) {
    return IOGLES2->glGetBufferParameteriv(target, pname, params);
}

static GLenum AmiglGetError (void) {
    return IOGLES2->glGetError();
}

static void AmiglGetFloatv (GLenum pname, GLfloat *data) {
    return IOGLES2->glGetFloatv(pname, data);
}

static void AmiglGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    return IOGLES2->glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

static void AmiglGetIntegerv (GLenum pname, GLint *data) {
    return IOGLES2->glGetIntegerv(pname, data);
}

static void AmiglGetProgramiv (GLuint program, GLenum pname, GLint *params) {
    return IOGLES2->glGetProgramiv(program, pname, params);
}

static void AmiglGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    return IOGLES2->glGetProgramInfoLog(program, bufSize, length, infoLog);
}

static void AmiglGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params) {
    return IOGLES2->glGetRenderbufferParameteriv(target, pname, params);
}

static void AmiglGetShaderiv (GLuint shader, GLenum pname, GLint *params) {
    return IOGLES2->glGetShaderiv(shader, pname, params);
}

static void AmiglGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    return IOGLES2->glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

static void AmiglGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    return IOGLES2->glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}

static void AmiglGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    return IOGLES2->glGetShaderSource(shader, bufSize, length, source);
}

static const GLubyte *AmiglGetString (GLenum name) {
    return IOGLES2->glGetString(name);
}

static void AmiglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {
    return IOGLES2->glGetTexParameterfv(target, pname, params);
}

static void AmiglGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {
    return IOGLES2->glGetTexParameteriv(target, pname, params);
}

static void AmiglGetUniformfv (GLuint program, GLint location, GLfloat *params) {
    return IOGLES2->glGetUniformfv(program, location, params);
}

static void AmiglGetUniformiv (GLuint program, GLint location, GLint *params) {
    return IOGLES2->glGetUniformiv(program, location, params);
}

static GLint AmiglGetUniformLocation (GLuint program, const GLchar *name) {
    return IOGLES2->glGetUniformLocation(program, name);
}

static void AmiglGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params) {
    return IOGLES2->glGetVertexAttribfv(index, pname, params);
}

static void AmiglGetVertexAttribiv (GLuint index, GLenum pname, GLint *params) {
    return IOGLES2->glGetVertexAttribiv(index, pname, params);
}

static void AmiglGetVertexAttribPointerv (GLuint index, GLenum pname, void **pointer) {
    return IOGLES2->glGetVertexAttribPointerv(index, pname, pointer);
}

static void AmiglHint (GLenum target, GLenum mode) {
    return IOGLES2->glHint(target, mode);
}

static GLboolean AmiglIsBuffer (GLuint buffer) {
    return IOGLES2->glIsBuffer(buffer);
}

static GLboolean AmiglIsEnabled (GLenum cap) {
    return IOGLES2->glIsEnabled(cap);
}

static GLboolean AmiglIsFramebuffer (GLuint framebuffer) {
    return IOGLES2->glIsFramebuffer(framebuffer);
}

static GLboolean AmiglIsProgram (GLuint program) {
    return IOGLES2->glIsProgram(program);
}

static GLboolean AmiglIsRenderbuffer (GLuint renderbuffer) {
    return IOGLES2->glIsRenderbuffer(renderbuffer);
}

static GLboolean AmiglIsShader (GLuint shader) {
    return IOGLES2->glIsShader(shader);
}

static GLboolean AmiglIsTexture (GLuint texture) {
    return IOGLES2->glIsTexture(texture);
}

static void AmiglLineWidth (GLfloat width) {
    return IOGLES2->glLineWidth(width);
}

static void AmiglLinkProgram (GLuint program) {
    return IOGLES2->glLinkProgram(program);
}

static void AmiglPixelStorei (GLenum pname, GLint param) {
    return IOGLES2->glPixelStorei(pname, param);
}

static void AmiglPolygonOffset (GLfloat factor, GLfloat units) {
    return IOGLES2->glPolygonOffset(factor, units);
}

static void AmiglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    return IOGLES2->glReadPixels(x, y, width, height, format, type, pixels);
}

static void AmiglReleaseShaderCompiler (void) {
    return IOGLES2->glReleaseShaderCompiler();
}

static void AmiglRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    return IOGLES2->glRenderbufferStorage(target, internalformat, width, height);
}

static void AmiglSampleCoverage (GLfloat value, GLboolean invert) {
    return IOGLES2->glSampleCoverage(value, invert);
}

static void AmiglScissor (GLint x, GLint y, GLsizei width, GLsizei height) {
    return IOGLES2->glScissor(x, y, width, height);
}

static void AmiglShaderBinary (GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) {
    return IOGLES2->glShaderBinary(count, shaders, binaryformat, binary, length);
}

static void AmiglShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    return IOGLES2->glShaderSource(shader, count, string, length);
}

static void AmiglStencilFunc (GLenum func, GLint ref, GLuint mask) {
    return IOGLES2->glStencilFunc(func, ref, mask);
}

static void AmiglStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask) {
    return IOGLES2->glStencilFuncSeparate(face, func, ref, mask);
}

static void AmiglStencilMask (GLuint mask) {
    return IOGLES2->glStencilMask(mask);
}

static void AmiglStencilMaskSeparate (GLenum face, GLuint mask) {
    return IOGLES2->glStencilMaskSeparate(face, mask);
}

static void AmiglStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
    return IOGLES2->glStencilOp(fail, zfail, zpass);
}

static void AmiglStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    return IOGLES2->glStencilOpSeparate(face, sfail, dpfail, dppass);
}

static void AmiglTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    return IOGLES2->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

static void AmiglTexParameterf (GLenum target, GLenum pname, GLfloat param) {
    return IOGLES2->glTexParameterf(target, pname, param);
}

static void AmiglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {
    return IOGLES2->glTexParameterfv(target, pname, params);
}

static void AmiglTexParameteri (GLenum target, GLenum pname, GLint param) {
    return IOGLES2->glTexParameteri(target, pname, param);
}

static void AmiglTexParameteriv (GLenum target, GLenum pname, const GLint *params) {
    return IOGLES2->glTexParameteriv(target, pname, params);
}

static void AmiglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    return IOGLES2->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void AmiglUniform1f (GLint location, GLfloat v0) {
    return IOGLES2->glUniform1f(location, v0);
}

static void AmiglUniform1fv (GLint location, GLsizei count, const GLfloat *value) {
    return IOGLES2->glUniform1fv(location, count, value);
}

static void AmiglUniform1i (GLint location, GLint v0) {
    return IOGLES2->glUniform1i(location, v0);
}

static void AmiglUniform1iv (GLint location, GLsizei count, const GLint *value) {
    return IOGLES2->glUniform1iv(location, count, value);
}

static void AmiglUniform2f (GLint location, GLfloat v0, GLfloat v1) {
    return IOGLES2->glUniform2f(location, v0, v1);
}

static void AmiglUniform2fv (GLint location, GLsizei count, const GLfloat *value) {
    return IOGLES2->glUniform2fv(location, count, value);
}

static void AmiglUniform2i (GLint location, GLint v0, GLint v1) {
    return IOGLES2->glUniform2i(location, v0, v1);
}

static void AmiglUniform2iv (GLint location, GLsizei count, const GLint *value) {
    return IOGLES2->glUniform2iv(location, count, value);
}

static void AmiglUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    return IOGLES2->glUniform3f(location, v0, v1, v2);
}

static void AmiglUniform3fv (GLint location, GLsizei count, const GLfloat *value) {
    return IOGLES2->glUniform3fv(location, count, value);
}

static void AmiglUniform3i (GLint location, GLint v0, GLint v1, GLint v2) {
    return IOGLES2->glUniform3i(location, v0, v1, v2);
}

static void AmiglUniform3iv (GLint location, GLsizei count, const GLint *value) {
    return IOGLES2->glUniform3iv(location, count, value);
}

static void AmiglUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    return IOGLES2->glUniform4f(location, v0, v1, v2, v3);
}

static void AmiglUniform4fv (GLint location, GLsizei count, const GLfloat *value) {
    return IOGLES2->glUniform4fv(location, count, value);
}

static void AmiglUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    return IOGLES2->glUniform4i(location, v0, v1, v2, v3);
}

static void AmiglUniform4iv (GLint location, GLsizei count, const GLint *value) {
    return IOGLES2->glUniform4iv(location, count, value);
}

static void AmiglUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return IOGLES2->glUniformMatrix2fv(location, count, transpose, value);
}

static void AmiglUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return IOGLES2->glUniformMatrix3fv(location, count, transpose, value);
}

static void AmiglUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return IOGLES2->glUniformMatrix4fv(location, count, transpose, value);
}

static void AmiglUseProgram (GLuint program) {
    return IOGLES2->glUseProgram(program);
}

static void AmiglValidateProgram (GLuint program) {
    return IOGLES2->glValidateProgram(program);
}

static void AmiglVertexAttrib1f (GLuint index, GLfloat x) {
    return IOGLES2->glVertexAttrib1f(index, x);
}

static void AmiglVertexAttrib1fv (GLuint index, const GLfloat *v) {
    return IOGLES2->glVertexAttrib1fv(index, v);
}

static void AmiglVertexAttrib2f (GLuint index, GLfloat x, GLfloat y) {
    return IOGLES2->glVertexAttrib2f(index, x, y);
}

static void AmiglVertexAttrib2fv (GLuint index, const GLfloat *v) {
    return IOGLES2->glVertexAttrib2fv(index, v);
}

static void AmiglVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    return IOGLES2->glVertexAttrib3f(index, x, y, z);
}

static void AmiglVertexAttrib3fv (GLuint index, const GLfloat *v) {
    return IOGLES2->glVertexAttrib3fv(index, v);
}

static void AmiglVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    return IOGLES2->glVertexAttrib4f(index, x, y, z, w);
}

static void AmiglVertexAttrib4fv (GLuint index, const GLfloat *v) {
    return IOGLES2->glVertexAttrib4fv(index, v);
}

static void AmiglVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    return IOGLES2->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

static void AmiglViewport (GLint x, GLint y, GLsizei width, GLsizei height) {
    return IOGLES2->glViewport(x, y, width, height);
}

// Using glXXX name, return the function pointer of that function in ogles2 library
#define MAP(func_name, func) \
    if (strcmp(name, func_name) == 0) return (void *)Ami##func;

#define EX(func_name) MAP(#func_name, func_name)

void* os4GetProcAddress(const char* name)
{
    if(!IOGLES2)
        return NULL;
    // All GL functions from OGLES2 drivers
    EX(glActiveTexture)
    EX(glAttachShader)
    EX(glBindAttribLocation)
    EX(glBindBuffer)
    EX(glBindFramebuffer)
    EX(glBindRenderbuffer)
    EX(glBindTexture)
    EX(glBlendColor)
    EX(glBlendEquation)
    EX(glBlendEquationSeparate)
    EX(glBlendFunc)
    EX(glBlendFuncSeparate)
    EX(glBufferData)
    EX(glBufferSubData)
    EX(glCheckFramebufferStatus)
    EX(glClear)
    EX(glClearColor)
    EX(glClearDepthf)
    EX(glClearStencil)
    EX(glColorMask)
    EX(glCompileShader)
    EX(glCompressedTexImage2D)
    EX(glCompressedTexSubImage2D)
    EX(glCopyTexImage2D)
    EX(glCopyTexSubImage2D)
    EX(glCreateProgram)
    EX(glCreateShader)
    EX(glCullFace)
    EX(glDeleteBuffers)
    EX(glDeleteFramebuffers)
    EX(glDeleteProgram)
    EX(glDeleteRenderbuffers)
    EX(glDeleteShader)
    EX(glDeleteTextures)
    EX(glDepthFunc)
    EX(glDepthMask)
    EX(glDepthRangef)
    EX(glDetachShader)
    EX(glDisable)
    EX(glDisableVertexAttribArray)
    EX(glDrawArrays)
    EX(glDrawElements)
    EX(glEnable)
    EX(glEnableVertexAttribArray)
    EX(glFinish)
    EX(glFlush)
    EX(glFramebufferRenderbuffer)
    EX(glFramebufferTexture2D)
    EX(glFrontFace)
    EX(glGenBuffers)
    EX(glGenerateMipmap)
    EX(glGenFramebuffers)
    EX(glGenRenderbuffers)
    EX(glGenTextures)
    EX(glGetActiveAttrib)
    EX(glGetActiveUniform)
    EX(glGetAttachedShaders)
    EX(glGetAttribLocation)
    EX(glGetBooleanv)
    EX(glGetBufferParameteriv)
    EX(glGetError)
    EX(glGetFloatv)
    EX(glGetFramebufferAttachmentParameteriv)
    EX(glGetIntegerv)
    EX(glGetProgramiv)
    EX(glGetProgramInfoLog)
    EX(glGetRenderbufferParameteriv)
    EX(glGetShaderiv)
    EX(glGetShaderInfoLog)
    EX(glGetShaderPrecisionFormat)
    EX(glGetShaderSource)
    EX(glGetString)
    EX(glGetTexParameterfv)
    EX(glGetTexParameteriv)
    EX(glGetUniformfv)
    EX(glGetUniformiv)
    EX(glGetUniformLocation)
    EX(glGetVertexAttribfv)
    EX(glGetVertexAttribiv)
    EX(glGetVertexAttribPointerv)
    EX(glHint)
    EX(glIsBuffer)
    EX(glIsEnabled)
    EX(glIsFramebuffer)
    EX(glIsProgram)
    EX(glIsRenderbuffer)
    EX(glIsShader)
    EX(glIsTexture)
    EX(glLineWidth)
    EX(glLinkProgram)
    EX(glPixelStorei)
    EX(glPolygonOffset)
    EX(glReadPixels)
    EX(glReleaseShaderCompiler)
    EX(glRenderbufferStorage)
    EX(glSampleCoverage)
    EX(glScissor)
    EX(glShaderBinary)
    EX(glShaderSource)
    EX(glStencilFunc)
    EX(glStencilFuncSeparate)
    EX(glStencilMask)
    EX(glStencilMaskSeparate)
    EX(glStencilOp)
    EX(glStencilOpSeparate)
    EX(glTexImage2D)
    EX(glTexParameterf)
    EX(glTexParameterfv)
    EX(glTexParameteri)
    EX(glTexParameteriv)
    EX(glTexSubImage2D)
    EX(glUniform1f)
    EX(glUniform1fv)
    EX(glUniform1i)
    EX(glUniform1iv)
    EX(glUniform2f)
    EX(glUniform2fv)
    EX(glUniform2i)
    EX(glUniform2iv)
    EX(glUniform3f)
    EX(glUniform3fv)
    EX(glUniform3i)
    EX(glUniform3iv)
    EX(glUniform4f)
    EX(glUniform4fv)
    EX(glUniform4i)
    EX(glUniform4iv)
    EX(glUniformMatrix2fv)
    EX(glUniformMatrix3fv)
    EX(glUniformMatrix4fv)
    EX(glUseProgram)
    EX(glValidateProgram)
    EX(glVertexAttrib1f)
    EX(glVertexAttrib1fv)
    EX(glVertexAttrib2f)
    EX(glVertexAttrib2fv)
    EX(glVertexAttrib3f)
    EX(glVertexAttrib3fv)
    EX(glVertexAttrib4f)
    EX(glVertexAttrib4fv)
    EX(glVertexAttribPointer)
    EX(glViewport)
    //EX(glPolygonMode) //This is a non-standard function, and gl4es will ignore it (and emulate it), even if Amiga OGLES2 driver implement it

    return IOGLES2->aglGetProcAddress(name);
}
