#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"

#else
#include <stdio.h>
#include <direct.h>
#define getcwd(a, b) _getcwd(a, b)
#define snprintf _snprintf
#endif
#include "../../version.h"
#include "../glx/glx_gbm.h"
#include "../glx/glx.h"
#include "../glx/streaming.h"
#include "gl4es.h"
#include "build_info.h"
#include "debug.h"
#include "loader.h"
#include "logs.h"
#include "fpe_cache.h"
#include "init.h"
#include "envvars.h"
#include "config.h"

#if defined(__EMSCRIPTEN__)
#define NO_INIT_CONSTRUCTOR
#endif

void gl_init();
void gl_close();

#ifdef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
#ifdef AMIGAOS4
void agl_reset_internals();
#endif
void fpe_shader_reset_internals();
#endif

globals4es_t globals4es = {0};

#if defined(PANDORA) || defined(CHIP) || (defined(GOA_CLONE) && !defined(__aarch64__))
static void fast_math() {
    // enable Cortex A8 RunFast
    int v = 0;
    __asm__ __volatile__("vmrs %0, fpscr\n"
                         "orr  %0, #((1<<25)|(1<<24))\n" // default NaN, flush-to-zero
                         "vmsr fpscr, %0\n"
                         //"vmrs %0, fpscr\n"
                         : "=&r"(v));
}
#endif

#ifndef DEFAULT_ES
#if defined(PANDORA)
#define DEFAULT_ES 1
#else
#define DEFAULT_ES 3
#endif
#endif

void load_libs();
void glx_init();

static int inited = 0;

EXPORT
void set_getmainfbsize(void(APIENTRY_GL4ES* new_getMainFBSize)(int* w, int* h)) {
    gl4es_getMainFBSize = (void*)new_getMainFBSize;
}

EXPORT
void set_getprocaddress(void*(APIENTRY_GL4ES* new_proc_address)(const char*)) {
    gles_getProcAddress = new_proc_address;
}

void init_internal_glDrawElementsBaseVertex();
void initialize_ng_gl4es() {
    /*
    int enableANGLE = config_get_int("enableANGLE");
    globals4es.enableANGLE = enableANGLE;
    if (enableANGLE) {
        setenv("LIBGL_GLES", "libGLESv2_angle.so", 1);
        setenv("LIBGL_EGL", "libEGL_angle.so", 1);
        LOGD("ANGLE enabled and used")
    } else {
        setenv("LIBGL_GLES", "", 1);
        setenv("LIBGL_EGL", "", 1);
        LOGD("ANGLE disabled and not used")
    }
        */
    globals4es.enableANGLE = false;
}
char* NGGDirectory;

#ifdef NO_INIT_CONSTRUCTOR
EXPORT
#else
#if defined(_WIN32) || defined(__CYGWIN__)
#define BUILD_WINDOWS_DLL
// dll can't initialize emulator in startup code :(
static unsigned char dll_inited;
EXPORT
#endif
#if !defined(_MSC_VER) || defined(__clang__)
__attribute__((constructor(101)))
#endif
#endif
void initialize_gl4es() {
    NGGDirectory = (char*)GetEnvVar("NGG_DIR_PATH");
    if (!NGGDirectory || strlen(NGGDirectory) == 0) {
        NGGDirectory = malloc(strlen(DEFAULT_NGG_DIRECTORY_PATH) + 1);
        strcpy(NGGDirectory, DEFAULT_NGG_DIRECTORY_PATH);
    }

    clear_log();
    SHUT_LOGD("Initialising Krypton Wrapper")

    config_refresh();

    initialize_ng_gl4es();

#ifdef BUILD_WINDOWS_DLL
    if (!dll_inited) {
        LOGE("Windows ES emulator's can't be initialized from DllMain (directX limitation)");
        return;
    }
#endif
    // only init 1 time
    if (inited++) return;
    // default init of globals
    memset(&globals4es, 0, sizeof(globals4es));
    globals4es.mergelist = 1;
    globals4es.queries = 1;
    globals4es.beginend = 1;
#ifdef PYRA
    GetEnvVarInt("LIBGL_DEEPBIND", &globals4es.deepbind, 0);
#else
    GetEnvVarInt("LIBGL_DEEPBIND", &globals4es.deepbind, 1);
#endif
    // overrides by env. variables
#ifdef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
    GetEnvVarInt("LIBGL_NOBANNER", &globals4es.nobanner, 1);
#else
    globals4es.nobanner = IsEnvVarTrue("LIBGL_NOBANNER");
#endif

    if (!globals4es.nobanner) print_build_infos();

#define env(name, global, message)                                                                                     \
    if (IsEnvVarTrue(#name)) {                                                                                         \
        SHUT_LOGD(message "");                                                                                         \
        global = true;                                                                                                 \
    }

    env(LIBGL_XREFRESH, globals4es.xrefresh, "xrefresh will be called on cleanup");
    env(LIBGL_STACKTRACE, globals4es.stacktrace, "stacktrace will be printed on crash");

    const int LIBGL_FB_ENV_VAR =
#ifndef LIBGL_FB
        ReturnEnvVarInt("LIBGL_FB")
#else
        ReturnEnvVarIntDef("LIBGL_FB", LIBGL_FB)
#endif
        ;
    switch (LIBGL_FB_ENV_VAR) {
    case 1:
        SHUT_LOGD("framebuffer output enabled");
        globals4es.usefb = 1;
        break;
    case 2:
        SHUT_LOGD("using framebuffer + fbo");
        globals4es.usefb = 1;
        globals4es.usefbo = 1;
        break;
#ifndef NOX11
    case 3:
        SHUT_LOGD("using pbuffer");
        globals4es.usefb = 0;
        globals4es.usepbuffer = 1;
        break;
#endif
    case 4:
#ifdef NO_GBM
        SHUT_LOGD("GBM support not built, cannot use it");
#else
        SHUT_LOGD("using GBM");
        globals4es.usefb = 0;
        globals4es.usegbm = 1;
#endif
        break;
    default:
        break;
    }
    env(LIBGL_BLITFB0, globals4es.blitfb0, "Blit to FB 0 force a SwapBuffer");
    env(LIBGL_FPS, globals4es.showfps, "fps counter enabled");
#if defined(USE_FBIO) || defined(PYRA)
    env(LIBGL_VSYNC, globals4es.vsync, "vsync enabled");
#endif
#ifdef PANDORA
    if (GetEnvVarFloat("LIBGL_GAMMA", &globals4es.gamma, 0.0f)) {
        SHUT_LOGD("Set gamma to %.2f", globals4es.gamma);
    }
#endif
    env(LIBGL_NOBGRA, globals4es.nobgra, "Ignore BGRA texture capability");
    env(LIBGL_NOTEXRECT, globals4es.notexrect, "Don't export Text Rectangle extension");
    if (globals4es.usefbo) {
        env(LIBGL_FBONOALPHA, globals4es.fbo_noalpha, "Main FBO has no alpha channel");
    }

    globals4es.es = ReturnEnvVarInt("LIBGL_ES");
    switch (globals4es.es) {
    case 1:
    case 2:
    case 3:
        break;
    default:
        // automatic ES backend selection
        globals4es.es = DEFAULT_ES;
        break;
    }
#ifndef __APPLE__
    globals4es.esversion = 0; // will be set in glGetString
#else
    globals4es.esversion = 300;
#endif
    init_internal_glDrawElementsBaseVertex();

    globals4es.use_mc_color = ReturnEnvVarInt("LIBGL_USE_MC_COLOR");

#define SET_CONFIG_INT(name) globals4es.name = config_get_int(#name)
#define SET_CONFIG_INT_0(name) globals4es.name = config_get_int(#name) != -1 ? config_get_int(#name) : 0
#define SET_CONFIG_STRING(name) globals4es.name = config_get_string(#name)
    SET_CONFIG_INT_0(force_es_copy_tex);
    // custom egl lib is useless now...
    // SET_CONFIG_STRING(force_egl_lib);
    // SET_CONFIG_STRING(force_gles_lib);

    globals4es.gl = ReturnEnvVarInt("LIBGL_GL");
    switch (globals4es.gl) {
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 20:
    case 21:
    case 30:
    case 31:
    case 32:
    case 33:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
        break;
    default:
        // automatic GL version selection
        globals4es.gl = (globals4es.es == 1) ? 15 : 31;
        break;
    }

    SHUT_LOGD("Using GLES %s backend", (globals4es.es == 1) ? "1.1" : ((globals4es.es > 2) ? "3.x" : "2.0"));

    env(LIBGL_NODEPTHTEX, globals4es.nodepthtex, "Disable usage of Depth Textures");

    const char* env_drmcard = GetEnvVar("LIBGL_DRMCARD");
    if (env_drmcard) {
#ifdef NO_GBM
        SHUT_LOGD("Warning, GBM not compiled in, cannot use LIBGL_DRMCARD");
#else
        strncpy(globals4es.drmcard, env_drmcard, 50);
    } else {
        strcpy(globals4es.drmcard, "/dev/dri/card0");
#endif
    }

#if !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
    load_libs();
#endif

#if (defined(NOEGL) && !defined(ANDROID) && !defined(__APPLE__)) || defined(__EMSCRIPTEN__)
    int gl4es_notest = !gles_getProcAddress;
#else
    int gl4es_notest = IsEnvVarTrue("LIBGL_NOTEST");
#endif
    env(LIBGL_NOHIGHP, globals4es.nohighp, "Do not use HIGHP in fragment shader even if detected");

    globals4es.floattex = ReturnEnvVarIntDef("LIBGL_FLOAT", 1);
    switch (globals4es.floattex) {
    case 0:
        SHUT_LOGD("Float and Half-Float texture support disabled");
        break;
    case 2:
        SHUT_LOGD("Float and Half-float texture support forced");
        break;
    default:
        globals4es.floattex = 1;
        break;
    }

    GetHardwareExtensions(gl4es_notest);

#if !defined(NO_LOADER) && !defined(NO_GBM)
    if (globals4es.usegbm) LoadGBMFunctions();
    if (globals4es.usegbm && !(gbm && drm)) {
        SHUT_LOGD("cannot use GBM, disabling");
        globals4es.usegbm = 0; // should do some smarter fallback?
    }
#else
    globals4es.usegbm = 0;
#endif
#if !defined(NOX11)
    glx_init();
#endif

    gl_init();

#ifdef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
    fpe_shader_reset_internals();
#ifdef AMIGAOS4
    agl_reset_internals();
#endif
#endif

    env(LIBGL_RECYCLEFBO, globals4es.recyclefbo, "Recycling of FBO enabled");

    // Texture hacks
    SHUT_LOGD("AutoMipMap forced");

    if (IsEnvVarTrue("LIBGL_TEXCOPY")) {
        globals4es.texcopydata = 1;
        SHUT_LOGD("Texture copy enabled");
    }

    globals4es.texshrink = ReturnEnvVarInt("LIBGL_SHRINK");
    switch (globals4es.texshrink) {
    case 10:
        SHUT_LOGD("Texture shrink, mode 10 selected (advertise 8192 max texture size, but >2048 are quadshrinked and > "
                  "512 are shrinked), but not for empty textures");
        break;
    case 11:
        SHUT_LOGD("Texture shrink, mode 11 selected (advertise a max texture size *2, but every texture with one "
                  "dimension > max texture size will get shrinked to max texture size), but not for empty textures");
        break;
    case 1:
        SHUT_LOGD("Texture shrink, mode 1 selected (everything / 2)");
        break;
    case 2:
        SHUT_LOGD("Texture shrink, mode 2 selected (only > 512 /2 )");
        break;
    case 3:
        SHUT_LOGD("Texture shrink, mode 3 selected (only > 256 /2 )");
        break;
    case 4:
        SHUT_LOGD("Texture shrink, mode 4 selected (only > 256 /2, >=1024 /4 )");
        break;
    case 5:
        SHUT_LOGD("Texture shrink, mode 5 selected (every > 256 is downscaled to 256 ), but not for empty textures");
        break;
    case 6:
        SHUT_LOGD("Texture shrink, mode 6 selected (only > 128 /2, >=512 is downscaled to 256 ), but not for empty "
                  "textures");
        break;
    case 7:
        SHUT_LOGD("Texture shrink, mode 7 selected (only > 512 /2 ), but not for empty textures");
        break;
    case 8:
        SHUT_LOGD("Texture shrink, mode 8 selected (advertise 8192 max texture size, but >2048 are shrinked to 2048)");
        break;
    case 9:
        SHUT_LOGD("Texture shrink, mode 9 selected (advertise 8192 max texture size, but >4096 are quadshrinked and > "
                  "512 are shrinked), but not for empty textures");
        break;
    default:
        globals4es.texshrink = 0;
        break;
    }

    env(LIBGL_TEXDUMP, globals4es.texdump, "Texture dump enabled");
    env(LIBGL_ALPHAHACK, globals4es.alphahack, "Alpha Hack enabled");

#ifdef TEXSTREAM
    switch (ReturnEnvVarInt("LIBGL_STREAM")) {
    case 1:
        globals4es.texstream = InitStreamingCache();
        SHUT_LOGD("Streaming texture %s", (globals4es.texstream) ? "enabled" : "not available");
        // FreeStreamed(AddStreamed(1024, 512, 0));
        break;
    case 2:
        globals4es.texstream = InitStreamingCache() ? 2 : 0;
        SHUT_LOGD("Streaming texture %s", (globals4es.texstream) ? "forced" : "not available");
        // FreeStreamed(AddStreamed(1024, 512, 0));
        break;
    default:
        break;
    }
#endif

    env(LIBGL_NOLUMALPHA, globals4es.nolumalpha, "GL_LUMINANCE_ALPHA hardware support disabled");
    env(LIBGL_BLENDHACK, globals4es.blendhack, "Change Blend GL_SRC_ALPHA, GL_ONE to GL_ONE, GL_ONE");
    env(LIBGL_BLENDCOLOR, globals4es.blendcolor, "Export a (faked) glBlendColor");
    env(LIBGL_NOERROR, globals4es.noerror, "glGetError() always return GL_NOERROR");

    globals4es.silentstub = 1;
    if (IsEnvVarInt("LIBGL_SILENTSTUB", 0)) {
        globals4es.silentstub = 0;
        SHUT_LOGD("Stub/non present functions are printed");
    }

    env(LIBGL_VABGRA, globals4es.vabgra, "Export GL_ARB_vertex_array_bgra extension");

    // const char *env_version = GetEnvVar("LIBGL_VERSION");
    bool env_version = false; // force it not to be modified

    char* version_color = "";
    if (globals4es.use_mc_color)
        version_color =
#if RELEASE == 1
            "§b"
#else
            "§d"
#endif
            ;
    if (env_version) {
        SHUT_LOGD("Overide version string with \"%s\" (should be in the form of \"1.x\")", env_version);
    }

    if (env_version) {
        snprintf(globals4es.version, 64,
                 globals4es.use_mc_color == 1 ? "%s §aKrypton Wrapper %s%s%d.%d.%d%s§r"
                                              : "%s Krypton Wrapper %s%s%d.%d.%d%s",
                 env_version, version_color, VERSION_TYPE, MAJOR, MINOR, REVISION, VERSION_SUFFIX);
        SHUT_LOGD("Targeting OpenGL %s", env_version);
    } else {
        snprintf(globals4es.version, 64,
                 globals4es.use_mc_color == 1 ? "%d.%d §aKrypton Wrapper %s%s%d.%d.%d%s§r"
                                              : "%d.%d Krypton Wrapper %s%s%d.%d.%d%s",
                 globals4es.gl / 10, globals4es.gl % 10, version_color, VERSION_TYPE, MAJOR, MINOR, REVISION,
                 VERSION_SUFFIX);
        SHUT_LOGD("Targeting OpenGL %d.%d", globals4es.gl / 10, globals4es.gl % 10);
    }

    SHUT_LOGD("Target OpenGL Version: %s", globals4es.version)

    if (hardext.srgb && IsEnvVarTrue("LIBGL_SRGB")) {
        globals4es.glx_surface_srgb = 2;
        SHUT_LOGD("enabling sRGB support");
    }

    if (IsEnvVarTrue("LIBGL_FASTMATH")) {
#if defined(PANDORA) || defined(CHIP) || (defined(GOA_CLONE) && !defined(__aarch64__))
        SHUT_LOGD("Enable FastMath for cortex-a8");
        fast_math();
#else
        SHUT_LOGD("No FastMath on this platform");
#endif
    }

    switch (hardext.npot) {
    case 0:
        globals4es.npot = 0;
        break;
    case 1:
    case 2:
        globals4es.npot = 1;
        break;
    case 3:
        globals4es.npot = 2;
        break;
    }
    switch (ReturnEnvVarInt("LIBGL_NPOT")) {
    case 1:
        if (globals4es.npot < 1) {
            globals4es.npot = 1;
            SHUT_LOGD("Expose limited NPOT extension");
        }
        break;
    case 2:
        if (globals4es.npot < 3) {
            globals4es.npot = 2;
            SHUT_LOGD("Expose GL_ARB_texture_non_power_of_two extension");
        }
        break;
    }

    if (IsEnvVarFalse("LIBGL_GLQUERIES")) {
        globals4es.queries = 0;
        SHUT_LOGD("Don't expose fake glQueries functions");
    }
    if (IsEnvVarTrue("LIBGL_NODOWNSAMPLING")) {
        globals4es.nodownsampling = 1;
        SHUT_LOGD("No downsampling of DXTc textures");
    }
    env(LIBGL_NOTEXMAT, globals4es.texmat, "Don't handle Texture Matrice internally");
    env(LIBGL_NOVAOCACHE, globals4es.novaocache, "Don't use VAO cache");
    if (IsEnvVarTrue("LIBGL_NOINTOVLHACK")) {
        globals4es.nointovlhack = 1;
        SHUT_LOGD("No hack in shader converter to define overloaded function with int");
    }
    if (IsEnvVarTrue("LIBGL_NOSHADERLOD")) {
        globals4es.noshaderlod = 1;
        SHUT_LOGD("No GL_EXT_shader_texture_lod used even if present");
        hardext.shaderlod = 0;
    }

    int env_begin_end;
    if (GetEnvVarInt("LIBGL_BEGINEND", &env_begin_end, 0)) {
        switch (env_begin_end) {
        case 0:
            globals4es.beginend = 0;
            globals4es.mergelist = 0;
            SHUT_LOGD("Don't try to merge subsequent glBegin/glEnd blocks");
            break;
        case 1:
        case 2:
            globals4es.beginend = 1;
            SHUT_LOGD("Try to merge subsequent glBegin/glEnd blocks, even if there is a glColor / glNormal in between");
            break;
        }
    }

    if (GetEnvVarBool("LIBGL_AVOID16BITS", &globals4es.avoid16bits, (hardext.vendor & VEND_IMGTEC) ? 0 : 1)) {
        if (globals4es.avoid16bits) {
            SHUT_LOGD("Avoid 16bits textures");
        } else {
            SHUT_LOGD("Don't avoid 16bits textures");
        }
    }

    if (GetEnvVarInt("LIBGL_AVOID24BITS", &globals4es.avoid24bits, 0)) {
        switch (globals4es.avoid24bits) {
        case 0:
            SHUT_LOGD("Don't try to avoid 24bits textures");
            break;
        case 1:
            globals4es.avoid24bits = 2;
            SHUT_LOGD("Avoid 24bits textures");
            break;
        default:
            globals4es.avoid24bits = 0;
            break;
        }
    }

    env(LIBGL_FORCE16BITS, globals4es.force16bits, "Force 16bits textures");
    env(LIBGL_POTFRAMEBUFFER, globals4es.potframebuffer, "Force framebuffers to be on POT size");

    int env_forcenpot = ReturnEnvVarIntDef("LIBGL_FORCENPOT", 0);
    if (env_forcenpot == 0 && (hardext.esversion == 2 && (hardext.npot == 1 || hardext.npot == 2))) {
        SHUT_LOGD("Not forcing NPOT support");
    } else if (env_forcenpot != 0 || (hardext.esversion == 2 && (hardext.npot == 1 || hardext.npot == 2))) {
        if (hardext.npot == 3) {
            SHUT_LOGD("NPOT texture handled in hardware");
        } else if (hardext.npot == 1) {
            globals4es.forcenpot = 1;
            SHUT_LOGD("Forcing NPOT support by disabling MIPMAP support for NPOT textures");
        } else {
            SHUT_LOGD("WARNING, No Limited or Full NPOT support in hardware, Forcing NPOT has no effect!");
        }
    }

#if defined(GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB) && defined(AMIGAOS4) // temporary workaround for not-working envs
    globals4es.maxbatch = 40;
#else
    globals4es.maxbatch = 0;
#endif
    globals4es.minbatch = 0;
    int tmp = 0, tmp2 = 0;
    switch (GetEnvVarFmt("LIBGL_BATCH", "%d-%d", &tmp, &tmp2)) {
    case 2:
        globals4es.maxbatch = tmp2;
        globals4es.minbatch = tmp;
        if (globals4es.minbatch > globals4es.maxbatch) {
            globals4es.maxbatch = tmp;
            globals4es.minbatch = tmp2;
        }
        break;
    case 1:
        globals4es.maxbatch = 10 * 10 * tmp;
        globals4es.minbatch = 0;
        break;
    }
    if (globals4es.maxbatch == 0) {
        SHUT_LOGD("Not trying to batch small subsequent glDrawXXXX");
    } else {
        SHUT_LOGD("Trying to batch subsequent glDrawXXXX of size between %d and %d vertice", globals4es.minbatch,
                  globals4es.maxbatch);
    }

    if (hardext.esversion == 1)
        globals4es.usevbo = 0; // VBO on ES1.1 backend will be too messy, so disabling
    else {
        globals4es.usevbo = ReturnEnvVarIntDef("LIBGL_USEVBO", 1);
        switch (globals4es.usevbo) {
        case 0:
            SHUT_LOGD("Use of VBO disabled");
            break;
        case 1:
            SHUT_LOGD("Trying to use VBO");
            break;
        case 2:
            SHUT_LOGD("Trying to use VBO (also with glLockArrays)");
            break;
        case 3:
            SHUT_LOGD("Trying to use VBO (special glLockArrays case for idtech3 engine)");
            break;
        default:
            globals4es.usevbo = 1;
            break;
        }
    }

    globals4es.fbomakecurrent = 0;
    if ((hardext.vendor & VEND_ARM) || (globals4es.usefb)) globals4es.fbomakecurrent = 1;
    switch (ReturnEnvVarIntDef("LIBGL_FBOMAKECURRENT", -1)) {
    case 0:
        if (globals4es.fbomakecurrent) {
            globals4es.fbomakecurrent = 0;
            SHUT_LOGD("glXMakeCurrent FBO workaround disabled");
        }
        break;
    case 1:
        globals4es.fbomakecurrent = 1;
        break;
    }
    if (globals4es.fbomakecurrent) {
        SHUT_LOGD("glXMakeCurrent FBO workaround enabled");
    }

    globals4es.fbounbind = 0;
    if ((hardext.vendor & VEND_ARM) || (hardext.vendor & VEND_IMGTEC)) globals4es.fbounbind = 1;
    switch (ReturnEnvVarIntDef("LIBGL_FBOUNBIND", -1)) {
    case 0:
        if (globals4es.fbounbind) {
            globals4es.fbounbind = 0;
            SHUT_LOGD("FBO workaround for using binded texture disabled");
        }
        break;
    case 1:
        globals4es.fbounbind = 1;
        break;
    }
    if (globals4es.fbounbind) {
        SHUT_LOGD("FBO workaround for using binded texture enabled");
    }

    globals4es.fboforcetex = 1;
    GetEnvVarInt("LIBGL_FBOFORCETEX", &globals4es.fboforcetex, globals4es.fboforcetex);
    if (globals4es.fboforcetex) SHUT_LOGD("Force texture for Attachment color0 on FBO");
    globals4es.blitfullscreen = 1;
    GetEnvVarInt("LIBGL_BLITFULLSCREEN", &globals4es.blitfullscreen, globals4es.blitfullscreen);
    if (globals4es.blitfullscreen)
        SHUT_LOGD("Hack to trigger a SwapBuffers when a Full Framebuffer Blit on default FBO is done");

    env(LIBGL_COMMENTS, globals4es.comments, "Keep comments in converted Shaders");

    env(LIBGL_NOARBPROGRAM, globals4es.noarbprogram, "Not exposing ARB Program extensions");

    if (hardext.npot == 3)
        globals4es.defaultwrap = 0;
    else
        globals4es.defaultwrap = 1;

    if (GetEnvVarInt("LIBGL_DEFAULTWRAP", &globals4es.defaultwrap, (hardext.npot == 3) ? 0 : 1)) {
        switch (globals4es.defaultwrap) {
        case 0:
            SHUT_LOGD("Default wrap mode is GL_REPEAT");
            break;
        case 1:
            SHUT_LOGD("Default wrap mode is GL_CLAMP_TO_EDGE");
            break;
        case 2:
        default:
            globals4es.defaultwrap = 2;
            SHUT_LOGD("Default wrap mode is GL_CLAMP_TO_EDGE, enforced");
            break;
        }
    }

    GetEnvVarBool("LIBGL_NOTEXARRAY", &globals4es.notexarray, 0);
    if (globals4es.notexarray) {
        SHUT_LOGD("No Texture Array in Shaders");
    }

    env(LIBGL_LOGSHADERERROR, globals4es.logshader, "Log errors compiling shaders to the console");
    env(LIBGL_SHADERNOGLES, globals4es.shadernogles, "Remove GLES part in shader");
    env(LIBGL_NOES2COMPAT, globals4es.noes2, "Don't expose GLX_EXT_create_context_es2_profile extension");
    env(LIBGL_NORMALIZE, globals4es.normalize, "Force normals to be normalized on FPE shaders");

    globals4es.dbgshaderconv = ReturnEnvVarIntDef("LIBGL_DBGSHADERCONV", 0);
    if (globals4es.dbgshaderconv) {
        if (globals4es.dbgshaderconv == 1) globals4es.dbgshaderconv = 15;
        if (!(globals4es.dbgshaderconv & 3))  // neither vertex or fragment
            globals4es.dbgshaderconv |= 3;    // select both
        if (!(globals4es.dbgshaderconv & 12)) // neither before or after
            globals4es.dbgshaderconv |= 12;   // select both
        SHUT_LOGD_NOPREFIX("Log all shaders, before and after conversion, to the console: ");
        if (globals4es.dbgshaderconv & 4) SHUT_LOGD_NOPREFIX("Before  ");
        if (globals4es.dbgshaderconv & 8) SHUT_LOGD_NOPREFIX("After  ");
        if (globals4es.dbgshaderconv & 1) SHUT_LOGD_NOPREFIX("Vertex  ");
        if (globals4es.dbgshaderconv & 2) SHUT_LOGD_NOPREFIX("Fragment  ");
        SHUT_LOGD_NOPREFIX("");
    }

    // VGPU pipeline stuff
    env(LIBGL_VGPU_DUMP, globals4es.vgpu_dump, "Dump the content of VGPU shader conversion");
    env(LIBGL_VGPU_FORCE, globals4es.vgpu_force_conv, "Force VGPU pipeline to convert every shader")
        env(LIBGL_VGPU_BACKPORT, globals4es.vgpu_backport, "Attempt HARD to backport shaders to #version 100")
            globals4es.vgpu_precision = ReturnEnvVarIntDef("LIBGL_VGPU_PRECISION", 0);
    if (globals4es.vgpu_precision != 0) {
        SHUT_LOGD("VGPU default precision overridden ! (%i)", globals4es.vgpu_precision);
    }

    env(LIBGL_NOCLEAN, globals4es.noclean, "Don't clean Context when destroyed");

    globals4es.glxrecycle = 1;
#ifndef NOEGL
    if ((globals4es.usepbuffer) || (globals4es.usefb)) globals4es.glxrecycle = 0;

    int env_glxrecycle = ReturnEnvVarIntDef("LIBGL_GLXRECYCLE", -1);
    if (globals4es.glxrecycle && env_glxrecycle == 0 && !((globals4es.usepbuffer) || (globals4es.usefb))) {
        globals4es.glxrecycle = 0;
        SHUT_LOGD("glX Will NOT try to recycle EGL Surface");
    }
    if (env_glxrecycle == 1) globals4es.glxrecycle = 1;
    if (globals4es.glxrecycle) {
        SHUT_LOGD("glX Will try to recycle EGL Surface");
    }
    env(LIBGL_GLXNATIVE, globals4es.glxnative, "Don't filter GLXConfig with GLX_X_NATIVE_TYPE");
#endif
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) SHUT_LOGD("Current folder is:%s", cwd);

    if (hardext.shader_fbfetch) {
        env(LIBGL_SHADERBLEND, globals4es.shaderblend, "Blend will be handled in shaders");
    }
    if (hardext.prgbin_n > 0 && !globals4es.notexarray) {
        env(LIBGL_NOPSA, globals4es.nopsa, "Don't use PrecompiledShaderArchive");
        if (globals4es.nopsa == 0) {
            cwd[0] = '\0';
            // TODO: What to do on ANDROID and EMSCRIPTEN?
            const char* custom_psa = GetEnvVar("LIBGL_PSA_FOLDER");
#ifdef __linux__
            const char* home = GetEnvVar("HOME");
            if (custom_psa)
                strcpy(cwd, custom_psa);
            else if (home)
                strcpy(cwd, home);
            if (strlen(cwd))
                if (cwd[strlen(cwd)] != '/') strcat(cwd, "/");
#elif defined AMIGAOS4
            if (custom_psa)
                strcpy(cwd, custom_psa);
            else
                strcpy(cwd, "PROGDIR:");
#endif
            if (strlen(cwd)) {
                strcat(cwd, ".gl4es.psa");
                fpe_InitPSA(cwd);
                fpe_readPSA();
            }
        }
    } else
        SHUT_LOGD("Not using PSA (prgbin_n=%d, notexarray=%d)", hardext.prgbin_n, globals4es.notexarray);

    env(LIBGL_SKIPTEXCOPIES, globals4es.skiptexcopies, "Texture Copies will be skipped");
    if (GetEnvVarFloat("LIBGL_FB_TEX_SCALE", &globals4es.fbtexscale, 0.0f)) {
        SHUT_LOGD("Framebuffer Textures will be scaled by %.2f", globals4es.fbtexscale);
    }
}

#ifndef NOX11
void FreeFBVisual();
#endif
#ifdef NO_INIT_CONSTRUCTOR
EXPORT
#else
#ifdef BUILD_WINDOWS_DLL
EXPORT // symmetric for init -- trivialize application code
#endif
#if !defined(_MSC_VER) || defined(__clang__)
    __attribute__((destructor))
#endif
#endif
void close_gl4es() {
#ifdef GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB
    SHUT_LOGD("Shutdown requested");
    if (--inited) return;
#endif
    config_cleanup();
    SHUT_LOGD("Shutting down");
#ifndef NOX11
    FreeFBVisual();
#endif
    gl_close();
    fpe_writePSA();
    fpe_FreePSA();
#if defined(GL4ES_COMPILE_FOR_USE_IN_SHARED_LIB) && defined(AMIGAOS4)
    os4CloseLib();
#endif
}

#ifdef BUILD_WINDOWS_DLL
#if !defined(_MSC_VER) || defined(__clang__)
__attribute__((constructor(103)))
#endif
void dll_init_done() {
    dll_inited = 1;
}
#endif

#if defined(_MSC_VER) && !defined(NO_INIT_CONSTRUCTOR) && !defined(__clang__)
#pragma const_seg(".CRT$XCU")
void (*const gl4es_ctors[])() = {initialize_gl4es, dll_init_done};
#pragma const_seg(".CRT$XTX")
void (*const gl4es_dtor)() = close_gl4es;
#pragma const_seg()
#endif
