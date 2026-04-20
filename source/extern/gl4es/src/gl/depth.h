#ifndef _GL4ES_DEPTH_H_
#define _GL4ES_DEPTH_H_

#include "gles.h"

void APIENTRY_GL4ES gl4es_glDepthFunc(GLenum func);
void APIENTRY_GL4ES gl4es_glDepthMask(GLboolean flag);
void APIENTRY_GL4ES gl4es_glDepthRangef(GLclampf nearVal, GLclampf farVal);
void APIENTRY_GL4ES gl4es_glClearDepthf(GLclampf depth);

#endif // _GL4ES_DEPTH_H_
