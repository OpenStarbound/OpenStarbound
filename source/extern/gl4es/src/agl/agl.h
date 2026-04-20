#ifndef _AGL_H_
#define _AGL_H_

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef EXEC_INTERFACES_H
#include <exec/interfaces.h>
#endif

#ifndef GRAPHICS_GFX_H
#include <graphics/gfx.h>
#endif

#ifndef OGLES2_OGLES2_DEFS_H
// it would be better to have an include with only the CreateContextTags enum difed, to avoid conflict
//  of other typedef with full OpenGL header file...
//#include <ogles2/ogles2_defs.h>
#endif

void* aglCreateContext(ULONG * errcode, struct TagItem * tags);
void* aglCreateContext2(ULONG * errcode, struct TagItem * tags);
//void* aglCreateContextTags(ULONG * errcode, ...);
void aglDestroyContext(void* context);
void aglMakeCurrent(void* context);
void aglSwapBuffers();
void aglSetParams2(struct TagItem * tags);
void aglSetBitmap(struct BitMap *bitmap);
void* aglGetProcAddress(const char* name);

#endif //_AGL_H_