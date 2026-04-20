#include "glx.h"
#include "../gl/init.h"

#if !defined(ANDROID) && !defined(AMIGAOS4)
#include <execinfo.h>
#endif // !defined(ANDROID) && !defined(AMIGAOS4)
#include <fcntl.h>
#include "khash.h"
#ifdef USE_FBIO
#include <linux/fb.h>
#endif // USE_FBIO
#include <signal.h>
#include <sys/ioctl.h>
#if defined(USE_CLOCK) || defined(PYRA)
#include <time.h>
#else
#include <sys/time.h>
#endif
#ifdef PANDORA
#include <sys/socket.h>
#include <sys/un.h>
#endif // PANDORA
#include <unistd.h>

#ifdef AMIGAOS4
#include "../agl/amigaos.h"
#endif // AMIGAOS4
#include "../gl/debug.h"
#include "../gl/framebuffers.h"
#include "../gl/init.h"
#include "../gl/loader.h"
#ifdef PANDORA
#include "../gl/pixel.h"
#endif
#include "glx_gbm.h"
#include "../gl/gl4es.h"
#include "hardext.h"
#include "streaming.h"
#include "utils.h"
#include "../gl/envvars.h"

// #define DEBUG
#ifdef DEBUG
#pragma GCC optimize 0
#define DBG(a) a
#else
#define DBG(a)
#endif

#ifndef EGL_GL_COLORSPACE_KHR
#define EGL_GL_COLORSPACE_KHR 0x309D
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#define EGL_GL_COLORSPACE_LINEAR_KHR 0x308A
#endif

#ifndef NOEGL
static bool eglInitialized = false;
static EGLDisplay eglDisplay = NULL;
static EGLSurface eglSurface = NULL;
static EGLConfig eglConfigs[1];
static EGLContext eglContext = EGL_NO_CONTEXT;
static int maxEGLConfig = 0;
static GLXFBConfig allFBConfig = NULL;
#endif
static int glx_default_depth = 0;
#ifdef PANDORA
static struct sockaddr_un sun;
static int sock = -2;
#endif

int8_t CheckEGLErrors() {
#ifndef NOEGL
    const char* errortext = PrintEGLError(1);

    if (errortext) {
        LOGE("ERROR: EGL Error detected: %s\n", errortext);
        return 1;
    }
#endif
    return 0;
}
#ifndef NOX11
typedef struct {
    int Width;
    int Height;
    EGLContext Context;
    EGLSurface Surface;
    EGLConfig Config;
    int Depth;
    Display* dpy;
    int Type;
    GC gc;
    XImage* frame;
} glx_buffSize;

// PBuffer should work under ANDROID / NOX11
static GLXPbuffer* pbufferlist = NULL;
static glx_buffSize* pbuffersize = NULL;
static int pbufferlist_cap = 0;
static int pbufferlist_size = 0;
static int isPBuffer(GLXDrawable drawable) {
    for (int i = 0; i < pbufferlist_size; i++)
        if (pbufferlist[i] == (GLXPbuffer)drawable) return i + 1;
    return 0;
}
static void delPBuffer(int j) {
    pbufferlist[j] = 0;
    pbuffersize[j].Width = 0;
    pbuffersize[j].Height = 0;
    pbuffersize[j].gc = 0;
    pbuffersize[j].Context = NULL;
    // should pack, but I think it's useless for common use
}
void BlitEmulatedPixmap(int win);
int createPBuffer(Display* dpy, const EGLint* egl_attribs, EGLSurface* Surface, EGLContext* Context, EGLConfig* Config,
                  int redBits, int greenBits, int blueBits, int alphaBits, int samplebuffers, int samples);
GLXPbuffer addPixBuffer(Display* dpy, EGLSurface surface, EGLConfig Config, int Width, int Height, EGLContext Context,
                        Pixmap pixmap, int depth, int emulated);

static Display* g_display = NULL;
static GLXContext glxContext = NULL;
static GLXContext fbContext = NULL;
static GLuint current_fb = 0;

#endif // NOX11
void glx_getMainFBSize(GLint* width, GLint* height) {
#if !defined(NOX11) && !defined(NOEGL) && !defined(ANDROID)
    // noegl, no updating of framebuffer size
    DBG(SHUT_LOGD("gl4es_getMainFBSize() %dx%d -> ", *width, *height);)
    LOAD_EGL(eglQuerySurface);
    egl_eglQuerySurface(eglDisplay, glxContext->eglSurface, EGL_WIDTH, width);
    egl_eglQuerySurface(eglDisplay, glxContext->eglSurface, EGL_HEIGHT, height);
    DBG(SHUT_LOGD("%dx%d (%s)\n", *width, *height, PrintEGLError(0));)
#endif
}

static int fbcontext_count = 0;

#ifdef USE_FBIO
#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif
static int fbdev = -1;
#endif

static int g_width = 0, g_height = 0;
static int swapinterval = 1; // default value. Also, should be tracked by drawable...
static int minswap = 0;
static int maxswap = 1;
// **** RPI stuffs ****
void (*bcm_host_init)();
void (*bcm_host_deinit)();
#ifndef ANDROID
#include "rpi.h"
#endif
// ***** end of RPI stuffs ****

// Generic create native window to use with "LIBGL_FB=1" (so with EGL_DEFAULT_DISPLAY and without X11)
static void* create_native_window(int w, int h) {
#if !defined(ANDROID) && !defined(AMIGAOS4)
    if (bcm_host) return create_rpi_window(w, h);
#endif
#ifndef NO_GBM
    if (globals4es.usegbm) return CreateGBMWindow(w, h);
#endif
    return NULL;
}
static void delete_native_window(void* win) {
#if !defined(ANDROID) && !defined(AMIGAOS4)
    if (bcm_host) return delete_rpi_window(win);
#endif
#ifndef NO_GBM
    if (globals4es.usegbm) return DeleteGBMWindow(win);
#endif
}

#ifndef NOEGL
static EGLint egl_context_attrib_es2[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

static EGLint egl_context_attrib[] = {EGL_NONE};

typedef struct {
    EGLSurface surf;
    int* cnt;
} SharedEGLSurface_t;

KHASH_MAP_INIT_INT(eglsurfacelist_t, SharedEGLSurface_t*);
static khash_t(eglsurfacelist_t) * eglsurfaces = NULL;

static void RecycleAddSurface(GLXDrawable drawable, SharedEGLSurface_t* surf) {
    if (!eglsurfaces) {
        eglsurfaces = kh_init(eglsurfacelist_t);
    }
    int ret;
    khint_t k;
    k = kh_put(eglsurfacelist_t, eglsurfaces, drawable, &ret);
    SharedEGLSurface_t* newsurf = malloc(sizeof(SharedEGLSurface_t));
    memcpy(newsurf, surf, sizeof(SharedEGLSurface_t));
    kh_value(eglsurfaces, k) = newsurf;
    DBG(SHUT_LOGD("LIBGL: EGLSurface for drawable %p Added\n", (void*)drawable);)
}

static SharedEGLSurface_t* RecycleGetSurface(GLXDrawable drawable) {
    if (!eglsurfaces) return NULL;
    int ret;
    khint_t k;
    k = kh_get(eglsurfacelist_t, eglsurfaces, drawable);
    if (k != kh_end(eglsurfaces)) {
        DBG(SHUT_LOGD("LIBGL: EGLSurface for drawable %p found\n", (void*)drawable);)
        return kh_value(eglsurfaces, k);
    }
    if ((globals4es.usefb || globals4es.usefbo) && kh_size(eglsurfaces)) {
        // all surface are in the same drawable
        // take the first one
        for (k = kh_begin(eglsurfaces); k != kh_end(eglsurfaces); ++k)
            if (kh_exist(eglsurfaces, k)) return kh_value(eglsurfaces, k);
    }
    return NULL;
}

static void RecycleDelSurface(GLXDrawable drawable) {
    int ret;
    khint_t k;
#ifndef NOX11
    ret = isPBuffer(drawable);
    if (ret) delPBuffer(ret - 1);
#endif
    if (!eglsurfaces) return;
    k = kh_get(eglsurfacelist_t, eglsurfaces, drawable);
    if (k != kh_end(eglsurfaces)) {
        /*LOAD_EGL(eglDestroySurface);
        egl_eglDestroySurface(eglDisplay, kh_value(eglsurfaces, k));*/
        free(kh_value(eglsurfaces, k));
        kh_del(eglsurfacelist_t, eglsurfaces, k);
        DBG(SHUT_LOGD("LIBGL: EGLSurface for drawable %p removed\n", (void*)drawable);)
    }
    return;
}
#endif

extern void* egl;
int globales2 = 0;
// GLState management
void* NewGLState(void* shared_glstate, int es2only);
void DeleteGLState(void* oldstate);
void ActivateGLState(void* new_glstate);
void CopyGLEShard(void* dst, const void* src);

typedef struct {
    int drawable;
#ifndef NOEGL
    EGLSurface surface;
#endif
    int PBuffer;
} map_drawable_t;
KHASH_MAP_INIT_INT(mapdrawable, map_drawable_t*)
khash_t(mapdrawable) * MapDrawable = NULL;

#ifndef NOX11
static int get_config_default(Display* display, int attribute, int* value) {
    switch (attribute) {
    case GLX_USE_GL:
    case GLX_RGBA:
    case GLX_X_RENDERABLE:
    case GLX_DOUBLEBUFFER:
        *value = 1;
        break;
    case GLX_LEVEL:
    case GLX_STEREO:
        *value = 0;
        break;
    case GLX_AUX_BUFFERS:
        *value = 0;
        break;
#ifdef PANDORA
    case GLX_RED_SIZE:
        *value = 5;
        break;
    case GLX_GREEN_SIZE:
        *value = 6;
        break;
    case GLX_BLUE_SIZE:
        *value = 5;
        break;
    case GLX_ALPHA_SIZE:
        *value = 8; // why not 0?
        break;
    case GLX_DEPTH_SIZE:
        *value = 16;
        break;
#else
    case GLX_RED_SIZE:
        *value = 8;
        break;
    case GLX_GREEN_SIZE:
        *value = 8;
        break;
    case GLX_BLUE_SIZE:
        *value = 8;
        break;
    case GLX_ALPHA_SIZE:
        *value = 8;
        break;
    case GLX_DEPTH_SIZE:
        *value = 24; // 32;
        break;
#endif
    case GLX_STENCIL_SIZE:
        *value = 8;
        break;
    case GLX_ACCUM_RED_SIZE:
    case GLX_ACCUM_GREEN_SIZE:
    case GLX_ACCUM_BLUE_SIZE:
    case GLX_ACCUM_ALPHA_SIZE:
        *value = 0;
        break;
    case GLX_TRANSPARENT_TYPE:
        *value = GLX_NONE;
        break;
    case GLX_RENDER_TYPE:
        *value = GLX_RGBA_TYPE;
        break;
    case GLX_VISUAL_ID: {
        XVisualInfo xvinfo = {0};
        xvinfo.depth = glx_default_depth;
        xvinfo.class = TrueColor;
        int n;
        XVisualInfo* visuals = XGetVisualInfo(display, VisualDepthMask | VisualClassMask, &xvinfo, &n);
        if (!n) {
            LOGD("Warning, get_config_default: XGetVisualInfo gives 0 VisualInfo for %d depth and TrueColor class\n",
                 glx_default_depth);
            *value = 0;
        } else {
            *value = visuals[0].visualid;
            XFree(visuals);
        }
    } break;
    case GLX_FBCONFIG_ID:
        *value = 0;
        break;
    case GLX_DRAWABLE_TYPE:
        *value = GLX_WINDOW_BIT;
        break;
    case GLX_BUFFER_SIZE:
        *value = 16;
        break;
    case GLX_X_VISUAL_TYPE:
        *value = GLX_TRUE_COLOR;
        break;
    case GLX_CONFIG_CAVEAT:
    case GLX_SAMPLE_BUFFERS:
    case GLX_SAMPLES:
        *value = 0;
        break;
    case GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB:
        *value = hardext.srgb;
        break;
    default:
        DBG(SHUT_LOGD(" => Unknown attrib\n");)
        LOGD("unknown attrib %i\n", attribute);
        *value = 0;
        return 1;
    }
    DBG(SHUT_LOGD(" -> 0x%04X\n", *value);)
    return 0;
}

static void init_display(Display* display) {
    LOAD_EGL(eglGetDisplay);

    if (!g_display) {
        g_display = display; // XOpenDisplay(NULL);
    }
    if (globals4es.usegbm) {
        eglDisplay = OpenGBMDisplay(display);
    }
    if (!eglDisplay) {
        if (globals4es.usefb || globals4es.usepbuffer) {
            eglDisplay = egl_eglGetDisplay(EGL_DEFAULT_DISPLAY);
        } else {
            eglDisplay = egl_eglGetDisplay(display);
        }
    }
}

static void fill1GLXFBConfig(Display* display, EGLConfig eglConfig, int DB, GLXFBConfig fbConfig) {
    LOAD_EGL(eglGetConfigAttrib);

    EGLint tmp;
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RED_SIZE, &fbConfig->redBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_GREEN_SIZE, &fbConfig->greenBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BLUE_SIZE, &fbConfig->blueBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_ALPHA_SIZE, &fbConfig->alphaBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_DEPTH_SIZE, &fbConfig->depthBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_STENCIL_SIZE, &fbConfig->stencilBits);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &fbConfig->multiSampleSize);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLE_BUFFERS, &fbConfig->nMultiSampleBuffers);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SURFACE_TYPE, &tmp);
    fbConfig->drawableType = 0;
    if (tmp & EGL_WINDOW_BIT) fbConfig->drawableType |= GLX_WINDOW_BIT;
    if (tmp & EGL_PBUFFER_BIT) fbConfig->drawableType |= GLX_PBUFFER_BIT;
    if (tmp & EGL_PIXMAP_BIT) fbConfig->drawableType |= GLX_PIXMAP_BIT;
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_MAX_PBUFFER_WIDTH, &fbConfig->maxPbufferWidth);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_MAX_PBUFFER_HEIGHT, &fbConfig->maxPbufferHeight);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_MAX_PBUFFER_PIXELS, &fbConfig->maxPbufferPixels);
    egl_eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &fbConfig->associatedVisualId);
    if (!fbConfig->associatedVisualId || globals4es.usefb || globals4es.usefbo || globals4es.usepbuffer) {
        // when using some FB driver, lets take a default VisualID, as the one from the EGLConfig is probably not the
        // correct one
        glx_default_depth = XDefaultDepth(display, 0);
        XVisualInfo xvinfo = {0};
        xvinfo.depth = glx_default_depth;
        xvinfo.class = TrueColor;
        int n;
        XVisualInfo* visuals = XGetVisualInfo(display, VisualDepthMask | VisualClassMask, &xvinfo, &n);
        if (!n) {
            LOGD("Warning, fillGLXFBConfig: XGetVisualInfo gives 0 VisualInfo for %d depth and TrueColor class\n",
                 glx_default_depth);
            fbConfig->associatedVisualId = 0;
        } else {
            fbConfig->associatedVisualId = visuals[0].visualid;
            XFree(visuals);
        }
    }
    fbConfig->doubleBufferMode = DB;
    fbConfig->id = eglConfig;
}

static void init_eglconfig(Display* display) {
    if (g_display != display) {
        if (allFBConfig) free(allFBConfig);
        allFBConfig = NULL;
        g_display = NULL; // should close properly
        init_display(display);
    }
    if (allFBConfig) return;
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglGetConfigAttrib);
    EGLint configAttribs[] = {EGL_RENDERABLE_TYPE, (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
                              EGL_NONE};
    int configsFound;
    // grab all config for the display
    egl_eglChooseConfig(eglDisplay, configAttribs, NULL, 0, &configsFound);
    EGLConfig allConfigs[configsFound];
    egl_eglChooseConfig(eglDisplay, configAttribs, allConfigs, configsFound, &configsFound);
    maxEGLConfig = -1;
    EGLint confID;
    for (int i = 0; i < configsFound; ++i) {
        egl_eglGetConfigAttrib(eglDisplay, allConfigs[i], EGL_CONFIG_ID, &confID);
        if (maxEGLConfig < confID) maxEGLConfig = confID; // get max number
    }
    ++maxEGLConfig;
    allFBConfig = (GLXFBConfig)calloc(maxEGLConfig * 2, sizeof(struct __GLXFBConfigRec));
    for (int i = 0; i < configsFound; ++i) {
        egl_eglGetConfigAttrib(eglDisplay, allConfigs[i], EGL_CONFIG_ID, &confID);
        fill1GLXFBConfig(display, allConfigs[i], 0, &allFBConfig[confID]);
        fill1GLXFBConfig(display, allConfigs[i], 1, &allFBConfig[confID + maxEGLConfig]);
    }
}

static int InitEGL(Display* display) {
    if (eglInitialized) return 1;

    if (!eglDisplay || eglDisplay == EGL_NO_DISPLAY) {
        init_display(display);
        if (eglDisplay == EGL_NO_DISPLAY) return 0;
    }
    LOAD_EGL(eglBindAPI);
    LOAD_EGL(eglInitialize);
    egl_eglBindAPI(EGL_OPENGL_ES_API);
    EGLint result = egl_eglInitialize(eglDisplay, NULL, NULL);
    if (result != EGL_TRUE) {
        CheckEGLErrors();
        LOGE("Unable to initialize EGL display.\n");
        return 0;
    }
    eglInitialized = true;
    return 1;
}

#endif // NOX11
static void init_vsync() {
#ifdef USE_FBIO
    fbdev = open("/dev/fb0", O_RDONLY);
    if (fbdev < 0) {
        LOGE("Could not open /dev/fb0 for vsync.\n");
    }
#endif
}

static void xrefresh() {
    int dummy = system("xrefresh");
}

#ifdef PANDORA
static void pandora_reset_gamma() {
    if (globals4es.gamma > 0.0f) system("sudo /usr/pandora/scripts/op_gamma.sh 0");
}
void pandora_set_gamma() {
    {
        char buf[50];
        if (globals4es.gamma > 0.0f)
            sprintf(buf, "sudo /usr/pandora/scripts/op_gamma.sh %.2f", globals4es.gamma);
        else
            sprintf(buf, "sudo /usr/pandora/scripts/op_gamma.sh 0");
        int dummy = system(buf);
    }
}
#endif

static void signal_handler(int sig) {
    if (globals4es.xrefresh) xrefresh();
#ifdef PANDORA
    pandora_reset_gamma();
#endif

#if defined(BCMHOST) && !defined(ANDROID)
    rpi_fini();
#endif
#ifdef HAS_BACKTRACE
    if (globals4es.stacktrace) {
        switch (sig) {
        case SIGBUS:
        case SIGFPE:
        case SIGILL:
        case SIGSEGV: {
            void* array[10];
            size_t size = backtrace(array, 10);
            if (!size) {
                LOGD("No stacktrace. Compile with -funwind-tables.\n");
            } else {
                LOGD("Stacktrace: %zd\n", size);
                backtrace_symbols_fd(array, size, 2);
            }
            break;
        }
        }
    }
#endif
    signal(sig, SIG_DFL);
    raise(sig);
}
#ifdef PANDORA
static void init_liveinfo() {
    static const char socket_name[] = "\0liveinfo";
    sock = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) {
        // no socket, so LiveInfo probably not active
        return;
    }

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, socket_name, sizeof(socket_name));
    // send a test string
    const char test_string[] = "gl: fpsinfo";
    if (sendto(sock, test_string, strlen(test_string), 0, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
        // error, so probably not present
        close(sock);
        sock = -1;
    } else
        fcntl(sock, F_SETFL, O_NONBLOCK);
}

#endif

void glx_init() {
    // init map_drawable
    int ret;
    if (!gl4es_getMainFBSize) gl4es_getMainFBSize = glx_getMainFBSize;
    MapDrawable = kh_init(mapdrawable);
    kh_put(mapdrawable, MapDrawable, 1, &ret);
    kh_del(mapdrawable, MapDrawable, 1);
#if defined(BCMHOST) && !defined(ANDROID)
    rpi_init();
#endif
    if (globals4es.usegbm) atexit(CloseGBMFunctions);
    if (globals4es.xrefresh || globals4es.stacktrace) {
        // TODO: a bit gross. Maybe look at this: http://stackoverflow.com/a/13290134/293352
        signal(SIGBUS, signal_handler);
        signal(SIGFPE, signal_handler);
        // signal(SIGILL, signal_handler);
        signal(SIGSEGV, signal_handler);
        if (globals4es.xrefresh) {
            signal(SIGINT, signal_handler);
            signal(SIGQUIT, signal_handler);
            signal(SIGTERM, signal_handler);
        }
        if (globals4es.xrefresh) atexit(xrefresh);
#if !defined(ANDROID) && !defined(AMIGAOS4)
#endif //! ANDROID && !AMIGAOS4
    }
#ifdef PANDORA
    atexit(pandora_reset_gamma);
#elif defined(BCMHOST)
    atexit(bcm_host_deinit);
#elif defined(AMIGAOS4)
#ifndef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
    atexit(os4CloseLib);
#endif
#endif
    // V-Sync
    if (globals4es.vsync) init_vsync();
#ifdef PANDORA

    init_liveinfo();
    if (sock > -1) {
        DBG(SHUT_LOGD("LiveInfo detected, fps will be shown\n");)
    }
#endif
}

#ifndef NOX11
KHASH_MAP_INIT_INT(fbvisual, GLXFBConfig*);
static kh_fbvisual_t* fbvisual = NULL;

void InitFBVisual() {
    if (fbvisual) return;
    fbvisual = kh_init(fbvisual);
}
void FreeFBVisual() {
    if (!fbvisual) return;
    GLXFBConfig* conf;
    kh_foreach_value(fbvisual, conf, free(conf));
    kh_destroy(fbvisual, fbvisual);
    fbvisual = NULL;
}
GLXFBConfig* FindFBVisual(XVisualInfo* visual) {
    if (!fbvisual) InitFBVisual();
    khint_t k = kh_get(fbvisual, fbvisual, (uintptr_t)visual);
    if (k == kh_end(fbvisual)) return NULL;
    return kh_value(fbvisual, k);
}
void AddFBVisual(XVisualInfo* visual, GLXFBConfig* conf) {
    if (!fbvisual) InitFBVisual();
    int ret;
    khint_t k = kh_put(fbvisual, fbvisual, (uintptr_t)visual, &ret);
    if (!ret) free(kh_value(fbvisual, k));
    kh_value(fbvisual, k) = conf;
}

GLXContext gl4es_glXCreateContext(Display* display, XVisualInfo* visual, GLXContext shareList, Bool isDirect) {
    DBG(SHUT_LOGD("glXCreateContext(%p, %p, %p, %i) fbcontext_count=%d ", display, visual, shareList, isDirect,
                  fbcontext_count);)

    static struct __GLXFBConfigRec default_glxfbconfig;
    GLXFBConfig glxfbconfig;
    GLXFBConfig* visualfbconfig = FindFBVisual(visual);
    if (visualfbconfig)
        glxfbconfig = visualfbconfig[0];
    else {
        glxfbconfig = &default_glxfbconfig;
        memset(glxfbconfig, 0, sizeof(struct __GLXFBConfigRec));
        default_glxfbconfig.redBits = (visual == 0) ? 0 : (visual->depth == 16) ? 5 : 8;
        default_glxfbconfig.greenBits = (visual == 0) ? 0 : (visual->depth == 16) ? 6 : 8;
        default_glxfbconfig.blueBits = (visual == 0) ? 0 : (visual->depth == 16) ? 5 : 8;
        default_glxfbconfig.alphaBits = (visual == 0) ? 0 : (visual->depth != 32) ? 0 : 8;
#ifdef PANDORA
        default_glxfbconfig.depthBits = 16;
#else
        default_glxfbconfig.depthBits = 24;
#endif
        default_glxfbconfig.stencilBits = 8;
        default_glxfbconfig.doubleBufferMode = 1;
    }
    int depthBits = glxfbconfig->depthBits;
    if (glxfbconfig->stencilBits > 8) glxfbconfig->stencilBits = 8;
    if (depthBits == 16 && glxfbconfig->stencilBits) glxfbconfig->stencilBits = EGL_DONT_CARE;
#ifdef PANDORA
    if (depthBits == 24 && glxfbconfig->stencilBits == 8 &&
        !(globals4es.usefbo || globals4es.usepbuffer || hardext.esversion >= 2))
        depthBits = 16;
    else if (depthBits == 16 && glxfbconfig->stencilBits == 8 && hardext.esversion >= 2)
        depthBits = 24;
    -if (depthBits == 16 && glxfbconfig->stencilBits) - glxfbconfig->stencilBits = EGL_DONT_CARE;
    +if (depthBits == 16 && glxfbconfig->stencilBits) + glxfbconfig->stencilBits = 0;
#endif

    DBG(SHUT_LOGD("Creating R:%d G:%d B:%d A:%d visual deth=%d Depth:%d Stencil:%d Multisample:%d/%d Doublebuff=%d\n",
                  glxfbconfig->redBits, glxfbconfig->greenBits, glxfbconfig->blueBits, glxfbconfig->alphaBits,
                  visual ? visual->depth : 0, depthBits, glxfbconfig->stencilBits, glxfbconfig->nMultiSampleBuffers,
                  glxfbconfig->multiSampleSize, glxfbconfig->doubleBufferMode);)
    EGLint configAttribs[] = {
#ifdef PANDORA
        EGL_RED_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_BLUE_SIZE, 5,
#else
        EGL_RED_SIZE, glxfbconfig->redBits, EGL_GREEN_SIZE, glxfbconfig->greenBits, EGL_BLUE_SIZE,
        glxfbconfig->blueBits, EGL_ALPHA_SIZE, (hardext.eglnoalpha) ? 0 : glxfbconfig->alphaBits,
#endif
        EGL_DEPTH_SIZE, depthBits, EGL_RENDERABLE_TYPE,
        (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
        // EGL_BUFFER_SIZE, depthBits,
        EGL_STENCIL_SIZE, glxfbconfig->stencilBits,

        EGL_SAMPLE_BUFFERS, glxfbconfig->nMultiSampleBuffers, EGL_SAMPLES, glxfbconfig->multiSampleSize,

        EGL_SURFACE_TYPE,
        (globals4es.usegbm) ? EGL_WINDOW_BIT
                            : (globals4es.usepbuffer ? EGL_PBUFFER_BIT : (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)),
        EGL_NONE};
    if (globals4es.usefb) ++fbcontext_count;

    LOAD_EGL(eglMakeCurrent);
    LOAD_EGL(eglDestroyContext);
    LOAD_EGL(eglDestroySurface);
    LOAD_EGL(eglCreateContext);
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglQueryString);

    GLXContext fake = calloc(1, sizeof(struct __GLXContextRec));

    // make an egl context here...
    EGLBoolean result;
    if (eglDisplay == NULL || eglDisplay == EGL_NO_DISPLAY) {
        init_display(display);
        if (eglDisplay == EGL_NO_DISPLAY) {
            DBG(SHUT_LOGD(" => %p\n", NULL);)
            CheckEGLErrors();
            LOGE("Unable to create EGL display.\n");
            free(fake);
            return 0;
        }
    }

    // first time?
    if (eglInitialized == false) {
        if (!InitEGL(display)) {
            DBG(SHUT_LOGD(" => %p\n", NULL);)
            CheckEGLErrors();
            LOGE("Unable to init EGL.\n");
            free(fake);
            return 0;
        }
    }

    result = egl_eglChooseConfig(eglDisplay, configAttribs, fake->eglConfigs, 64, &fake->eglConfigsCount);
    if (fake->eglConfigsCount && globals4es.usegbm)
        fake->eglconfigIdx = FindGBMConfig(eglDisplay, fake->eglConfigs, fake->eglConfigsCount);

    CheckEGLErrors();
    if (result != EGL_TRUE || fake->eglConfigsCount == 0) {
        DBG(SHUT_LOGD(" => %p\n", NULL);)
        LOGE("No EGL configs found (depth=%d, stencil=%d).\n", depthBits, glxfbconfig->stencilBits);
        CheckEGLErrors();
        free(fake);
        return 0;
    }
    EGLContext shared = (shareList) ? shareList->eglContext : EGL_NO_CONTEXT;
    fake->eglContext = egl_eglCreateContext(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], shared,
                                            (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);

    CheckEGLErrors();

    // need to return a glx context pointing at it
    fake->display = display;
    fake->direct = true;
    fake->xid = 1;         // TODO: Proper handling of that id...
    fake->contextType = 0; // Window
    fake->doublebuff = glxfbconfig->doubleBufferMode;
#ifdef PANDORA
    fake->rbits = 5;
    fake->gbits = 6;
    fake->bbits = 5;
    fake->abits = 0;
#else
    fake->rbits = (visual == 0)           ? 8
                  : (visual->depth == 16) ? 5
                                          : 8,
    fake->gbits = (visual == 0)           ? 8
                  : (visual->depth == 16) ? 6
                                          : 8,
    fake->bbits = (visual == 0)           ? 8
                  : (visual->depth == 16) ? 5
                                          : 8,
    fake->abits = (visual == 0)           ? 8
                  : (visual->depth != 32) ? 0
                                          : 8,
#endif
    fake->samples = 0;
    fake->samplebuffers = 0;
    fake->shared = (shareList) ? shareList->glstate : NULL;

    DBG(SHUT_LOGD(" => %p\n", fake);)
    return fake;
}

GLXContext createPBufferContext(Display* display, GLXContext shareList, GLXFBConfig config) {

    EGLint configAttribs[] = {EGL_RED_SIZE,
                              (config) ? config->redBits : 0,
                              EGL_GREEN_SIZE,
                              (config) ? config->greenBits : 0,
                              EGL_BLUE_SIZE,
                              (config) ? config->blueBits : 0,
                              EGL_ALPHA_SIZE,
                              (hardext.eglnoalpha) ? 0 : ((config) ? config->alphaBits : 0),
                              EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
                              EGL_SAMPLE_BUFFERS,
                              (config) ? config->nMultiSampleBuffers : 0,
                              EGL_SAMPLES,
                              (config) ? config->multiSampleSize : 0,
                              EGL_NONE};

    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglCreateContext);
    LOAD_EGL(eglGetConfigAttrib);

    // Check that the config is for PBuffer
    if (!(config->drawableType & GLX_PBUFFER_BIT)) return 0;

    // Init what need to be done
    if (!InitEGL(display)) return NULL;

    // select a configuration
    EGLBoolean result;
    int configsFound;
    static EGLConfig pbufConfigs[1];
    result = egl_eglChooseConfig(eglDisplay, configAttribs, pbufConfigs, 1, &configsFound);

    CheckEGLErrors();
    if (result != EGL_TRUE || configsFound == 0) {
        LOGE("No EGL configs found.\n");
        return 0;
    }

    EGLContext shared = (shareList) ? shareList->eglContext : EGL_NO_CONTEXT;

    GLXContext fake = malloc(sizeof(struct __GLXContextRec));
    memset(fake, 0, sizeof(struct __GLXContextRec));
    fake->es2only = globales2;
    fake->shared = (shareList) ? shareList->glstate : NULL;
    fake->eglConfigs[0] = pbufConfigs[0];
    fake->eglConfigsCount = 1;
    fake->eglconfigIdx = 0;

    fake->eglContext = egl_eglCreateContext(eglDisplay, fake->eglConfigs[0], shared,
                                            (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);

    CheckEGLErrors();

    // need to return a glx context pointing at it
    fake->display = display;
    fake->direct = true;
    fake->xid = 1;         // TODO: Proper handling of that id...
    fake->contextType = 1; // PBuffer
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_RED_SIZE, &fake->rbits);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_GREEN_SIZE, &fake->gbits);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_BLUE_SIZE, &fake->bbits);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_ALPHA_SIZE, &fake->abits);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_DEPTH_SIZE, &fake->depth);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_STENCIL_SIZE, &fake->stencil);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_SAMPLES, &fake->samples);
    egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[0], EGL_SAMPLE_BUFFERS, &fake->samplebuffers);

    DBG(SHUT_LOGD(" => return PBufferContext %p (context->shared=%p)\n", fake, fake->shared);)
    return fake;
}

GLXContext gl4es_glXCreateContextAttribsARB(Display* display, GLXFBConfig config, GLXContext share_context, Bool direct,
                                            const int* attrib_list) {
    // DBG(SHUT_LOGD("glXCreateContextAttribsARB(%p, %p, %p, %d, %p) ", display, config, share_context, direct,
    // attrib_list);
    //     if(config)
    //         SHUT_LOGD("config is RGBA:%d%d%d%d, depth=%d, stencil=%d, multisample=%d/%d doublebuff=%d,
    //         drawable=%d\n", config->redBits, config->greenBits, config->blueBits, config->alphaBits,
    //         config->depthBits, config->stencilBits, config->multiSampleSize, config->nMultiSampleBuffers,
    //         config->doubleBufferMode, config->drawableType);
    //     else SHUT_LOGD("\n");
    //)
    if (config && config->drawableType == GLX_PBUFFER_BIT) {
        return createPBufferContext(display, share_context, config);
    } else {
        EGLint type = 0;
        if (config->drawableType & GLX_PIXMAP_BIT) type |= EGL_PIXMAP_BIT;
        if (config->drawableType & GLX_WINDOW_BIT) type |= EGL_WINDOW_BIT;
        if (config->drawableType & GLX_PBUFFER_BIT) type |= EGL_PBUFFER_BIT;
        EGLint configAttribs[] = {
#ifdef PANDORA
            EGL_RED_SIZE,
            (config->drawableType == GLX_PIXMAP_BIT) ? config->redBits : 5,
            EGL_GREEN_SIZE,
            (config->drawableType == GLX_PIXMAP_BIT) ? config->greenBits : 6,
            EGL_BLUE_SIZE,
            (config->drawableType == GLX_PIXMAP_BIT) ? config->blueBits : 5,
            EGL_ALPHA_SIZE,
            (config->drawableType == GLX_PIXMAP_BIT) ? config->alphaBits : 0,
            EGL_DEPTH_SIZE,
            (globals4es.usefb) ? 24 : config->depthBits,
            EGL_STENCIL_SIZE,
            (globals4es.usefb) ? 8 : config->stencilBits,
#else
            EGL_RED_SIZE,
            config->redBits,
            EGL_GREEN_SIZE,
            config->greenBits,
            EGL_BLUE_SIZE,
            config->blueBits,
            EGL_ALPHA_SIZE,
            (hardext.eglnoalpha) ? 0 : config->alphaBits,
            EGL_DEPTH_SIZE,
            config->depthBits,
            EGL_STENCIL_SIZE,
            config->stencilBits,
#endif
            EGL_SAMPLES,
            config->multiSampleSize,
            EGL_SAMPLE_BUFFERS,
            config->nMultiSampleBuffers,
            EGL_RENDERABLE_TYPE,
            (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,
            globals4es.usepbuffer ? EGL_PBUFFER_BIT : type,
            EGL_NONE};
        if (globals4es.usefb) ++fbcontext_count;

        LOAD_EGL(eglMakeCurrent);
        LOAD_EGL(eglDestroyContext);
        LOAD_EGL(eglDestroySurface);
        LOAD_EGL(eglCreateContext);
        LOAD_EGL(eglChooseConfig);
        LOAD_EGL(eglQueryString);
        LOAD_EGL(eglGetConfigAttrib);

        GLXContext fake = calloc(1, sizeof(struct __GLXContextRec));
        fake->es2only = globales2;

        fake->shared = (share_context) ? share_context->glstate : NULL;

        // make an egl context here...
        EGLBoolean result;
        if (eglDisplay == NULL || eglDisplay == EGL_NO_DISPLAY) {
            init_display(display);
            if (eglDisplay == EGL_NO_DISPLAY) {
                LOGE("Unable to create EGL display.\n");
                return fake;
            }
        }

        // first time?
        if (eglInitialized == false) {
            result = InitEGL(display);
            if (!result) {
                CheckEGLErrors();
                LOGE("Unable to initialize EGL display.\n");
                return fake;
            }
        }

        result = egl_eglChooseConfig(eglDisplay, configAttribs, fake->eglConfigs, 64, &fake->eglConfigsCount);
        if (fake->eglConfigsCount && globals4es.usegbm)
            fake->eglconfigIdx = FindGBMConfig(eglDisplay, fake->eglConfigs, fake->eglConfigsCount);
        else
            fake->eglconfigIdx = 0;

        CheckEGLErrors();
        if (result != EGL_TRUE || fake->eglConfigsCount == 0) {
            LOGE("No EGL configs found.\n");
            return fake;
        }
        EGLContext shared = (share_context) ? share_context->eglContext : EGL_NO_CONTEXT;
        fake->eglContext = egl_eglCreateContext(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], shared,
                                                (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);

        CheckEGLErrors();

        // need to return a glx context pointing at it
        fake->display = display;
        fake->direct = true;
        fake->xid = 1;                                                       // TODO: Proper handling of that id...
        fake->contextType = (config->drawableType & GLX_WINDOW_BIT) ? 0 : 2; // Window:Pixmap
        fake->doublebuff = config->doubleBufferMode;

        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_RED_SIZE, &fake->rbits);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_GREEN_SIZE, &fake->gbits);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_BLUE_SIZE, &fake->bbits);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_ALPHA_SIZE, &fake->abits);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_DEPTH_SIZE, &fake->depth);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_STENCIL_SIZE, &fake->stencil);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_SAMPLES, &fake->samples);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_SAMPLE_BUFFERS,
                               &fake->samplebuffers);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_MIN_SWAP_INTERVAL, &minswap);
        egl_eglGetConfigAttrib(eglDisplay, fake->eglConfigs[fake->eglconfigIdx], EGL_MAX_SWAP_INTERVAL, &maxswap);
        DBG(SHUT_LOGD(" => return %p (eglContext=%p, context->shared=%p)\n", fake, fake->eglContext, fake->shared);)
        return fake;
    }
}

void gl4es_glXDestroyContext(Display* display, GLXContext ctx) {
    DBG(SHUT_LOGD("glXDestroyContext(%p, %p), fbcontext_count=%d, ctx_type=%d\n", display, ctx, fbcontext_count,
                  (ctx) ? ctx->contextType : 0);)
    if (globals4es.usefb) --fbcontext_count;
    if (ctx->eglContext) {
        // need to bind back the context to delete stuff?
        LOAD_EGL(eglMakeCurrent);
        if (eglSurface != ctx->eglSurface || eglContext != ctx->eglContext) {
            DBG(SHUT_LOGD("  temporary switch to eglSurface:%p(from %p), eglContext:%p(from %p)\n", ctx->eglSurface,
                          eglSurface, ctx->eglContext, eglContext);)
#ifndef NO_GBM
            if (globals4es.usegbm)
                GBMMakeCurrent(eglDisplay, ctx->eglSurface, ctx->eglSurface, ctx->eglContext);
            else
#endif
                egl_eglMakeCurrent(eglDisplay, ctx->eglSurface, ctx->eglSurface, ctx->eglContext);
        }

        if (globals4es.usefbo && ctx->contextType == 0) {
            deleteMainFBO(ctx->glstate);
        }

        DeleteGLState(ctx->glstate);

        // bind context back
        if (eglSurface != ctx->eglSurface || eglContext != ctx->eglContext) {
            DBG(SHUT_LOGD("  switch back to eglSurface:%p, eglContext:%p\n", eglSurface, eglContext);)
#ifndef NO_GBM
            if (globals4es.usegbm)
                GBMMakeCurrent(eglDisplay, eglContext ? eglSurface : NULL, eglContext ? eglSurface : NULL, eglContext);
            else
#endif
                egl_eglMakeCurrent(eglDisplay, eglContext ? eglSurface : NULL, eglContext ? eglSurface : NULL,
                                   eglContext);
        }

        LOAD_EGL(eglDestroyContext);
        LOAD_EGL(eglDestroySurface);

        EGLBoolean result = egl_eglDestroyContext(eglDisplay, ctx->eglContext);
        ctx->eglContext = 0;
        if (ctx->eglSurface != 0) {
            if (globals4es.usefb != 1 || !fbcontext_count) { // (This may cause troble on Pandora, has some driver
                                                             // doesn't seems to like to many Creation of the surface)
                int destroySurf = 1;
                if (ctx->shared_eglsurface && (--(*ctx->shared_eglsurface)) > 0) destroySurf = 0;
                if (destroySurf) {
                    if (!globals4es.glxrecycle) {
                        DBG(SHUT_LOGD("  egDestroySurface(%p, %p), drawable=%p\n", eglDisplay, ctx->eglSurface,
                                      (void*)ctx->drawable);)
                        egl_eglDestroySurface(eglDisplay, ctx->eglSurface);
                        RecycleDelSurface(ctx->drawable);
                    }
                }
                eglSurface = 0;
            }
            ctx->eglSurface = 0;
        }
        if (ctx->shared_eglsurface && (*ctx->shared_eglsurface) <= 0 && !globals4es.glxrecycle)
            free(ctx->shared_eglsurface);

        if (result != EGL_TRUE) {
            CheckEGLErrors();
            LOGE("Failed to destroy EGL context.\n");
        }
        /*if (fbdev >= 0) {
            close(fbdev);
            fbdev = -1;
        }*/
    }
    if (glxContext == ctx) glxContext = NULL;

    free(ctx);
    return;
}

Display* gl4es_glXGetCurrentDisplay() {
    DBG(SHUT_LOGD("glXGetCurrentDisplay()\n");)
    if (g_display && eglContext) {
        return g_display;
    }

    return XOpenDisplay(NULL);
}

XVisualInfo* gl4es_glXChooseVisual(Display* display, int screen, int* attributes) {
    DBG(SHUT_LOGD("glXChooseVisual(%p, %d, %p[", display, screen, attributes);)
    DBG(if (attributes) {
        for (int* a = attributes; *a != 0; ++a)
            printf("%x,", *a);
        printf("0");
    })
    DBG(SHUT_LOGD("])\n");)

    // create a new attribute list for glXChooseFBConfig based on the attributes liste given...
    int attr[50];
    int idx = 0;
    int cur = 0;
    int ask_depth = 0;
    int vis_class = TrueColor;
    if (attributes) {

        int ask_rgba = 0;
        while (attributes[cur]) {
            switch (attributes[cur]) {
            case GLX_RGBA:
                ask_rgba = 1; // only rgba will be supported?
                break;
            case GLX_USE_GL: // yeah, I know
                break;
            case GLX_BUFFER_SIZE:
            case GLX_STEREO:
            case GLX_ACCUM_RED_SIZE:
            case GLX_ACCUM_GREEN_SIZE:
            case GLX_ACCUM_BLUE_SIZE:
            case GLX_ACCUM_ALPHA_SIZE:
                ++cur; // ignored
                break;
            case GLX_DOUBLEBUFFER:
                attr[idx++] = GLX_DOUBLEBUFFER;
                attr[idx++] = 1;
                break;
            case GLX_RED_SIZE:
            case GLX_GREEN_SIZE:
            case GLX_BLUE_SIZE:
            case GLX_ALPHA_SIZE:
                ask_depth += attributes[cur + 1];
                // fallback is intended
            case GLX_DEPTH_SIZE:
            case GLX_STENCIL_SIZE:
            case GLX_LEVEL:
            case GLX_SAMPLE_BUFFERS:
            case GLX_SAMPLES:
                attr[idx++] = attributes[cur++];
                attr[idx++] = attributes[cur];
                break;
            case GLX_X_VISUAL_TYPE:
                attr[idx++] = attributes[cur++];
                attr[idx++] = attributes[cur];
                if (attributes[cur] == GLX_DIRECT_COLOR) vis_class = DirectColor;
                break;
            }
            ++cur;
        }
        attr[idx++] = 0; // end list

        if (!ask_rgba) return NULL; // only TrueColor profile...
    }
    glx_default_depth = XDefaultDepth(display, screen);
    if (glx_default_depth != 16 && glx_default_depth != 24 && glx_default_depth != 32)
        LOGD("unusual desktop color depth %d\n", glx_default_depth);

#ifndef PANDORA
    // PANDORA only has 16bits X11, lets ignore 32bits requests
/*    if(ask_depth>glx_default_depth)
        glx_default_depth = ask_depth;  // higher depth...
*/  // this makes window of TokiTory transparent...
#endif

    XVisualInfo xvinfo = {0};
    xvinfo.depth = glx_default_depth;
    xvinfo.class = TrueColor;
    int n;
    XVisualInfo* visuals = XGetVisualInfo(display, VisualDepthMask | VisualClassMask, &xvinfo, &n);
    if (!n) {
        LOGD("Warning, gl4es_glXChooseVisual: XGetVisualInfo gives 0 VisualInfo for %d depth and TrueColor class\n",
             glx_default_depth);
        return NULL;
    }

    // create and store the glxConfig that goes with thoses attributes
    int count = 1;
    GLXFBConfig* confs = NULL;
    if (cur)
        confs = gl4es_glXChooseFBConfig(display, screen, attr, &count);
    else
        confs = gl4es_glXGetFBConfigs(display, screen, &count);
    if (!count) {
        DBG(SHUT_LOGD("glXChooseVisual return %p (because no Config found)\n", NULL);)
        return NULL;
    }
    AddFBVisual(visuals, confs);

    DBG(SHUT_LOGD("glXChooseVisual return %p\n", visuals);)
    return visuals;
}

/*
EGL_BAD_MATCH is generated if draw or read are not compatible with context
or if context is set to EGL_NO_CONTEXT and draw or read are not set to
EGL_NO_SURFACE, or if draw or read are set to EGL_NO_SURFACE and context is
not set to EGL_NO_CONTEXT.
*/

Bool gl4es_glXMakeCurrent(Display* display, GLXDrawable drawable, GLXContext context) {
#ifdef NOX11
    DBG(SHUT_LOGD("glXMakeCurrent(%p, %p, %p), context->drawable=%p, context->eglSurface=%p(%p), "
                  "context->doublebuff=%d, glxContext=%p\n",
                  display, (void*)drawable, context, (void*)(context ? context->drawable : 0),
                  context ? context->eglSurface : 0, eglSurface, context ? context->doublebuff : 0, glxContext);)
#else
    DBG(SHUT_LOGD("glXMakeCurrent(%p, %p, %p), isPBuffer(drawable)=%d, context->drawable=%p, "
                  "context->eglSurface=%p(%p), context->doublebuff=%d, glxContext=%p\n",
                  display, (void*)drawable, context, isPBuffer(drawable), (void*)(context ? context->drawable : 0),
                  context ? context->eglSurface : 0, eglSurface, context ? context->doublebuff : 0, glxContext);)
#endif
    LOAD_EGL(eglMakeCurrent);
    LOAD_EGL(eglDestroySurface);
    LOAD_EGL(eglCreateWindowSurface);
    LOAD_EGL(eglQuerySurface);
#ifdef NOX11
    int created = 0;
#else
    int created = (context) ? isPBuffer(drawable) : 0;
#endif
    EGLContext eglCtx = EGL_NO_CONTEXT;
    EGLSurface eglSurf = 0;
    EGLConfig eglConfig = 0;
    // flush current context if exist...
    if (glxContext && glxContext->glstate) {
        /*if(!context && !glxContext->doublebuff && !glxContext->contextType) {
            gl4es_glXSwapBuffers(display, glxContext->drawable);
        } else*/
        gl4es_glFlush();
    }
    if (context && glxContext == context && context->drawable == drawable) {
        DBG(SHUT_LOGD(" => True\n");)
        // same context, all is done bye
        DBG(SHUT_LOGD("Same context and drawable, doing nothing\n");)
        return true;
    }
    if (glxContext && glxContext->drawable == drawable && glxContext->eglSurface && !context->eglSurface &&
        ((glxContext->shared == context->shared && glxContext->shared) || glxContext == context->shared ||
         context == glxContext->shared)) {

        context->eglSurface = glxContext->eglSurface; // lets hope eglContexts are compatible
        if (!glxContext->shared_eglsurface) glxContext->shared_eglsurface = (int*)calloc(1, sizeof(int));
        context->shared_eglsurface = glxContext->shared_eglsurface;
        (*glxContext->shared_eglsurface)++;
        if (!context->glstate) context->glstate = NewGLState(context->shared, context->es2only);
        DBG(SHUT_LOGD("Same drawable and compatible context: sharing everything...\n");)
    }
    if (context && glxContext && context->drawable == drawable && context->eglSurface == eglSurface) {
        gl4es_saveCurrentFBO();
        CopyGLEShard(context->glstate, glxContext->glstate);
        ActivateGLState(context->glstate);
        glxContext = context;
        gl4es_restoreCurrentFBO();

        DBG(SHUT_LOGD(" => True\n");)
        // same context, all is done bye (only one context per surface anyway in EGL, iirc)
        DBG(SHUT_LOGD("Same Surface and Drawable, doing (almost) nothing\n");)
        return true;
    }

    void* old_glstate = NULL;
    if (glxContext && glxContext->glstate) {
        gl4es_saveCurrentFBO();
        old_glstate = glxContext->glstate;
    }

    if (context) {
        if (context->drawable == drawable && context->eglSurface) {
            // same-same, recycling...
            // eglSurf = context->eglSurface;    // no need, it's done after the ifs...
        } else if (glxContext && glxContext->drawable == drawable && glxContext->eglSurface && !context->eglSurface) {
            context->eglSurface = glxContext->eglSurface; // lets hope eglContexts are compatible
            if (!glxContext->shared_eglsurface) glxContext->shared_eglsurface = (int*)calloc(1, sizeof(int));
            context->shared_eglsurface = glxContext->shared_eglsurface;
            (*glxContext->shared_eglsurface)++;
        } else {
            // new one
            if (created) {
#ifndef NOX11
                eglSurf = context->eglSurface = pbuffersize[created - 1].Surface; //(EGLSurface)drawable;
                eglCtx = context->eglContext = pbuffersize[created - 1].Context;  // this context is ok for the PBuffer
                eglConfig = context->eglConfigs[context->eglconfigIdx] = pbuffersize[created - 1].Config;
                /*if (context->contextType != pbuffersize[created-1].Type) {    // Context / buffer not aligned, create
                a new glstate tracker if(context->glstate) DeleteGLState(context->glstate); context->glstate = NULL;
                    context->shared = NULL;
                }*/
#endif
            } else {
                EGLint attrib_list[5] = {0};
                int cnt = 0;
                if (!context->doublebuff) {
                    attrib_list[cnt++] = EGL_RENDER_BUFFER;
                    attrib_list[cnt++] = EGL_SINGLE_BUFFER;
                }
                if (globals4es.glx_surface_srgb) {
                    attrib_list[cnt++] = EGL_GL_COLORSPACE_KHR;
                    attrib_list[cnt++] = EGL_GL_COLORSPACE_SRGB_KHR;
                }
                attrib_list[cnt++] = EGL_NONE;

                unsigned int width = 0, height = 0, depth = 0;
#ifndef NOX11
                if (globals4es.usepbuffer) {
                    // Get Window size and all...
                    unsigned int border;
                    Window root;
                    int x, y;
                    XGetGeometry(display, drawable, &root, &x, &y, &width, &height, &border, &depth);
                    DBG(SHUT_LOGD("XGetGeometry gives %dx%d for drawable %p\n", width, height, (void*)drawable);)
                } else if ((globals4es.usefb) || globals4es.usegbm) {
                    // Get size of desktop
                    Screen* screen = DefaultScreenOfDisplay(display);
                    width = WidthOfScreen(screen);
                    height = HeightOfScreen(screen);
                    DBG(SHUT_LOGD("X11 gives a size of desktop %dx%d for drawable %p\n", width, height,
                                  (void*)drawable);)
                }
                if (globals4es.usepbuffer) {
                    // let's create a PBuffer attributes
                    EGLint egl_attribs[10]; // should be enough
                    int i = 0;
                    egl_attribs[i++] = EGL_WIDTH;
                    egl_attribs[i++] = width;
                    egl_attribs[i++] = EGL_HEIGHT;
                    egl_attribs[i++] = height;
                    egl_attribs[i++] = EGL_NONE;

                    if (createPBuffer(display, egl_attribs, &eglSurf, &eglCtx, &eglConfig, (depth > 16) ? 8 : 5,
                                      (depth == 15)  ? 5
                                      : (depth > 16) ? 8
                                                     : 6,
                                      (depth > 16) ? 8 : 5, (depth == 32) ? 8 : 0, context->samplebuffers,
                                      context->samples) == 0) {
                        // fail too, abort
                        SHUT_LOGE("PBuffer creation failed too\n");
                        return 0;
                    }
                    int Width, Height;

                    egl_eglQuerySurface(eglDisplay, eglSurf, EGL_WIDTH, &Width);
                    egl_eglQuerySurface(eglDisplay, eglSurf, EGL_HEIGHT, &Height);
                    DBG(SHUT_LOGD("New surface %p is %dx%d\n", eglSurf, Width, Height);)

                    addPixBuffer(display, eglSurf, eglConfig, Width, Height, eglCtx, drawable, depth, 2);
                    context->eglSurface = eglSurf;
                    if (context->eglContext && context->eglContext != eglCtx) {
                        // remove old context before putting PBuffer specific one
                        LOAD_EGL(eglDestroyContext);
                        DBG(SHUT_LOGD("Remove old Cotnext %p, new is %p\n", context->eglContext, eglCtx);)
                        egl_eglDestroyContext(eglDisplay, context->eglContext);
                    }
                    context->eglContext = eglCtx;
                    // update, that context is a created emulated one...
                    created = isPBuffer(drawable);
                } else
#endif
                {
                    if (globals4es.usefb || globals4es.usefbo || globals4es.usegbm) {
                        if (eglSurface) // cannot create multiple eglSurface for the same Framebuffer?
                            eglSurf = context->eglSurface = eglSurface;
                        else {
                            SharedEGLSurface_t* oldsurf = RecycleGetSurface(drawable);
                            if (oldsurf) {
                                eglSurf = oldsurf->surf;
                                if (eglSurf != EGL_NO_SURFACE) {
                                    context->shared_eglsurface = oldsurf->cnt;
                                    ++(*context->shared_eglsurface);
                                }
                            }
                            if (eglSurf == EGL_NO_SURFACE) {
                                context->nativewin = create_native_window(width, height);
#if 0 // ndef NO_GBM
                                if(globals4es.usegbm) {
                                    LOAD_EGL_EXT(eglCreatePlatformWindowSurface);
                                    eglSurf = egl_eglCreatePlatformWindowSurface(eglDisplay, context->eglConfigs[context->eglconfigIdx], context->nativewin, attrib_list);
                                } else
#endif
                                eglSurf =
                                    egl_eglCreateWindowSurface(eglDisplay, context->eglConfigs[context->eglconfigIdx],
                                                               (EGLNativeWindowType)context->nativewin, attrib_list);
                            } else {
                                DBG(SHUT_LOGD("LIBGL: EglSurf Recycled\n");)
                            }
                            eglSurface = context->eglSurface = eglSurf;
                            if (!eglSurf) {
                                DBG(SHUT_LOGD("LIBGL: Warning, EglSurf is null\n");)
                                CheckEGLErrors();
                            }
                        }
                    } else {
                        if (context->eglSurface) {
                            int destroySurf = 1;
                            if (context->shared_eglsurface && (--(*context->shared_eglsurface)) > 0) destroySurf = 0;
                            if (destroySurf) {
                                if (!globals4es.glxrecycle) {
                                    egl_eglDestroySurface(eglDisplay, context->eglSurface);
                                    RecycleDelSurface(context->drawable);
                                }
                            }
                        }
                        SharedEGLSurface_t* oldsurf = RecycleGetSurface(drawable);
                        if (oldsurf) {
                            eglSurf = oldsurf->surf;
                            if (eglSurf != EGL_NO_SURFACE) {
                                context->shared_eglsurface = oldsurf->cnt;
                                ++(*context->shared_eglsurface);
                            }
                        }
                        if (eglSurf == EGL_NO_SURFACE) {
                            eglSurf = context->eglSurface =
                                egl_eglCreateWindowSurface(eglDisplay, context->eglConfigs[0], drawable, attrib_list);
                        } else {
                            DBG(SHUT_LOGD("LIBGL: eglSurf Recycled\n");)
                            context->eglSurface = eglSurf;
                        }
                        if (!eglSurf) {
                            DBG(SHUT_LOGD("LIBGL: Warning, eglSurf is null\n");)
                            CheckEGLErrors();
                        }
                    }
                }
            }
        }
        eglSurf = context->eglSurface;
        eglCtx = context->eglContext;
    }
    EGLBoolean result;
#ifndef NO_GBM
    if (globals4es.usegbm)
        result = GBMMakeCurrent(eglDisplay, eglSurf, eglSurf, eglCtx);
    else
#endif
        result = egl_eglMakeCurrent(eglDisplay, eglSurf, eglSurf, eglCtx);
    DBG(SHUT_LOGD("LIBGL: eglMakeCurrent(%p, %p, %p, %p)\n", eglDisplay, eglSurf, eglSurf, eglCtx);)
    CheckEGLErrors();
    glxContext = context;
    if (!result) {
        // error switching context, don't change glstate and abort...
        DBG(SHUT_LOGD(" => False\n");)
        return false;
    } else {
        eglSurface = eglSurf;
        eglContext = eglCtx;
        eglConfigs[0] = eglConfig;
    }
    // update MapDrawable
    {
        int ret;
        khint_t k = kh_get(mapdrawable, MapDrawable, drawable);
        map_drawable_t* map = NULL;
        if (k == kh_end(MapDrawable)) {
            k = kh_put(mapdrawable, MapDrawable, drawable, &ret);
            map = kh_value(MapDrawable, k) = (map_drawable_t*)malloc(sizeof(map_drawable_t));
            map->drawable = drawable;
        } else {
            map = kh_value(MapDrawable, k);
        }
        map->surface = eglSurf;
        map->PBuffer = created;
    }
    if (context) {
        // update Recycle
        if (!RecycleGetSurface(drawable)) {
            // add tracking, so add shared_counter now
            if (!glxContext->shared_eglsurface) {
                glxContext->shared_eglsurface = (int*)calloc(1, sizeof(int));
            }
            (*glxContext->shared_eglsurface)++;
            SharedEGLSurface_t surf;
            surf.surf = glxContext->eglSurface;
            surf.cnt = glxContext->shared_eglsurface;
            RecycleAddSurface(drawable, &surf);
        }

        if (!context->glstate) {
            context->glstate = NewGLState(context->shared, context->es2only);
            if (created && pbuffersize[created - 1].Type >= 3) {
                ((glstate_t*)context->glstate)->emulatedPixmap = created;
                ((glstate_t*)context->glstate)->emulatedWin = pbuffersize[created - 1].Type == 4 ? 1 : 0;
            }
        }
        context->drawable = drawable;

        ActivateGLState(context->glstate);
#ifdef PANDORA
        if (!created) pandora_set_gamma();
#endif

        CheckEGLErrors();
        if (result) {
            if (globals4es.usefbo && !created) {
                // get size of the surface...
                egl_eglQuerySurface(eglDisplay, eglSurf, EGL_WIDTH, &g_width);
                egl_eglQuerySurface(eglDisplay, eglSurf, EGL_HEIGHT, &g_height);
                int fbo_width, fbo_height;
                if (GetEnvVarFmt("LIBGL_FBO", "%dx%d", &fbo_width, &fbo_height) == 2) {
                    DBG(SHUT_LOGD("Forcing FBO size %dx%d (%dx%d)\n", fbo_width, fbo_height, g_width, g_height);)
                    g_width = fbo_width;
                    g_height = fbo_height;
                }
                // create the main_fbo...
                createMainFBO(g_width, g_height);
            }

            gl4es_restoreCurrentFBO();

            // finished
            DBG(SHUT_LOGD(" => True (glstate=%p)\n", context ? context->glstate : NULL);)
            return true;
        }
        DBG(SHUT_LOGD(" => False\n");)
        return false;
    }
    DBG(SHUT_LOGD(" => True (glstate=%p)\n", context ? context->glstate : NULL);)
    return true;
}

Bool gl4es_glXMakeContextCurrent(Display* display, int drawable, int readable, GLXContext context) {
    DBG(SHUT_LOGD("glXMakeContextCurrent(%p, %X, %X, %p)\n", display, drawable, readable, context);)
    return gl4es_glXMakeCurrent(display, drawable, context);
}

void gl4es_glXSwapBuffers(Display* display, GLXDrawable drawable) {
    static int frames = 0;
    DBG(SHUT_LOGD("\rglXSwapBuffers(%p, %p) ", display, (void*)drawable);)
    LOAD_EGL(eglSwapBuffers);
    // TODO: what if active context is not on the drawable?
    realize_textures(0);
    if (glstate->list.active) {
        gl4es_flush();
    }
    if (glstate->raster.bm_drawing) bitmap_flush();
    EGLSurface surface = eglSurface;
    int PBuffer = 0;
    {
        // get MapDrawable surface
        khint_t k = kh_get(mapdrawable, MapDrawable, drawable);
        map_drawable_t* map = NULL;
        if (k != kh_end(MapDrawable)) {
            map = kh_value(MapDrawable, k);
            surface = map->surface;
            PBuffer = map->PBuffer;
        }
    }
#ifdef USE_FBIO
    if (globals4es.vsync && fbdev >= 0 && PBuffer == 0) {
        // TODO: can I just return if I don't meet vsync over multiple frames?
        // this will just block otherwise.
        int arg = 0;
        for (int i = 0; i < swapinterval; i++) {
            ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
        }
    }
#endif
#ifdef PYRA
    if (globals4es.vsync && PBuffer == 0) {
        static int64_t old_time = 0;
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
#define TARGET_FPS 1000000000LL / 61
        int64_t new_time = tp.tv_sec * 1000000000LL + tp.tv_nsec;
        if (old_time) {
            int64_t time_wait = new_time - old_time;
            if (time_wait < TARGET_FPS) {
                time_wait = TARGET_FPS - time_wait;
                tp.tv_nsec = time_wait;
                tp.tv_sec = 0;
                nanosleep(&tp, NULL);
                clock_gettime(CLOCK_REALTIME, &tp);
                new_time = tp.tv_sec * 1000000000LL + tp.tv_nsec;
            }
        }
        old_time = new_time;
    }
#endif
    if (globals4es.usefbo && PBuffer == 0) {
        unbindMainFBO();
        int x = 0, y = 0;
        unsigned int width = 0, height = 0;
#ifndef NOX11
        {
            unsigned int border, depth;
            Window root;
            XWindowAttributes xwa;
            XGetGeometry(display, drawable, &root, &x, &y, &width, &height, &border,
                         &depth); // get geometry (relative to window)
            XTranslateCoordinates(display, drawable, root, 0, 0, &x, &y,
                                  &root); // translate to get x,y absolute to screen
        }
#endif
        DBG(SHUT_LOGD("blitMainFBO(%d, %d, %u, %u)\n", x, y, width, height);)
        blitMainFBO(x, y, width, height);
        // blit the main_fbo before swap
        // adjust FBO size if needed
        if ((width && height && (width != glstate->fbo.mainfbo_width || height != glstate->fbo.mainfbo_height))) {
            createMainFBO(width, height); // adjust mainFBO
        }
    }
    // check emulated Pixmap
    if (PBuffer && glstate->emulatedPixmap) {
        LOAD_GLES(glFinish);
        gles_glFinish();
        BlitEmulatedPixmap(drawable);
    } else
        egl_eglSwapBuffers(eglDisplay, surface);
    // CheckEGLErrors();     // not sure it's a good thing to call a eglGetError() after all eglSwapBuffers, performance
    // wize (plus result is discarded anyway)
#ifdef PANDORA
    if (globals4es.showfps || (sock > -1))
#else
    if (globals4es.showfps)
#endif
    {
        // framerate counter
        static float avg, fps = 0;
        static int frame1, last_frame, frame, now, current_frames;
#ifdef USE_CLOCK
        struct timespec out;
        clock_gettime(CLOCK_MONOTONIC_RAW, &out);
        now = out.tv_sec;
#else
        struct timeval out;
        gettimeofday(&out, NULL);
        now = out.tv_sec;
#endif
        frame++;
        current_frames++;

        if (frame == 1) {
            frame1 = now;
        } else if (frame1 < now) {
            if (last_frame < now) {
                float change = current_frames / (float)(now - last_frame);
                float weight = 0.7;
                if (!fps) {
                    fps = change;
                } else {
                    fps = (1 - weight) * fps + weight * change;
                }
                current_frames = 0;

                avg = frame / (float)(now - frame1);
                if (globals4es.showfps) LOGD("fps: %.2f, avg: %.2f\n", fps, avg);
#ifdef PANDORA
                if (sock > -1) {
                    char tmp[60];
                    snprintf(tmp, 60, "gl:  %2.2f", fps);
                    sendto(sock, tmp, strlen(tmp), 0, (struct sockaddr*)&sun, sizeof(sun));
                }
#endif
            }
        }
        last_frame = now;
    }
    if (globals4es.usefbo && PBuffer == 0) {
        bindMainFBO();
    }
}

void gl4es_SwapBuffers_currentContext() {
    if (glxContext) gl4es_glXSwapBuffers(glxContext->display, glxContext->drawable);
}

int gl4es_glXGetConfig(Display* display, XVisualInfo* visual, int attribute, int* value) {
    DBG(SHUT_LOGD("glXGetConfig(%p, %p, 0x%x, %p)\n", display, visual, attribute, value);)
    GLXFBConfig* config = FindFBVisual(visual);
    if (config)
        return gl4es_glXGetFBConfigAttrib(display, *config, attribute, value);
    else
        return get_config_default(display, attribute, value);
}

int gl4es_glXQueryContext(Display* dpy, GLXContext ctx, int attribute, int* value) {
    DBG(SHUT_LOGD("glXQueryContext(%p, %p, %d, %p)\n", dpy, ctx, attribute, value);)
    *value = 0;
    if (ctx) switch (attribute) {
        case GLX_FBCONFIG_ID:
            *value = (int)(uintptr_t)ctx->eglConfigs[ctx->eglconfigIdx];
            break;
        case GLX_RENDER_TYPE:
            *value = GLX_RGBA_TYPE;
            break;
        case GLX_SCREEN:
            break; // screen n# is always 0
        }
    return 0;
}

// stubs for glfw (GLX 1.3)
GLXContext gl4es_glXGetCurrentContext() {
    DBG(SHUT_LOGD("glXGetCurrentContext()\n");)

    return glxContext;
}

#ifndef NO_EGL
GLXFBConfig* fillGLXFBConfig(EGLConfig* eglConfigs, int count, int withDB, Display* display) {
    init_eglconfig(display);
    LOAD_EGL(eglGetConfigAttrib);
    EGLint confID;
    if (withDB == 2) count *= 2;
    GLXFBConfig* configs = (GLXFBConfig*)calloc(count, sizeof(GLXFBConfig));
    for (int j = 0; j < count; ++j) {
        int i = (withDB != 2) ? j : (j / 2);
        egl_eglGetConfigAttrib(eglDisplay, eglConfigs[i], EGL_CONFIG_ID, &confID);
        confID += ((withDB == 2) ? (j % 2) : withDB) * maxEGLConfig;
        configs[j] = &allFBConfig[confID];
    }

    return configs;
}
#endif

GLXFBConfig* gl4es_glXChooseFBConfig(Display* display, int screen, const int* attrib_list, int* count) {
    DBG(SHUT_LOGD("glXChooseFBConfig(%p, %d, %p, %p)\n", display, screen, attrib_list, count);)
    // Maybe it would be easier to simply return an EGLConfig array?
#ifdef NO_EGL
    static struct __GLXFBConfigRec currentConfig[8];
    static int idx = 0;
    *count = 1;
    GLXFBConfig* configs = (GLXFBConfig*)malloc(sizeof(GLXFBConfig));
    configs[0] = &currentConfig[idx];
    idx = (idx + 1) % 8;
    memset(configs[0], 0, sizeof(struct __GLXFBConfigRec));
    // fill that config with some of the attrib_list info...
    configs[0]->drawableType = GLX_WINDOW_BIT;
    configs[0]->screen = 0;
    configs[0]->maxPbufferWidth = configs[0]->maxPbufferHeight = 2048;
    configs[0]->redBits = configs[0]->greenBits = configs[0]->blueBits = configs[0]->alphaBits = 0;
    configs[0]->nMultiSampleBuffers = 0;
    configs[0]->multiSampleSize = 0;
    configs[0]->depthBits = 16;
    configs[0]->stencilBits = 8;

    return configs;
#else
    // first build a table of EGL attributes
    int attr[50];
    int i = 0;
    int cur = 0;
    int cr = 0, cg = 0, cb = 0, ca = 0, vt = 0;
    int tmp;
    int drawable_set = 0;
    int doublebuffer = 2;
    int glxconfig = -1;
    attr[cur++] = EGL_SURFACE_TYPE;
    attr[cur++] = 0;

    if (attrib_list) {
        i = 0;
        while (attrib_list[i] != 0) {
            switch (attrib_list[i++]) {
            case GLX_RED_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_RED_SIZE;
                cr = cur;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig redBits=%d\n", tmp);)
                break;
            case GLX_GREEN_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_GREEN_SIZE;
                cg = cur;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig greenBits=%d\n", tmp);)
                break;
            case GLX_BLUE_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_BLUE_SIZE;
                cb = cur;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig blueBits=%d\n", tmp);)
                break;
            case GLX_ALPHA_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_ALPHA_SIZE;
                ca = cur;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig alphaBits=%d\n", tmp);)
                break;
            case GLX_DEPTH_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_DEPTH_SIZE;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig depthBits=%d\n", tmp);)
                break;
            case GLX_STENCIL_SIZE:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_STENCIL_SIZE;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig stencilBits=%d\n", tmp);)
                break;
            case GLX_DRAWABLE_TYPE:
                tmp = attrib_list[i++];
                // attr[0] = EGL_SURFACE_TYPE;
                // attr[1] = 0;
                if (tmp & GLX_WINDOW_BIT) attr[1] |= EGL_WINDOW_BIT;
                if (tmp & GLX_PIXMAP_BIT) attr[1] |= EGL_PIXMAP_BIT;
                if (tmp & GLX_PBUFFER_BIT) attr[1] |= EGL_PIXMAP_BIT;
                drawable_set = 1;
                DBG(SHUT_LOGD("FBConfig drawableType=0x%X\n", tmp);)
                break;
            case GLX_SAMPLE_BUFFERS:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_SAMPLE_BUFFERS;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig multisampleBuffers=%d\n", tmp);)
                break;
            case GLX_SAMPLES:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_SAMPLES;
                attr[cur++] = (tmp < 0) ? 0 : tmp;
                DBG(SHUT_LOGD("FBConfig multiSampleSize=%d\n", tmp);)
                break;
            case GLX_DOUBLEBUFFER:
                if (attrib_list[i] == 0 || attrib_list[i] == 1)
                    doublebuffer = attrib_list[i++];
                else
                    doublebuffer = 1;
                DBG(SHUT_LOGD("FBConfig doubleBufferMode=%d\n", doublebuffer);)
                break;
            case GLX_X_RENDERABLE:
                ++i; // value ignored
                DBG(SHUT_LOGD("FBConfig renderable=%d\n", attrib_list[i - 1]);)
                break;
            case GLX_LEVEL:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_LEVEL;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig level=%d\n", tmp);)
                break;
            case GLX_VISUAL_ID:
                tmp = attrib_list[i++];
                attr[cur++] = EGL_NATIVE_VISUAL_ID;
                attr[cur++] = tmp;
                DBG(SHUT_LOGD("FBConfig visual id=%d\n", tmp);)
                break;
            case GLX_FBCONFIG_ID:
                glxconfig = attrib_list[i++];
                DBG(SHUT_LOGD("GLXFBConfigID=%d\n", glxconfig);)
                break;
            case GLX_X_VISUAL_TYPE:
                tmp = attrib_list[i++];
                if (!(globals4es.usepbuffer || globals4es.usefb || globals4es.usefbo || globals4es.glxnative)) {
                    if (tmp != GLX_TRUE_COLOR) { // GLX_TRUE_COLOR is ok, don't add it to the list
                        attr[cur++] = EGL_NATIVE_VISUAL_TYPE;
                        vt = cur;
                        attr[cur++] = tmp;
                    }
                } // re-enabled, seems to be needed now on ODROID...
                DBG(SHUT_LOGD("FBConfig visual type=%d\n", tmp);)
                break;
            default:
                ++i;
                // discard other stuffs
            }
        }
    } else {
        // choose a context with DEPTH & STENCIL if no attribute are given
        attr[cur++] = EGL_DEPTH_SIZE;
        attr[cur++] = 16;
        attr[cur++] = EGL_STENCIL_SIZE;
        attr[cur++] = 8;
    }
    attr[1] |= (globals4es.usepbuffer) ? (/*EGL_PBUFFER_BIT|*/ EGL_PIXMAP_BIT) : EGL_WINDOW_BIT;

    attr[cur++] = EGL_RENDERABLE_TYPE;
    attr[cur++] = (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT;

    attr[cur++] = EGL_NONE; // end list

    // get the number of EGL config matching!
    if (eglInitialized == false) {
        if (!InitEGL((globals4es.usefb || globals4es.usepbuffer) ? g_display : display)) {
            CheckEGLErrors();
            LOGE("Unable to initialize EGL.\n");
            return NULL;
            *count = 0;
        }
    }
    LOAD_EGL(eglChooseConfig);
    if (glxconfig == -1) {
        egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
        if ((*count == 0) && (globals4es.usepbuffer)) {
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config with PixMap, trying again with PBuffer\n");)
            // try again, but with PBuffer
            attr[1] = EGL_PBUFFER_BIT;
            egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
            // On Pandora and GLES2, only 565 PBuffer are available!
            if (cr || cg || cb || ca) {
                --cur;
                if (cr)
                    attr[cr] = 5;
                else {
                    attr[cur] = EGL_RED_SIZE;
                    attr[cur++] = 5;
                }
                if (cg)
                    attr[cg] = 6;
                else {
                    attr[cur] = EGL_GREEN_SIZE;
                    attr[cur++] = 6;
                }
                if (cb)
                    attr[cb] = 5;
                else {
                    attr[cur] = EGL_BLUE_SIZE;
                    attr[cur++] = 5;
                }
                if (ca)
                    attr[ca] = 0;
                else {
                    attr[cur] = EGL_ALPHA_SIZE;
                    attr[cur++] = 0;
                }
                attr[cur++] = EGL_NONE; // just in case
                egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
            }
        }
        if ((*count == 0) && (!globals4es.usepbuffer) && ca && attr[ca + 1]) {
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config with an Alpha channel, trying without\n");)
            attr[ca] = 0;
            egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
        }
        if ((*count == 0) && (!globals4es.usepbuffer) && ca && attr[ca + 1] &&
            (attr[cr] > 5 || attr[cg] > 5 || attr[cb] > 5)) {
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config without an Alpha channel, but 8bits rgb, trying lowering "
                          "bitness\n");)
            attr[cr] = attr[cg] = attr[cb] = 5;
            egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
        }
#ifdef PANDORA
        if ((*count == 0) && (!globals4es.usepbuffer) && (attr[cr] > 5 || attr[cg] > 5 || attr[cb] > 5)) {
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config with 8bits rgb, trying lowering bitness\n");)
            attr[cr] = attr[cg] = attr[cb] = 5;
            egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
        }
#endif
        /*if((*count==0) && (vt) && (attr[vt]!=-1)) {
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config with VisualType, trying without\n");)
            attr[vt] = -1;  //EGL_DONT_CARE
            egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
        }*/
        if (*count == 0) { // NO Config found....
            DBG(SHUT_LOGD("glXChooseFBConfig found 0 config\n");)
            return NULL;
        }
    } else {
        *count = 1;
    }
    EGLConfig* eglConfigs = (EGLConfig*)calloc((*count), sizeof(EGLConfig));
    if (glxconfig == -1) {
        DBG(SHUT_LOGD("glxconfig == -1");)
        egl_eglChooseConfig(eglDisplay, attr, eglConfigs, *count, count);
    } else {
        // Instead of using the eglConfigs array, look for the config with the given ID in the allFBConfig array
        init_eglconfig(display); // builds allFBConfig if not already built
        int found = -1;
        for (int i = 0; i < maxEGLConfig * 2; i++) {
            // Compare the stored ID with the requested config ID
            if (allFBConfig[i].id == (uint32_t)glxconfig) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            found = 0;
            // fallback to 0
            //*count = 0;
            // free(eglConfigs);
            // DBG(SHUT_LOGD("found == -1");)
            // return NULL;
        }
        // Return the found config
        *count = 1;
        GLXFBConfig* configs = (GLXFBConfig*)malloc(sizeof(GLXFBConfig));
        configs[0] = &allFBConfig[found];
        free(eglConfigs);
        DBG(SHUT_LOGD("glXChooseFBConfig found 1 config by ID\n");)
        DBG(SHUT_LOGD(" config[0] = %p\n", configs[0]);)
        return configs;
    }

    // and now, build a config list!
    GLXFBConfig* configs = fillGLXFBConfig(eglConfigs, *count, doublebuffer, display);
    if (doublebuffer == 2) *count *= 2;
    free(eglConfigs);
    DBG(SHUT_LOGD("glXChooseFBConfig found %d config\n", *count);
        for (int i = 0; i < *count; ++i) SHUT_LOGD(" config[%d] = %p\n", i, configs[i]);)

    return configs;
#endif
}

GLXFBConfig* gl4es_glXGetFBConfigs(Display* display, int screen, int* count) {
    DBG(SHUT_LOGD("glXGetFBConfigs(%p, %d, %p)\n", display, screen, count);)
#ifdef NO_EGL
    // this is wrong! The config table should be a static one built according to EGL Config capabilities...
    *count = 1;
    // this is to only do 1 malloc instead of 1 for the array and one for the element...
    GLXFBConfig* configs = (GLXFBConfig*)malloc(sizeof(GLXFBConfig) + sizeof(struct __GLXFBConfigRec));
    configs[0] = (GLXFBConfig)((char*)(&configs[0]) + sizeof(GLXFBConfig));
    memset(configs[0], 0, sizeof(struct __GLXFBConfigRec));
    configs[0]->drawableType = GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
    configs[0]->redBits = configs[0]->greenBits = configs[0]->blueBits = configs[0]->alphaBits = 8;
    configs[0]->depthBits = 24;
    configs[0]->stencilBits = 8;
    configs[0]->multiSampleSize = 0;
    configs[0]->nMultiSampleBuffers = 0;
#else
    int attr[50];
    int cur = 0;
    int tmp;
    attr[cur++] = EGL_SURFACE_TYPE;
    attr[cur++] = (globals4es.usepbuffer) ? (EGL_PBUFFER_BIT | EGL_PIXMAP_BIT) : EGL_WINDOW_BIT;
    attr[cur++] = EGL_RENDERABLE_TYPE;
    attr[cur++] = (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT;
    attr[cur++] = EGL_NONE; // end list

    // get the number of EGL config matching!
    if (eglInitialized == false) {
        if (!InitEGL((globals4es.usefb || globals4es.usepbuffer) ? g_display : display)) {
            CheckEGLErrors();
            LOGE("Unable to initialize EGL.\n");
            return NULL;
            *count = 0;
        }
    }
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglGetConfigAttrib);
    egl_eglChooseConfig(eglDisplay, attr, NULL, 0, count);
    if (*count == 0) // NO Config found....
        return NULL;
    EGLConfig* eglConfigs = (EGLConfig*)calloc((*count), sizeof(EGLConfig));
    egl_eglChooseConfig(eglDisplay, attr, eglConfigs, *count, count);
    // and now, build a config list!
    GLXFBConfig* configs = fillGLXFBConfig(eglConfigs, *count, 2, display);
    *count *= 2;
    free(eglConfigs);
    DBG(SHUT_LOGD("glXGetFBConfig found %d config\n", *count);)
#endif
    return configs;
}

int gl4es_glXGetFBConfigAttrib(Display* display, GLXFBConfig config, int attribute, int* value) {
    DBG(SHUT_LOGD("glXGetFBConfigAttrib(%p, %p, 0x%04X, %p)", display, config, attribute, value);)
    if (!config) {
        return get_config_default(display, attribute, value);
    }

    switch (attribute) {
    case GLX_RGBA:
        *value = config->alphaBits > 0 ? 1 : 0;
    case GLX_RED_SIZE:
        *value = config->redBits;
        break;
    case GLX_GREEN_SIZE:
        *value = config->greenBits;
        break;
    case GLX_BLUE_SIZE:
        *value = config->blueBits;
        break;
    case GLX_ALPHA_SIZE:
        *value = config->alphaBits;
        break;
    case GLX_DEPTH_SIZE:
        *value = config->depthBits;
        break;
    case GLX_STENCIL_SIZE:
        *value = config->stencilBits;
        break;
    case GLX_ACCUM_RED_SIZE:
    case GLX_ACCUM_GREEN_SIZE:
    case GLX_ACCUM_BLUE_SIZE:
    case GLX_ACCUM_ALPHA_SIZE:
        *value = 0;
        break;
    case GLX_TRANSPARENT_TYPE:
        *value = GLX_NONE;
        break;
    case GLX_RENDER_TYPE:
        *value = GLX_RGBA_BIT;
        break;
    case GLX_VISUAL_ID:
        *value = config->associatedVisualId;
        break;
    case GLX_FBCONFIG_ID:
        *value = (int)(uintptr_t)config->id;
        break;
    case GLX_DRAWABLE_TYPE:
        *value = config->drawableType; // GLX_WINDOW_BIT
        break;
    case GLX_X_VISUAL_TYPE:
    case GLX_CONFIG_CAVEAT:
        *value = 0;
        break;
    case GLX_SAMPLE_BUFFERS:
        *value = config->nMultiSampleBuffers;
        break;
    case GLX_SAMPLES:
        *value = config->multiSampleSize;
        break;
    case GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB:
        *value = hardext.srgb;
        break;
    case GLX_DOUBLEBUFFER:
        *value = config->doubleBufferMode;
        break;
    default:
        return get_config_default(display, attribute, value);
    }
    DBG(SHUT_LOGD(" => 0x%04X\n", *value);)
    return Success;
}

XVisualInfo* gl4es_glXGetVisualFromFBConfig(Display* display, GLXFBConfig config) {
    DBG(SHUT_LOGD("glXGetVisualFromFBConfig(%p, %p)\n", display, config);)
    /*if (g_display == NULL) {
        g_display = XOpenDisplay(NULL);
    }*/
    if (glx_default_depth == 0) glx_default_depth = XDefaultDepth(display, 0);

    int depth = glx_default_depth;
    /*if(config) {  // seems to upset Colin McRea Rally (Unity game)
        int d = config->redBits + config->greenBits + config->blueBits + config->alphaBits;
        if(d==16 || d==24 || d==32)
            depth = d;
    }*/
    XVisualInfo xvinfo = {0};
    xvinfo.depth = depth;
    xvinfo.class = TrueColor;
    // TODO: add more filter in here?

    int n;
    XVisualInfo* visuals = XGetVisualInfo(display, VisualDepthMask | VisualClassMask, &xvinfo, &n);
    if (!n) {
        LOGD("Warning, gl4es_glXGetVisualFromFBConfig: XGetVisualInfo gives 0 VisualInfo for %d depth and TrueColor "
             "class\n",
             glx_default_depth);
        return NULL;
    }
    return visuals;
}

GLXContext gl4es_glXCreateNewContext(Display* display, GLXFBConfig config, int render_type, GLXContext share_list,
                                     Bool is_direct) {
    // DBG(SHUT_LOGD("glXCreateNewContext(%p, %p, %d, %p, %i), drawableType=0x%02X\n", display, config, render_type,
    // share_list, is_direct, (config)?config->drawableType:0);)
    if (render_type != GLX_RGBA_TYPE) return 0;
    if (config && (config->drawableType == GLX_PBUFFER_BIT)) {
        return createPBufferContext(display, share_list, config);
    } else
        return gl4es_glXCreateContextAttribsARB(display, config, share_list, is_direct, NULL);
    // return glXCreateContext(display, 0, share_list, is_direct);
}
#endif // NOX11

void gl4es_glXSwapInterval(int interval) {
    DBG(SHUT_LOGD("glXSwapInterval(%i)\n", interval);)
#ifdef NOEGL
    // nothing
#elif defined(NOX11)
    LOAD_EGL(eglSwapInterval);
    egl_eglSwapInterval(eglDisplay, swapinterval);
#elif defined(USE_FBIO)
    if (!globals4es.vsync) {
        static int warned = 0;
        if (!warned) {
            LOGD("Enable LIBGL_VSYNC=1 if you want to use vsync.\n");
            warned++;
        }
    }
    swapinterval = interval;
#else
    if (glxContext) {
        if (globals4es.usepbuffer) {
            DBG(SHUT_LOGD("Ignoring glXSwapInterval(%d) on PBuffer surface\n", interval);)
            swapinterval = 0;
        } else {
            LOAD_EGL(eglSwapInterval);
            egl_eglSwapInterval(eglDisplay, swapinterval);
            CheckEGLErrors();
            if (interval < minswap || interval > maxswap) {
                SHUT_LOGE("Warning, Swap Interval %d is out of possible values %d, %d\n", interval, minswap, maxswap);
            } else
                swapinterval = interval;
        }
    } else {
        DBG(SHUT_LOGD("LIBGL: glXSwapInterval called before Context is current.\n");)
        swapinterval = interval;
    }
#endif
}

#ifndef NOX11
void gl4es_glXSwapIntervalEXT(Display* display, int drawable, int interval) {
    gl4es_glXSwapInterval(interval);
}

// misc stubs
void gl4es_glXCopyContext(Display* display, GLXContext src, GLXContext dst, GLuint mask) {
    DBG(SHUT_LOGD("glXCopyContext(%p, %p, %p, %04X)\n", display, src, dst, mask);)
    // mask is ignored for now, but should include glPushAttrib / glPopAttrib
    memcpy(dst, src, sizeof(struct __GLXContextRec));
}

Window gl4es_glXCreateWindow(Display* display, GLXFBConfig config, Window win, int* attrib_list) {
    // should return GLXWindow
    DBG(SHUT_LOGD("glXCreateWindow(%p, %p, %p, %p)\n", display, config, (void*)win, attrib_list);)
    return win;
}
void gl4es_glXDestroyWindow(Display* display, void* win) {
    // really wants a GLXWindow
    DBG(SHUT_LOGD("glXDestroyWindow(%p, %p)\n", display, win);)
}

GLXDrawable gl4es_glXGetCurrentDrawable() {
    DBG(SHUT_LOGD("glXGetCurrentDrawable()\n");)
    if (glxContext)
        return glxContext->drawable;
    else
        return 0;
} // this should actually return GLXDrawable.

Bool gl4es_glXIsDirect(Display* display, GLXContext ctx) {
    DBG(SHUT_LOGD("glXIsDirect(%p, %p)\n", display, ctx);)
    return true;
}

void gl4es_glXUseXFont(Font font, int first, int count, int listBase) {
    DBG(SHUT_LOGD("glXUseXFont(%p, %d, %d, %d)\n", (void*)font, first, count, listBase);)
    /* Mostly from MesaGL-9.0.1
     *
     */
    // First get current Display and Window
    XFontStruct* fs;
    unsigned int max_width, max_height, max_bm_width, max_bm_height;
    Pixmap pixmap;
    XGCValues values;
    GC gc;
    int i;
    unsigned long valuemask;
    GLubyte* bm;
    Display* dpy;
    Window win;
    if (0 /*globals4es.usefb*/) {
        dpy = g_display;
        win = RootWindow(dpy, XDefaultScreen(dpy));
    } else {
        dpy = glxContext->display;
        win = glxContext->drawable; // TODO, check that drawable is a window and not a pixmap ?
    }

    // Grab font params
    fs = XQueryFont(dpy, font);
    if (!fs) {
        LOGE("error, no font set before call to glXUseFont\n");
        return;
    }
    max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
    max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
    max_bm_width = (max_width + 7) / 8;
    max_bm_height = max_height;

    bm = (GLubyte*)malloc((max_bm_width * max_bm_height) * sizeof(GLubyte));
    if (!bm) {
        XFreeFontInfo(NULL, fs, 1);
        return;
    }
    // Save GL texture parameters
    GLint swapbytes, lsbfirst, rowlength;
    GLint skiprows, skippixels, alignment;
    gl4es_glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
    gl4es_glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
    gl4es_glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
    gl4es_glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
    gl4es_glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
    gl4es_glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    // Set Safe Texture params
    gl4es_glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    gl4es_glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    gl4es_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl4es_glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    gl4es_glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    gl4es_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Create GC and Pixmap
    pixmap = XCreatePixmap(dpy, win, 10, 10, 1);
    values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
    values.background = WhitePixel(dpy, DefaultScreen(dpy));
    values.font = fs->fid;
    valuemask = GCForeground | GCBackground | GCFont;
    gc = XCreateGC(dpy, pixmap, valuemask, &values);
    XFreePixmap(dpy, pixmap);
    // Loop each chars
    for (i = 0; i < count; i++) {
        unsigned int width, height, bm_width, bm_height;
        GLfloat x0, y0, dx, dy;
        XCharStruct* ch;
        int x, y;
        unsigned int c = first + i;
        int list = listBase + i;
        int valid;

        /* check on index validity and get the bounds */
        ch = isvalid(fs, c);
        if (!ch) {
            ch = &fs->max_bounds;
            valid = 0;
        } else {
            valid = 1;
        }
        /* glBitmap()' parameters:
            straight from the glXUseXFont(3) manpage.  */
        width = ch->rbearing - ch->lbearing;
        height = ch->ascent + ch->descent;
        x0 = -ch->lbearing;
        y0 = ch->descent - 1;
        dx = ch->width;
        dy = 0;
        /* X11's starting point.  */
        x = -ch->lbearing;
        y = ch->ascent;
        /* Round the width to a multiple of eight.  We will use this also
          for the pixmap for capturing the X11 font.  This is slightly
          inefficient, but it makes the OpenGL part real easy.  */
        bm_width = (width + 7) / 8;
        bm_height = height;
        gl4es_glNewList(list, GL_COMPILE);
        if (valid && (bm_width > 0) && (bm_height > 0)) {

            memset(bm, '\0', bm_width * bm_height);
            fill_bitmap(dpy, win, gc, bm_width, bm_height, x, y, c, bm);

            gl4es_glBitmap(width, height, x0, y0, dx, dy, bm);
        } else {
            gl4es_glBitmap(0, 0, 0.0, 0.0, dx, dy, NULL);
        }
        gl4es_glEndList();
    }

    // Free GC & Pixmap
    free(bm);
    XFreeFontInfo(NULL, fs, 1);
    XFreeGC(dpy, gc);

    // Restore saved packing modes.
    gl4es_glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
    gl4es_glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
    gl4es_glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
    gl4es_glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
    gl4es_glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
    gl4es_glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    // All done
}
#endif // NOX11
void gl4es_glXWaitGL() {}
void gl4es_glXWaitX() {}
void gl4es_glXReleaseBuffersMESA() {}

#ifndef NOX11
/* TODO proper implementation */
int gl4es_glXQueryDrawable(Display* dpy, GLXDrawable draw, int attribute, unsigned int* value) {
    DBG(SHUT_LOGD("glXQueryDrawable(%p, %p", dpy, (void*)draw);)
    int pbuf = isPBuffer(draw);
    if (pbuf) {
        if (pbuffersize[pbuf - 1].Type != 1) pbuf = 0; // the drawable can be used
    }
    *value = 0;
    int width = 800;
    int height = 480;
    if ((attribute == GLX_WIDTH || attribute == GLX_HEIGHT)) {
        // Get Window size and all...
        EGLSurface surf = EGL_NO_SURFACE;
        if (pbuf) { // PBuffer are not linked to the drawable...
            surf = pbuffersize[pbuf - 1].Surface;
        }
#ifdef NO_EGL
        else {
            SharedEGLSurface_t* sharedsurf = RecycleGetSurface(draw);
            if (sharedsurf) surf = sharedsurf->surf;
        }
#endif
        if (surf != EGL_NO_SURFACE) {
            LOAD_EGL(eglQuerySurface);
            EGLint tmp;
            if (attribute == GLX_WIDTH)
                if (egl_eglQuerySurface(eglDisplay, surf, EGL_WIDTH, &tmp) == EGL_TRUE) width = tmp;
            if (attribute == GLX_HEIGHT)
                if (egl_eglQuerySurface(eglDisplay, surf, EGL_HEIGHT, &tmp) == EGL_TRUE) height = tmp;
        }
#ifndef NO_EGL
        else {
            unsigned int border, depth;
            Window root;
            int x, y;
            XGetGeometry(dpy, draw, &root, &x, &y, &width, &height, &border, &depth);
        }
#endif
    }
    switch (attribute) {
    case GLX_WIDTH:
        *value = width;
        DBG(SHUT_LOGD("(%d), GLX_WIDTH, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_HEIGHT:
        *value = height;
        DBG(SHUT_LOGD("(%d), GLX_HEIGHT, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_PRESERVED_CONTENTS:
        if (pbuf) *value = 1;
        DBG(SHUT_LOGD("(%d), GLX_PRESERVED_CONTENTS, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_LARGEST_PBUFFER:
        if (pbuf) *value = 0;
        DBG(SHUT_LOGD("(%d), GLX_LARGEST_PBUFFER, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_FBCONFIG_ID:
        *value = 0;
        DBG(SHUT_LOGD("(%d), GLX_FBCONFIG_ID, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_SWAP_INTERVAL_EXT:
        *value = swapinterval;
        DBG(SHUT_LOGD("(%d), GLX_SWAP_INTERVAL_EXT, %p = %d)\n", pbuf, value, *value);)
        return 1;
    case GLX_MAX_SWAP_INTERVAL_EXT:
        *value = maxswap; // fake, should eglQuery the Config for EGL_MAX_SWAP_INTERVAL (and EGL_MIN_SWAP_INTERVAL)
        DBG(SHUT_LOGD("(%d), GLX_MAX_SWAP_INTERVAL_EXT, %p = %d)\n", pbuf, value, *value);)
        return 1;
    }
    DBG(SHUT_LOGD("(%d), %04x, %p)\n", pbuf, attribute, value);)
    return 0;
}

GLXPbuffer addPBuffer(EGLSurface surface, int Width, int Height, EGLContext Context, EGLConfig Config) {
    if (pbufferlist_cap <= pbufferlist_size) {
        pbufferlist_cap += 4;
        pbufferlist = (GLXPbuffer*)realloc(pbufferlist, sizeof(GLXPbuffer) * pbufferlist_cap);
        pbuffersize = (glx_buffSize*)realloc(pbuffersize, sizeof(glx_buffSize) * pbufferlist_cap);
    }
    pbufferlist[pbufferlist_size] = (GLXPbuffer)surface;
    pbuffersize[pbufferlist_size].Width = Width;
    pbuffersize[pbufferlist_size].Height = Height;
    pbuffersize[pbufferlist_size].Context = Context;
    pbuffersize[pbufferlist_size].Surface = surface;
    pbuffersize[pbufferlist_size].Config = Config;
    pbuffersize[pbufferlist_size].gc = NULL;
    pbuffersize[pbufferlist_size].Type = 1; // 1 = pbuffer
    return pbufferlist[pbufferlist_size++];
}
static void delPBufferContext(int j) {
    LOAD_EGL(eglDestroyContext);
    egl_eglDestroyContext(eglDisplay, pbuffersize[j].Context);
    CheckEGLErrors();
    // should pack, but I think it's useless for common use
}

void gl4es_glXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf) {
    DBG(SHUT_LOGD("glxDestroyPBuffer(%p, %p)\n", dpy, (void*)pbuf);)
    LOAD_EGL(eglDestroySurface);
    int j = 0;
    while (j < pbufferlist_size && pbufferlist[j] != pbuf)
        j++;
    if (j == pbufferlist_size) {
        DBG(SHUT_LOGD("PBuff not found in pbufferlist\n");)
        return;
    }
    // delete de Surface
    EGLSurface surface = (EGLSurface)pbufferlist[j];
    egl_eglDestroySurface(eglDisplay, surface);
    CheckEGLErrors();

    delPBufferContext(j);
    delPBuffer(j);
}

int createPBuffer(Display* dpy, const EGLint* egl_attribs, EGLSurface* Surface, EGLContext* Context, EGLConfig* Config,
                  int redBits, int greenBits, int blueBits, int alphaBits, int samplebuffers, int samples) {
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglCreatePbufferSurface);
    LOAD_EGL(eglCreateContext);

    EGLint configAttribs[] = {EGL_RED_SIZE,
                              redBits,
                              EGL_GREEN_SIZE,
                              greenBits,
                              EGL_BLUE_SIZE,
                              blueBits,
                              EGL_ALPHA_SIZE,
                              (hardext.eglnoalpha) ? 0 : alphaBits,
                              EGL_DEPTH_SIZE,
                              1,
                              EGL_STENCIL_SIZE,
                              1,
                              EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
                              EGL_SAMPLE_BUFFERS,
                              samplebuffers,
                              EGL_SAMPLES,
                              samples,
                              EGL_NONE};

    // Init what need to be done
    if (!InitEGL(dpy)) return 0;

    // select a configuration
    EGLBoolean result;
    int configsFound;

    result = egl_eglChooseConfig(eglDisplay, configAttribs, Config, 1, &configsFound);

    CheckEGLErrors();
    if (result != EGL_TRUE || configsFound == 0) {
        LOGD("No EGL configs found.\n");
        return 0;
    }

    // now, create the PBufferSurface
    (*Surface) = egl_eglCreatePbufferSurface(eglDisplay, Config[0], egl_attribs);

    if ((*Surface) == EGL_NO_SURFACE) {
        CheckEGLErrors();
        LOGD("Error creating PBuffer\n");
        return 0;
    }
    (*Context) = egl_eglCreateContext(eglDisplay, Config[0], EGL_NO_CONTEXT,
                                      (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);
    CheckEGLErrors();

    return 1;
}

GLXPbuffer gl4es_glXCreatePbuffer(Display* dpy, GLXFBConfig config, const int* attrib_list) {
    DBG(SHUT_LOGD("glXCreatePbuffer(%p, %p, %p)\n", dpy, config, attrib_list);)
    LOAD_EGL(eglQuerySurface);

    EGLSurface Surface = 0;
    EGLContext Context = 0;
    EGLConfig Config[1];
    // let's create a PBuffer attributes
    EGLint egl_attribs[128]; // should be enough
    int i = 0;
    if (attrib_list) {
        int j = 0;
        while (attrib_list[j] != 0) {
            switch (attrib_list[j++]) {
            case GLX_PBUFFER_WIDTH:
                egl_attribs[i++] = EGL_WIDTH;
                egl_attribs[i++] = attrib_list[j++];
                break;
            case GLX_PBUFFER_HEIGHT:
                egl_attribs[i++] = EGL_HEIGHT;
                egl_attribs[i++] = attrib_list[j++];
                break;
            case GLX_LARGEST_PBUFFER:
                egl_attribs[i++] = EGL_LARGEST_PBUFFER;
                egl_attribs[i++] = (attrib_list[j++]) ? EGL_TRUE : EGL_FALSE;
                break;
            case GLX_PRESERVED_CONTENTS:
                j++;
                // ignore this one
                break;
                // nothing, ignore unknown attribs
            }
        }
    }
    egl_attribs[i++] = EGL_NONE;

    // Check that the config is for PBuffer
    if (!(config->drawableType & GLX_PBUFFER_BIT)) return 0;

    if (createPBuffer(dpy, egl_attribs, &Surface, &Context, Config, config->redBits, config->greenBits,
                      config->blueBits, config->alphaBits, config->nMultiSampleBuffers, config->multiSampleSize) == 0) {
        return 0;
    }

    int Width, Height;

    egl_eglQuerySurface(eglDisplay, Surface, EGL_WIDTH, &Width);
    egl_eglQuerySurface(eglDisplay, Surface, EGL_HEIGHT, &Height);

    return addPBuffer(Surface, Width, Height, Context, Config[0]);
}

GLXPbuffer addPixBuffer(Display* dpy, EGLSurface surface, EGLConfig Config, int Width, int Height, EGLContext Context,
                        Pixmap pixmap, int depth, int emulated) {
    if (pbufferlist_cap <= pbufferlist_size) {
        pbufferlist_cap += 4;
        pbufferlist = (GLXPbuffer*)realloc(pbufferlist, sizeof(GLXPbuffer) * pbufferlist_cap);
        pbuffersize = (glx_buffSize*)realloc(pbuffersize, sizeof(glx_buffSize) * pbufferlist_cap);
    }
    pbufferlist[pbufferlist_size] = (GLXPbuffer)pixmap;
    pbuffersize[pbufferlist_size].Width = Width;
    pbuffersize[pbufferlist_size].Height = Height;
    pbuffersize[pbufferlist_size].Context = Context;
    pbuffersize[pbufferlist_size].Surface = surface;
    pbuffersize[pbufferlist_size].Config = Config;
    pbuffersize[pbufferlist_size].Depth = depth;
    pbuffersize[pbufferlist_size].dpy = dpy;
    pbuffersize[pbufferlist_size].gc = (emulated) ? XCreateGC(dpy, pixmap, 0, NULL) : NULL;
    pbuffersize[pbufferlist_size].frame = NULL;

    pbuffersize[pbufferlist_size].Type = 2 + emulated; // 2 = pixmap, 3 = emulated pixmap, 4 = emulated win
    return pbufferlist[pbufferlist_size++];
}
void delPixBuffer(int j) {
    LOAD_EGL(eglDestroyContext);
    if (pbuffersize[j].gc) XFree(pbuffersize[j].gc);
    if (pbuffersize[j].frame) {
        XDestroyImage(pbuffersize[j].frame);
    }
    pbufferlist[j] = 0;
    pbuffersize[j].Width = 0;
    pbuffersize[j].Height = 0;
    pbuffersize[j].Depth = 0;
    pbuffersize[j].dpy = 0;
    pbuffersize[j].gc = 0;
    pbuffersize[j].Surface = 0;
    egl_eglDestroyContext(eglDisplay, pbuffersize[j].Context);
    CheckEGLErrors();
    // should pack, but I think it's useless for common use
}

int createPixBuffer(Display* dpy, int bpp, const EGLint* egl_attribs, NativePixmapType nativepixmap,
                    EGLSurface* Surface, EGLContext* Context) {
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglCreatePixmapSurface);
    LOAD_EGL(eglCreateContext);

    EGLint configAttribs[] = {EGL_RED_SIZE,
                              (bpp > 16) ? 8 : 5,
                              EGL_GREEN_SIZE,
                              (bpp == 15)  ? 5
                              : (bpp > 16) ? 8
                                           : 6,
                              EGL_BLUE_SIZE,
                              (bpp > 16) ? 8 : 5,
                              EGL_ALPHA_SIZE,
                              (bpp == 32) ? 8 : 0,
                              EGL_DEPTH_SIZE,
                              1, // some depth
                              EGL_STENCIL_SIZE,
                              1, // some stencil too
                              EGL_SURFACE_TYPE,
                              EGL_PIXMAP_BIT,
                              EGL_RENDERABLE_TYPE,
                              (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES3_BIT,
                              EGL_NONE};

    // Init what need to be done
    EGLBoolean result;
    if (eglDisplay == NULL || eglDisplay == EGL_NO_DISPLAY) {
        init_display((globals4es.usefb || globals4es.usepbuffer) ? g_display : dpy);
        if (eglDisplay == EGL_NO_DISPLAY) {
            CheckEGLErrors();
            LOGE("Unable to create EGL display.\n");
            return 0;
        }
    }

    // first time?
    if (eglInitialized == false) {
        result = InitEGL((globals4es.usefb || globals4es.usepbuffer) ? g_display : dpy);
        if (!result) {
            CheckEGLErrors();
            LOGE("Unable to initialize EGL display.\n");
            return 0;
        }
    }

    // select a configuration
    int configsFound;
    static EGLConfig pixbufConfigs[1];
    result = egl_eglChooseConfig(eglDisplay, configAttribs, pixbufConfigs, 1, &configsFound);

    CheckEGLErrors();
    if (result != EGL_TRUE || configsFound == 0) {
        LOGE("No EGL configs found.\n");
        return 0;
    }

    // now, create the PixmapSurface
    (*Surface) = egl_eglCreatePixmapSurface(eglDisplay, pixbufConfigs[0], nativepixmap, egl_attribs);

    if ((*Surface) == EGL_NO_SURFACE) {
        CheckEGLErrors();
        LOGE("Error creating PixmapSurface\n");
        return 0;
    }

    (*Context) = egl_eglCreateContext(eglDisplay, pixbufConfigs[0], EGL_NO_CONTEXT,
                                      (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);
    CheckEGLErrors();

    return 1;
}

GLXPixmap gl4es_glXCreateGLXPixmap(Display* display, XVisualInfo* visual, Pixmap pixmap) {
    DBG(SHUT_LOGD("glXCreateGLXPixmap(%p, %p, %p)\n", display, visual, (void*)pixmap);)
    LOAD_EGL(eglQuerySurface);

    EGLSurface Surface = 0;
    EGLContext Context = 0;
    EGLConfig Config[1] = {0};
    // first, analyse PixMap to get it's dimensions and color depth...
    unsigned int width, height, border, depth;
    Window root;
    int x, y;
    int emulated = 0;
    XGetGeometry(display, pixmap, &root, &x, &y, &width, &height, &border, &depth);
    // let's try to create a PixmapSurface directly
    if (globals4es.usefb || createPixBuffer(display, depth, NULL, (NativePixmapType)pixmap, &Surface, &Context) == 0) {
        // fail, so emulate with a PBuffer
        SHUT_LOGE("Pixmap creation failed, trying PBuffer instead\n");
        // let's create a PixBuffer attributes
        EGLint egl_attribs[10]; // should be enough
        int i = 0;
        egl_attribs[i++] = EGL_WIDTH;
        egl_attribs[i++] = width;
        egl_attribs[i++] = EGL_HEIGHT;
        egl_attribs[i++] = height;
        egl_attribs[i++] = EGL_NONE;

        if (createPBuffer(display, egl_attribs, &Surface, &Context, Config, (depth > 16) ? 8 : 5,
                          (depth == 15)  ? 5
                          : (depth > 16) ? 8
                                         : 6,
                          (depth > 16) ? 8 : 5, (depth == 32) ? 8 : 0, 0, 0) == 0) {
            // fail too, abort
            SHUT_LOGE("PBuffer creation failed too\n");
            return 0;
        }
        emulated = 1;
    }
    int Width, Height;

    egl_eglQuerySurface(eglDisplay, Surface, EGL_WIDTH, &Width);
    egl_eglQuerySurface(eglDisplay, Surface, EGL_HEIGHT, &Height);

    return addPixBuffer(display, Surface, Config[0], Width, Height, Context, pixmap, depth, emulated);
}

GLXPixmap gl4es_glXCreatePixmap(Display* dpy, GLXFBConfig config, Pixmap pixmap, const int* attrib_list) {
    DBG(SHUT_LOGD("glXCreatePixmap(%p, %p, %p, %p)\n", dpy, config, (void*)pixmap, attrib_list);)
    // Check that the config is for PBuffer
    if (!(config->drawableType & GLX_PIXMAP_BIT)) return 0;

    return gl4es_glXCreateGLXPixmap(dpy, NULL, pixmap);
}

void gl4es_glXDestroyGLXPixmap(Display* display, void* pixmap) {
    DBG(SHUT_LOGD("glXDestroyGLXPixmap(%p, %p)\n", display, pixmap);)
    LOAD_EGL(eglDestroySurface);
    int j = 0;
    while (j < pbufferlist_size && pbufferlist[j] != (GLXPbuffer)pixmap)
        j++;
    if (j == pbufferlist_size) return;
    // delete de Surface
    EGLSurface surface = pbuffersize[j].Surface; // (EGLSurface)pbufferlist[j];
    egl_eglDestroySurface(eglDisplay, surface);
    CheckEGLErrors();

    delPixBuffer(j);
}

void gl4es_glXDestroyPixmap(Display* display, void* pixmap) {
    DBG(SHUT_LOGD("glXDestroyPixmap(%p, %p)\n", display, pixmap);)
    gl4es_glXDestroyGLXPixmap(display, pixmap);
}

void actualBlit(int reverse, int Width, int Height, int Depth, Display* dpy, Pixmap drawable, GC gc, XImage* frame,
                uintptr_t pix, void* tmp) {
    const int sbuf = Width * Height * (Depth == 16 ? 2 : 4);
#ifdef PANDORA
    if (tmp) {
        if (reverse) {
            int stride = Width * 2;
            uintptr_t src_pos = (uintptr_t)tmp;
            uintptr_t dst_pos = (uintptr_t)pix + sbuf - stride;
            for (int i = 0; i < Height; i++) {
                for (int j = 0; j < Width; j++) {
                    *(GLushort*)dst_pos = ((GLushort)(((char*)src_pos)[0] & 0xf8) >> (3)) |
                                          ((GLushort)(((char*)src_pos)[1] & 0xfc) << (5 - 2)) |
                                          ((GLushort)(((char*)src_pos)[2] & 0xf8) << (11 - 3));
                    src_pos += 4;
                    dst_pos += 2;
                }
                dst_pos -= 2 * stride;
            }
        } else
            pixel_convert(tmp, (void**)&pix, Width, Height, GL_BGRA, GL_UNSIGNED_BYTE, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                          0, glstate->texture.unpack_align);
    } else
#endif
        if (reverse) {
        int stride = Width * (Depth == 16 ? 2 : 4);
        uintptr_t end = (uintptr_t)pix + sbuf - stride;
        uintptr_t beg = (uintptr_t)pix;
        void* const tmp = (void*)(pix + sbuf);
        for (; beg < end; beg += stride, end -= stride) {
            memcpy(tmp, (void*)end, stride);
            memcpy((void*)end, (void*)beg, stride);
            memcpy((void*)beg, tmp, stride);
        }
    }

    // blit
    XPutImage(dpy, drawable, gc, frame, 0, 0, 0, 0, Width, Height);
}

void BlitEmulatedPixmap(int win) {
    if (!glstate->emulatedPixmap) return;

    Pixmap drawable = (Pixmap)pbufferlist[glstate->emulatedPixmap - 1];

    glx_buffSize* buff = &pbuffersize[glstate->emulatedPixmap - 1];

    int Width = buff->Width;
    int Height = buff->Height;
    int Depth = buff->Depth;
    Display* dpy = buff->dpy;
    GC gc = buff->gc;
    // the reverse stuff can probably be better!
    int reverse = buff->Type == 4 ? 1 : 0;
    const int sbuf = Width * Height * (Depth == 16 ? 2 : 4);
    XImage* frame = buff->frame;

    // grab the size of the drawable if it has changed
    if (reverse) {
        drawable = (Pixmap)win;
        // Get Window size and all...
        unsigned int width, height, border, depth;
        Window root;
        int x, y;
        XGetGeometry(dpy, drawable, &root, &x, &y, &width, &height, &border, &depth);
        if (width != Width || height != Height /*|| depth!=Depth*/) {
            LOAD_EGL(eglDestroySurface);
            LOAD_EGL(eglMakeCurrent);
            LOAD_EGL(eglCreatePbufferSurface);
            // destroy old stuff
            XSync(dpy, False); // synch seems needed before the DestroyImage...
            if (frame) XDestroyImage(frame);
            buff->frame = 0;

            // let's create a PBuffer attributes
            EGLint egl_attribs[10]; // should be enough
            int i = 0;
            egl_attribs[i++] = EGL_WIDTH;
            egl_attribs[i++] = width;
            egl_attribs[i++] = EGL_HEIGHT;
            egl_attribs[i++] = height;
            egl_attribs[i++] = EGL_NONE;

            DBG(SHUT_LOGD("LIBGL: Recreate PBuffer %dx%dx%d => %dx%dx%d\n", Width, Height, Depth, width, height,
                          depth);)

#ifndef NO_GBM
            if (globals4es.usegbm)
                GBMMakeCurrent(eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
            else
#endif
                egl_eglMakeCurrent(eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
            egl_eglDestroySurface(eglDisplay, buff->Surface);
            CheckEGLErrors();

            EGLSurface Surface = NULL;
            Surface = egl_eglCreatePbufferSurface(eglDisplay, buff->Config, egl_attribs);
            if (Surface == EGL_NO_SURFACE)
                LOGE("Warning, Recration of pbuffer failed (from %dx%dx%d => %dx%dx%d)\n", buff->Width, buff->Height,
                     buff->Depth, width, height, depth);
            CheckEGLErrors();
#ifndef NO_GBM
            if (globals4es.usegbm)
                GBMMakeCurrent(eglDisplay, Surface, Surface, buff->Context);
            else
#endif
                egl_eglMakeCurrent(eglDisplay, Surface, Surface, buff->Context);
            CheckEGLErrors();
            glxContext->eglSurface = buff->Surface = Surface;
            buff->Width = width;
            buff->Height = height;
            // buff->Depth = depth;
            return;
        }
    }

    // create things if needed
    if (!buff->frame) {
        int sz = Width * (Height + reverse) * (Depth == 16 ? 2 : 4);
#ifdef PANDORA
        if (hardext.esversion == 1 && Depth == 16) {
            sz += Width * Height * 4;
        }
#endif
        frame = buff->frame = XCreateImage(dpy, NULL /*visual*/, Depth, ZPixmap, 0, malloc(sz), Width, Height,
                                           (Depth == 16) ? 16 : 32, 0);
    }

    if (!frame) {
        return;
    }
    static int direct_copy = 1;
    if (direct_copy) {
        LOAD_EGL(eglCopyBuffers);
        if (!egl_eglCopyBuffers(eglDisplay, buff->Surface, (EGLNativePixmapType)frame)) {
            LOGE("Cannot use eglCopyBuffers, disabling it's use: ");
            CheckEGLErrors();
            direct_copy = 0;
        } else
            return;
    }
    uintptr_t pix = (uintptr_t)frame->data;

    // grab framebuffer
    void* tmp = NULL;
#ifdef PANDORA
    LOAD_GLES(glReadPixels);
    if (hardext.esversion == 1) {
        if (Depth == 16) {
            tmp = (void*)(pix + Width * Height * 2);
            gles_glReadPixels(0, 0, Width, Height, GL_BGRA, GL_UNSIGNED_BYTE, tmp);
        } else {
            gles_glReadPixels(0, 0, Width, Height, GL_BGRA, GL_UNSIGNED_BYTE, (void*)pix);
        }
    } else if (Depth == 16)
        gles_glReadPixels(0, 0, Width, Height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (void*)pix);
    else
#endif
        gl4es_glReadPixels(0, 0, Width, Height, (Depth == 16) ? GL_RGB : GL_BGRA,
                           (Depth == 16) ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE, (void*)pix);

    actualBlit(reverse, Width, Height, Depth, dpy, drawable, gc, frame, pix, tmp);
}

GLXContext gl4es_glXCreateContextAttribs(Display* dpy, GLXFBConfig config, GLXContext share_context, Bool direct,
                                         const int* attrib_list) {
    DBG(SHUT_LOGD("glXCreateContextAttribs(%p, %p, %p, %d, %p)\n", dpy, config, share_context, direct, attrib_list);)
    int ask_es = 0;
    int ask_shaders = 0;
    int majver = 0, minver = 0;
    int flags = 0;
    int mask = 0;
    const int* attr = attrib_list;
    while (attr && (*attr) != 0) {
        int name = (*(attr++));
        int pair = (*(attr++));
        switch (name) {
        case GLX_CONTEXT_MAJOR_VERSION_ARB:
            majver = pair;
            break;
        case GLX_CONTEXT_MINOR_VERSION_ARB:
            minver = pair;
            break;
        case GLX_CONTEXT_FLAGS_ARB:
            flags = pair;
            break;
        case GLX_CONTEXT_PROFILE_MASK_ARB:
            mask = pair;
            break;
        default: {
            DBG(SHUT_LOGD(" unknown Attrib %04X (value=%d)\n", name, pair);)
        }
        }
    }
    if (majver * 10 + minver != 0) {
        DBG(SHUT_LOGD(" Required context version %d.%d\n", majver, minver);)
        if (majver * 10 + minver > 21) {
            LOGE("Asked for unsupported context version %d.%d\n", majver, minver);
            return 0;
        }
        if (majver * 10 + minver > globals4es.gl) {
            LOGE("Asked for unsupported context version %d.%d (max version is %d.%d)\n", majver, minver,
                 globals4es.gl / 10, globals4es.gl % 10);
            return 0;
        }
        if ((mask & GLX_CONTEXT_ES2_PROFILE_BIT_EXT) && hardext.esversion < 2) {
            LOGE("Asked for ES2 compatible context on GLES1.1 Backend\n");
            return 0;
        }
    }
    if (mask & GLX_CONTEXT_ES2_PROFILE_BIT_EXT) globales2 = 1;
    GLXContext context = gl4es_glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, share_context, direct);
    globales2 = 0;
    return context;
}
#ifndef NO_EGL
void refreshMainFBO() {
    if (!eglSurface || !eglDisplay) return;
    if (eglSurface != EGL_NO_SURFACE) {
        LOAD_EGL(eglQuerySurface);
        EGLint tmp;
        if (egl_eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &tmp) == EGL_TRUE) glstate->fbo.mainfbo_width = tmp;
        if (egl_eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &tmp) == EGL_TRUE)
            glstate->fbo.mainfbo_height = tmp;
    }
}
#endif // NO_EGL

#endif // NOX11

const char* gl4es_glXQueryExtensionsString(Display* display, int screen) {
    DBG(SHUT_LOGD("glXQueryExtensionString(%p, %d)\n", display, screen);)
    static const char* ext = NULL;
    if (!ext) {
        static char buffer[512];
        strcpy(buffer, "GLX_ARB_create_context "
                       "GLX_ARB_create_context_profile "
                       "GLX_ARB_get_proc_address "
                       "GLX_ARB_multisample "
                       "GLX_SGI_swap_control "
                       "GLX_MESA_swap_control "
                       "GLX_EXT_swap_control "
                       "GLX_SGIX_pbuffer "
                       "GLX_EXT_framebuffer_sRGB ");
        if (globals4es.es > 1 && !globals4es.noes2) strcat(buffer, "GLX_EXT_create_context_es2_profile ");
        ext = buffer;
    }
    DBG(SHUT_LOGD("glXQueryExtensionString: %s\n", ext);)
    return ext;
}

const char* gl4es_glXQueryServerString(Display* display, int screen, int name) {
    DBG(SHUT_LOGD("glXQueryServerString(%p, %d, %d)\n", display, screen, name);)
    switch (name) {
    case GLX_VENDOR:
        return gl4es_glGetString(GL_VENDOR);
    case GLX_VERSION:
        return gl4es_glGetString(GL_VERSION);
    case GLX_EXTENSIONS:
        return gl4es_glXQueryExtensionsString(display, screen);
    }
    return 0;
}

Bool gl4es_glXQueryExtension(Display* display, int* errorBase, int* eventBase) {
    DBG(SHUT_LOGD("glXQueryExtension(%p, %p, %p)\n", display, errorBase, eventBase);)
    if (errorBase) *errorBase = 0;

    if (eventBase) *eventBase = 0;

    return 1;
}

Bool gl4es_glXQueryVersion(Display* display, int* major, int* minor) {
    DBG(SHUT_LOGD("glXQueryVersion(%p, %p, %p)\n", display, major, minor);)
    // TODO: figure out which version we want to pretend to implement
    *major = 1;
    *minor = 4;
    return 1;
}

const char* gl4es_glXGetClientString(Display* display, int name) {
    DBG(SHUT_LOGD("glXGetClientString(%p, %d)\n", display, name);)
    switch (name) {
    case GLX_VENDOR:
        return "ptitSeb & BZLZHH"; // gl4es_glGetString(GL_VENDOR);
    case GLX_VERSION:
        return "3.0"; // gl4es_glGetString(GL_VERSION);
    case GLX_EXTENSIONS:
        return gl4es_glXQueryExtensionsString(display, 0);
    }
    return 0;
}

// New export the Alias
#ifndef NOX11
AliasExport(GLXContext, glXCreateContext, ,
            (Display * display, XVisualInfo* visual, GLXContext shareList, Bool isDirect));
AliasExport(GLXContext, glXCreateContextAttribs, ARB,
            (Display * display, GLXFBConfig config, GLXContext share_context, Bool direct, const int* attrib_list));
AliasExport(void, glXDestroyContext, , (Display * display, GLXContext ctx));
AliasExport(Display*, glXGetCurrentDisplay, , ());
AliasExport(XVisualInfo*, glXChooseVisual, , (Display * display, int screen, int* attributes));
AliasExport(Bool, glXMakeCurrent, , (Display * display, GLXDrawable drawable, GLXContext context));
AliasExport(Bool, glXMakeContextCurrent, , (Display * display, int drawable, int readable, GLXContext context));
AliasExport(void, glXSwapBuffers, , (Display * display, GLXDrawable drawable));
AliasExport(int, glXGetConfig, , (Display * display, XVisualInfo* visual, int attribute, int* value));
AliasExport(int, glXQueryContext, , (Display * dpy, GLXContext ctx, int attribute, int* value));
AliasExport(GLXContext, glXGetCurrentContext, , ());
AliasExport(GLXFBConfig*, glXChooseFBConfig, , (Display * display, int screen, const int* attrib_list, int* count));
AliasExport(GLXFBConfig*, glXChooseFBConfig, SGIX, (Display * display, int screen, const int* attrib_list, int* count));
AliasExport(GLXFBConfig*, glXGetFBConfigs, , (Display * display, int screen, int* count));
AliasExport(int, glXGetFBConfigAttrib, , (Display * display, GLXFBConfig config, int attribute, int* value));
AliasExport(XVisualInfo*, glXGetVisualFromFBConfig, , (Display * display, GLXFBConfig config));
AliasExport(GLXContext, glXCreateNewContext, ,
            (Display * display, GLXFBConfig config, int render_type, GLXContext share_list, Bool is_direct));
AliasExport(void, glXSwapIntervalEXT, , (Display * display, int drawable, int interval));
AliasExport(void, glXCopyContext, , (Display * display, GLXContext src, GLXContext dst, GLuint mask));
AliasExport(Window, glXCreateWindow, , (Display * display, GLXFBConfig config, Window win, int* attrib_list));
AliasExport(void, glXDestroyWindow, , (Display * display, void* win));
AliasExport(GLXDrawable, glXGetCurrentDrawable, , ());
AliasExport(Bool, glXIsDirect, , (Display * display, GLXContext ctx));
AliasExport(void, glXUseXFont, , (Font font, int first, int count, int listBase));
AliasExport(int, glXQueryDrawable, , (Display * dpy, GLXDrawable draw, int attribute, unsigned int* value));
AliasExport(void, glXDestroyPbuffer, , (Display * dpy, GLXPbuffer pbuf));
AliasExport(GLXPbuffer, glXCreatePbuffer, , (Display * dpy, GLXFBConfig config, const int* attrib_list));
AliasExport(GLXPixmap, glXCreateGLXPixmap, , (Display * display, XVisualInfo* visual, Pixmap pixmap));
AliasExport(GLXPixmap, glXCreatePixmap, , (Display * dpy, GLXFBConfig config, Pixmap pixmap, const int* attrib_list));
AliasExport(void, glXDestroyGLXPixmap, , (Display * display, void* pixmap));
AliasExport(void, glXDestroyPixmap, , (Display * display, void* pixmap));
AliasExport(GLXContext, glXCreateContextAttribs, ,
            (Display * dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int* attrib_list));
#endif

AliasExport(const char*, glXQueryExtensionsString, , (Display * display, int screen));
AliasExport(const char*, glXQueryServerString, , (Display * display, int screen, int name));
AliasExport(Bool, glXQueryExtension, , (Display * display, int* errorBase, int* eventBase));
AliasExport(Bool, glXQueryVersion, , (Display * display, int* major, int* minor));
AliasExport(const char*, glXGetClientString, , (Display * display, int name));

AliasExport(void, glXSwapInterval, , (int interval));
AliasExport(void, glXSwapInterval, MESA, (int interval));
AliasExport(void, glXSwapInterval, SGI, (int interval));

AliasExport(void, glXWaitGL, , ());
AliasExport(void, glXWaitX, , ());
AliasExport(void, glXReleaseBuffersMESA, , ());

typedef struct {
    GLXFBConfig* (*ptr_glXChooseFBConfig)(Display* dpy, int screen, const int* attrib_list, int* nelements);
    XVisualInfo* (*ptr_glXChooseVisual)(Display* dpy, int screen, int* attribList);
    void (*ptr_glXCopyContext)(Display* dpy, GLXContext src, GLXContext dst, GLuint mask);
    GLXContext (*ptr_glXCreateContext)(Display* dpy, XVisualInfo* vis, GLXContext shareList, Bool direct);
    GLXPixmap (*ptr_glXCreateGLXPixmap)(Display* dpy, XVisualInfo* visual, Pixmap pixmap);
    GLXContext (*ptr_glXCreateNewContext)(Display* dpy, GLXFBConfig config, int render_type, GLXContext share_list,
                                          Bool direct);
    GLXPbuffer (*ptr_glXCreatePbuffer)(Display* dpy, GLXFBConfig config, const int* attrib_list);
    GLXPixmap (*ptr_glXCreatePixmap)(Display* dpy, GLXFBConfig config, Pixmap pixmap, const int* attrib_list);
    Window (*ptr_glXCreateWindow)(Display* dpy, GLXFBConfig config, Window win, int* attrib_list);
    void (*ptr_glXDestroyContext)(Display* dpy, GLXContext ctx);
    void (*ptr_glXDestroyGLXPixmap)(Display* dpy, void* pixmap);
    void (*ptr_glXDestroyPbuffer)(Display* dpy, GLXPbuffer pbuf);
    void (*ptr_glXDestroyPixmap)(Display* dpy, void* pixmap);
    void (*ptr_glXDestroyWindow)(Display* dpy, void* win);
    const char* (*ptr_glXGetClientString)(Display* dpy, int name);
    int (*ptr_glXGetConfig)(Display* dpy, XVisualInfo* visual, int attrib, int* value);
    GLXContext (*ptr_glXGetCurrentContext)(void);
    GLXDrawable (*ptr_glXGetCurrentDrawable)(void);
    GLXDrawable (*ptr_glXGetCurrentReadDrawable)(void);
    int (*ptr_glXGetFBConfigAttrib)(Display* dpy, GLXFBConfig config, int attribute, int* value);
    GLXFBConfig* (*ptr_glXGetFBConfigs)(Display* dpy, int screen, int* nelements);
    void* (*ptr_glXGetProcAddress)(const char* procName);
    void* (*ptr_glXGetProcAddressARB)(const char* procName);
    void (*ptr_glXGetSelectedEvent)(Display* dpy, GLXDrawable draw, unsigned long* event_mask);
    XVisualInfo* (*ptr_glXGetVisualFromFBConfig)(Display* dpy, GLXFBConfig config);
    Bool (*ptr_glXIsDirect)(Display* dpy, GLXContext ctx);
    Bool (*ptr_glXMakeContextCurrent)(Display* dpy, int draw, int read, GLXContext ctx);
    Bool (*ptr_glXMakeCurrent)(Display* dpy, GLXDrawable drawable, GLXContext ctx);
    int (*ptr_glXQueryContext)(Display* dpy, GLXContext ctx, int attribute, int* value);
    int (*ptr_glXQueryDrawable)(Display* dpy, GLXDrawable draw, int attribute, unsigned int* value);
    Bool (*ptr_glXQueryExtension)(Display* dpy, int* errorb, int* event);
    const char* (*ptr_glXQueryExtensionsString)(Display* dpy, int screen);
    const char* (*ptr_glXQueryServerString)(Display* dpy, int screen, int name);
    Bool (*ptr_glXQueryVersion)(Display* dpy, int* maj, int* min);
    void (*ptr_glXSelectEvent)(Display* dpy, GLXDrawable draw, unsigned long event_mask);
    void (*ptr_glXSwapBuffers)(Display* dpy, GLXDrawable drawable);
    void (*ptr_glXUseXFont)(Font font, int first, int count, int list);
    void (*ptr_glXWaitGL)(void);
    void (*ptr_glXWaitX)(void);
} __glXGLCoreFunctions;

__attribute__((visibility("default"))) const __glXGLCoreFunctions __GLXGL_CORE_FUNCTIONS = {
    .ptr_glXChooseFBConfig = gl4es_glXChooseFBConfig,
    .ptr_glXChooseVisual = gl4es_glXChooseVisual,
    .ptr_glXCopyContext = gl4es_glXCopyContext,
    .ptr_glXCreateContext = gl4es_glXCreateContext,
    .ptr_glXCreateGLXPixmap = gl4es_glXCreateGLXPixmap,
    .ptr_glXCreateNewContext = gl4es_glXCreateNewContext,
    .ptr_glXCreatePbuffer = gl4es_glXCreatePbuffer,
    .ptr_glXCreatePixmap = gl4es_glXCreatePixmap,
    .ptr_glXCreateWindow = gl4es_glXCreateWindow,
    .ptr_glXDestroyContext = gl4es_glXDestroyContext,
    .ptr_glXDestroyGLXPixmap = gl4es_glXDestroyGLXPixmap,
    .ptr_glXDestroyPbuffer = gl4es_glXDestroyPbuffer,
    .ptr_glXDestroyPixmap = gl4es_glXDestroyPixmap,
    .ptr_glXDestroyWindow = gl4es_glXDestroyWindow,
    .ptr_glXGetClientString = gl4es_glXGetClientString,
    .ptr_glXGetConfig = gl4es_glXGetConfig,
    .ptr_glXGetCurrentContext = gl4es_glXGetCurrentContext,
    .ptr_glXGetCurrentDrawable = gl4es_glXGetCurrentDrawable,
    //.ptr_glXGetCurrentReadDrawable = gl4es_glXGetCurrentReadDrawable,
    .ptr_glXGetFBConfigAttrib = gl4es_glXGetFBConfigAttrib,
    .ptr_glXGetFBConfigs = gl4es_glXGetFBConfigs,
    .ptr_glXGetProcAddress = gl4es_glXGetProcAddress,
    .ptr_glXGetProcAddressARB = gl4es_glXGetProcAddress,
    //.ptr_glXGetSelectedEvent    = gl4es_glXGetSelectedEvent,
    .ptr_glXGetVisualFromFBConfig = gl4es_glXGetVisualFromFBConfig,
    .ptr_glXIsDirect = gl4es_glXIsDirect,
    .ptr_glXMakeContextCurrent = gl4es_glXMakeContextCurrent,
    .ptr_glXMakeCurrent = gl4es_glXMakeCurrent,
    .ptr_glXQueryContext = gl4es_glXQueryContext,
    .ptr_glXQueryDrawable = gl4es_glXQueryDrawable,
    .ptr_glXQueryExtension = gl4es_glXQueryExtension,
    .ptr_glXQueryExtensionsString = gl4es_glXQueryExtensionsString,
    .ptr_glXQueryServerString = gl4es_glXQueryServerString,
    .ptr_glXQueryVersion = gl4es_glXQueryVersion,
    //.ptr_glXSelectEvent         = gl4es_glXSelectEvent,
    .ptr_glXSwapBuffers = gl4es_glXSwapBuffers,
    .ptr_glXUseXFont = gl4es_glXUseXFont,
    .ptr_glXWaitGL = gl4es_glXWaitGL,
    .ptr_glXWaitX = gl4es_glXWaitX,
};

typedef void (*__GLXextFuncPtr)(void);
typedef int glvnd_mutex_t;

__attribute__((visibility("default"))) __GLXextFuncPtr __glXGLLoadGLXFunction(const char* name, __GLXextFuncPtr* ptr,
                                                                              glvnd_mutex_t* mutex) {
    if (*ptr != NULL) {
        return *ptr;
    }

    *ptr = (__GLXextFuncPtr)gl4es_glXGetProcAddress((const GLubyte*)name);

    return *ptr;
}

__attribute__((visibility("default"))) int XDefaultDepth(Display* display, int screen_number) {
    DBG(SHUT_LOGD("XDefaultDepth"))
    if (!display) return 0;

    int screen = DefaultScreen(display);
    Screen* scr = ScreenOfDisplay(display, screen);
    if (!scr) return 0;

    DBG(SHUT_LOGD("XDefaultDepth -> %d", scr->root_depth))
    return scr->root_depth;
}

#ifndef XFree
#define XFree(ptr) free(ptr)
#endif

__attribute__((visibility("default")))
XVisualInfo* XGetVisualInfo(Display* display, long vinfo_mask, XVisualInfo* vinfo_template, int* nitems_return) {
    if (!display || !nitems_return) return NULL;

    int n_screens = ScreenCount(display);
    size_t cap = 16;
    XVisualInfo* out = (XVisualInfo*)malloc(cap * sizeof(XVisualInfo));
    if (!out) {
        *nitems_return = 0;
        return NULL;
    }
    size_t out_count = 0;

    for (int scr = 0; scr < n_screens; ++scr) {
        Screen* screen = ScreenOfDisplay(display, scr);
        if (!screen) continue;

        int ndepths = screen->ndepths;
        for (int di = 0; di < ndepths; ++di) {
            Depth* depthp = &screen->depths[di];
            int depth_val = depthp->depth;
            int nvisuals = depthp->nvisuals;
            for (int vi = 0; vi < nvisuals; ++vi) {
                Visual* visual = NULL;
                visual = &depthp->visuals[vi];

                if (!visual) continue;

                XVisualInfo xi;
                memset(&xi, 0, sizeof(xi));
                xi.visual = visual;
                xi.visualid = XVisualIDFromVisual(visual);
                xi.screen = scr;
                xi.depth = depth_val;

                xi.class = visual->class;
                xi.red_mask = visual->red_mask;
                xi.green_mask = visual->green_mask;
                xi.blue_mask = visual->blue_mask;

#ifdef MAP_ENTRIES_MEMBER_EXISTS
                xi.colormap_size = visual->map_entries;
#else
                /* most X Visual has map_entries; we'll try to access it safely */
                xi.colormap_size =
                    (int)(visual->map_entries ? visual->map_entries : (1 << (xi.depth > 0 ? xi.depth : 8)));
#endif
                xi.bits_per_rgb = visual->bits_per_rgb;

                bool match = true;
                if (vinfo_mask & VisualIDMask) {
                    if (xi.visualid != vinfo_template->visualid) match = false;
                }
                if (vinfo_mask & VisualScreenMask) {
                    if (xi.screen != vinfo_template->screen) match = false;
                }
                if (vinfo_mask & VisualDepthMask) {
                    if (xi.depth != vinfo_template->depth) match = false;
                }
                if (vinfo_mask & VisualClassMask) {
                    if (xi.class != vinfo_template->class) match = false;
                }
                if (vinfo_mask & VisualRedMaskMask) {
                    if (xi.red_mask != vinfo_template->red_mask) match = false;
                }
                if (vinfo_mask & VisualGreenMaskMask) {
                    if (xi.green_mask != vinfo_template->green_mask) match = false;
                }
                if (vinfo_mask & VisualBlueMaskMask) {
                    if (xi.blue_mask != vinfo_template->blue_mask) match = false;
                }
                if (vinfo_mask & VisualColormapSizeMask) {
                    if (xi.colormap_size != vinfo_template->colormap_size) match = false;
                }
                if (vinfo_mask & VisualBitsPerRGBMask) {
                    if (xi.bits_per_rgb != vinfo_template->bits_per_rgb) match = false;
                }

                if (!match) continue;

                /* append */
                if (out_count + 1 > cap) {
                    size_t newcap = cap * 2;
                    XVisualInfo* tmp = (XVisualInfo*)realloc(out, newcap * sizeof(XVisualInfo));
                    if (!tmp) {
                        XFree(out);
                        *nitems_return = 0;
                        return NULL;
                    }
                    out = tmp;
                    cap = newcap;
                }
                out[out_count++] = xi;
            } /* each visual */
        } /* each depth */
    } /* each screen */

    if (out_count == 0) {
        XFree(out);
        *nitems_return = 0;
        return NULL;
    }

    XVisualInfo* result = (XVisualInfo*)realloc(out, out_count * sizeof(XVisualInfo));
    if (result) out = result;

    *nitems_return = (int)out_count;
    return out;
}

__attribute__((visibility("default"))) VisualID XVisualIDFromVisual(Visual* visual) {
    if (!visual) return (VisualID)0;
    return (VisualID)visual->visualid;
}
