#include "hardext.h"
#include "../gl/loader.h"
#include "../gl/init.h"
#include "../gl/gl4es.h"
#include "glx_gbm.h"

#define SHUT(a)                                                                                                        \
    if (!globals4es.nobanner) a

// #define DEBUG
#ifdef DEBUG
#define DBG(a) a
#define DBGLOGD(...) SHUT_LOGD(__VA_ARGS__)
#else
#define DBG(a)
#define DBGLOGD(...)                                                                                                   \
    {}
#endif

#ifndef NO_GBM
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <errno.h>

#ifndef NO_GBM
#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif
#endif

// define static function pointers
// for GBM
#define GBMFUNC(ret, name, args)                                                                                       \
    typedef ret(*PFN##name) args;                                                                                      \
    static PFN##name gbmdrm_##name = NULL;
#include "gbmfunc.h"
#undef GBMFUNC
// for DRM
#define DRMFUNC(ret, name, args)                                                                                       \
    typedef ret(*PFN##name) args;                                                                                      \
    static PFN##name gbmdrm_##name = NULL;
#include "drmfunc.h"
#undef DRMFUNC

struct drm_fb {
    struct gbm_bo* bo;
    uint32_t fb_id;
};

static int drm_fd = -1; // drm handle
static drmModeModeInfo* drm_mode = NULL;
static uint32_t drm_crtc_id = 0;
static uint32_t drm_connector_id = 0;
static struct gbm_device* gbmdev = NULL;
static struct gbm_surface* gbmsurf = NULL;
static struct gbm_bo* gbm_bo = NULL;
static uint32_t fb_id = 0;

// code here from icecream95 https://gitlab.com/icecream95/drmegl-wrapper
static uint32_t find_crtc_for_encoder(const drmModeRes* resources, const drmModeEncoder* encoder) {
    int i;

    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }

    /* no match found */
    return -1;
}
static uint32_t find_crtc_for_connector(const drmModeRes* resources, const drmModeConnector* connector) {
    int i;

    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder* encoder = gbmdrm_drmModeGetEncoder(drm_fd, encoder_id);

        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

            gbmdrm_drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }

    /* no match found */
    return -1;
}

static void CancelGBM() {
    if (gbm) dlclose(gbm);
#define GBMFUNC(ret, name, args) gbmdrm_##name = NULL;
#include "gbmfunc.h"
#undef GBMFUNC
    if (drm) dlclose(drm);
#define DRMFUNC(ret, name, args) gbmdrm_##name = NULL;
#include "drmfunc.h"
#undef DRMFUNC

    gbm = NULL;
    globals4es.usegbm = 0;
}

static int init_drm_and_gbm() {
    drmModeRes* resources;
    drmModeConnector* connector = NULL;
    drmModeEncoder* encoder = NULL;
    int i, area;

    drm_fd = open(globals4es.drmcard, O_RDWR | O_CLOEXEC);

    if (drm_fd < 0) {
        return 0;
    }

    resources = gbmdrm_drmModeGetResources(drm_fd);
    if (!resources) {
        DBGLOGD("LIBGL: Error initializing drm resources\n");
        return 0;
    }
    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = gbmdrm_drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            break;
        }
        gbmdrm_drmModeFreeConnector(connector);
        connector = NULL;
    }
    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
         * a connector..
         */
        DBGLOGD("LIBGL: DRM no connected connector!\n");
        return 0;
    }
    /* find preferred mode or the highest resolution mode: */
    for (i = 0, area = 0; i < connector->count_modes; i++) {
        drmModeModeInfo* current_mode = &connector->modes[i];

        if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
            drm_mode = current_mode;
        }

        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if (current_area > area) {
            drm_mode = current_mode;
            area = current_area;
        }
    }
    if (!drm_mode) {
        DBGLOGD("LIBGL: DRM could not find mode!\n");
        return 0;
    }
    /* find encoder: */
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = gbmdrm_drmModeGetEncoder(drm_fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id) break;
        gbmdrm_drmModeFreeEncoder(encoder);
        encoder = NULL;
    }
    if (encoder) {
        drm_crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(resources, connector);
        if (crtc_id == 0) {
            DBGLOGD("LIBGL: DRM no crtc found!\n");
            return 0;
        }

        drm_crtc_id = crtc_id;
    }

    drm_connector_id = connector->connector_id;

    gbmdev = gbmdrm_gbm_create_device(drm_fd);
    if (!gbmdev) {
        DBGLOGD("LIBGL: Error initializing gbm device\n");
        return 0;
    }

    gbmsurf = gbmdrm_gbm_surface_create(gbmdev, drm_mode->hdisplay, drm_mode->vdisplay, GBM_FORMAT_XRGB8888,
                                        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbmsurf) {
        DBGLOGD("LIBGL: Error initializing gbm surface\n");
        return 0;
    }

    return 1;
}
static void drm_fb_destroy_callback(struct gbm_bo* bo, void* data) {
    struct drm_fb* fb = data;
    struct gbm_device* gbm = gbmdrm_gbm_bo_get_device(bo);

    if (fb->fb_id) gbmdrm_drmModeRmFB(drm_fd, fb->fb_id);

    free(fb);
}

static struct drm_fb* drm_fb_get_from_bo(struct gbm_bo* bo) {
    struct drm_fb* fb = gbmdrm_gbm_bo_get_user_data(bo);
    uint32_t width, height, stride, handle;
    int ret;

    if (fb) return fb;

    fb = calloc(1, sizeof(*fb));
    fb->bo = bo;

    width = gbmdrm_gbm_bo_get_width(bo);
    height = gbmdrm_gbm_bo_get_height(bo);
    stride = gbmdrm_gbm_bo_get_stride(bo);
    handle = gbmdrm_gbm_bo_get_handle(bo).u32;

    ret = gbmdrm_drmModeAddFB(drm_fd, width, height, 24, 32, stride, handle, &fb->fb_id);
    if (ret) {
        DBGLOGD("failed to create fb: %s\n", strerror(errno));
        free(fb);
        return NULL;
    }

    gbmdrm_gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    return fb;
}

static int OpenGBM() {
    int available = 0;

    available = init_drm_and_gbm();

    SHUT(DBGLOGD("LIBGL: GBM on card %s is %s\n", globals4es.drmcard, available ? "Available" : "Not available"));

    return available;
}

void LoadGBMFunctions() {
    static int hasrun = 0;
    if (hasrun) return;
    hasrun = 1;
    if (!gbm || !drm) return;
// load functions
#define GBMFUNC(ret, name, args)                                                                                       \
    gbmdrm_##name = (PFN##name)dlsym(gbm, #name);                                                                      \
    if (!gbmdrm_##name) {                                                                                              \
        DBGLOGD("LIBGL: libgbm function %s missing, no gbm surface enabled\n", #name);                                 \
        CancelGBM();                                                                                                   \
        return;                                                                                                        \
    }
#include "gbmfunc.h"
#undef DRMFUNC
// load functions
#define DRMFUNC(ret, name, args)                                                                                       \
    gbmdrm_##name = (PFN##name)dlsym(drm, #name);                                                                      \
    if (!gbmdrm_##name) {                                                                                              \
        DBGLOGD("LIBGL: libdrm function %s missing, no gbm surface enabled\n", #name);                                 \
        CancelGBM();                                                                                                   \
        return;                                                                                                        \
    }
#include "drmfunc.h"
#undef DRMFUNC
    // Check card
    if (!OpenGBM()) {
        CancelGBM();
        return;
    }
    // all done
    return;
}

void CloseGBMFunctions() {
    if (gbmdev) {
        gbmdrm_gbm_device_destroy(gbmdev);
        gbmdev = NULL;
    }
    if (drm_fd >= 0) {
        close(drm_fd);
        drm_fd = -1;
    }
}

void* OpenGBMDisplay(void* display) {
    LOAD_EGL_EXT(eglGetPlatformDisplay);
    if (egl_eglGetPlatformDisplay) {
        return egl_eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbmdev, NULL);
    } else {
        LOAD_EGL(eglGetDisplay);
        return egl_eglGetDisplay((EGLNativeDisplayType)gbmdev);
    }
}

void* CreateGBMWindow(int w, int h) {
    void* ret = gbmsurf;
    if (!ret) DBGLOGD("LIBGL: Warning, cannot create gbm surface %dx%d\n", w, h);
}

void DeleteGBMWindow(void* win) {
    gbmdrm_gbm_surface_destroy((struct gbm_surface*)win);
}

int FindGBMConfig(EGLDisplay eglDisp, EGLConfig* configs, int numFounds) {
    LOAD_EGL(eglGetConfigAttrib);
    int idx = 0;
    while (idx < numFounds) {
        EGLint gbm_format;
        if (egl_eglGetConfigAttrib(eglDisp, configs[idx], EGL_NATIVE_VISUAL_ID, &gbm_format)) {
            if (gbm_format == GBM_FORMAT_XRGB8888) {
                return idx;
            }
        }
        ++idx;
    }
    DBGLOGD("LIBGL: Warning, no EGLConfig matching GBM Format found\n");
    return 0; // not found...
}
EGLBoolean GBMMakeCurrent(EGLDisplay eglDisp, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    LOAD_EGL(eglMakeCurrent);
    LOAD_EGL(eglSwapBuffers);

    EGLBoolean res = egl_eglMakeCurrent(eglDisp, draw, read, ctx);

    if (ctx == EGL_NO_CONTEXT || draw == NULL) {
        // clean up gbm/drm stuff?
        return res;
    }

    egl_eglSwapBuffers(eglDisp, draw);
    struct gbm_bo* bo = gbmdrm_gbm_surface_lock_front_buffer(gbmsurf);
    if (!bo) {
        // probably a PBuffer
        // printf("LIBGL: gbm BO is NULL\n");
        return EGL_TRUE;
    }
    struct drm_fb* fb = drm_fb_get_from_bo(bo);
    // Is it safe to do this here?
    gbmdrm_gbm_surface_release_buffer(gbmsurf, bo);

    int r = gbmdrm_drmModeSetCrtc(drm_fd, drm_crtc_id, fb->fb_id, 0, 0, &drm_connector_id, 1, drm_mode);
    if (r) {
        DBGLOGD("LIBGL: GBM failed to set mode: %s\n", strerror(errno));
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

#else // NO_GBM

void LoadGBMFunctions() {}

void CloseGBMFunctions() {}

void* OpenGBMDisplay(void* display) {
    return NULL;
}

void* CreateGBMWindow(int w, int h) {
    return NULL;
}

void DeleteGBMWindow(void* win) {}

#if !defined(NOEGL) && !defined(ANDROID)
int FindGBMConfig(EGLDisplay eglDisp, EGLConfig* configs, int numFounds) {
    return 0;
}
EGLBoolean GBMMakeCurrent(EGLDisplay eglDisp, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    return EGL_FALSE;
}
#endif

#endif // NO_GBM
