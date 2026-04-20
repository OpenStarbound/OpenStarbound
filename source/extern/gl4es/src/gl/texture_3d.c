#include "texture.h"

#include <stdlib.h>
#include <string.h>

#include "../glx/hardext.h"
#include "../glx/streaming.h"
#include "array.h"
#include "blit.h"
#include "decompress.h"
#include "debug.h"
#include "enum_info.h"
#include "fpe.h"
#include "framebuffers.h"
#include "gles.h"
#include "init.h"
#include "loader.h"
#include "matrix.h"
#include "pixel.h"
#include "raster.h"

typedef void(APIENTRY_GLES* glTexImage3D_PTR)(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                                              GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type,
                                              const GLvoid* data);
typedef void(APIENTRY_GLES* glTexSubImage3D_PTR)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                                 GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                                 GLenum format, GLenum type, const GLvoid* data);
typedef void(APIENTRY_GLES* glCopyTexSubImage3D_PTR)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                                     GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

// #define DEBUG
#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

static int inline nlevel3d(int size, int level) {
    if (size) {
        size >>= level;
        if (!size) size = 1;
    }
    return size;
}

static int inline maxlevel3d(int w, int h, int d) {
    int mlevel = 0;
    while (w != 1 || h != 1 || d != 1) {
        w >>= 1;
        h >>= 1;
        d >>= 1;
        if (!w) w = 1;
        if (!h) h = 1;
        if (!d) d = 1;
        ++mlevel;
    }
    return mlevel;
}

static size_t pad_to(size_t v, GLint align) {
    if (align <= 1) return v;
    size_t rem = v % (size_t)align;
    return rem ? v + ((size_t)align - rem) : v;
}

void APIENTRY_GL4ES gl4es_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                                       GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data) {

    if (width == 0 || height == 0 || depth == 0) {
        DBG(SHUT_LOGE("Error: width, height or depth is zero."))
        return;
    }
    if (format == GL_DEPTH_COMPONENT) {
        internalformat = GL_DEPTH_COMPONENT;
        type = GL_UNSIGNED_INT;
    }
    if (internalformat == GL_RGBA16) {
        internalformat = GL_RGBA16F;
        type = GL_FLOAT;
#ifdef GL_RGBA16_SNORM
    } else if (internalformat == GL_RGBA16_SNORM) {
        internalformat = GL_RGBA;
#endif
    }

    internal_convert(&internalformat, &type, &format);

    if (data == NULL && (internalformat == GL_RGB16F || internalformat == GL_RGBA16F))
        internal2format_type(&internalformat, &format, &type);
    if (internalformat == GL_R16F) internal2format_type(&internalformat, &format, &type);
    if (data == NULL && (internalformat == GL_RED || internalformat == GL_RGB))
        internal2format_type(&internalformat, &format, &type);

    if (internalformat == GL_DEPTH32F_STENCIL8 && type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV) {
        internalformat = GL_DEPTH24_STENCIL8;
        type = GL_UNSIGNED_INT_24_8;
    }

    const GLuint itarget = what_target(target);
    const GLuint rtarget = map_tex_target(target);

    if (globals4es.force16bits) {
        if (internalformat == GL_RGBA || internalformat == 4 || internalformat == GL_RGBA8)
            internalformat = GL_RGBA4;
        else if (internalformat == GL_RGB || internalformat == 3 || internalformat == GL_RGB8)
            internalformat = GL_RGB5;
    }

    if (rtarget == GL_PROXY_TEXTURE_2D) {
        int max1 = hardext.maxsize;
        glstate->proxy_width = ((width << level) > max1) ? 0 : width;
        glstate->proxy_height = ((height << level) > max1) ? 0 : height;
        glstate->proxy_intformat = swizzle_internalformat((GLenum*)&internalformat, format, type);
        return;
    }

    realize_bound(glstate->texture.active, target);

    if (glstate->list.pending) {
        gl4es_flush();
    } else {
        PUSH_IF_COMPILING(glTexImage3D);
    }

    if (type == GL_HALF_FLOAT) type = GL_HALF_FLOAT_OES;

    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];
    bound->alpha = pixel_hasalpha(format);

    if (glstate->fpe_state) {
        switch (internalformat) {
        case GL_COMPRESSED_ALPHA:
        case GL_ALPHA4:
        case GL_ALPHA8:
        case GL_ALPHA16:
        case GL_ALPHA16F:
        case GL_ALPHA32F:
        case GL_ALPHA:
            bound->fpe_format = FPE_TEX_ALPHA;
            break;
        case 1:
        case GL_COMPRESSED_LUMINANCE:
        case GL_LUMINANCE4:
        case GL_LUMINANCE8:
        case GL_LUMINANCE16:
        case GL_LUMINANCE16F:
        case GL_LUMINANCE32F:
        case GL_LUMINANCE:
            bound->fpe_format = FPE_TEX_LUM;
            break;
        case 2:
        case GL_COMPRESSED_LUMINANCE_ALPHA:
        case GL_LUMINANCE4_ALPHA4:
        case GL_LUMINANCE8_ALPHA8:
        case GL_LUMINANCE16_ALPHA16:
        case GL_LUMINANCE_ALPHA16F:
        case GL_LUMINANCE_ALPHA32F:
        case GL_LUMINANCE_ALPHA:
            bound->fpe_format = FPE_TEX_LUM_ALPHA;
            break;
        case GL_COMPRESSED_INTENSITY:
        case GL_INTENSITY8:
        case GL_INTENSITY16:
        case GL_INTENSITY16F:
        case GL_INTENSITY32F:
        case GL_INTENSITY:
            bound->fpe_format = FPE_TEX_INTENSITY;
            break;
        case 3:
        case GL_RED:
        case GL_RG:
        case GL_RGB:
        case GL_RGB5:
        case GL_RGB565:
        case GL_RGB8:
        case GL_RGB16:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGB:
        case GL_R11F_G11F_B10F:
        case GL_R32F:
        case GL_RGB10_A2:
            bound->fpe_format = FPE_TEX_RGB;
            break;
        default:
            bound->fpe_format = FPE_TEX_RGBA;
        }
    }

    if (GL4ES_AUTOMIPMAP_PLACEHOLDER) {
        if (level > 0) {
            if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 3) {
                return;
            } else if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 2) {
                bound->mipmap_need = 1;
            }
        }
    }

    if (level == 0 || !bound->valid) {
        bound->wanted_internal = internalformat;
    }
    GLenum new_format = swizzle_internalformat((GLenum*)&internalformat, format, type);
    if (level == 0 || !bound->valid) {
        bound->orig_internal = internalformat;
        bound->internalformat = new_format;
    }

    bound->format = format;
    bound->type = type;
    bound->inter_format = format;
    bound->inter_type = type;

    bound->width = width;
    bound->height = height;
    bound->depth = depth;
    GLsizei nwidth = (hardext.npot) ? width : npot(width);
    GLsizei nheight = (hardext.npot) ? height : npot(height);
    GLsizei ndepth = (hardext.npot) ? depth : npot(depth);
    bound->nwidth = nwidth;
    bound->nheight = nheight;
    bound->ndepth = ndepth;
    bound->npot = (nwidth != width || nheight != height || ndepth != depth);

    if (level == 0) bound->valid = 1;

    noerrorShim();
    LOAD_GLES3(glTexImage3D);
    gles_glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, data);

    if (glstate->bound_changed < glstate->texture.active + 1) glstate->bound_changed = glstate->texture.active + 1;
}
void APIENTRY_GL4ES gl4es_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                          const GLvoid* data) {

    if (width == 0 || height == 0 || depth == 0) {
        DBG(SHUT_LOGE("Error: width, height or depth is zero."))
        return;
    }

    if (glstate->list.pending) {
        gl4es_flush();
    } else {
        PUSH_IF_COMPILING(glTexSubImage3D);
    }

    realize_bound(glstate->texture.active, target);

    if (!data) {
        DBG(SHUT_LOGD("LIBGL: glTexSubImage3D called with NULL data\n");)
        return;
    }
    int pixelSize = pixel_sizeof(format, type);
    if (pixelSize <= 0) {
        DBG(SHUT_LOGD("LIBGL: invalid pixel size (format/type) in glTexSubImage3D\n");)
        return;
    }

    const GLubyte* pixels_src = (const GLubyte*)data;
    GLubyte* temp_pixels = NULL;

    if (glstate->texture.unpack_row_length != 0 || glstate->texture.unpack_skip_pixels != 0 ||
        glstate->texture.unpack_skip_rows != 0 || glstate->texture.unpack_align != 4 ||
        glstate->texture.unpack_image_height != 0) {

        size_t ui_width = (size_t)width;
        size_t ui_height = (size_t)height;
        size_t ui_depth = (size_t)depth;
        size_t up_row_pixels =
            (glstate->texture.unpack_row_length ? (size_t)glstate->texture.unpack_row_length : ui_width);
        size_t up_img_height =
            (glstate->texture.unpack_image_height ? (size_t)glstate->texture.unpack_image_height : ui_height);
        GLint up_align = glstate->texture.unpack_align;
        if (up_align <= 0) up_align = 1;

        size_t src_row_raw = up_row_pixels * (size_t)pixelSize;
        size_t src_row_bytes = pad_to(src_row_raw, up_align);
        size_t src_img_bytes = up_img_height * src_row_bytes;

        size_t dst_row_bytes = ui_width * (size_t)pixelSize;
        size_t dst_img_bytes = dst_row_bytes * ui_height;
        size_t total_dst = dst_img_bytes * ui_depth;

        size_t skip_pixels = (size_t)glstate->texture.unpack_skip_pixels;
        size_t skip_rows = (size_t)glstate->texture.unpack_skip_rows;
        size_t skip_pixels_bytes = skip_pixels * (size_t)pixelSize;
        size_t skip_rows_bytes = skip_rows * src_row_bytes;

        temp_pixels = (GLubyte*)malloc(total_dst);
        if (!temp_pixels) {
            DBG(SHUT_LOGD("LIBGL: malloc failed in glTexSubImage3D (bytes=%zu)\n", total_dst));
            return;
        }

        const GLubyte* src_base = (const GLubyte*)pixels_src;
        for (size_t z = 0; z < ui_depth; ++z) {
            const GLubyte* src_slice = src_base + z * src_img_bytes + skip_rows_bytes + skip_pixels_bytes;
            GLubyte* dst_slice = temp_pixels + z * dst_img_bytes;
            for (size_t y = 0; y < ui_height; ++y) {
                const GLubyte* src_row = src_slice + y * src_row_bytes;
                GLubyte* dst_row = dst_slice + y * dst_row_bytes;
                memcpy(dst_row, src_row, dst_row_bytes);
            }
        }

        pixels_src = (const GLubyte*)temp_pixels;
    }

    LOAD_GLES3(glTexSubImage3D);
    gles_glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type,
                         (const GLvoid*)pixels_src);

    if (temp_pixels) free(temp_pixels);
}

void APIENTRY_GL4ES gl4es_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
                                         GLsizei height, GLsizei depth) {
    DBG(SHUT_LOGD("glTexStorage3D(%s, %d, %s, %d, %d, %d)\n", PrintEnum(target), levels, PrintEnum(internalformat),
                  width, height, depth);)
    if (!levels) {
        noerrorShim();
        return;
    }
    if ((internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalformat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) &&
        !globals4es.avoid16bits)
        gl4es_glTexImage3D(target, 0, internalformat, width, height, depth, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    else if (((internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
               internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT)) &&
             !globals4es.avoid16bits)
        gl4es_glTexImage3D(target, 0, internalformat, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
                           NULL);
    else if ((internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
              internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
              internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ||
              internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) &&
             !globals4es.avoid16bits)
        gl4es_glTexImage3D(target, 0, internalformat, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,
                           NULL);
    else
        gl4es_glTexImage3D(target, 0, internalformat, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    int mlevel = maxlevel3d(width, height, depth);
    gltexture_t* bound = gl4es_getCurrentTexture(target);
    if (levels > 1 && isDXTc(internalformat)) {
        bound->mipmap_need = 1;
        bound->mipmap_auto = 1;
        for (int i = 1; i <= mlevel; ++i)
            gl4es_glTexImage3D(target, i, internalformat, nlevel3d(width, i), nlevel3d(height, i), nlevel3d(depth, i),
                               0, bound->format, bound->type, NULL);
        noerrorShim();
        return;
    }
    if (mlevel > levels - 1) {
        bound->max_level = levels - 1;
        if (levels > 1 && GL4ES_AUTOMIPMAP_PLACEHOLDER != 3) bound->mipmap_need = 1;
    }
}

void APIENTRY_GL4ES gl4es_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                              GLint x, GLint y, GLsizei width, GLsizei height) {
    FLUSH_BEGINEND;

    if (globals4es.skiptexcopies) {
        DBG(SHUT_LOGD("glCopyTexSubImage3D skipped.\n"));
        return;
    }

    LOAD_GLES3(glCopyTexSubImage3D);
    errorGL();
    realize_bound(glstate->texture.active, target);

    glbuffer_t* pack = glstate->vao->pack;
    glbuffer_t* unpack = glstate->vao->unpack;
    glstate->vao->pack = NULL;
    glstate->vao->unpack = NULL;

    readfboBegin();
    gles_glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    readfboEnd();

    glstate->vao->pack = pack;
    glstate->vao->unpack = unpack;
}

// Direct wrapper
AliasExport(void, glTexImage3D, ,
            (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth,
             GLint border, GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glTexImage3D, EXT,
            (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth,
             GLint border, GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glTexSubImage3D, ,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
             GLsizei depth, GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glCopyTexSubImage3D, ,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width,
             GLsizei height));

// EXT mapper
AliasExport(void, glTexSubImage3D, EXT,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
             GLsizei depth, GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glCopyTexSubImage3D, EXT,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width,
             GLsizei height));

// ARB mapper
AliasExport(void, glTexSubImage3D, ARB,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
             GLsizei depth, GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glCopyTexSubImage3D, ARB,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width,
             GLsizei height));

// TexStorage
AliasExport(void, glTexStorage3D, ,
            (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth));
