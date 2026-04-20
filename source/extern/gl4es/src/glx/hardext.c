#include "hardext.h"

#include "../gl/debug.h"
#include "../gl/gl4es.h"
#include "../gl/init.h"
#include "../gl/logs.h"
#include "../gl/loader.h"
#include "GLES3/gl32.h"
#ifndef ANDROID
#include "rpi.h"
#endif
#include "glx_gbm.h"

#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

static int tested = 0;

hardext_t hardext = {0};

int testGenericShader(struct shader_s* shader_source) {
    // check the current shader is valid for compilation
    LOAD_GLES2(glCreateShader);
    LOAD_GLES2(glShaderSource);
    LOAD_GLES2(glCompileShader);
    LOAD_GLES2(glGetShaderiv);
    LOAD_GLES2(glDeleteShader);

    GLuint shad = gles_glCreateShader(shader_source->type);
    gles_glShaderSource(shad, 1, (const GLchar* const*)(&shader_source->converted), NULL);
    gles_glCompileShader(shad);
    GLint compiled;
    gles_glGetShaderiv(shad, GL_COMPILE_STATUS, &compiled);
    /*
    if(!compiled) {
        LOAD_GLES2(glGetShaderInfoLog)
        char buff[500];
        gles_glGetShaderInfoLog(shad, 500, NULL, buff);
        SHUT_LOGD("LIBGL: \"%s\" failed, message:\n%s\n", version, buff);
    }
    */
    gles_glDeleteShader(shad);

    return compiled;
}

static int testGLSL(const char* version, int uniformLoc) {
    // check if glsl 120 shaders are supported... by compiling one !
    LOAD_GLES2(glCreateShader);
    LOAD_GLES2(glShaderSource);
    LOAD_GLES2(glCompileShader);
    LOAD_GLES2(glGetShaderiv);
    LOAD_GLES2(glDeleteShader);

    GLuint shad = gles_glCreateShader(GL_VERTEX_SHADER);
    const char* shadTest[4] = {version,
                               "\n"
                               "layout(location = 0) in vec4 vecPos;\n",
                               uniformLoc ? "layout(location = 0) uniform mat4 matMVP;\n" : "uniform mat4 matMVP;\n",
                               "void main() {\n"
                               " gl_Position = matMVP * vecPos;\n"
                               "}\n"};
    gles_glShaderSource(shad, 4, shadTest, NULL);
    gles_glCompileShader(shad);
    GLint compiled;
    gles_glGetShaderiv(shad, GL_COMPILE_STATUS, &compiled);
    /*
    if(!compiled) {
        LOAD_GLES2(glGetShaderInfoLog)
        char buff[500];
        gles_glGetShaderInfoLog(shad, 500, NULL, buff);
        printf("LIBGL: \"%s\" failed, message:\n%s", version, buff);
    }
    */
    gles_glDeleteShader(shad);

    return compiled;
}

static int testTextureCubeLod() {
    LOAD_GLES2(glCreateShader);
    LOAD_GLES2(glShaderSource);
    LOAD_GLES2(glCompileShader);
    LOAD_GLES2(glGetShaderiv);
    LOAD_GLES2(glDeleteShader);
    LOAD_GLES(glGetError);

    GLuint shad = gles_glCreateShader(GL_FRAGMENT_SHADER);
    const char* shadTest[3] = {"#version 100",
                               "\n"
                               "#extension GL_EXT_shader_texture_lod : enable\n"
                               "uniform samplerCube samCube;\n"
                               "varying mediump vec3 coordCube;\n",
                               "void main() {\n"
                               " gl_FragColor = textureCubeLod(samCube, coordCube, 0.0);\n"
                               "}\n"};
    gles_glShaderSource(shad, 3, shadTest, NULL);
    gles_glCompileShader(shad);
    GLint compiled;
    gles_glGetShaderiv(shad, GL_COMPILE_STATUS, &compiled);
    gles_glDeleteShader(shad);
    gles_glGetError(); // reset GL Error

    return compiled;
}

EXPORT
void GetHardwareExtensions(int notest) {
    if (tested) return;
    // put some default values
    hardext.maxtex = 2;
    hardext.maxsize = 2048;
    hardext.maxlights = 8;
    hardext.maxplanes = 6;
    hardext.maxdrawbuffers = 1;

    if (notest) {
#ifndef AMIGAOS4
        SHUT_LOGD("Hardware test disabled, nothing activated...");
#endif
        if (globals4es.es >= 2) {
            hardext.maxteximage = 4;
            hardext.maxvarying = 8;
            hardext.maxtex = 8;
            hardext.maxvattrib = 16;
#ifdef AMIGAOS4
            hardext.npot = 3;
#else
            hardext.npot = 1;
#endif
            hardext.fbo = 1;
            hardext.blendcolor = 1;
            hardext.blendsub = 1;
            hardext.blendfunc = 1;
            hardext.blendeq = 1;
            hardext.mirrored = 1;
            hardext.pointsprite = 1;
            hardext.pointsize = 1;
            hardext.cubemap = 1;
            hardext.maxdrawbuffers = 1;
#ifdef AMIGAOS4
            hardext.glsl300es = 1;
#endif
        }
        return;
    }
#if defined(BCMHOST) && !defined(ANDROID)
    rpi_init();
#endif
#ifdef NOEGL
    SHUT_LOGD("Hardware test on current Context...");
#else
    // used EGL & GLES functions
    LOAD_EGL(eglBindAPI);
    LOAD_EGL(eglInitialize);
    LOAD_EGL(eglGetDisplay);
    LOAD_EGL(eglCreatePbufferSurface);
    LOAD_EGL(eglDestroySurface);
    LOAD_EGL(eglDestroyContext);
    LOAD_EGL(eglMakeCurrent);
    LOAD_EGL(eglChooseConfig);
    LOAD_EGL(eglCreateContext);
    LOAD_EGL(eglQueryString);
    LOAD_EGL(eglTerminate);

    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLContext eglContext;

    SHUT_LOGD("Using GLES %s backend", (hardext.esversion == 1) ? "1.1" : (hardext.esversion == 2) ? "2.0" : "3.x");

    // Create a PBuffer first...
    EGLint egl_context_attrib_es2[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    EGLint egl_context_attrib[] = {EGL_NONE};

    // let's create a PBuffer attributes
    EGLint egl_attribs[10]; // should be enough
    int i = 0;
    egl_attribs[i++] = EGL_WIDTH;
    egl_attribs[i++] = 32;
    egl_attribs[i++] = EGL_HEIGHT;
    egl_attribs[i++] = 32;
    egl_attribs[i++] = EGL_NONE;

    EGLint configAttribs[] = {
#ifdef PANDORA
        // on the Pandora, there don't seem to exist a 8888 PBuffer config for GLES2.
        EGL_RED_SIZE,
        (hardext.esversion == 1) ? 8 : 5,
        EGL_GREEN_SIZE,
        (hardext.esversion == 1) ? 8 : 6,
        EGL_BLUE_SIZE,
        (hardext.esversion == 1) ? 8 : 5,
        EGL_ALPHA_SIZE,
        (hardext.esversion == 1) ? 8 : 0,
#else
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
#endif
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        (hardext.esversion == 1) ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES2_BIT,
        EGL_NONE};

    int configsFound;
    static EGLConfig pbufConfigs[1];

#ifndef NO_GBM
    if (globals4es.usegbm) {
        LoadGBMFunctions();
        eglDisplay = OpenGBMDisplay(EGL_DEFAULT_DISPLAY);
    } else
#endif
        eglDisplay = egl_eglGetDisplay(EGL_DEFAULT_DISPLAY);

    egl_eglBindAPI(EGL_OPENGL_ES_API);
    if (egl_eglInitialize(eglDisplay, NULL, NULL) != EGL_TRUE) {
        LOGE("Error while gathering supported extension (eglInitialize: %s), default to none", PrintEGLError(0));
        egl_eglTerminate(eglDisplay);
        return;
    }

    egl_eglChooseConfig(eglDisplay, configAttribs, pbufConfigs, 1, &configsFound);
#ifndef NO_GBM
    const char* eglExts = egl_eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (eglExts && strstr(eglExts, "EGL_KHR_platform_gbm ")) {
        SHUT_LOGD("GBM Surfaces supported%s", globals4es.usegbm ? " and used" : "");
        hardext.gbm = 1;
    }
#endif
#ifndef PANDORA
    if (!configsFound) {
        // try without alpha channel
        configAttribs[4 * 2 - 1] = 0;
        egl_eglChooseConfig(eglDisplay, configAttribs, pbufConfigs, 1, &configsFound);
        if (configsFound) {
            SHUT_LOGD("No alpha channel in PBuffer context, disabling EGL Alpha channel...");
            hardext.eglnoalpha = 1;
        }
    }
#endif
    if (!configsFound) {
        SHUT_LOGE("Error while gathering supported extension (eglChooseConfig: %s), default to none", PrintEGLError(0));
        egl_eglTerminate(eglDisplay);
        return;
    }
    eglContext = egl_eglCreateContext(eglDisplay, pbufConfigs[0], EGL_NO_CONTEXT,
                                      (hardext.esversion == 1) ? egl_context_attrib : egl_context_attrib_es2);
    if (!eglContext) {
        SHUT_LOGE("Error while gathering supported extension (eglCreateContext: %s), default to none",
                  PrintEGLError(0));
        return;
    }
    eglSurface = egl_eglCreatePbufferSurface(eglDisplay, pbufConfigs[0], egl_attribs);
    if (!eglSurface) {
        SHUT_LOGE("Error while gathering supported extension (eglCreatePBufferSurface: %s), default to none",
                  PrintEGLError(0));
        egl_eglDestroyContext(eglDisplay, eglContext);
        egl_eglTerminate(eglDisplay);
        return;
    }
    egl_eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
#endif
    tested = 1;
    LOAD_GLES(glGetString);
    LOAD_GLES(glGetIntegerv);
    LOAD_GLES(glGetError);

    // gles version detection
    const char* verStr = (const char*)gles_glGetString(GL_VERSION);
    if (verStr) {
        sscanf(verStr, "OpenGL ES %d", &hardext.esversion);
    } else {
        hardext.esversion = globals4es.es;
    }

    if (hardext.esversion != globals4es.es) {
        SHUT_LOGD("Real GLES major version: %d", hardext.esversion);
        globals4es.es = hardext.esversion;
    }

    // Now get extensions
    const char* Exts = (const char*)gles_glGetString(GL_EXTENSIONS);
// Parse them!
#define S(A, B, C)                                                                                                     \
    if (strstr(Exts, A)) {                                                                                             \
        hardext.B = 1;                                                                                                 \
        SHUT_LOGD("Extension %s detected%s", A, C ? " and used" : "");                                                 \
    }
    if (hardext.esversion > 1) hardext.npot = 1;
    if (strstr(Exts, "GL_APPLE_texture_2D_limited_npot ")) hardext.npot = 1;
    if (strstr(Exts, "GL_IMG_texture_npot "))
        hardext.npot =
            1; // it should enable mipmap (so hardext.npot=2), but mipmap (so level > 0) needs to be POT-sized?!!
    if (strstr(Exts, "GL_ARB_texture_non_power_of_two ") || strstr(Exts, "GL_OES_texture_npot ")) hardext.npot = 3;
    if (hardext.npot > 0) {
        SHUT_LOGD("Hardware %s NPOT detected and used",
                  hardext.npot == 3 ? "Full" : (hardext.npot == 2 ? "Limited+Mipmap" : "Limited"));
    }
    S("GL_EXT_blend_minmax ", blendminmax, 1);
    if (hardext.esversion > 2) {
        SHUT_LOGD("Extension GL_EXT_draw_buffers is in core ES3, and so used");
        hardext.drawbuffers = 1;
    } else {
        S("GL_EXT_draw_buffers ", drawbuffers, 1);
    }
    /*if(hardext.blendcolor==0) {
        // try by just loading the function
        LOAD_GLES_OR_OES(glBlendColor);
        if(gles_glBlendColor != NULL) {
            hardext.blendcolor = 1;
            SHUT_LOGD("Extension glBlendColor found and used");
        }
    }*/ // I don't think this is correct
    if (hardext.esversion < 2) {
        S("GL_OES_framebuffer_object ", fbo, 1);
        S("GL_OES_point_sprite ", pointsprite, 1);
        S("GL_OES_point_size_array ", pointsize, 0);
        S("GL_OES_texture_cube_map ", cubemap, 1);
        S("GL_EXT_blend_color ", blendcolor, 1);
        S("GL_OES_blend_subtract ", blendsub, 1);
        S("GL_OES_blend_func_separate ", blendfunc, 1);
        S("GL_OES_blend_equation_separate ", blendeq, 1);
        S("GL_OES_texture_mirrored_repeat ", mirrored, 1);
    } else {
        hardext.fbo = 1;
        SHUT_LOGD("FBO are in core, and so used");
        hardext.pointsprite = 1;
        SHUT_LOGD("PointSprite are in core, and so used");
        hardext.pointsize = 1;
        SHUT_LOGD("CubeMap are in core, and so used");
        hardext.cubemap = 1;
        SHUT_LOGD("BlendColor is in core, and so used");
        hardext.blendcolor = 1;
        SHUT_LOGD("Blend Subtract is in core, and so used");
        hardext.blendsub = 1;
        SHUT_LOGD("Blend Function and Equation Separation is in core, and so used");
        hardext.blendfunc = 1;
        hardext.blendeq = 1;
        SHUT_LOGD("Texture Mirrored Repeat is in core, and so used");
        hardext.mirrored = 1;
    }
    S("GL_OES_mapbuffer ", mapbuffer, 0);
    S("GL_OES_element_index_uint ", elementuint, 1);
    S("GL_OES_packed_depth_stencil ", depthstencil, 1);
    S("GL_OES_depth24 ", depth24, 1);
    S("GL_OES_rgb8_rgba8 ", rgba8, 1);
    S("GL_EXT_blend_func_extended ", blend_func_extended, 0);
    S("GL_EXT_multi_draw_arrays ", multidraw, 0);
    if (!globals4es.nobgra) {
        // S("GL_EXT_texture_format_BGRA8888 ", bgra8888, 1);
        hardext.bgra8888 = false;
        LOGD("Extension GL_EXT_texture_format_BGRA8888 never used")
    }
    if (!globals4es.nodepthtex) {
        S("GL_OES_depth_texture ", depthtex, 1);
        S("GL_OES_texture_stencil8 ", stenciltex, 1);
    }
    S("GL_OES_draw_texture ", drawtex, 1);
    S("GL_EXT_texture_rg ", rgtex, 1);
    if (globals4es.floattex) {
        S("GL_OES_texture_float ", floattex, 1);
        S("GL_OES_texture_half_float ", halffloattex, 1);
        S("GL_EXT_color_buffer_float ", floatfbo, 1);
        S("GL_EXT_color_buffer_half_float ", halffloatfbo, 1);
    }
    S("GL_AOS4_texture_format_RGB332", rgb332, 0);
    S("GL_AOS4_texture_format_RGB332REV", rgb332rev, 0);
    S("GL_AOS4_texture_format_RGBA1555REV", rgba1555rev, 1);
    S("GL_AOS4_texture_format_RGBA8888", rgba8888, 1);
    S("GL_AOS4_texture_format_RGBA8888REV", rgba8888rev, 1);

    if (hardext.esversion > 1) {
        if (!globals4es.nohighp) {
            S("GL_OES_fragment_precision_high ", highp, 1);
            if (!hardext.highp) {
                // check if highp is supported anyway
                LOAD_GLES2(glGetShaderPrecisionFormat);
                if (gles_glGetShaderPrecisionFormat) {
                    GLint range[2] = {0};
                    GLint precision = 0;
                    gles_glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
                    if (!(range[0] == 0 && range[1] == 0 && precision == 0)) {
                        hardext.highp = 2; // no need to declare #entension here
                        SHUT_LOGD("high precision float in fragment shader available and used");
                    }
                }
            }
        }
        if (!globals4es.noshaderlod) S("GL_EXT_shader_texture_lod", shaderlod, 1);
        if (hardext.shaderlod) {
            // test is textureCubeLod need EXT or not (seems to be a bug in some PVR driver)
            if (testTextureCubeLod()) {
                hardext.cubelod = 1;
                SHUT_LOGD("textureCubeLod in fragment doesn't need trailing EXT");
            }
        }
        S("GL_EXT_frag_depth ", fragdepth, 1);
        gles_glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &hardext.maxvattrib);
        SHUT_LOGD("Max vertex attrib: %d", hardext.maxvattrib);
        S("GL_OES_standard_derivatives ", derivatives, 1);
        S("GL_ARM_shader_framebuffer_fetch", shader_fbfetch, 1);
        S("GL_OES_get_program ", prgbinary, 1);
        if (!hardext.prgbinary) {
            S("GL_OES_get_program_binary ", prgbinary, 1);
        }
        if (hardext.prgbinary) {
            gles_glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS_OES, &hardext.prgbin_n);
            SHUT_LOGD("Number of supported Program Binary Format: %d", hardext.prgbin_n);
        }
        S("GL_EXT_draw_elements_base_vertex", basevertex, 1);
        int versionMajor, versionMinor;
        gles_glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
        gles_glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

        if (versionMajor > 3 || (versionMajor == 3 && versionMinor >= 2)) { // GLES 3.2+
            hardext.basevertex = 1;
        }
    }

    // Now get some max stuffs
    gles_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &hardext.maxsize);
    SHUT_LOGD("Max texture size: %d", hardext.maxsize);
    gles_glGetIntegerv((hardext.esversion == 1) ? GL_MAX_TEXTURE_UNITS : GL_MAX_TEXTURE_IMAGE_UNITS, &hardext.maxtex);
    if (hardext.esversion == 1) {
        gles_glGetIntegerv(GL_MAX_LIGHTS, &hardext.maxlights);
        gles_glGetIntegerv(GL_MAX_CLIP_PLANES, &hardext.maxplanes);
        hardext.maxteximage = hardext.maxtex;
    } else {
        // simulated stuff using the FPE
        hardext.maxlights = 8;
        hardext.maxplanes = 6;
        gles_glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &hardext.maxteximage);
        gles_glGetIntegerv(GL_MAX_VARYING_VECTORS, &hardext.maxvarying);
        SHUT_LOGD("Max Varying Vector: %d", hardext.maxvarying);
        if (hardext.maxvattrib < 16 && hardext.maxtex > 4)
            hardext.maxtex = 4; // with less then 16 vertexattrib, more then 4 textures seems unreasonnable
    }
    int hardmaxtex = hardext.maxtex;
    if (hardext.maxtex > MAX_TEX) hardext.maxtex = MAX_TEX; // caping, as there are some fixed-sized array...
    if (hardext.maxteximage > MAX_TEX) hardext.maxteximage = MAX_TEX;
    if (hardext.maxlights > MAX_LIGHT) hardext.maxlights = MAX_LIGHT; // caping lights too
    if (hardext.maxplanes > MAX_CLIP_PLANES)
        hardext.maxplanes = MAX_CLIP_PLANES; // caping planes, even 6 should be the max supported anyway
    SHUT_LOGD("Texture Units: %d/%d (hardware: %d), Max lights: %d, Max planes: %d", hardext.maxtex,
              hardext.maxteximage, hardmaxtex, hardext.maxlights, hardext.maxplanes);
    S("GL_EXT_texture_filter_anisotropic ", aniso, 1);
    if (hardext.aniso) {
        gles_glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &hardext.aniso);
        if (gles_glGetError() != GL_NO_ERROR) hardext.aniso = 0;
        if (hardext.aniso) SHUT_LOGD("Max Anisotropic filtering: %d", hardext.aniso);
    }
    // get GLES driver signatures...
    const char* vendor = (const char*)gles_glGetString(GL_VENDOR);
    SHUT_LOGD("Hardware vendor is %s", vendor);
    if (strstr(vendor, "ARM"))
        hardext.vendor = VEND_ARM;
    else if (strstr(vendor, "Imagination Technologies"))
        hardext.vendor = VEND_IMGTEC;
    if (hardext.esversion > 1) {
        if (testGLSL("#version 120", 1)) hardext.glsl120 = 1;
        if (testGLSL("#version 300 es", 0)) hardext.glsl300es = 1;
        if (testGLSL("#version 310 es", 1)) hardext.glsl310es = 1;
        if (testGLSL("#version 320 es", 1)) hardext.glsl320es = 1;
    }
    if (hardext.glsl120) {
        SHUT_LOGD("GLSL 120 supported and used");
    }
    if (hardext.glsl300es) {
        SHUT_LOGD("GLSL 300 es supported%s", (hardext.glsl120 || hardext.glsl310es) ? "" : " and used");
    }
    if (hardext.glsl310es) {
        SHUT_LOGD("GLSL 310 es supported%s", hardext.glsl310es ? "" : " and used");
    }
    if (hardext.glsl320es) {
        SHUT_LOGD("GLSL 320 es supported%s", hardext.glsl320es ? "" : " and used");
    }
    if (!hardext.glsl300es) {
        // We can't use the vgpu forward conversion
        globals4es.vgpu_backport = 1;
    }

    if (hardext.drawbuffers) {
        gles_glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &hardext.maxcolorattach);
        gles_glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &hardext.maxdrawbuffers);
    }
    if (hardext.maxcolorattach < 1) hardext.maxcolorattach = 1;
    if (hardext.maxcolorattach > MAX_DRAW_BUFFERS) hardext.maxcolorattach = MAX_DRAW_BUFFERS;
    if (hardext.maxdrawbuffers < 1) hardext.maxdrawbuffers = 1;
    if (hardext.maxdrawbuffers > MAX_DRAW_BUFFERS) hardext.maxdrawbuffers = MAX_DRAW_BUFFERS;
    SHUT_LOGD("Max Color Attachments: %d / Draw buffers: %d", hardext.maxdrawbuffers, hardext.maxcolorattach);
#ifndef NOEGL
    if (strstr(egl_eglQueryString(eglDisplay, EGL_EXTENSIONS), "EGL_KHR_gl_colorspace")) {
        SHUT_LOGD("sRGB surface supported");
        hardext.srgb = 1;
    }
    if (strstr(egl_eglQueryString(eglDisplay, EGL_EXTENSIONS), "EGL_KHR_image_pixmap")) {
        SHUT_LOGD("EGLImage from Pixmap supported");
        hardext.khr_pixmap = 1;
    }
    if (strstr(egl_eglQueryString(eglDisplay, EGL_EXTENSIONS), "EGL_KHR_gl_texture_2D_image")) {
        SHUT_LOGD("EGLImage to Texture2D supported");
        hardext.khr_texture_2d = 1;
    }
    if (strstr(egl_eglQueryString(eglDisplay, EGL_EXTENSIONS), "EGL_KHR_gl_renderbuffer_image")) {
        SHUT_LOGD("EGLImage to RenderBuffer supported");
        hardext.khr_renderbuffer = 1;
    }

    // End, cleanup
    egl_eglMakeCurrent(eglDisplay, 0, 0, EGL_NO_CONTEXT);
    egl_eglDestroySurface(eglDisplay, eglSurface);
    egl_eglDestroyContext(eglDisplay, eglContext);

    egl_eglTerminate(eglDisplay);
#endif
}
