#include "glx.h"
#include "../gl/init.h"
#include "../gl/logs.h"


#if defined(NOX11) && defined(GLX_STUBS)
//#define WARN_GLX_STUB() LOGD("Warning, stubbed %s\n", __FUNCTION__); return NULL;
#define WARN_GLX_STUB() return NULL;

GLXContext  gl4es_glXCreateContext(Display *display, XVisualInfo *visual, GLXContext shareList, Bool isDirect){ WARN_GLX_STUB(); }
GLXContext  gl4es_glXCreateContextAttribsARB(Display *display, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list){ WARN_GLX_STUB(); }
void  gl4es_glXDestroyContext(Display *display, GLXContext ctx){ WARN_GLX_STUB(); }
Display  *gl4es_glXGetCurrentDisplay(){ WARN_GLX_STUB(); }
XVisualInfo  *gl4es_glXChooseVisual(Display *display, int screen, int *attributes){ WARN_GLX_STUB(); }
Bool  gl4es_glXMakeCurrent(Display *display, GLXDrawable drawable, GLXContext context){ WARN_GLX_STUB(); }
Bool  gl4es_glXMakeContextCurrent(Display *display, int drawable, int readable, GLXContext context){ WARN_GLX_STUB(); }
void  gl4es_glXSwapBuffers(Display *display, GLXDrawable drawable){ WARN_GLX_STUB(); }
int  gl4es_glXGetConfig(Display *display, XVisualInfo *visual, int attribute, int *value){ WARN_GLX_STUB(); }
int  gl4es_glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value){ WARN_GLX_STUB(); }
GLXContext  gl4es_glXGetCurrentContext(){ WARN_GLX_STUB(); }
GLXFBConfig  *gl4es_glXChooseFBConfig(Display *display, int screen, const int *attrib_list, int *count){ WARN_GLX_STUB(); }
GLXFBConfig  *gl4es_glXChooseFBConfigSGIX(Display *display, int screen, const int *attrib_list, int *count){ WARN_GLX_STUB(); }
GLXFBConfig  *gl4es_glXGetFBConfigs(Display *display, int screen, int *count){ WARN_GLX_STUB(); }
int  gl4es_glXGetFBConfigAttrib(Display *display, GLXFBConfig config, int attribute, int *value){ WARN_GLX_STUB(); }
XVisualInfo  *gl4es_glXGetVisualFromFBConfig(Display *display, GLXFBConfig config){ WARN_GLX_STUB(); }
GLXContext  gl4es_glXCreateNewContext(Display *display, GLXFBConfig config, int render_type, GLXContext share_list, Bool is_direct){ WARN_GLX_STUB(); }
void  gl4es_glXSwapIntervalEXT(Display *display, int drawable, int interval){ WARN_GLX_STUB(); }
void  gl4es_glXCopyContext(Display *display, GLXContext src, GLXContext dst, GLuint mask){ WARN_GLX_STUB(); }
Window  gl4es_glXCreateWindow(Display *display, GLXFBConfig config, Window win, int *attrib_list){ WARN_GLX_STUB(); }
void  gl4es_glXDestroyWindow(Display *display, void *win){ WARN_GLX_STUB(); }
GLXDrawable  gl4es_glXGetCurrentDrawable(){ WARN_GLX_STUB(); }
Bool  gl4es_glXIsDirect(Display * display, GLXContext ctx){ WARN_GLX_STUB(); }
void  gl4es_glXUseXFont(Font font, int first, int count, int listBase){ WARN_GLX_STUB(); }
int  gl4es_glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value){ WARN_GLX_STUB(); }
void  gl4es_glXDestroyPbuffer(Display * dpy, GLXPbuffer pbuf){ WARN_GLX_STUB(); }
GLXPbuffer  gl4es_glXCreatePbuffer(Display * dpy, GLXFBConfig config, const int * attrib_list){ WARN_GLX_STUB(); }
GLXPixmap  gl4es_glXCreateGLXPixmap(Display *display, XVisualInfo * visual, Pixmap pixmap){ WARN_GLX_STUB(); }
GLXPixmap  gl4es_glXCreatePixmap(Display * dpy, GLXFBConfig config, Pixmap pixmap, const int * attrib_list){ WARN_GLX_STUB(); }
void  gl4es_glXDestroyGLXPixmap(Display *display, void *pixmap){ WARN_GLX_STUB(); }
void  gl4es_glXDestroyPixmap(Display *display, void *pixmap){ WARN_GLX_STUB(); }
GLXContext  gl4es_glXCreateContextAttribs(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list){ WARN_GLX_STUB(); }
void  gl4es_glXSwapIntervalMESA(int interval){ WARN_GLX_STUB(); }
void  gl4es_glXSwapIntervalSGI(int interval){ WARN_GLX_STUB(); }

AliasExport(GLXContext,glXCreateContext,,(Display *display, XVisualInfo *visual, GLXContext shareList, Bool isDirect));
AliasExport(GLXContext,glXCreateContextAttribs,ARB,(Display *display, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list));
AliasExport(void,glXDestroyContext,,(Display *display, GLXContext ctx));
AliasExport(Display*,glXGetCurrentDisplay,,());
AliasExport(XVisualInfo*,glXChooseVisual,,(Display *display, int screen, int *attributes));
AliasExport(Bool,glXMakeCurrent,,(Display *display, GLXDrawable drawable, GLXContext context));
AliasExport(Bool,glXMakeContextCurrent,,(Display *display, int drawable, int readable, GLXContext context));
AliasExport(void,glXSwapBuffers,,(Display *display, GLXDrawable drawable));
AliasExport(int,glXGetConfig,,(Display *display, XVisualInfo *visual, int attribute, int *value));
AliasExport(int,glXQueryContext,,( Display *dpy, GLXContext ctx, int attribute, int *value));
AliasExport(GLXContext,glXGetCurrentContext,,());
AliasExport(GLXFBConfig*,glXChooseFBConfig,,(Display *display, int screen, const int *attrib_list, int *count));
AliasExport(GLXFBConfig*,glXChooseFBConfig,SGIX,(Display *display, int screen, const int *attrib_list, int *count));
AliasExport(GLXFBConfig*,glXGetFBConfigs,,(Display *display, int screen, int *count));
AliasExport(int,glXGetFBConfigAttrib,,(Display *display, GLXFBConfig config, int attribute, int *value));
AliasExport(XVisualInfo*,glXGetVisualFromFBConfig,,(Display *display, GLXFBConfig config));
AliasExport(GLXContext,glXCreateNewContext,,(Display *display, GLXFBConfig config, int render_type, GLXContext share_list, Bool is_direct));
AliasExport(void,glXSwapIntervalEXT,,(Display *display, int drawable, int interval));
AliasExport(void,glXCopyContext,,(Display *display, GLXContext src, GLXContext dst, GLuint mask));
AliasExport(Window,glXCreateWindow,,(Display *display, GLXFBConfig config, Window win, int *attrib_list));
AliasExport(void,glXDestroyWindow,,(Display *display, void *win));
AliasExport(GLXDrawable,glXGetCurrentDrawable,,());
AliasExport(Bool,glXIsDirect,,(Display * display, GLXContext ctx));
AliasExport(void,glXUseXFont,,(Font font, int first, int count, int listBase));
AliasExport(int,glXQueryDrawable,,(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value));
AliasExport(void,glXDestroyPbuffer,,(Display * dpy, GLXPbuffer pbuf));
AliasExport(GLXPbuffer,glXCreatePbuffer,,(Display * dpy, GLXFBConfig config, const int * attrib_list));
AliasExport(GLXPixmap,glXCreateGLXPixmap,,(Display *display, XVisualInfo * visual, Pixmap pixmap));
AliasExport(GLXPixmap,glXCreatePixmap,,(Display * dpy, GLXFBConfig config, Pixmap pixmap, const int * attrib_list));
AliasExport(void,glXDestroyGLXPixmap,,(Display *display, void *pixmap));
AliasExport(void,glXDestroyPixmap,,(Display *display, void *pixmap));
AliasExport(GLXContext,glXCreateContextAttribs,,(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list));
#endif