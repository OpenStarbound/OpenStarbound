#include "agl.h"

#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
//#include <stdarg.h>
#undef __USE_INLINE__
#include <proto/exec.h>
#include <interfaces/ogles2.h>

extern struct OGLES2IFace *IOGLES2;

void* NewGLState(void* shared_glstate, int es2only);
void DeleteGLState(void* oldstate);
void ActivateGLState(void* new_glstate);
void GetHardwareExtensions(int notest);

typedef struct _agl_ctx_glstate {
    void* context;
    void* glstate;
} agl_ctx_glstate;

static agl_ctx_glstate *agl_context = NULL;
static int agl_context_len = 0;
static int agl_context_cap = 0;
static void* agl_current_ctx = NULL;

// find (or add if not found) a context in the list, and activate glstate...
void agl_context_find(void* ctx) {
    if(!ctx)
        return;
    if(!agl_context) {
        agl_context_cap = 10;
        agl_context = (agl_ctx_glstate*)malloc(sizeof(agl_ctx_glstate)*agl_context_cap);
        memset(agl_context, 0, sizeof(agl_ctx_glstate)*agl_context_cap);
    }
    int idx = 0;
    while (idx<agl_context_len && agl_context[idx].context!=ctx) idx++;
    if(idx==agl_context_len) {
        agl_context = (agl_ctx_glstate*)realloc(agl_context, sizeof(agl_ctx_glstate)*(agl_context_cap+10));
        memset(agl_context+agl_context_cap, 0, sizeof(agl_ctx_glstate)*10);
        agl_context_cap+=10;
    }
    agl_context[idx].context = ctx;
    if(idx==agl_context_len) ++agl_context_len;

    // create glstate if needed
    if(!agl_context[idx].glstate) {
        agl_context[idx].glstate = NewGLState(NULL, 0);
        // Hardware testing
        GetHardwareExtensions(0);
    }
    ActivateGLState(agl_context[idx].glstate);
}

// remove a context (delete array if size is null)
void agl_context_remove(void* ctx) {
    if (!ctx)
        return;
    if(!agl_context)
        return; // empty list?
    int idx = 0;
    while (idx<agl_context_len && agl_context[idx].context!=ctx) idx++;
    if(idx==agl_context_len)
        return; // not found...
    if(agl_context[idx].glstate) {
        DeleteGLState(agl_context[idx].glstate);
        agl_context[idx].glstate = NULL;
    }
    agl_context[idx].context = 0;
    // shrink if possible
    while(agl_context_len && !agl_context[agl_context_len-1].context) --agl_context_len;
    if(!agl_context_len) {
        agl_context_cap = 0;
        free(agl_context);
        agl_context = NULL;
    }
}

// AGL functions

void* aglCreateContext(ULONG * errcode, struct TagItem * tags) {
    if(IOGLES2)
        return IOGLES2->aglCreateContext2(errcode, tags);
    return NULL;
}
void* aglCreateContext2(ULONG * errcode, struct TagItem * tags) {
    return aglCreateContext(errcode, tags);
}
/*
void* VARARGS68K aglCreateContextTags(ULONG * errcode, ...) {
    void* ret = NULL;
    if(IOGLES2) {
        struct TagItem tags[100];
        VA_LIST args;
        VA_START(args, errcode);
        int i = 0;
        do {
            struct Tagitem tag = VA_ARG(args, struct TagItem);
            tags[i++] = tag;
        } while (tag!=TAG_DONE);
        VA_END(args);
        ret = IOGLES2->aglCreateContext2(errcode, tags);
    }
    return ret;
}
*/
void aglDestroyContext(void* context) {
    if(IOGLES2) {
        //bind the context before deleting stuffs.
        if(context!=agl_current_ctx)
            IOGLES2->aglMakeCurrent(context);

        agl_context_remove(context); // remove the associated glstate

        if(context!=agl_current_ctx)    // rebind old context if needed
            IOGLES2->aglMakeCurrent(agl_current_ctx);
        IOGLES2->aglDestroyContext(context);

    }
}

void aglMakeCurrent(void* context) {
    agl_current_ctx = context;
    if(IOGLES2) {
        IOGLES2->aglMakeCurrent(context);

        if (context)
            agl_context_find(context);  // activate (and create if needed) the correct glstate
    }
}

void aglSetParams2(struct TagItem * tags) {
    if(IOGLES2) {
        return IOGLES2->aglSetParams2(tags);
    }
} 

void amiga_pre_swap();
void amiga_post_swap();
void aglSwapBuffers() {
    amiga_pre_swap();
    // Swap the Buffers!
    if(IOGLES2) {
        IOGLES2->aglSwapBuffers();
    }
    amiga_post_swap();
}

// what is the use of this function?
void aglSetBitmap(struct BitMap *bitmap) {
    if(IOGLES2) {
        IOGLES2->aglSetBitmap(bitmap);
    }
}

//void* aglGetProcAddress(const char* name); //-> declared in agl/lookup.c

#ifdef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
void agl_reset_internals() {
	if(agl_context) {
		free(agl_context);
		agl_context=NULL;
	}
	agl_context_len=0;
	agl_context_cap=0;
	agl_current_ctx=NULL;
}
#endif