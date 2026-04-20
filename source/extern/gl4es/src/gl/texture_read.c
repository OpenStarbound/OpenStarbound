#include "texture.h"

#include "../glx/hardext.h"
#include "../glx/streaming.h"
#include "array.h"
#include "blit.h"
#include "decompress.h"
#include "debug.h"
#include "enum_info.h"
#include "fpe.h"
#include "framebuffers.h"
#include "GL/gl.h"
#include "init.h"
#include "loader.h"
#include "matrix.h"
#include "pixel.h"
#include "raster.h"

// #define DEBUG
#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

static int inline nlevel(int size, int level) {
    if (size) {
        size >>= level;
        if (!size) size = 1;
    }
    return size;
}

static int is_depth_format(GLenum format) {
    switch (format) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
        return 1;
    default:
        return 0;
    }
}

static GLenum get_binding_for_target(GLenum target) {
    switch (target) {
    case GL_TEXTURE_2D:
        return GL_TEXTURE_BINDING_2D;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return GL_TEXTURE_BINDING_CUBE_MAP;
    default:
        return 0;
    }
}

void APIENTRY_GL4ES gl4es_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y,
                                           GLsizei width, GLsizei height, GLint border) {
    DBG(SHUT_LOGD("glCopyTexImage2D(%s, %i, %s, %i, %i, %i, %i, %i), glstate->fbo.current_fb=%p\n", PrintEnum(target),
                  level, PrintEnum(internalformat), x, y, width, height, border, glstate->fbo.current_fb);)
    // PUSH_IF_COMPILING(glCopyTexImage2D);
    FLUSH_BEGINEND;
    const GLuint itarget = what_target(target);

    // actually bound if targeting shared TEX2D
    realize_bound(glstate->texture.active, target);

    if (globals4es.skiptexcopies) {
        DBG(SHUT_LOGD("glCopyTexImage2D skipped.\n"));
        return;
    }

    errorGL();

    // "Unmap" if buffer mapped...
    glbuffer_t* pack = glstate->vao->pack;
    glbuffer_t* unpack = glstate->vao->unpack;
    glstate->vao->pack = NULL;
    glstate->vao->unpack = NULL;

    readfboBegin(); // multiple readfboBegin() can be chained...
    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];

    if (glstate->fbo.current_fb->read_type == 0) {
        LOAD_GLES(glGetIntegerv);
        gles_glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, (GLint*)&glstate->fbo.current_fb->read_format);
        gles_glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, (GLint*)&glstate->fbo.current_fb->read_type);
    }

    int copytex = (!is_depth_format(internalformat)) || (bound->format == glstate->fbo.current_fb->read_format &&
                                                         bound->type == glstate->fbo.current_fb->read_type);

    if (copytex) {
        GLenum fmt;
        switch (internalformat) {
        case GL_ALPHA:
        case GL_ALPHA8:
            fmt = GL_ALPHA;
            break;
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            fmt = GL_LUMINANCE;
            break;
        case GL_LUMINANCE_ALPHA:
        case GL_LUMINANCE8_ALPHA8:
            fmt = GL_LUMINANCE_ALPHA;
            break;
        case GL_RGB:
        case 3:
            fmt = GL_RGB;
            break;
        default:
            fmt = GL_RGBA;
        }
        LOAD_GLES(glCopyTexImage2D);
        gles_glCopyTexImage2D(target, level, fmt, x, y, width, height, border);
    } else {
        DBG(SHUT_LOGD(" -No use of gles_glCopyTexImage2D"))

        LOAD_GLES(glTexImage2D)
        LOAD_GLES(glGetIntegerv)
        LOAD_GLES(glGenFramebuffers)
        LOAD_GLES(glBindFramebuffer)
        LOAD_GLES(glFramebufferTexture2D)
        LOAD_GLES(glCheckFramebufferStatus)
        LOAD_GLES(glDeleteFramebuffers)
        LOAD_GLES(glBlitFramebuffer)
        GLenum format = GL_DEPTH_COMPONENT;
        GLenum type = GL_UNSIGNED_INT;
        internal2format_type(&internalformat, &format, &type);
        gles_glTexImage2D(target, level, (GLint)internalformat, width, height, border, format, type, NULL);
        GLint prevDrawFBO;
        gles_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
        GLuint tempDrawFBO;
        gles_glGenFramebuffers(1, &tempDrawFBO);
        gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);
        GLint currentTex;
        gles_glGetIntegerv(get_binding_for_target(target), &currentTex);
        gles_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);

        if (gles_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            gles_glDeleteFramebuffers(1, &tempDrawFBO);
            gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            return;
        }

        gles_glBlitFramebuffer(x, y, x + width, y + height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        gles_glDeleteFramebuffers(1, &tempDrawFBO);
    }

    readfboEnd();
    // "Remap" if buffer mapped...
    glstate->vao->pack = pack;
    glstate->vao->unpack = unpack;
}

void APIENTRY_GL4ES gl4es_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x,
                                              GLint y, GLsizei width, GLsizei height) {
    const GLuint itarget = what_target(target);
    // WARNING: It seems glColorMask has an impact on what channel are actually copied by this. The crude glReadPixel /
    // glTexSubImage cannot emulate that, and proper emulation will take need 2 read pixels.
    //  And using the real glCopyTexSubImage2D needs that the FrameBuffer were data are read is compatible with the
    //  Texture it's copied to...
    DBG(SHUT_LOGD("glCopyTexSubImage2D(%s, %i, %i, %i, %i, %i, %i, %i), bounded texture=%u format/type=%s, %s\n",
                  PrintEnum(target), level, xoffset, yoffset, x, y, width, height,
                  (glstate->texture.bound[glstate->texture.active][itarget])
                      ? glstate->texture.bound[glstate->texture.active][itarget]->texture
                      : 0,
                  PrintEnum((glstate->texture.bound[glstate->texture.active][itarget])
                                ? glstate->texture.bound[glstate->texture.active][itarget]->format
                                : 0),
                  PrintEnum((glstate->texture.bound[glstate->texture.active][itarget])
                                ? glstate->texture.bound[glstate->texture.active][itarget]->type
                                : 0));)
    // PUSH_IF_COMPILING(glCopyTexSubImage2D);
    FLUSH_BEGINEND;

    if (globals4es.skiptexcopies) {
        DBG(SHUT_LOGD("glCopyTexSubImage2D skipped.\n"));
        return;
    }

    LOAD_GLES(glCopyTexSubImage2D);
    errorGL();
    realize_bound(glstate->texture.active, target);

    // "Unmap" if buffer mapped...
    glbuffer_t* pack = glstate->vao->pack;
    glbuffer_t* unpack = glstate->vao->unpack;
    glstate->vao->pack = NULL;
    glstate->vao->unpack = NULL;

    readfboBegin(); // multiple readfboBegin() can be chained...

    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];
#ifdef TEXSTREAM
    if (bound->streamed) {
        void* buff = GetStreamingBuffer(bound->streamingID);
        if ((bound->width == width) && (bound->height == height) && (xoffset == 0) && (yoffset == 0)) {
            gl4es_glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buff);
        } else {
            void* tmp = malloc(width * height * 2);
            gl4es_glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tmp);
            for (int y = 0; y < height; y++) {
                memcpy(buff + ((yoffset + y) * bound->width + xoffset) * 2, tmp + y * width * 2, width * 2);
            }
            free(tmp);
        }
    } else
#endif
    {
        int copytex = 0;
        if (glstate->fbo.current_fb->read_type == 0) {
            LOAD_GLES(glGetIntegerv);
            gles_glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, (GLint*)&glstate->fbo.current_fb->read_format);
            gles_glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, (GLint*)&glstate->fbo.current_fb->read_type);
        }

        LOAD_GLES2(glGetTexLevelParameteriv)
        GLint internalFormat;
        gles_glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        copytex = (!is_depth_format(internalFormat)) || (bound->format == glstate->fbo.current_fb->read_format &&
                                                         bound->type == glstate->fbo.current_fb->read_type);
        if (copytex || !glstate->colormask[0] || !glstate->colormask[1] || !glstate->colormask[2] ||
            !glstate->colormask[3]) {
            gles_glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
            if (((((bound->max_level == level) && (level || bound->mipmap_need)) &&
                  (GL4ES_AUTOMIPMAP_PLACEHOLDER != 3) && (bound->mipmap_need != 0))) &&
                !(bound->max_level == bound->base_level && bound->base_level == 0)) {
                LOAD_GLES2_OR_OES(glGenerateMipmap);
                if (gles_glGenerateMipmap) {
                    gles_glGenerateMipmap(to_target(itarget));
                }
            }
        } else {
            DBG(SHUT_LOGD(" -No use of gles_glCopyTexSubImage2D");)
            LOAD_GLES2(glGetIntegerv)
            LOAD_GLES2(glGenFramebuffers)
            LOAD_GLES2(glBindFramebuffer)
            LOAD_GLES2(glFramebufferTexture2D)
            LOAD_GLES2(glCheckFramebufferStatus)
            LOAD_GLES2(glDeleteFramebuffers)
            LOAD_GLES2(glBlitFramebuffer)

            GLint prevReadFBO, prevDrawFBO;
            gles_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
            gles_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);

            GLuint tempDrawFBO;
            gles_glGenFramebuffers(1, &tempDrawFBO);
            gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);

            GLint currentTex;
            gles_glGetIntegerv(get_binding_for_target(target), &currentTex);
            gles_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);

            if (gles_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                gles_glDeleteFramebuffers(1, &tempDrawFBO);
                gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
                return;
            }

            gles_glBlitFramebuffer(x, y, x + width, y + height, xoffset, yoffset, xoffset + width, yoffset + height,
                                   GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            gles_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            gles_glDeleteFramebuffers(1, &tempDrawFBO);
        }
    }
    readfboEnd();
    // "Remap" if buffer mapped...
    glstate->vao->pack = pack;
    glstate->vao->unpack = unpack;
}

void APIENTRY_GL4ES gl4es_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                       GLvoid* data) {
    DBG(SHUT_LOGD("glReadPixels(%i, %i, %i, %i, %s, %s, 0x%p)\n", x, y, width, height, PrintEnum(format),
                  PrintEnum(type), data);)
    FLUSH_BEGINEND;

    if (glstate->list.compiling && glstate->list.active) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }

    LOAD_GLES(glReadPixels);
    errorGL();

    GLvoid* dst = data;
    if (glstate->vao->pack) {
        dst = (char*)dst + (uintptr_t)glstate->vao->pack->data;
    }

    readfboBegin();

    if ((format == GL_RGBA && type == GL_UNSIGNED_BYTE) || (format == glstate->readf && type == glstate->readt) ||
        (format == GL_DEPTH_COMPONENT && (type == GL_FLOAT || type == GL_HALF_FLOAT || type == GL_UNSIGNED_INT))) {
        gles_glReadPixels(x, y, width, height, format, type, dst);
        readfboEnd();
        return;
    }

    int use_bgra = (glstate->readf == GL_BGRA && glstate->readt == GL_UNSIGNED_BYTE) ? 1 : 0;

    GLvoid* pixels = malloc(width * height * 4);
    if (!pixels) {
        LOGE("Memory allocation failed for temporary pixel buffer\n");
        readfboEnd();
        return;
    }

    gles_glReadPixels(x, y, width, height, use_bgra ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    if (!pixel_convert(pixels, &dst, width, height, use_bgra ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, format, type, 0,
                       glstate->texture.pack_align)) {
        LOGE("ReadPixels error: (%s, UNSIGNED_BYTE -> %s, %s )\n", PrintEnum(use_bgra ? GL_BGRA : GL_RGBA),
             PrintEnum(format), PrintEnum(type));
    }

    free(pixels);
    readfboEnd();
}

void APIENTRY_GL4ES gl4es_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* img) {
    DBG(SHUT_LOGD("glGetTexImage(%s, %i, %s, %s, %p)\n", PrintEnum(target), level, PrintEnum(format), PrintEnum(type),
                  img);)
    FLUSH_BEGINEND;
    const GLuint itarget = what_target(target);

    realize_bound(glstate->texture.active, target);

    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];
    int width = bound->width;
    int height = bound->height;
    int nwidth = bound->nwidth;
    int nheight = bound->nheight;
    int shrink = bound->shrink;
    if (level != 0) {
        // printf("STUBBED glGetTexImage with level=%i\n", level);
        void* tmp = malloc(width * height * pixel_sizeof(format, type)); // tmp space...
        void* tmp2;
        gl4es_glGetTexImage(map_tex_target(target), 0, format, type, tmp);
        for (int i = 0; i < level; i++) {
            pixel_halfscale(tmp, &tmp2, width, height, format, type);
            free(tmp);
            tmp = tmp2;
            width = nlevel(width, 1);
            height = nlevel(height, 1);
        }
        memcpy(img, tmp, width * height * pixel_sizeof(format, type));
        free(tmp);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        return;
    }

    DBG(SHUT_LOGD("glGetTexImage(%s, %i, %s, %s, 0x%p), texture=0x%x, size=%i,%i\n", PrintEnum(target), level,
                  PrintEnum(format), PrintEnum(type), img, bound->glname, width, height);)

    GLvoid* dst = img;
    if (glstate->vao->pack) dst = (char*)dst + (uintptr_t)glstate->vao->pack->data;
#ifdef TEXSTREAM
    if (globals4es.texstream && bound->streamed) {
        noerrorShim();
        pixel_convert(GetStreamingBuffer(bound->streamingID), &dst, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      format, type, 0, glstate->texture.unpack_align);
        readfboEnd();
        return;
    }
#endif
    if (globals4es.texcopydata && bound->data) {
        // printf("texcopydata* glGetTexImage(0x%04X, %d, 0x%04x, 0x%04X, %p)\n", target, level, format, type, img);
        noerrorShim();
        if (!pixel_convert(bound->data, &dst, width, height, GL_RGBA, GL_UNSIGNED_BYTE, format, type, 0,
                           glstate->texture.pack_align)) {
            DBG(SHUT_LOGD("LIBGL: Error on pixel_convert while glGetTexImage\n"));
        }
    } else {
        // Setup an FBO the same size of the texture
        GLuint oldBind = bound->glname;
        GLuint old_fbo = glstate->fbo.current_fb->id;
        GLuint fbo;

        // if the texture is not RGBA or RGB or ALPHA, the "just attach texture to the fbo" trick will not work, and a
        // full Blit has to be done
        if ((bound->format == GL_RGBA || bound->format == GL_RGB || (bound->format == GL_BGRA && hardext.bgra8888) ||
             bound->format == GL_ALPHA) &&
            (shrink == 0)) {
            gl4es_glGenFramebuffers(1, &fbo);
            gl4es_glBindFramebuffer(GL_FRAMEBUFFER_OES, fbo);
            gl4es_glFramebufferTexture2D(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, oldBind, 0);
            // Read the pixels!
            gl4es_glReadPixels(0, nheight - height, width, height, format, type,
                               img); // using "full" version with conversion of format/type
            gl4es_glBindFramebuffer(GL_FRAMEBUFFER_OES, old_fbo);
            gl4es_glDeleteFramebuffers(1, &fbo);
            noerrorShim();
        } else {
            gl4es_glGenFramebuffers(1, &fbo);
            gl4es_glBindFramebuffer(GL_FRAMEBUFFER_OES, fbo);
            GLuint temptex;
            gl4es_glGenTextures(1, &temptex);
            gl4es_glBindTexture(GL_TEXTURE_2D, temptex);
            gl4es_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nwidth << shrink, nheight << shrink, 0, GL_RGBA,
                               GL_UNSIGNED_BYTE, 0);
            gl4es_glFramebufferTexture2D(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, temptex, 0);
            gl4es_glBindTexture(GL_TEXTURE_2D, oldBind);
            // blit the texture
            gl4es_glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl4es_glClear(GL_COLOR_BUFFER_BIT);
            gl4es_blitTexture(oldBind, 0.f, 0.f, width, height, nwidth, nheight, 1.0f, 1.0f, nwidth << shrink,
                              nheight << shrink, 0, 0, BLIT_OPAQUE);
            // Read the pixels!
            gl4es_glReadPixels(0, (nheight - height) << shrink, width << shrink, height << shrink, format, type,
                               img); // using "full" version with conversion of format/type
            gl4es_glBindFramebuffer(GL_FRAMEBUFFER_OES, old_fbo);
            gl4es_glDeleteFramebuffers(1, &fbo);
            gl4es_glDeleteTextures(1, &temptex);
            noerrorShim();
        }
    }
}

void APIENTRY_GL4ES gl4es_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y,
                                           GLsizei width, GLint border) {
    gl4es_glCopyTexImage2D(GL_TEXTURE_1D, level, internalformat, x, y, width, 1, border);
}

void APIENTRY_GL4ES gl4es_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y,
                                              GLsizei width) {
    gl4es_glCopyTexSubImage2D(GL_TEXTURE_1D, level, xoffset, 0, x, y, width, 1);
}

// Direct wrapper
AliasExport(void, glGetTexImage, , (GLenum target, GLint level, GLenum format, GLenum type, GLvoid* img));
AliasExport(void, glReadPixels, ,
            (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data));
AliasExport(void, glCopyTexImage1D, ,
            (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border));
AliasExport(void, glCopyTexImage2D, ,
            (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height,
             GLint border));
AliasExport(void, glCopyTexSubImage2D, ,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width,
             GLsizei height));
AliasExport(void, glCopyTexSubImage1D, , (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width));
