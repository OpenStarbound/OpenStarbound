#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <dlfcn.h>

#include "rpi.h"

// #define DEBUG
#ifdef DEBUG
#define DBG(a) a
#define DBGLOGD(...) SHUT_LOGD(__VA_ARGS__)
#else
#define DBG(a)
#define DBGLOGD(...)                                                                                                   \
    {}
#endif

// Code specific to RPI
// everything is dynamicaly linked, so this can be saffely compiled everywhere
extern void* bcm_host;
extern void* vcos;

extern void (*bcm_host_init)();
extern void (*bcm_host_deinit)();

typedef uint32_t DISPMANX_DISPLAY_HANDLE_T;
typedef uint32_t DISPMANX_UPDATE_HANDLE_T;
typedef uint32_t DISPMANX_ELEMENT_HANDLE_T;
typedef uint32_t DISPMANX_RESOURCE_HANDLE_T;
typedef uint32_t DISPMANX_PROTECTION_T;
typedef uint32_t DISPMANX_FLAGS_ALPHA_T;
typedef struct tag_VC_RECT_T {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} VC_RECT_T;
typedef struct {
    DISPMANX_ELEMENT_HANDLE_T element;
    int width;
    int height;
} EGL_DISPMANX_WINDOW_T;
typedef struct {
    DISPMANX_FLAGS_ALPHA_T flags;
    uint32_t opacity;
    DISPMANX_RESOURCE_HANDLE_T mask;
} VC_DISPMANX_ALPHA_T;
int32_t (*graphics_get_display_size)(const uint16_t, uint32_t*, uint32_t*);
DISPMANX_DISPLAY_HANDLE_T (*vc_dispmanx_display_open)(uint32_t);
DISPMANX_UPDATE_HANDLE_T (*vc_dispmanx_update_start)(int32_t);
DISPMANX_ELEMENT_HANDLE_T (*vc_dispmanx_element_add)(DISPMANX_UPDATE_HANDLE_T, DISPMANX_DISPLAY_HANDLE_T, int32_t,
                                                     VC_RECT_T*, DISPMANX_RESOURCE_HANDLE_T, VC_RECT_T*,
                                                     DISPMANX_PROTECTION_T,
                                                     /*VC_DISPMANX_ALPHA_T*/ void*, /*DISPMANX_CLAMP_T*/ void*,
                                                     /*DISPMANX_TRANSFORM_T*/ int32_t);
int (*vc_dispmanx_update_submit_sync)(DISPMANX_RESOURCE_HANDLE_T);
int (*vc_dispmanx_element_remove)(DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element);
static DISPMANX_UPDATE_HANDLE_T dispman_update;
static DISPMANX_DISPLAY_HANDLE_T dispman_display;
static VC_RECT_T dst_rect;
static VC_RECT_T src_rect;

static int rpi_inited = 0;
void rpi_init() {
    if (!bcm_host || rpi_inited) return;
    rpi_inited++;
    bcm_host_init = dlsym(bcm_host, "bcm_host_init");
    bcm_host_deinit = dlsym(bcm_host, "bcm_host_deinit");
    if (!bcm_host_init || !bcm_host_deinit) {
        DBG(SHUT_LOGD("LIBGL: Warning, bcm_host function missing (init=%p, deinit=%p)\n", bcm_host_init,
                      bcm_host_deinit);)
        return;
    }
    bcm_host_init();

#define GO(A)                                                                                                          \
    A = dlsym(bcm_host, #A);                                                                                           \
    if (A == NULL) A = dlsym(vcos, #A);                                                                                \
    if (A == NULL) {                                                                                                   \
        DBG(SHUT_LOGD("LIBGL: Warning, " #A " is null"))                                                               \
    }
    GO(graphics_get_display_size);
    GO(vc_dispmanx_display_open);
    GO(vc_dispmanx_update_start);
    GO(vc_dispmanx_element_add);
    GO(vc_dispmanx_update_submit_sync);
    GO(vc_dispmanx_element_remove);
#undef GO
}

void rpi_fini() {
    if (!bcm_host || !rpi_inited) return;
    --rpi_inited;
    if (!bcm_host_init || !bcm_host_deinit) return;
    bcm_host_deinit();
}

void* create_rpi_window(int w, int h) {
    EGL_DISPMANX_WINDOW_T* nativewindow = (EGL_DISPMANX_WINDOW_T*)calloc(1, sizeof(EGL_DISPMANX_WINDOW_T));
    if (!bcm_host) return NULL;
    // create a simple RPI nativewindow of size w*h, on output 0 (i.e. LCD)...
    // code heavily inspired from Allegro 5.2
    uint32_t screenwidth, screenheight;
    graphics_get_display_size(/*LCD*/ 0, &screenwidth, &screenheight);
    if (w == 0) w = screenwidth;
    if (h == 0) h = screenheight;
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    VC_RECT_T dst_rect, src_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screenwidth;
    dst_rect.height = screenheight;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = w << 16;
    src_rect.height = h << 16;
    dispman_display = vc_dispmanx_display_open(/*LCD*/ 0);
    dispman_update = vc_dispmanx_update_start(0);
    VC_DISPMANX_ALPHA_T alpha = {/*DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS*/ 1, 255, 0};
    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0, &dst_rect, 0, &src_rect,
                                              /*DISPMANX_PROTECTION_NONE*/ 0, &alpha, 0,
                                              /*DISPMAN_NO_ROTATE*/ 0);
    nativewindow->element = dispman_element;
    nativewindow->width = w;
    nativewindow->height = h;
    vc_dispmanx_update_submit_sync(dispman_update);

    return nativewindow;
}

void delete_rpi_window(void* win) {
    EGL_DISPMANX_WINDOW_T* nativewindow = (EGL_DISPMANX_WINDOW_T*)win;

    vc_dispmanx_element_remove(dispman_update, nativewindow->element);
    vc_dispmanx_update_submit_sync(dispman_update);

    free(win);
}
