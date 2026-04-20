#include "texture.h"

#include <GL/gl.h>
#include "../glx/hardext.h"
#include "../glx/streaming.h"
#include "GL/glext.h"
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

// #define DEBUG
#ifdef DEBUG
#define DBG(a) a
#define DBGLOGD(...) SHUT_LOGD(__VA_ARGS__)
#else
#define DBG(a)
#define DBGLOGD(...)                                                                                                   \
    {}
#endif

#ifndef GL_TEXTURE_STREAM_IMG
#define GL_TEXTURE_STREAM_IMG 0x8C0D
#endif
#ifdef TEXSTREAM
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

// expand non-power-of-two sizes
// TODO: what does this do to repeating textures?
int npot(int n) {
    if (n == 0) return 0;

    int i = 1;
    while (i < n)
        i <<= 1;
    return i;
}

static int inline nlevel(int size, int level) {
    if (size) {
        size >>= level;
        if (!size) size = 1;
    }
    return size;
}

// return the max level for that WxH size
static int inline maxlevel(int w, int h) {
    int mlevel = 0;
    while (w != 1 || h != 1) {
        w >>= 1;
        h >>= 1;
        if (!w) w = 1;
        if (!h) h = 1;
        ++mlevel;
    }
    return mlevel;
}

static inline GLboolean bgra_supported_type(GLenum type) {
    // GL_EXT_texture_format_BGRA8888 only guarantees BGRA with UNSIGNED_BYTE on GLES.
    return hardext.bgra8888 && (type == GL_UNSIGNED_BYTE);
}

static int is_fake_compressed_rgb(GLenum internalformat) {
    if (internalformat == GL_COMPRESSED_RGB) return 1;
    if (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) return 1;
    if (internalformat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) return 1;
    return 0;
}
static int is_fake_compressed_rgba(GLenum internalformat) {
    if (internalformat == GL_COMPRESSED_RGBA) return 1;
    if (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) return 1;
    if (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) return 1;
    if (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) return 1;
    if (internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT) return 1;
    if (internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT) return 1;
    if (internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) return 1;
    return 0;
}

// The real function to convert format
void internal_convert(GLenum* internal_format, GLenum* type, GLenum* format) {
    if (format && (*format == GL_BGRA || *format == GL_BGR || *format == GL_BGRA8_EXT)) return;
    if (type && *type == GL_UNSIGNED_INT_8_8_8_8) return;

    switch (*internal_format) {
    case GL_DEPTH_COMPONENT16:
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_DEPTH_COMPONENT24:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32:
        *internal_format = GL_DEPTH_COMPONENT;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_DEPTH_COMPONENT:
        if (type) {
            *internal_format = GL_DEPTH_COMPONENT;
            *type = GL_UNSIGNED_INT;
        }
        break;
    case GL_DEPTH_STENCIL:
        *internal_format = GL_DEPTH32F_STENCIL8;
        if (type) *type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        break;
    case GL_RGB10_A2:
        if (type) *type = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    case GL_RGB5_A1:
        if (type) *type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case GL_COMPRESSED_RED_RGTC1:
    case GL_COMPRESSED_RG_RGTC2:
        break;
    case GL_SRGB8:
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RGBA32F:
    case GL_RGB32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB9_E5:
        if (type) *type = GL_UNSIGNED_INT_5_9_9_9_REV;
        break;
    case GL_R11F_G11F_B10F:
        if (type) *type = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (format) *format = GL_RGB;
        break;
    case GL_RGBA32UI:
    case GL_RGB32UI:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RGBA32I:
    case GL_RGB32I:
        if (type) *type = GL_INT;
        break;
    case GL_RGBA16: {
        *internal_format = GL_RGBA16F;
        if (type) *type = GL_FLOAT;
        break;
    }
    case GL_RGBA8:
    case GL_RGBA:
        if (type) *type = GL_UNSIGNED_BYTE;
        if (format) *format = GL_RGBA;
        break;
    case GL_RGBA16F:
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_R16:
        *internal_format = GL_R16F;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB16:
        *internal_format = GL_RGB16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RGB16F:
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RG16:
        *internal_format = GL_RG16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RG;
        break;
        // Inline R and RG channel mappings
    case GL_R8:
        if (format) *format = GL_RED;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8_SNORM:
        if (format) *format = GL_RED;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16F:
        if (format) *format = GL_RED;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RED:
        if (type) {
            switch (*type) {
            case GL_UNSIGNED_BYTE:
                *internal_format = GL_R8;
                if (format) *format = GL_RED;
                break;
            case GL_BYTE:
                *internal_format = GL_R8_SNORM;
                if (format) *format = GL_RED;
                break;
            case GL_HALF_FLOAT:
                *internal_format = GL_R16F;
                if (format) *format = GL_RED;
                break;
            case GL_FLOAT:
                *internal_format = GL_R32F;
                if (format) *format = GL_RED;
                break;
            default:
                if (type) *type = GL_UNSIGNED_BYTE; // Fallback to unsigned byte
                *internal_format = GL_R8;           // Fallback to R8
                if (format) *format = GL_RED;
                break;
            }
        }
        break;
    case GL_R8UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_R16I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_R32UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_R32I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_RG8:
        if (format) *format = GL_RG;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8_SNORM:
        if (format) *format = GL_RG;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16F:
        if (format) *format = GL_RG;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RG32F:
        if (format) *format = GL_RG;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RG8UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_RG16I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_RG32UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RG32I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_RGBA8_SNORM:
        if (format) *format = GL_RGBA;
        if (type) *type = GL_BYTE;
        break;
    case GL_R32F:
        if (format) *format = GL_RED;
        if (type) *type = GL_FLOAT;
        break;
    default:
        // fallback handling for GL_RGB8, GL_RGBA16_SNORM etc.
        if (*internal_format == GL_RGB8) {
            if (type && *type != GL_UNSIGNED_BYTE) *type = GL_UNSIGNED_BYTE;
            if (format) *format = GL_RGB;
        } else if (*internal_format == GL_RGBA16_SNORM) {
            if (type && *type != GL_SHORT) *type = GL_SHORT;
        }
        break;
    }
}

void internal2format_type(GLenum* internalformat, GLenum* format, GLenum* type) {
    if (format && *format != GL_BGRA && *format != GL_BGR && *format != GL_BGRA8_EXT) return;
    DBG(char log_buffer[512]; int offset = snprintf(log_buffer, sizeof(log_buffer), "tex format converting... ");
        if (internalformat) offset +=
        snprintf(log_buffer + offset, sizeof(log_buffer) - offset, "internalFormat: %s", PrintEnum(*internalformat));
        if (format) offset +=
        snprintf(log_buffer + offset, sizeof(log_buffer) - offset, ", format: %s", PrintEnum(*format));
        if (type) offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset, ", type: %s", PrintEnum(*type));
        snprintf(log_buffer + offset, sizeof(log_buffer) - offset, "\n"); DBGLOGD("%s", log_buffer))
    switch (*internalformat) {

    case GL_RGB10_A2:
        if (type) *type = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    case GL_RGB5_A1:
        if (type) *type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case GL_SRGB8:
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RGBA32F:
    case GL_RGB32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB9_E5:
        if (type) *type = GL_UNSIGNED_INT_5_9_9_9_REV;
        break;
    case GL_R11F_G11F_B10F:
        if (type) *type = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (format) *format = GL_RGB;
        break;
    case GL_RGBA32UI:
    case GL_RGB32UI:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RGBA32I:
    case GL_RGB32I:
        if (type) *type = GL_INT;
        break;
    case GL_RGBA16: {
        *internalformat = GL_RGBA16F;
        if (type) *type = GL_FLOAT;
        break;
    }
    case GL_RGBA8:
    case GL_RGBA:
        if (type) *type = GL_UNSIGNED_BYTE;
        if (format) *format = GL_RGBA;
        break;
    case GL_RGBA16F:
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_R16:
        *internalformat = GL_R16F;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB16:
        *internalformat = GL_RGB16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RGB16F:
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RG16:
        *internalformat = GL_RG16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RG;
        break;
        // Inline R and RG channel mappings
    case GL_R8:
        if (format) *format = GL_RED;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8_SNORM:
        if (format) *format = GL_RED;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16F:
        if (format) *format = GL_RED;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RED:
        if (type) {
            switch (*type) {
            case GL_UNSIGNED_BYTE:
                *internalformat = GL_R8;
                if (format) *format = GL_RED;
                break;
            case GL_BYTE:
                *internalformat = GL_R8_SNORM;
                if (format) *format = GL_RED;
                break;
            case GL_HALF_FLOAT:
                *internalformat = GL_R16F;
                if (format) *format = GL_RED;
                break;
            case GL_FLOAT:
                *internalformat = GL_R32F;
                if (format) *format = GL_RED;
                break;
            default:
                if (type) *type = GL_UNSIGNED_BYTE; // Fallback to unsigned byte
                *internalformat = GL_R8;            // Fallback to R8
                if (format) *format = GL_RED;
                break;
            }
        }
        break;
    case GL_R8UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_R16I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_R32UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_R32I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_RG8:
        if (format) *format = GL_RG;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8_SNORM:
        if (format) *format = GL_RG;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16F:
        if (format) *format = GL_RG;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RG32F:
        if (format) *format = GL_RG;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RG8UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_RG16I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_RG32UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RG32I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_R:
        if (format) *format = GL_RED;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG:
        if (!hardext.rgtex) {
            *format = GL_RGB;
            *type = GL_UNSIGNED_BYTE;
        } else {
            *format = GL_RG;
            *type = GL_UNSIGNED_BYTE;
        }
        break;
    case GL_COMPRESSED_ALPHA:
    case GL_ALPHA:
        *format = GL_ALPHA;
        *type = GL_UNSIGNED_BYTE;
        break;
    case 1: // is this here or with GL_RED?
    case GL_COMPRESSED_LUMINANCE:
    case GL_LUMINANCE:
        *format = GL_LUMINANCE;
        *type = GL_UNSIGNED_BYTE;
        break;
    case 2:
    case GL_COMPRESSED_LUMINANCE_ALPHA:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE_ALPHA:
        if (globals4es.nolumalpha) {
            *format = GL_RGBA;
            *type = GL_UNSIGNED_BYTE;
        } else {
            *format = GL_LUMINANCE_ALPHA;
            *type = GL_UNSIGNED_BYTE;
        }
        break;
    case GL_RGB5:
    case GL_RGB565:
        *format = GL_RGB;
        *type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case GL_RGB:
        if (globals4es.avoid24bits)
            *format = GL_RGBA;
        else
            *format = GL_RGB;
        *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RGBA4:
        *format = GL_RGBA;
        *type = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    case GL_BGRA:
        if (hardext.bgra8888)
            *format = GL_BGRA;
        else
            *format = GL_RGBA;
        *type = GL_UNSIGNED_BYTE;
        break;
    case GL_DEPTH_COMPONENT16:
        if (type) *type = GL_UNSIGNED_SHORT;
        break;

    case GL_DEPTH_COMPONENT24:
        if (type) *type = GL_UNSIGNED_INT;
        break;

    case GL_DEPTH_COMPONENT32:
        *internalformat = GL_DEPTH_COMPONENT;
        if (type) *type = GL_UNSIGNED_INT;
        break;

    case GL_DEPTH_COMPONENT32F:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT:
        *format = GL_DEPTH_COMPONENT;
        *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
        *format = GL_DEPTH_STENCIL;
        *type = GL_UNSIGNED_INT_24_8;
        break;
    default:
        // fallback handling for GL_RGB8, GL_RGBA16_SNORM etc.
        if (*internalformat == GL_RGB8) {
            if (type && *type != GL_UNSIGNED_BYTE) *type = GL_UNSIGNED_BYTE;
            if (format) *format = GL_RGB;
        } else if (*internalformat == GL_RGBA16_SNORM) {
            if (type && *type != GL_SHORT) *type = GL_SHORT;
        }
        break;
    }
    DBG(char log_buffer2[512]; int offset2 = snprintf(log_buffer, sizeof(log_buffer), "converted: ");
        if (internalformat) offset2 +=
        snprintf(log_buffer + offset2, sizeof(log_buffer) - offset2, "internalFormat: %s", PrintEnum(*internalformat));
        if (format) offset2 +=
        snprintf(log_buffer + offset2, sizeof(log_buffer) - offset2, ", format: %s", PrintEnum(*format));
        if (type) offset2 +=
        snprintf(log_buffer + offset2, sizeof(log_buffer) - offset2, ", type: %s", PrintEnum(*type));
        snprintf(log_buffer2 + offset2, sizeof(log_buffer2) - offset2, "\n"); DBGLOGD("%s", log_buffer))
}

static void* swizzle_texture(GLsizei width, GLsizei height, GLenum* format, GLenum* type, GLenum intermediaryformat,
                             GLenum internalformat, const GLvoid* data, gltexture_t* bound) {
    if (format && *format != GL_BGRA && *format != GL_BGR && *format != GL_BGRA8_EXT &&
        *type != GL_UNSIGNED_INT_8_8_8_8)
        return data;
    if (format && *format == GL_BGRA8_EXT) *format = GL_BGRA;
    int convert = 0;
    GLenum dest_format = GL_RGBA;
    GLenum dest_type = GL_UNSIGNED_BYTE;
    int check = 1;
    const GLboolean bgra_ok = bgra_supported_type(*type);
    // compressed format are not handled here, so mask them....
    if (is_fake_compressed_rgb(intermediaryformat)) intermediaryformat = GL_RGB;
    if (is_fake_compressed_rgba(intermediaryformat)) intermediaryformat = GL_RGBA;
    if (is_fake_compressed_rgb(internalformat)) internalformat = GL_RGB;
    if (is_fake_compressed_rgba(internalformat)) internalformat = GL_RGBA;
    if (intermediaryformat == GL_COMPRESSED_LUMINANCE) intermediaryformat = GL_LUMINANCE;
    if (internalformat == GL_COMPRESSED_LUMINANCE) internalformat = GL_LUMINANCE;

    // if (*format != intermediaryformat || intermediaryformat != internalformat) {
    //     internal2format_type(&intermediaryformat, &dest_format, &dest_type);
    //     convert = 1;
    //     check = 0;
    // } else

    {
        if ((*type) == GL_HALF_FLOAT) (*type) = GL_HALF_FLOAT_OES; // the define is different between GL and GLES...
        switch (*format) {
        case GL_R:
        case GL_RED:
            dest_format = GL_RED;
            check = 0;
            break;
        case GL_RG:
            dest_format = GL_RG;
            check = 0;
            break;
        case GL_COMPRESSED_LUMINANCE:
            *format = GL_LUMINANCE;
        case GL_LUMINANCE:
            dest_format = GL_LUMINANCE;
            break;
        case GL_LUMINANCE16F:
            dest_format = GL_LUMINANCE;
            if (hardext.halffloattex) {
                dest_type = GL_HALF_FLOAT_OES;
                check = 0;
            }
            break;
        case GL_LUMINANCE32F:
            dest_format = GL_LUMINANCE;
            if (hardext.floattex) {
                dest_type = GL_FLOAT;
                check = 0;
            }
            break;
        case GL_RGB:
            dest_format = GL_RGB;
            check = 0;
            break;
        case GL_COMPRESSED_ALPHA:
            *format = GL_ALPHA;
        case GL_ALPHA:
            dest_format = GL_ALPHA;
            break;
        case GL_ALPHA16F:
            dest_format = GL_ALPHA;
            if (hardext.halffloattex) {
                dest_type = GL_HALF_FLOAT_OES;
                check = 0;
            }
            break;
        case GL_ALPHA32F:
            dest_format = GL_ALPHA;
            if (hardext.floattex) {
                dest_type = GL_FLOAT;
                check = 0;
            }
            break;
        case GL_RGBA:
            check = 0;
            break;
        case GL_LUMINANCE8_ALPHA8:
        case GL_COMPRESSED_LUMINANCE_ALPHA:
            if (globals4es.nolumalpha)
                convert = 1;
            else {
                dest_format = GL_LUMINANCE_ALPHA;
                *format = GL_LUMINANCE_ALPHA;
            }
            break;
        case GL_LUMINANCE_ALPHA:
            if (globals4es.nolumalpha)
                convert = 1;
            else
                dest_format = GL_LUMINANCE_ALPHA;
            break;
        case GL_LUMINANCE_ALPHA16F:
            if (globals4es.nolumalpha)
                convert = 1;
            else
                dest_format = GL_LUMINANCE_ALPHA;
            if (hardext.halffloattex) {
                dest_type = GL_HALF_FLOAT_OES;
                check = 0;
            }
            break;
        case GL_LUMINANCE_ALPHA32F:
            if (globals4es.nolumalpha)
                convert = 1;
            else
                dest_format = GL_LUMINANCE_ALPHA;
            if (hardext.floattex) {
                dest_type = GL_FLOAT;
                check = 0;
            }
            break;
            // vvvvv all this are internal formats, so it should not happens
        case GL_RGB565:
            check = 0;
            break;
        case GL_RGB5:
            dest_format = GL_RGB;
            dest_type = GL_UNSIGNED_SHORT_5_6_5;
            convert = 1;
            check = 0;
            break;
        case GL_RGB8:
            check = 0;
            break;
        case GL_RGBA4:
            check = 0;
            break;
        case GL_RGBA8:
            check = 0;
            break;
        case GL_BGRA:
            if (bgra_ok) {
                dest_format = GL_BGRA;
                //*format = GL_BGRA;
            } else {
                convert = 1;
                dest_format = GL_RGBA;
            }
            break;
        case GL_BGR:
            dest_format = GL_RGB;
            convert = 1;
            break;
        case GL_DEPTH32F_STENCIL8:
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH_STENCIL:
            // if (hardext.depthtex && hardext.depthstencil) {
            const int is32F = *format == GL_DEPTH32F_STENCIL8;
            *format = dest_format = GL_DEPTH_STENCIL;
            dest_type = is32F ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_UNSIGNED_INT_24_8;
            //   check = 0;
            //}
            // else convert = 1;
            break;
        case GL_DEPTH_COMPONENT:
            check = 0;
            // if (hardext.depthtex) {
            *format = dest_format = GL_DEPTH_COMPONENT;
            // if (dest_type != GL_UNSIGNED_INT) {
            //     convert = 1;
            // }
            dest_type = GL_UNSIGNED_INT;
            //    check = 0;
            //}
            // else
            //    convert = 1;
            break;
        case GL_DEPTH_COMPONENT16:
            check = 0;
            *format = dest_format = GL_DEPTH_COMPONENT;
            dest_type = GL_UNSIGNED_SHORT;
            break;
        case GL_DEPTH_COMPONENT24:
            check = 0;
            *format = dest_format = GL_DEPTH_COMPONENT;
            dest_type = GL_UNSIGNED_INT;
            break;
        case GL_DEPTH_COMPONENT32:
            check = 0;
            *format = dest_format = GL_DEPTH_COMPONENT;
            dest_type = GL_UNSIGNED_INT;
            break;
        case GL_DEPTH_COMPONENT32F:
            check = 0;
            *format = dest_format = GL_DEPTH_COMPONENT;
            dest_type = GL_FLOAT;
            break;
        case GL_STENCIL_INDEX8:
            check = 0;
            if (hardext.stenciltex)
                *format = dest_format = GL_STENCIL_INDEX8;
            else
                convert = 1;
            break;
        default:
            check = 0;
            // convert = 1;
            break;
        }
        if (check) switch (*type) {
            case GL_UNSIGNED_SHORT_4_4_4_4_REV:
                if (dest_format == GL_RGBA) dest_type = GL_UNSIGNED_SHORT_4_4_4_4;
                convert = 1;
                break;
            case GL_UNSIGNED_SHORT_4_4_4_4:
                if (dest_format == GL_RGBA)
                    dest_type = GL_UNSIGNED_SHORT_4_4_4_4;
                else
                    convert = 1;
                break;
            case GL_UNSIGNED_SHORT_1_5_5_5_REV:
                if (!hardext.rgba1555rev) {
                    if (dest_format == GL_RGBA) dest_type = GL_UNSIGNED_SHORT_5_5_5_1;
                    convert = 1;
                }
                break;
            case GL_UNSIGNED_SHORT_5_5_5_1:
                if (dest_format == GL_RGBA)
                    dest_type = GL_UNSIGNED_SHORT_5_5_5_1;
                else
                    convert = 1;
                break;
            case GL_UNSIGNED_SHORT_5_6_5_REV:
                if (dest_format == GL_RGB) dest_type = GL_UNSIGNED_SHORT_5_6_5;
                convert = 1;
                break;
            case GL_UNSIGNED_SHORT_5_6_5:
                if (dest_format == GL_RGB)
                    dest_type = GL_UNSIGNED_SHORT_5_6_5;
                else
                    convert = 1;
                break;
#ifdef __BIG_ENDIAN__
            case GL_UNSIGNED_INT_8_8_8_8:
#else
            case GL_UNSIGNED_INT_8_8_8_8_REV:
#endif
                *type = GL_UNSIGNED_BYTE;
                // fall through
            case GL_UNSIGNED_BYTE:
                if (dest_format == GL_RGB && globals4es.avoid24bits) {
                    dest_format = GL_RGBA;
                    convert = 1;
                }
                break;
#ifdef __BIG_ENDIAN__
            case GL_UNSIGNED_INT_8_8_8_8_REV:
                if (!hardext.rgba8888rev) {
                    dest_type = GL_UNSIGNED_BYTE;
                    convert = 1;
                }
                break;
#else
            case GL_UNSIGNED_INT_8_8_8_8:
                if (!hardext.rgba8888) {
                    dest_type = GL_UNSIGNED_BYTE;
                    convert = 1;
                }
                break;
#endif
            case GL_UNSIGNED_INT_24_8:
                if (hardext.depthtex && hardext.depthstencil) {
                    dest_type = GL_UNSIGNED_INT_24_8;
                } else {
                    *type = GL_UNSIGNED_BYTE; // will probably do nothing good!
                    convert = 1;
                }
                break;
            case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
                if (hardext.floattex && hardext.depthstencil) {
                    dest_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
                } else {
                    *type = GL_UNSIGNED_BYTE; // will probably do nothing good!
                    convert = 1;
                }
                break;
            case GL_FLOAT:
                if (hardext.floattex)
                    dest_type = GL_FLOAT;
                else
                    convert = 1;
                break;
            case GL_HALF_FLOAT:
            case GL_HALF_FLOAT_OES:
                if (hardext.halffloattex)
                    dest_type = GL_HALF_FLOAT_OES;
                else
                    convert = 1;
                break;
            default:
                convert = 1;
                break;
            }
    }
    if (data) {
        if (convert) {
            GLvoid* pixels = (GLvoid*)data;
            bound->inter_format = dest_format;
            bound->format = dest_format;
            bound->inter_type = dest_type;
            bound->type = dest_type;
            if (!pixel_convert(data, &pixels, width, height, *format, *type, dest_format, dest_type, 0,
                               glstate->texture.unpack_align)) {
                DBGLOGD("LIBGL: swizzle error: (%s, %s -> %s, %s)\n", PrintEnum(*format), PrintEnum(*type),
                        PrintEnum(dest_format), PrintEnum(dest_type));
                return NULL;
            }
            *type = dest_type;
            *format = dest_format;
            if (dest_format != internalformat) {
                GLvoid* pix2 = (GLvoid*)pixels;
                internal2format_type(&internalformat, &dest_format, &dest_type);
                bound->format = dest_format;
                bound->type = dest_type;
                if (!pixel_convert(pixels, &pix2, width, height, *format, *type, dest_format, dest_type, 0,
                                   glstate->texture.unpack_align)) {
                    DBGLOGD("LIBGL: swizzle error: (%s, %s -> %s, %s)\n", PrintEnum(dest_format), PrintEnum(dest_type),
                            PrintEnum(internalformat), PrintEnum(dest_type));
                    return NULL;
                }
                if (pix2 != pixels) {
                    if (pixels != data) free(pixels);
                    pixels = pix2;
                }
                *type = dest_type;
                *format = dest_format;
            }
            GLvoid* pix2 = pixels;
            if (raster_need_transform())
                if (!pixel_transform(data, &pixels, width, height, *format, *type, glstate->raster.raster_scale,
                                     glstate->raster.raster_bias)) {
                    DBGLOGD("LIBGL: swizzle/convert error: (%s, %s -> %s, %s)\n", PrintEnum(*format), PrintEnum(*type),
                            PrintEnum(dest_format), PrintEnum(dest_type));
                    pix2 = pixels;
                }
            if (pix2 != pixels && pixels != data) free(pixels);
            return pix2;
        } else {
            bound->inter_format = dest_format;
            bound->format = dest_format;
            bound->inter_type = dest_type;
            bound->type = dest_type;
        }
    } else {
        bound->inter_format = dest_format;
        bound->inter_type = dest_type;
        if (convert) {
            internal2format_type(&internalformat, &dest_format, &dest_type); // in case they are differents
            *type = dest_type;
            *format = dest_format;
        }
        bound->format = dest_format;
        bound->type = dest_type;
    }
    return (void*)data;
}

GLenum swizzle_internalformat(GLenum* internalformat, GLenum format, GLenum type) {
    if (format == GL_BGRA8_EXT) format = GL_BGRA;
    if (internalformat && format && format != GL_BGRA && format != GL_BGR && format != GL_BGRA8_EXT)
        return *internalformat;
    GLenum ret = *internalformat;
    GLenum sret = ret;
    const GLboolean bgra_ok = bgra_supported_type(type);
    switch (*internalformat) {
    case GL_RED:
    case GL_R:
    case GL_R8:
        sret = GL_RED;
        break;
    case GL_R32F:
        ret = sret = GL_R32F;
        break;
    case GL_RGB10_A2:
        ret = sret = GL_RGB10_A2;
        break;
    case GL_RG:
        sret = GL_RG;
        break;
    case GL_RGB565:
        ret = GL_RGB5;
    case GL_RGB5:
        sret = GL_RGB5;
        break;
    case GL_RGB:
        if (globals4es.avoid16bits == 0 && format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) {
            sret = ret = GL_RGB5;
            break;
        }
    case GL_RGB8:
    case GL_RGB16:
    case GL_RGB16F:
    case GL_RGB32F:
        break;
    case GL_BGR:
    case 3:
        ret = GL_RGB;
        sret = GL_RGB;
        break;
    case GL_RGBA4:
        sret = GL_RGBA4;
        break;
    case GL_RGB5_A1:
        sret = GL_RGB5_A1;
        break;
    case GL_RGBA:
        if (globals4es.avoid16bits == 0 && format == GL_RGBA && type == GL_UNSIGNED_SHORT_5_5_5_1) {
            sret = ret = GL_RGB5_A1;
            break;
        }
        if (globals4es.avoid16bits == 0 && format == GL_RGBA && type == GL_UNSIGNED_SHORT_4_4_4_4) {
            sret = ret = GL_RGBA4;
            break;
        }
        if (format == GL_BGRA && bgra_ok) {
            sret = ret = GL_BGRA;
        }
    case GL_RGBA8:
    case GL_RGBA16:
    case GL_RGBA16F:
    case GL_RGBA32F:
        break;
    case 4:
        if (format == GL_BGRA && bgra_ok) {
            ret = GL_BGRA;
            sret = GL_BGRA;
        } else {
            ret = GL_RGBA;
            sret = GL_RGBA;
        }
        break;
    case GL_ALPHA32F:
    case GL_ALPHA16F:
    case GL_ALPHA8:
    case GL_ALPHA:
        ret = GL_ALPHA;
        sret = GL_ALPHA;
        break;
    case 1:
    case GL_LUMINANCE32F:
    case GL_LUMINANCE16F:
    case GL_LUMINANCE8:
    case GL_LUMINANCE16:
    case GL_LUMINANCE:
        if (format == GL_RED && hardext.rgtex) {
            ret = GL_RED;
            sret = GL_RED;
        } else {
            ret = GL_LUMINANCE;
            sret = GL_LUMINANCE;
        }
        break;
    case 2:
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE16_ALPHA16:
    case GL_LUMINANCE_ALPHA32F:
    case GL_LUMINANCE_ALPHA16F:
    case GL_LUMINANCE_ALPHA:
        ret = GL_LUMINANCE_ALPHA;
        if (globals4es.nolumalpha)
            sret = GL_RGBA;
        else
            sret = GL_LUMINANCE_ALPHA;
        break;
        // compressed format...
    case GL_COMPRESSED_ALPHA:
        ret = GL_ALPHA; // GL_COMPRESSED_RGBA;
        sret = GL_ALPHA;
        break;
    case GL_COMPRESSED_LUMINANCE:
        ret = GL_LUMINANCE; // GL_COMPRESSED_RGB;
        sret = GL_LUMINANCE;
        break;
    case GL_COMPRESSED_LUMINANCE_ALPHA:
        if (globals4es.nolumalpha) {
            ret = GL_COMPRESSED_RGBA;
            sret = GL_RGBA;
        } else {
            ret = GL_LUMINANCE_ALPHA;
            sret = GL_LUMINANCE_ALPHA;
        }
        break;
    case GL_COMPRESSED_RGB:
        sret = GL_RGB;
        break;
    case GL_COMPRESSED_RGBA:
        sret = GL_RGBA;
        break;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: // should be sRGB...
        ret = GL_COMPRESSED_RGB;
        sret = GL_RGB;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: // not good...
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: // not good, but there is no DXT3 compressor
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        ret = GL_COMPRESSED_RGBA;
        sret = GL_RGBA;
        break;
    case GL_BGRA8_EXT:
    case GL_BGRA:
        if (bgra_ok) {
            ret = GL_BGRA;
            sret = GL_BGRA;
        } else {
            ret = GL_RGBA;
            sret = GL_RGBA;
        }
        break;
    case GL_DEPTH_COMPONENT:
        // if (hardext.depthtex) {
        sret = ret = GL_DEPTH_COMPONENT;
        //}
        // else {
        //    sret = ret = GL_RGBA;
        //}
        break;
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
        // if (hardext.depthtex) {
        switch (type) {
        case GL_UNSIGNED_SHORT:
            sret = ret = GL_DEPTH_COMPONENT;
            break;
        case GL_UNSIGNED_INT:
            sret = ret = GL_DEPTH_COMPONENT;
            break;
        case GL_FLOAT:
            sret = ret = GL_DEPTH_COMPONENT32F;
            break;
        default:
            sret = ret = GL_DEPTH_COMPONENT;
            break;
        }
        //}
        // else {
        //    sret = ret = GL_RGBA;
        //}
        break;
    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        if (hardext.depthtex) {
            switch (type) {
            case GL_UNSIGNED_INT:
                sret = ret = GL_DEPTH24_STENCIL8;
                break;
            case GL_FLOAT:
                sret = ret = GL_DEPTH32F_STENCIL8;
                break;
            default:
                sret = ret = GL_DEPTH_STENCIL;
                break;
            }
        } else {
            sret = ret = GL_RGBA;
        }
        break;
    case GL_STENCIL_INDEX8:
        if (hardext.stenciltex) {
            sret = ret = GL_STENCIL_INDEX8;
        } else {
            sret = ret = (hardext.rgtex) ? GL_RED : GL_LUMINANCE;
        }
        break;
    case GL_R11F_G11F_B10F:
        ret = GL_R11F_G11F_B10F;
        sret = GL_R11F_G11F_B10F;
        break;
    default:
        if (hardext.depthstencil && format == GL_DEPTH_STENCIL) {
            sret = ret = GL_DEPTH_STENCIL;
            break;
        }
        if (hardext.depthtex && format == GL_DEPTH_COMPONENT) {
            sret = ret = GL_DEPTH_COMPONENT;
            break;
        }
        ret = GL_RGBA;
        sret = GL_RGBA;
        break;
        // Default...RGBA / RGBA will be fine....
    }
    *internalformat = ret;
    return sret;
}

static int get_shrinklevel(int width, int height, int level) {
    int shrink = 0;
    int mipwidth = width << level;
    int mipheight = height << level;
    switch (globals4es.texshrink) {
    case 0: // nothing
        break;
    case 1: // everything / 2
        if ((mipwidth > 1) && (mipheight > 1)) {
            shrink = 1;
        }
        break;
    case 2: // only > 512 /2
    case 7: // only > 512 /2 , but not for empty texture
        if (((mipwidth % 2 == 0) && (mipheight % 2 == 0)) &&
            (((mipwidth > 512) && (mipheight > 8)) || ((mipheight > 512) && (mipwidth > 8)))) {
            shrink = 1;
        }
        break;
    case 3: // only > 256 /2
        if (((mipwidth % 2 == 0) && (mipheight % 2 == 0)) &&
            (((mipwidth > 256) && (mipheight > 8)) || ((mipheight > 256) && (mipwidth > 8)))) {
            shrink = 1;
        }
        break;
    case 4: // only > 256 /2, >=1024 /4
        if (((mipwidth % 4 == 0) && (mipheight % 4 == 0)) &&
            (((mipwidth > 256) && (mipheight > 8)) || ((mipheight > 256) && (mipwidth > 8)))) {
            if ((mipwidth >= 1024) || (mipheight >= 1024)) {
                shrink = 2;
            } else {
                shrink = 1;
            }
        }
        break;
    case 5: // every > 256 is downscaled to 256, but not for empty texture
        if (((mipwidth % 4 == 0) && (mipheight % 4 == 0)) &&
            (((mipwidth > 256) && (mipheight > 8)) || ((mipheight > 256) && (mipwidth > 8)))) {
            if ((mipwidth > 256) || (mipheight > 256)) {
                while (((mipwidth > 256) && (mipheight > 4)) || ((mipheight > 256) && (mipwidth > 4))) {
                    width /= 2;
                    height /= 2;
                    mipwidth /= 2;
                    mipheight /= 2;
                    shrink += 1;
                }
            } else {
                shrink = 1;
            }
        }
        break;
    case 6: // only > 128 /2, >=512 is downscaled to 256, but not for empty texture
        if (((mipwidth % 2 == 0) && (mipheight % 2 == 0)) &&
            (((mipwidth > 128) && (mipheight > 8)) || ((mipheight > 128) && (mipwidth > 8)))) {
            if ((mipwidth >= 512) || (mipheight >= 512)) {
                while (((mipwidth > 256) && (mipheight > 8)) || ((mipheight > 256) && (mipwidth > 8))) {
                    width /= 2;
                    height /= 2;
                    mipwidth /= 2;
                    mipheight /= 2;
                    shrink += 1;
                }
            } else {
                shrink = 1;
            }
        }
        break;
    case 8: // advertise *4 max texture size, but >2048 are shrinked to 2048
        if ((mipwidth > hardext.maxsize * 2) || (mipheight > hardext.maxsize * 2)) {
            shrink = 2;
        } else if ((mipwidth > hardext.maxsize) || (mipheight > hardext.maxsize)) {
            shrink = 1;
        }
        break;
    case 9: // advertise 8192 max texture size, but >4096 are quadshrinked and >512 are shrinked, but not for empty
            // texture
        if ((mipwidth > hardext.maxsize * 2) || (mipheight > hardext.maxsize * 2)) {
            shrink = 2;
        } else if (mipwidth > (hardext.maxsize >> 2) || mipheight > (hardext.maxsize >> 2)) {
            shrink = 1;
        }
        break;
    case 10: // advertise 8192 max texture size, but >2048 are quadshrinked and >512 are shrinked, but not for empty
             // texture
        if ((mipwidth > hardext.maxsize) || (mipheight > hardext.maxsize)) {
            shrink = 2;
        } else if ((mipwidth > (hardext.maxsize >> 2)) || (mipheight > (hardext.maxsize >> 2))) {
            shrink = 1;
        }
        break;
    case 11: // scale down to maxres any dimension > maxres
        if (mipwidth > hardext.maxsize || mipheight > hardext.maxsize) shrink = 1;
        break;
    }

    return shrink;
}

int wrap_npot(GLenum wrap) {
    switch (wrap) {
    case 0:
        return (globals4es.defaultwrap) ? 1 : 0;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
    case GL_CLAMP_TO_BORDER:
        return 1;
    }
    return 0;
}
int minmag_npot(GLenum mag) {
    switch (mag) {
    case 0:
        return 0; // default is not good
    case GL_NEAREST:
    case GL_LINEAR:
        return 1;
    }
    return 0;
}

GLenum minmag_forcenpot(GLenum filt) {
    switch (filt) {
    case GL_LINEAR:
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR;
    /*case 0:
    case GL_NEAREST:
    case GL_NEAREST_MIPMAP_NEATEST:
    case GL_NEAREST_MIPMAP_LINEAR:*/
    default:
        return GL_NEAREST;
    }
}

GLenum wrap_forcenpot(GLenum wrap) {
    switch (wrap) {
    case 0:
        return GL_CLAMP_TO_EDGE;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
    case GL_CLAMP_TO_BORDER:
        return wrap;
        /*case GL_MIRROR_CLAMP_TO_EDGE_EXT:
            return wrap;*/
    }
    return GL_CLAMP_TO_EDGE;
}

GLenum minmag_float(GLenum filt) {
    switch (filt) {
    case GL_LINEAR:
        return GL_NEAREST;
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_LINEAR:
    case GL_NEAREST_MIPMAP_LINEAR:
        return GL_NEAREST_MIPMAP_NEAREST;
    default:
        return filt;
    }
}

void APIENTRY_GL4ES gl4es_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                                       GLint border, GLenum format, GLenum type, const GLvoid* data) {
    DBG(DBGLOGD("glTexImage2D on target=%s with unpack_row_length(%i), size(%i,%i) and skip(%i,%i), "
                "format(internal)=%s(%s), type=%s, data=%p, level=%i (mipmap_need=%i, mipmap_auto=%i, base_level=%i, "
                "max_level=%i) => texture=%u (streamed=%i), glstate->list.compiling=%d\n",
                PrintEnum(target), glstate->texture.unpack_row_length, width, height,
                glstate->texture.unpack_skip_pixels, glstate->texture.unpack_skip_rows, PrintEnum(format),
                (internalformat == 3) ? "3" : (internalformat == 4 ? "4" : PrintEnum(internalformat)), PrintEnum(type),
                data, level, glstate->texture.bound[glstate->texture.active][what_target(target)]->mipmap_need,
                glstate->texture.bound[glstate->texture.active][what_target(target)]->mipmap_auto,
                glstate->texture.bound[glstate->texture.active][what_target(target)]->base_level,
                glstate->texture.bound[glstate->texture.active][what_target(target)]->max_level,
                glstate->texture.bound[glstate->texture.active][what_target(target)]->texture,
                glstate->texture.bound[glstate->texture.active][what_target(target)]->streamed,
                glstate->list.compiling);)

    if (width == 0 || height == 0) {
        DBG(SHUT_LOGE("Error: width or height is zero."))
        return;
    }
    // pre-format handling
    if (format == GL_DEPTH_COMPONENT) {
        internalformat = GL_DEPTH_COMPONENT;
        type = GL_UNSIGNED_INT;
    }
    if (internalformat == GL_RGBA16) {
        internalformat = GL_RGBA16F;
        type = GL_FLOAT;
    } else if (internalformat == GL_RGBA16_SNORM) {
        internalformat = GL_RGBA;
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

    // proxy case
    const GLuint itarget = what_target(target);
    const GLuint rtarget = map_tex_target(target);
    LOAD_GLES(glTexImage2D);
    LOAD_GLES(glTexSubImage2D);
    void gles_glTexParameteri(glTexParameteri_ARG_EXPAND); // LOAD_GLES(glTexParameteri);

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
    // actually bound if targeting shared TEX2D
    realize_bound(glstate->texture.active, target);

    if (glstate->list.pending) {
        gl4es_flush();
    } else {
        PUSH_IF_COMPILING(glTexImage2D);
    }

#ifdef __BIG_ENDIAN__
    if (type == GL_UNSIGNED_INT_8_8_8_8)
#else
    if (type == GL_UNSIGNED_INT_8_8_8_8_REV)
#endif
        type = GL_UNSIGNED_BYTE;

    if (type == GL_HALF_FLOAT) type = GL_HALF_FLOAT_OES;

    /*if(format==GL_COMPRESSED_LUMINANCE)
        format = GL_RGB;*/    // Danger from the Deep does that.
        //That's odd, probably a bug (line 453 of src/texture.cpp, it should be interformat instead of format)

    GLvoid* datab = (GLvoid*)data;

    if (glstate->vao->unpack) datab = (char*)datab + (uintptr_t)glstate->vao->unpack->data;

    GLvoid* pixels = (GLvoid*)datab;
    border = 0; // TODO: something?
    noerrorShim();

    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];

    // Special case when resizing an attached to FBO texture, that is attached to depth and/or stencil => resizing is
    // specific then
    if (bound->binded_fbo &&
        (bound->binded_attachment == GL_DEPTH_ATTACHMENT || bound->binded_attachment == GL_STENCIL_ATTACHMENT ||
         bound->binded_attachment == GL_DEPTH_STENCIL_ATTACHMENT)) {
        // non null data should be handled, but need to convert then...
        if (data != NULL) {
            DBGLOGD("LIBGL: Warning, Depth/stencil texture resized and with data\n");
        }
        // get new size...
        GLsizei nheight = (hardext.npot) ? height : npot(height);
        GLsizei nwidth = (hardext.npot) ? width : npot(width);
        bound->npot = (nheight != height || nwidth != width);
        bound->nwidth = nwidth;
        bound->nheight = nheight;
        bound->width = width;
        bound->height = height;
        // resize depth texture of renderbuffer?
        if (bound->binded_attachment == GL_DEPTH_ATTACHMENT ||
            bound->binded_attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            if (bound->renderdepth) {
                gl4es_glBindRenderbuffer(GL_RENDERBUFFER, bound->renderdepth);
                gl4es_glRenderbufferStorage(GL_RENDERBUFFER,
                                            (bound->binded_attachment == GL_DEPTH_ATTACHMENT) ? GL_DEPTH_COMPONENT16
                                                                                              : GL_DEPTH24_STENCIL8,
                                            nwidth, nheight);
                gl4es_glBindRenderbuffer(GL_RENDERBUFFER, 0);
            } else {
                errorGL();
                DBG(DBGLOGD("gles_glTexImage2D(%d, %d, %s, %d, %d, %d, %s, %s, 0x%x)\n", GL_TEXTURE_2D, 0,
                            PrintEnum(bound->internalformat), bound->nwidth, bound->nheight, 0,
                            PrintEnum(bound->format), PrintEnum(bound->type), NULL);)
                gles_glTexImage2D(GL_TEXTURE_2D, 0, bound->internalformat, bound->nwidth, bound->nheight, 0,
                                  bound->format, bound->type, NULL);
                DBG(CheckGLError(1);)
            }
        }
        if ((bound->binded_attachment == GL_STENCIL_ATTACHMENT ||
             bound->binded_attachment == GL_DEPTH_STENCIL_ATTACHMENT) &&
            bound->renderstencil) {
            gl4es_glBindRenderbuffer(GL_RENDERBUFFER, bound->renderstencil);
            gl4es_glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, nwidth, nheight);
            gl4es_glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
        // all done, exit
        errorGL();
        return;
    }

    if (target == GL_TEXTURE_RECTANGLE_ARB) {
        // change sampler state
        bound->sampler.min_filter = minmag_forcenpot(bound->sampler.min_filter);
        bound->sampler.wrap_s = wrap_forcenpot(bound->sampler.wrap_s);
        bound->sampler.wrap_t = wrap_forcenpot(bound->sampler.wrap_t);
        bound->sampler.wrap_r = wrap_forcenpot(bound->sampler.wrap_r);
    }

    if (target == GL_TEXTURE_RECTANGLE_ARB) {
        // change sampler state
        bound->sampler.min_filter = minmag_forcenpot(bound->sampler.min_filter);
        bound->sampler.wrap_s = wrap_forcenpot(bound->sampler.wrap_s);
        bound->sampler.wrap_t = wrap_forcenpot(bound->sampler.wrap_t);
        bound->sampler.wrap_r = wrap_forcenpot(bound->sampler.wrap_r);
    }

    bound->alpha = pixel_hasalpha(format);
    // fpe internal format tracking
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
        /*case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
        case GL_DEPTH_STENCIL:
        case GL_DEPTH24_STENCIL8:
            bound->fpe_format = FPE_TEX_COMPONENT;*/
        default:
            bound->fpe_format = FPE_TEX_RGBA;
        }
    }
    if (GL4ES_AUTOMIPMAP_PLACEHOLDER) {
        if (level > 0) {
            if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 3) {
                return; // no mipmap support
            } else if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 2) {
                bound->mipmap_need = 1;
            }
        }
    }
    // if (level > 0 && (bound->npot && globals4es.forcenpot)) return; // no mipmap...
    if (level == 0 || !bound->valid) {
        bound->wanted_internal = internalformat; // save it before transformation
    }
    GLenum new_format = swizzle_internalformat((GLenum*)&internalformat, format, type);
    if (level == 0 || !bound->valid) {
        bound->orig_internal = internalformat;
        bound->internalformat = new_format;
    }
    // shrink checking
    int mipwidth = width << level;
    int mipheight = height << level;
    int shrink = 0;
    if (!bound->valid)
        bound->shrink = shrink = get_shrinklevel(width, height, level);
    else
        shrink = bound->shrink;

    // if (((width >> shrink) == 0) && ((height >> shrink) == 0)) return; // nothing to do
    if (datab) {

        // implements GL_UNPACK_ROW_LENGTH
        if ((glstate->texture.unpack_row_length && glstate->texture.unpack_row_length != width) ||
            glstate->texture.unpack_skip_pixels || glstate->texture.unpack_skip_rows) {
            int imgWidth, pixelSize, dstWidth;
            pixelSize = pixel_sizeof(format, type);
            imgWidth = ((glstate->texture.unpack_row_length) ? glstate->texture.unpack_row_length : width) * pixelSize;
            GLubyte* dst = (GLubyte*)malloc(width * height * pixelSize);
            pixels = (GLvoid*)dst;
            dstWidth = width * pixelSize;
            const GLubyte* src = (GLubyte*)datab;
            src += glstate->texture.unpack_skip_pixels * pixelSize + glstate->texture.unpack_skip_rows * imgWidth;

            for (int y = height; y; --y) { /*
                 if (dst == NULL || src == NULL) {
                     DBGLOGD("LIBGL: Invalid memory pointers in memcpy (src=%p, dst=%p)\n", src, dst);
                     return;  // Exit early or handle the error
                 }
                 // Before the unpacking loop
                 if (dstWidth <= 0 || imgWidth <= 0) {
                     DBGLOGD("LIBGL: Invalid buffer sizes for memcpy (dstWidth=%d, imgWidth=%d)\n", dstWidth,
                 imgWidth); return;  // Exit early or handle the error
                 }
                 if ((uintptr_t)src % sizeof(void*) != 0 || (uintptr_t)dst % sizeof(void*) != 0) {
                     DBGLOGD("LIBGL: Memory is not aligned correctly for memcpy (src=%p, dst=%p)\n", src, dst);
                     return;  // Exit early or handle the error
                 }
                 if (width <= 0 || height <= 0) {
                     DBGLOGD("LIBGL: Invalid width or height for texture update (width=%d, height=%d)\n", width,
                 height); return;  // Exit early or handle the error
                 }
                 if ((uintptr_t)src + height * imgWidth > (uintptr_t)(src + height * imgWidth)) {
                     DBGLOGD("LIBGL: Source buffer overflow detected (src=%p, expected=%p)\n", src, (src + height *
                 imgWidth)); return;  // Exit early or handle the error
                 }
                 if ((uintptr_t)dst + height * dstWidth > (uintptr_t)(dst + height * dstWidth)) {
                     DBGLOGD("LIBGL: Destination buffer overflow detected (dst=%p, expected=%p)\n", dst, (dst + height
                 * dstWidth)); return;  // Exit early or handle the error
                 }*/

                memcpy(dst, src, dstWidth);
                src += imgWidth;
                dst += dstWidth;
            }
        }

#ifndef __BIG_ENDIAN__
        if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8 && !hardext.rgba8888) {
            GLvoid* conv = pixels;
            if (!pixel_convert(pixels, &conv, width, height, format, type, format, GL_UNSIGNED_BYTE, 0,
                               glstate->texture.unpack_align)) {
                DBGLOGD("LIBGL: Error converting GL_UNSIGNED_INT_8_8_8_8 to GL_UNSIGNED_BYTE\n");
            } else {
                if (conv != pixels && pixels != datab) free(pixels);
                pixels = conv;
                type = GL_UNSIGNED_BYTE;
            }
        }
#endif

        GLvoid* old = pixels;
        pixels = (GLvoid*)swizzle_texture(width, height, &format, &type, internalformat, new_format, old, bound);
        if (old != pixels && old != datab) {
            free(old);
        }

        if (bound->shrink != 0) {
            switch (globals4es.texshrink) {
            case 0: // nothing ???
                break;
            case 1: // everything / 2
            case 11:
                if ((mipwidth > 1) && (mipheight > 1)) {
                    GLvoid* out = pixels;
                    GLfloat ratiox, ratioy;
                    int newwidth = mipwidth;
                    int newheight = mipheight;
                    if (globals4es.texshrink == 11) {
                        if (mipwidth > hardext.maxsize) newwidth = hardext.maxsize;
                        if (mipheight > hardext.maxsize) newheight = hardext.maxsize;
                    } else {
                        newwidth = mipwidth / 2;
                        newheight = mipheight / 2;
                        if (!newwidth) newwidth = 1;
                        if (!newheight) newheight = 1;
                    }
                    if (level && bound->valid) {
                        // don't recalculate ratio...
                        ratiox = bound->ratiox;
                        ratioy = bound->ratioy;
                    } else {
                        ratiox = newwidth / ((float)mipwidth);
                        ratioy = newheight / ((float)mipheight);
                    }
                    newwidth = width * ratiox;
                    newheight = height * ratioy;
                    if (ratiox == 0.5f && ratioy == 0.5f && npot(width) == width && npot(height) == height) {
                        // prefer the fast and clean way first
                        pixel_halfscale(pixels, &out, width, height, format, type);
                    } else {
                        bound->ratiox = ratiox;
                        bound->ratioy = ratioy;
                        bound->useratio = 1;
                        pixel_scale(pixels, &out, width, height, newwidth, newheight, format, type);
                    }
                    if (out != pixels && pixels != datab) free(pixels);
                    pixels = out;
                    width = newwidth;
                    height = newheight;
                    bound->shrink = 1;
                }
                break;
            default:
                if (!bound->valid) bound->ratiox = bound->ratioy = 1.0f;
                while (shrink) {
                    int toshrink = (shrink > 1) ? 2 : 1;
                    GLvoid* out = pixels;
                    if (toshrink == 1) {
                        pixel_halfscale(pixels, &out, width, height, format, type);
                        if (!bound->valid) {
                            bound->ratiox *= 0.5f;
                            bound->ratioy *= 0.5f;
                        }
                    } else {
                        pixel_quarterscale(pixels, &out, width, height, format, type);
                        if (!bound->valid) {
                            bound->ratiox *= 0.25f;
                            bound->ratioy *= 0.25f;
                        }
                    }
                    if (out != pixels && pixels != datab) free(pixels);
                    pixels = out;
                    width = nlevel(width, toshrink);
                    height = nlevel(height, toshrink);
                    shrink -= toshrink;
                }
            }
        }

        if (globals4es.texdump) {
            pixel_to_ppm(pixels, width, height, format, type, bound->texture, glstate->texture.pack_align);
        }
    } else {
#ifdef TEXSTREAM
        if (globals4es.texstream && (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE_ARB) &&
                (width >= 256 && height >= 224) &&
                ((internalformat == GL_RGB) || (internalformat == 3) || (internalformat == GL_RGB8) ||
                 (internalformat == GL_BGR) || (internalformat == GL_RGB5) || (internalformat == GL_RGB565)) ||
            (globals4es.texstream == 2)) {
            bound->streamingID = AddStreamed(width, height, bound->texture);
            if (bound->streamingID > -1) { // success
                bound->shrink = 0;         // no shrink on Stream texture
                bound->streamed = true;
                glsampler_t* sampler = glstate->samplers.sampler[glstate->texture.active];
                if (!sampler) sampler = &bound->sampler;
                ApplyFilterID(bound->streamingID, sampler->min_filter, sampler->mag_filter);
                GLboolean tmp = IS_ANYTEX(glstate->enable.texture[glstate->texture.active]);
                LOAD_GLES(glDisable);
                LOAD_GLES(glEnable);
                if (tmp) gles_glDisable(GL_TEXTURE_2D);
                ActivateStreaming(bound->streamingID); // Activate the newly created texture
                format = GL_RGB;
                type = GL_UNSIGNED_SHORT_5_6_5;
                if (tmp) gles_glEnable(GL_TEXTURE_STREAM_IMG);
            }
            glstate->bound_stream[glstate->texture.active] = 1;
        }
#endif
        if (!bound->streamed && internalformat != GL_R11F_G11F_B10F && internalformat != GL_R32F &&
            internalformat != GL_RGB10_A2)
            swizzle_texture(width, height, &format, &type, internalformat, new_format, NULL,
                            bound); // convert format even if data is NULL
        if (internalformat == GL_R11F_G11F_B10F || internalformat == GL_R32F) {
            type = GL_FLOAT;
            if (format == GL_BGRA) format = GL_RGB;
        }
        if (internalformat == GL_RGB10_A2) {
            if (format == GL_BGRA) format = GL_RGBA;
            type = GL_UNSIGNED_INT_2_10_10_10_REV;
        }
        if (internalformat == GL_RGB8 && format == GL_RGBA) {
            internalformat = GL_RGBA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
        }
        if (type == GL_HALF_FLOAT_OES) {
            type = GL_HALF_FLOAT;
        }
        if (bound->shrink != 0) {
            switch (globals4es.texshrink) {
            case 1: // everything / 2
            case 11:
                if ((mipwidth > 1) && (mipheight > 1)) {
                    GLfloat ratiox, ratioy;
                    if (globals4es.texshrink == 11) {
                        if (mipwidth > hardext.maxsize)
                            ratiox = hardext.maxsize / ((float)mipwidth);
                        else
                            ratiox = 1.0f;
                        if (mipheight > hardext.maxsize)
                            ratioy = hardext.maxsize / ((float)mipheight);
                        else
                            ratioy = 1.0f;
                    } else
                        ratiox = ratioy = 0.5;
                    bound->ratiox = ratiox;
                    bound->ratioy = ratioy;
                    bound->useratio = 1;
                    int newwidth = width * bound->ratiox;
                    int newheight = height * bound->ratioy;
                    if (globals4es.texshrink == 11 && newwidth > hardext.maxsize)
                        newwidth = hardext.maxsize; // in case of some rounding error
                    if (globals4es.texshrink == 11 && newheight > hardext.maxsize)
                        newheight = hardext.maxsize; // in case of some rounding error
                    width = newwidth;
                    height = newheight;
                    bound->shrink = 1;
                }
                break;
            default:
                bound->ratiox = bound->ratioy = 1.0f;
                while (shrink) {
                    int toshrink = (shrink > 1) ? 2 : 1;
                    if (toshrink == 1) {
                        bound->ratiox *= 0.5f;
                        bound->ratioy *= 0.5f;
                    } else {
                        bound->ratiox *= 0.25f;
                        bound->ratioy *= 0.25f;
                    }
                    width = nlevel(width, toshrink);
                    height = nlevel(height, toshrink);
                    shrink -= toshrink;
                }
            }
        }
    }

    /* TODO:
    GL_INVALID_VALUE is generated if border is not 0.
    GL_INVALID_OPERATION is generated if type is
    GL_UNSIGNED_SHORT_5_6_5 and format is not GL_RGB.

    GL_INVALID_OPERATION is generated if type is one of
    GL_UNSIGNED_SHORT_4_4_4_4, or GL_UNSIGNED_SHORT_5_5_5_1
    and format is not GL_RGBA.
    */

    // Guard against illegal GLES combos like format=RGBA with single/dual channel internalformat.
    if ((format == GL_RGBA || format == GL_BGRA) &&
        (internalformat == GL_R || internalformat == GL_RED || internalformat == GL_R8 ||
         internalformat == GL_R8_SNORM || internalformat == GL_R16F || internalformat == GL_R32F ||
         internalformat == GL_RG || internalformat == GL_RG8 || internalformat == GL_RG8_SNORM ||
         internalformat == GL_RG16F || internalformat == GL_RG32F)) {
        internalformat = GL_RGBA;
    }

    int limitednpot = 0;
    {
        GLsizei nheight = (hardext.npot == 3) ? height : npot(height);
        GLsizei nwidth = (hardext.npot == 3) ? width : npot(width);
        bound->npot = (nheight != height || nwidth != width); // hardware that fully support NPOT doesn't care anyway
        if (bound->npot) {
            if (target == GL_TEXTURE_RECTANGLE_ARB && hardext.npot)
                limitednpot = 1;
            else if (hardext.npot == 1 &&
                     ((bound->base_level <= 0 && bound->max_level == 0) || (GL4ES_AUTOMIPMAP_PLACEHOLDER == 3) ||
                      (GL4ES_AUTOMIPMAP_PLACEHOLDER == 4 && width != height) || (globals4es.forcenpot == 1)) &&
                     (wrap_npot(bound->sampler.wrap_s) && wrap_npot(bound->sampler.wrap_t)))
                limitednpot = 1;
            else if (hardext.esversion > 1 && hardext.npot == 1 &&
                     (!bound->mipmap_auto || !minmag_npot(bound->sampler.min_filter) ||
                      !minmag_npot(bound->sampler.mag_filter)) &&
                     (wrap_npot(bound->sampler.wrap_s) && wrap_npot(bound->sampler.wrap_t)))
                limitednpot = 1;
            else if (hardext.esversion > 1 && hardext.npot == 2 &&
                     (wrap_npot(bound->sampler.wrap_s) && wrap_npot(bound->sampler.wrap_t)))
                limitednpot = 1;

            if (limitednpot) {
                nwidth = width;
                nheight = height;
            }
        }
#ifdef PANDORA
#define NO_1x1
#endif
#ifdef NO_1x1
#define MIN_SIZE 2
        if (level == 0 && hardext.esversion == 1) {
            if (nwidth < MIN_SIZE && nheight < MIN_SIZE) {
                nwidth = MIN_SIZE;
                nheight = MIN_SIZE;
            }
        }
#undef MIN_SIZE
#endif
        if (globals4es.texstream && bound->streamed) {
            nwidth = width;
            nheight = height;
        }

        if (bound->npot) {
            if (limitednpot && rtarget == GL_TEXTURE_2D) {
                bound->sampler.wrap_t = bound->sampler.wrap_s = GL_CLAMP_TO_EDGE;
            } else if (!wrap_npot(bound->sampler.wrap_s) || !wrap_npot(bound->sampler.wrap_t)) {
                // resize to npot boundaries (not ideal if the wrap value is change after upload of the texture)
                if (level == 0 || bound->width == 0) {
                    nwidth = npot(width);
                    nheight = npot(height);
                } else {
                    nwidth = npot(nlevel(bound->width, level));
                    nheight = npot(nlevel(bound->height, level));
                }
                float ratiox, ratioy;
                ratiox = ((float)nwidth) / width;
                ratioy = ((float)nheight) / height;

                GLvoid* out = pixels;
                if (pixels) pixel_scale(pixels, &out, width, height, nwidth, nheight, format, type);
                if (out != pixels && pixels != datab) free(pixels);
                pixels = out;
                width = nwidth;
                height = nheight;
                limitednpot = 0;

                if (level == 0) {
                    if (!bound->useratio) {
                        bound->useratio = 1;
                        if (bound->ratiox == 0.f) bound->ratiox = bound->ratioy = 1.0f;
                    }
                    bound->ratiox *= ratiox;
                    bound->ratioy *= ratioy;
                    bound->npot = 0;
                    bound->shrink = 1;
                }
            }
        }
        if ((GL4ES_AUTOMIPMAP_PLACEHOLDER == 4) && (nwidth != nheight)) bound->mipmap_auto = 0;

        if (level == 0) {
            bound->width = width;
            bound->height = height;
            bound->nwidth = nwidth;
            bound->nheight = nheight;
            if (target == GL_TEXTURE_RECTANGLE_ARB && hardext.esversion == 2) {
                bound->adjust = 0; // because this test is used in a lot of places
                bound->adjustxy[0] = 1.0f / width;
                bound->adjustxy[1] = 1.0f / height;
            } else {
                // TEXTURE_RECTANGLE could be mutualize with npot texture probably
                bound->adjust = (width != nwidth || height != nheight);
                bound->adjustxy[0] = (float)width / nwidth;
                bound->adjustxy[1] = (float)height / nheight;
            }
            bound->compressed = 0;
            bound->valid = 1;
        }

        int callgeneratemipmap = 0;
        if (!(globals4es.texstream && bound->streamed)) {
            if ((target != GL_TEXTURE_RECTANGLE_ARB) && (GL4ES_AUTOMIPMAP_PLACEHOLDER != 3) &&
                (bound->mipmap_need || bound->mipmap_auto) && !(bound->npot && hardext.npot < 2) &&
                (bound->max_level == -1)) {
                if (hardext.esversion < 2)
                    gles_glTexParameteri(rtarget, GL_GENERATE_MIPMAP, GL_TRUE);
                else
                    callgeneratemipmap = 1;
            } else {
                // if(target!=GL_TEXTURE_RECTANGLE_ARB) gles_glTexParameteri( rtarget, GL_GENERATE_MIPMAP, GL_FALSE );
                if ((itarget != ENABLED_CUBE_MAP && target != GL_TEXTURE_RECTANGLE_ARB) &&
                    (bound->mipmap_need || GL4ES_AUTOMIPMAP_PLACEHOLDER == 3)) {
                    // remove the need for mipmap...
                    bound->mipmap_need = 0;
                    gl4es_glTexParameteri(rtarget, GL_TEXTURE_MIN_FILTER,
                                          bound->sampler.min_filter); // forcing min_filter to be recomputed
                }
            }

            if (height != nheight || width != nwidth) {
                errorGL();
                gles_glTexImage2D(rtarget, level, internalformat, nwidth, nheight, border, format, type, NULL);
                DBG(CheckGLError(1);)
                if (pixels) {
                    gles_glTexSubImage2D(rtarget, level, 0, 0, width, height, format, type, pixels);
                    DBG(CheckGLError(1);)
                }
#ifdef NO_1x1
                if (level == 0 && (width == 1 && height == 1 && pixels)) {
                    // complete the texture, juste in case it use GL_REPEAT
                    // also, don't keep the fact we have resized, the non-adjusted coordinates will work (as the texture
                    // is enlarged)
                    if (width == 1) {
                        gles_glTexSubImage2D(rtarget, level, 1, 0, width, height, format, type, pixels);
                        nwidth = 1;
                    }
                    if (height == 1) {
                        gles_glTexSubImage2D(rtarget, level, 0, 1, width, height, format, type, pixels);
                        nheight = 1;
                    }
                    if (width == 1 && height == 1) { // create a manual mipmap just in case_state
                        gles_glTexSubImage2D(rtarget, level, 1, 1, width, height, format, type, pixels);
                        gles_glTexImage2D(rtarget, 1, internalformat, 1, 1, 0, format, type, pixels);
                    }
                }
#endif
            } else {
                errorGL();
                gles_glTexImage2D(rtarget, level, internalformat, width, height, border, format, type, pixels);
                DBG(CheckGLError(1);)
            }
            // check if base_level is set... and calculate lower level mipmap
            if (bound->base_level == level && !(bound->max_level == level && level == 0)) {
                int leveln = level, nw = width, nh = height, nww = nwidth, nhh = nheight;
                int pot = (nh == nhh && nw == nww);
                void* ndata = pixels;
                while (leveln) {
                    if (pixels) {
                        GLvoid* out = ndata;
                        pixel_doublescale(ndata, &out, nw, nh, format, type);
                        if (out != ndata && ndata != pixels) free(ndata);
                        ndata = out;
                    }
                    nw <<= 1;
                    nh <<= 1;
                    nww <<= 1;
                    nhh <<= 1;
                    --leveln;
                    gles_glTexImage2D(rtarget, leveln, internalformat, nww, nhh, border, format, type,
                                      (pot) ? ndata : NULL);
                    if (!pot && pixels) gles_glTexSubImage2D(rtarget, leveln, 0, 0, nw, nh, format, type, ndata);
                }
                if (ndata != pixels) free(ndata);
            }
            if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 5 && !level) bound->mipmap_done = 0;
            // check if max_level is set... and calculate higher level mipmap
            if (((bound->max_level == level && (level || bound->mipmap_need)) || (callgeneratemipmap && level == 0) ||
                 (GL4ES_AUTOMIPMAP_PLACEHOLDER == 5 && level && !bound->mipmap_done)) &&
                !(bound->max_level == bound->base_level && bound->max_level == 0)) {
                if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 5 && level == 1) bound->mipmap_done = 1;
                int leveln = level, nw = nwidth, nh = nheight, nww = width, nhh = height;
                int pot = (nh == nhh && nw == nww);
                void* ndata = pixels;
                while (nw != 1 || nh != 1) {
                    if (pixels) {
                        GLvoid* out = ndata;
                        pixel_halfscale(ndata, &out, nww, nhh, format, type);
                        if (out != ndata && ndata != pixels) free(ndata);
                        ndata = out;
                    }
                    nw = nlevel(nw, 1);
                    nh = nlevel(nh, 1);
                    nww = nlevel(nww, 1);
                    nhh = nlevel(nhh, 1);
                    ++leveln;
                    gles_glTexImage2D(rtarget, leveln, internalformat, nw, nh, border, format, type,
                                      (pot) ? ndata : NULL);
                    if (!pot && pixels) gles_glTexSubImage2D(rtarget, leveln, 0, 0, nww, nhh, format, type, ndata);
                }
                if (ndata != pixels) free(ndata);
            }
            /*if (bound && bound->mipmap_need && !bound->mipmap_auto && (GL4ES_AUTOMIPMAP_PLACEHOLDER!=3))
                gles_glTexParameteri( rtarget, GL_GENERATE_MIPMAP, GL_FALSE );*/
        } else {
            if (pixels)
                gl4es_glTexSubImage2D(rtarget, level, 0, 0, width, height, format, type,
                                      pixels); // (should never happens) updload the 1st data...
        }
        if (bound->mipmap_need && !bound->mipmap_done) {
            if (bound->mipmap_auto) bound->mipmap_done = 1;
            if (level == maxlevel(bound->width, bound->height)) bound->mipmap_done = 1;
        }
    }
    if ((target == GL_TEXTURE_2D) && globals4es.texcopydata &&
        ((globals4es.texstream && !bound->streamed) || !globals4es.texstream)) {
        if (bound->data)
            bound->data = realloc(bound->data, width * height * 4);
        else
            bound->data = malloc(width * height * 4);
        if (datab) {
            if (!pixel_convert(pixels, &bound->data, width, height, format, type, GL_RGBA, GL_UNSIGNED_BYTE, 0,
                               glstate->texture.unpack_align))
                DBGLOGD("LIBGL: Error on pixel_convert when TEXCOPY in glTexImage2D\n");
        } else {
            // memset(bound->data, 0, width*height*4);
        }
    }
    if (pixels != datab) {
        free(pixels);
    }
    // update max bound to be sure "sampler" is applied
    if (glstate->bound_changed < glstate->texture.active + 1) glstate->bound_changed = glstate->texture.active + 1;
}

static size_t pad_to(size_t v, GLint align) {
    if (align <= 1) return v;
    size_t rem = v % (size_t)align;
    return rem ? v + ((size_t)align - rem) : v;
}

void old_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, const GLvoid* data) {

    if (width == 0 || height == 0) {
        DBG(SHUT_LOGE("Error: width or height is zero."))
        return;
    }

    if (glstate->list.pending) {
        gl4es_flush();
    } else {
        PUSH_IF_COMPILING(glTexSubImage2D);
    }
    realize_bound(glstate->texture.active, target);

#ifdef __BIG_ENDIAN__
    if (type == GL_UNSIGNED_INT_8_8_8_8)
#else
    if (type == GL_UNSIGNED_INT_8_8_8_8_REV)
#endif
        type = GL_UNSIGNED_BYTE;

    GLvoid* datab = (GLvoid*)data;
    if (glstate->vao->unpack) datab = (char*)datab + (uintptr_t)glstate->vao->unpack->data;
    GLvoid* pixels = (GLvoid*)datab;

    const GLuint itarget = what_target(target);
    const GLuint rtarget = map_tex_target(target);

    LOAD_GLES(glTexSubImage2D);
    // void gles_glTexParameteri(glTexParameteri_ARG_EXPAND);
    LOAD_GLES(glTexParameteri);
    noerrorShim();
    DBG(DBGLOGD(
            "glTexSubImage2D on target=%s with unpack_row_length(%d), size(%d,%d), pos(%d,%d) and skip={%d,%d}, "
            "format=%s, type=%s, level=%d(base=%d, max=%d), mipmap={need=%d, auto=%d}, texture=%u, data=%p(vao=%p)\n",
            PrintEnum(target), glstate->texture.unpack_row_length, width, height, xoffset, yoffset,
            glstate->texture.unpack_skip_pixels, glstate->texture.unpack_skip_rows, PrintEnum(format), PrintEnum(type),
            level, glstate->texture.bound[glstate->texture.active][itarget]->base_level,
            glstate->texture.bound[glstate->texture.active][itarget]->max_level,
            glstate->texture.bound[glstate->texture.active][itarget]->mipmap_need,
            glstate->texture.bound[glstate->texture.active][itarget]->mipmap_auto,
            glstate->texture.bound[glstate->texture.active][itarget]->texture, data, glstate->vao->unpack);)
    if (width == 0 || height == 0) {
        return;
    }

    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];
    if (bound == NULL) {
        DBGLOGD("LIBGL: Invalid bound texture (bound=%p)\n", bound);
        return; // Exit early or handle the error
    }
    if (GL4ES_AUTOMIPMAP_PLACEHOLDER) {
        if (level > 0) {
            if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 3) {
                return; // no mipmap support
            }
            if (GL4ES_AUTOMIPMAP_PLACEHOLDER == 2) bound->mipmap_need = 1;
        }
    } else if (level && bound->mipmap_auto)
        return;
    if (!(format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_INT) &&
        ((glstate->texture.unpack_row_length && glstate->texture.unpack_row_length != width) ||
         glstate->texture.unpack_skip_pixels || glstate->texture.unpack_skip_rows)) {
        int imgWidth, pixelSize, dstWidth;
        pixelSize = pixel_sizeof(format, type);
        imgWidth = ((glstate->texture.unpack_row_length) ? glstate->texture.unpack_row_length : width) * pixelSize;
        GLubyte* dst = (GLubyte*)malloc(width * height * pixelSize);
        pixels = (GLvoid*)dst;
        dstWidth = width * pixelSize;
        const GLubyte* src = (GLubyte*)datab;
        src += glstate->texture.unpack_skip_pixels * pixelSize + glstate->texture.unpack_skip_rows * imgWidth;
        for (int y = height; y; --y) { /*
             if (dst == NULL || src == NULL) {
                 DBGLOGD("LIBGL: Invalid memory pointers in memcpy (src=%p, dst=%p)\n", src, dst);
                 return;  // Exit early or handle the error
             }
             // Before the unpacking loop
             if (dstWidth <= 0 || imgWidth <= 0) {
                 DBGLOGD("LIBGL: Invalid buffer sizes for memcpy (dstWidth=%d, imgWidth=%d)\n", dstWidth, imgWidth);
                 return;  // Exit early or handle the error
             }
             if ((uintptr_t)src % sizeof(void*) != 0 || (uintptr_t)dst % sizeof(void*) != 0) {
                 DBGLOGD("LIBGL: Memory is not aligned correctly for memcpy (src=%p, dst=%p)\n", src, dst);
                 return;  // Exit early or handle the error
             }
             if (width <= 0 || height <= 0) {
                 DBGLOGD("LIBGL: Invalid width or height for texture update (width=%d, height=%d)\n", width, height);
                 return;  // Exit early or handle the error
             }
             if ((uintptr_t)src + height * imgWidth > (uintptr_t)(src + height * imgWidth)) {
                 DBGLOGD("LIBGL: Source buffer overflow detected (src=%p, expected=%p)\n", src, (src + height *
             imgWidth)); return;  // Exit early or handle the error
             }
             if ((uintptr_t)dst + height * dstWidth > (uintptr_t)(dst + height * dstWidth)) {
                 DBGLOGD("LIBGL: Destination buffer overflow detected (dst=%p, expected=%p)\n", dst, (dst + height *
             dstWidth)); return;  // Exit early or handle the error
             }*/

            memcpy(dst, src, dstWidth);
            src += imgWidth;
            dst += dstWidth;
        }
    }

    GLvoid* old = pixels;
#ifdef TEXSTREAM
    if (globals4es.texstream && (bound->streamed)) {
        // Optimisation, let's do convert directly to the right place...
        GLvoid* tmp = GetStreamingBuffer(bound->streamingID);
        tmp += (yoffset * bound->width + xoffset) * 2;
        if (!pixel_convert(old, &tmp, width, height, format, type, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, bound->width,
                           glstate->texture.unpack_align)) {
            DBGLOGD("LIBGL: swizzle error: (%#4x, %#4x -> GL_RGB, UNSIGNED_SHORT_5_6_5)\n", format, type);
        }
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
    } else
#endif
    {
        if (!pixel_convert(old, &pixels, width, height, format, type, bound->inter_format, bound->inter_type, 0,
                           glstate->texture.unpack_align)) {
            DBGLOGD("LIBGL: Error in pixel_convert while glTexSubImage2D\n");
        } else {
            format = bound->inter_format;
            type = bound->inter_type;
            if (bound->inter_format != bound->format || bound->inter_type != bound->type) {
                GLvoid* pix2 = pixels;
                if (!pixel_convert(pixels, &pix2, width, height, format, type, bound->format, bound->type, 0,
                                   glstate->texture.unpack_align)) {
                    DBGLOGD("LIBGL: Error in pixel_convert while glTexSubImage2D\n");
                }
                if (pixels != pix2 && pixels != old) free(pixels);
                pixels = pix2;
                format = bound->format;
                type = bound->type;
            }
        }
    }
    if (old != pixels && old != datab) free(old);

    if (bound->shrink || bound->useratio) {
        // special case for width/height == 1
        if (width == 1) width += (xoffset % 2);
        if (height == 1) height += (yoffset % 2);
        if ((width == 1) || (height == 1)) {
            // nothing to do...
            if (pixels != datab) free((GLvoid*)pixels);
            return;
        }
        // ok, now standard cases....
        old = pixels;
        if (bound->useratio) {
            xoffset *= bound->ratiox;
            yoffset *= bound->ratioy;
            int newwidth = width * bound->ratiox;
            int newheight = height * bound->ratioy;
            pixel_scale(pixels, &old, width, height, newwidth, newheight, format, type);
            width = newwidth;
            height = newheight;
            if (old != pixels && pixels != datab) free(pixels);
            pixels = old;
        } else {
            if (bound->shrink > 0) {
                int div = 1 << bound->shrink;
                xoffset /= div;
                yoffset /= div;
            }
            int shrink = bound->shrink;
            while (shrink) {
                int toshrink = (shrink > 1) ? 2 : 1;
                GLvoid* out = pixels;
                if (toshrink == 1)
                    pixel_halfscale(pixels, &old, width, height, format, type);
                else
                    pixel_quarterscale(pixels, &old, width, height, format, type);
                if (old != pixels && pixels != datab) free(pixels);
                pixels = old;
                width = nlevel(width, toshrink);
                height = nlevel(height, toshrink);
                shrink -= toshrink;
            }
        }
    }

    if (globals4es.texdump) {
        pixel_to_ppm(pixels, width, height, format, type, bound->texture, glstate->texture.pack_align);
    }

    int callgeneratemipmap = 0;
    if ((target != GL_TEXTURE_RECTANGLE_ARB) && (bound->mipmap_need || bound->mipmap_auto)) {
        if (hardext.esversion < 2) {
            // gles_glTexParameteri( rtarget, GL_GENERATE_MIPMAP, GL_TRUE );
        } else
            callgeneratemipmap = 1;
    }

    if (globals4es.texstream && bound->streamed) {
        /*    // copy the texture to the buffer
            void* tmp = GetStreamingBuffer(bound->streamingID);
            for (int yy=0; yy<height; yy++) {
                memcpy(tmp+((yy+yoffset)*bound->width+xoffset)*2, pixels+(yy*width)*2, width*2);
            }*/
    } else {
        errorGL();
        gles_glTexSubImage2D(rtarget, level, xoffset, yoffset, width, height, format, type, pixels);
        DBG(CheckGLError(1);)
        // check if base_level is set... and calculate lower level mipmap
        if (bound->base_level == level && !(bound->max_level == level && level == 0)) {
            int leveln = level, nw = width, nh = height, xx = xoffset, yy = yoffset;
            void* ndata = pixels;
            while (leveln) {
                if (pixels) {
                    GLvoid* out = ndata;
                    pixel_doublescale(ndata, &out, nw, nh, format, type);
                    if (out != ndata && ndata != pixels) free(ndata);
                    ndata = out;
                }
                nw <<= 1;
                nh <<= 1;
                xx <<= 1;
                yy <<= 1;
                --leveln;
                gles_glTexSubImage2D(rtarget, leveln, xx, yy, nw, nh, format, type, ndata);
            }
            if (ndata != pixels) free(ndata);
        }
        // check if max_level is set... and calculate higher level mipmap
        int genmipmap = 0;
        if (((bound->max_level == level) && (level || bound->mipmap_need))) genmipmap = 1;
        if (callgeneratemipmap && ((level == 0) || (level == bound->max_level))) genmipmap = 1;
        if ((bound->max_level == bound->base_level) && (bound->base_level == 0)) genmipmap = 0;
        if (genmipmap && (GL4ES_AUTOMIPMAP_PLACEHOLDER != 3)) {
            int leveln = level, nw = width, nh = height, xx = xoffset, yy = yoffset;
            void* ndata = pixels;
            while (nw != 1 || nh != 1) {
                if (pixels) {
                    GLvoid* out = ndata;
                    pixel_halfscale(ndata, &out, nw, nh, format, type);
                    if (out != ndata && ndata != pixels) free(ndata);
                    ndata = out;
                }
                nw = nlevel(nw, 1);
                nh = nlevel(nh, 1);
                xx = xx >> 1;
                yy = yy >> 1;
                ++leveln;
                gles_glTexSubImage2D(rtarget, leveln, xx, yy, nw, nh, format, type, ndata);
            }
            if (ndata != pixels) free(ndata);
        }
    }

    /*if (bound->mipmap_need && !bound->mipmap_auto && (GL4ES_AUTOMIPMAP_PLACEHOLDER!=3) && (!globals4es.texstream ||
       (globals4es.texstream && !bound->streamed))) gles_glTexParameteri( rtarget, GL_GENERATE_MIPMAP, GL_FALSE );*/

    if ((target == GL_TEXTURE_2D) && globals4es.texcopydata &&
        ((globals4es.texstream && !bound->streamed) || !globals4es.texstream)) {
        // printf("*texcopy* glTexSubImage2D, xy=%i,%i, size=%i,%i=>%i,%i, format=%s, type=%s, tex=%u\n", xoffset,
        // yoffset, width, height, bound->width, bound->height, PrintEnum(format), PrintEnum(type), bound->glname);
        GLvoid* tmp = (char*)bound->data + (yoffset * bound->width + xoffset) * 4;
        if (!pixel_convert(pixels, &tmp, width, height, format, type, GL_RGBA, GL_UNSIGNED_BYTE, bound->width,
                           glstate->texture.unpack_align))
            DBGLOGD("LIBGL: Error on pixel_convert while TEXCOPY in glTexSubImage2D\n");
    }

    if (pixels != datab) free((GLvoid*)pixels);
}

void APIENTRY_GL4ES gl4es_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                          GLsizei height, GLenum format, GLenum type, const GLvoid* data) {
    if (width == 0 || height == 0) {
        DBG(SHUT_LOGE("Error: width or height is zero."))
        return;
    }

    if (format == GL_BGRA || format == GL_BGR || format == GL_BGRA8_EXT) {
        old_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
        return;
    }

    const GLuint itarget = what_target(target);
    gltexture_t* bound = glstate->texture.bound[glstate->texture.active][itarget];
    if (!bound || glstate->vao->unpack || bound->streamed || bound->shrink || bound->useratio ||
        ((target == GL_TEXTURE_2D) && globals4es.texcopydata &&
         ((globals4es.texstream && !bound->streamed) || !globals4es.texstream))) {
        old_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
        return;
    }

    if (glstate->list.pending) {
        gl4es_flush();
    } else {
        PUSH_IF_COMPILING(glTexSubImage2D);
    }

    realize_bound(glstate->texture.active, target);

    if (width <= 0 || height <= 0) {
        return;
    }
    if (!data) {
        DBGLOGD("LIBGL: glTexSubImage2D called with NULL data\n");
        return;
    }
    int pixelSize = pixel_sizeof(format, type);
    if (pixelSize <= 0) {
        DBGLOGD("LIBGL: invalid pixel size (format/type) in glTexSubImage2D\n");
        return;
    }

    const GLubyte* pixels_src = (const GLubyte*)data;
    GLubyte* temp_pixels = NULL;

    if (glstate->texture.unpack_row_length != 0 || glstate->texture.unpack_skip_pixels != 0 ||
        glstate->texture.unpack_skip_rows != 0 || glstate->texture.unpack_align != 4) {

        size_t ui_width = (size_t)width;
        size_t ui_height = (size_t)height;
        size_t up_row_pixels =
            (glstate->texture.unpack_row_length ? (size_t)glstate->texture.unpack_row_length : ui_width);
        GLint up_align = glstate->texture.unpack_align;
        if (up_align <= 0) up_align = 1;

        size_t src_row_raw = up_row_pixels * (size_t)pixelSize;
        if (up_row_pixels != 0 && src_row_raw / up_row_pixels != (size_t)pixelSize) {
            DBGLOGD("LIBGL: overflow src_row_raw\n");
            return;
        }
        size_t src_row_bytes = pad_to(src_row_raw, up_align);

        size_t dst_row_bytes = ui_width * (size_t)pixelSize;
        if (ui_width != 0 && dst_row_bytes / ui_width != (size_t)pixelSize) {
            DBGLOGD("LIBGL: overflow dst_row_bytes\n");
            return;
        }

        size_t total_dst = dst_row_bytes * ui_height;
        if (ui_height != 0 && total_dst / ui_height != dst_row_bytes) {
            DBGLOGD("LIBGL: overflow total_dst\n");
            return;
        }

        size_t skip_pixels = (size_t)glstate->texture.unpack_skip_pixels;
        size_t skip_rows = (size_t)glstate->texture.unpack_skip_rows;

        size_t skip_pixels_bytes = skip_pixels * (size_t)pixelSize;
        if (skip_pixels != 0 && skip_pixels_bytes / skip_pixels != (size_t)pixelSize) {
            DBGLOGD("LIBGL: overflow skip_pixels_bytes\n");
            return;
        }

        size_t skip_rows_bytes = skip_rows * src_row_bytes;
        if (skip_rows != 0 && skip_rows_bytes / skip_rows != src_row_bytes) {
            DBGLOGD("LIBGL: overflow skip_rows_bytes\n");
            return;
        }

        if (up_row_pixels < (skip_pixels + ui_width)) {
            DBGLOGD("LIBGL: unpack_row_length (%zu) too small for skip+width (%zu)\n", up_row_pixels,
                    (size_t)(skip_pixels + ui_width));
            return;
        }

        size_t src_offset = skip_rows_bytes + skip_pixels_bytes;
        if (src_offset < skip_rows_bytes || src_offset < skip_pixels_bytes) {
            DBGLOGD("LIBGL: overflow src_offset\n");
            return;
        }

        temp_pixels = (GLubyte*)malloc(total_dst);
        if (!temp_pixels) {
            DBGLOGD("LIBGL: malloc failed in glTexSubImage2D (bytes=%zu)\n", total_dst);
            return;
        }

        const GLubyte* src_base = (const GLubyte*)pixels_src;
        const GLubyte* src_start = src_base + src_offset;

        size_t y;
        for (y = 0; y < ui_height; ++y) {
            const GLubyte* src_row = src_start + y * src_row_bytes;
            GLubyte* dst_row = temp_pixels + y * dst_row_bytes;
            memcpy(dst_row, src_row, dst_row_bytes);
        }

        pixels_src = (const GLubyte*)temp_pixels;
    }

    LOAD_GLES2(glTexSubImage2D);
    gles_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, (const GLvoid*)pixels_src);

    if (temp_pixels) free(temp_pixels);
}

// 1d stubs
void APIENTRY_GL4ES gl4es_glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border,
                                       GLenum format, GLenum type, const GLvoid* data) {

    // TODO: maybe too naive to force GL_TEXTURE_2D here?
    gl4es_glTexImage2D(GL_TEXTURE_1D, level, internalFormat, width, 1, border, format, type, data);
}
void APIENTRY_GL4ES gl4es_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format,
                                          GLenum type, const GLvoid* data) {

    gl4es_glTexSubImage2D(GL_TEXTURE_1D, level, xoffset, 0, width, 1, format, type, data);
}

GLboolean APIENTRY_GL4ES gl4es_glIsTexture(GLuint texture) {
    DBG(DBGLOGD("glIsTexture(%d):", texture);)
    if (!glstate) {
        DBG(DBGLOGD("GL_FALSE\n");) return GL_FALSE;
    }
    noerrorShim();
    if (!texture) {
        DBG(DBGLOGD("%s\n", glstate->texture.zero->valid ? "GL_TRUE" : "GL_FALSE");)
        return glstate->texture.zero->valid;
    }
    khint_t k;
    khash_t(tex)* list = glstate->texture.list;
    if (!list) {
        DBG(DBGLOGD("GL_FALSE\n");)
        return GL_FALSE;
    }
    k = kh_get(tex, list, texture);
    gltexture_t* tex = NULL;
    if (k == kh_end(list)) {
        DBG(DBGLOGD("GL_FALSE\n");)
        return GL_FALSE;
    }
    DBG(DBGLOGD("GL_TRUE\n");)
    return GL_TRUE;
}

void APIENTRY_GL4ES gl4es_glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
    DBG(DBGLOGD("glTexStorage1D(%s, %d, %s, %d)\n", PrintEnum(target), levels, PrintEnum(internalformat), width);)
    gl4es_glTexImage1D(target, 0, internalformat, width, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}
void APIENTRY_GL4ES gl4es_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
                                         GLsizei height) {
    // (could be implemented in GLES3.0)
    DBG(DBGLOGD("glTexStorage2D(%s, %d, %s, %d, %d)\n", PrintEnum(target), levels, PrintEnum(internalformat), width,
                height);)
    if (!levels) {
        noerrorShim();
        return;
    }
    if ((internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalformat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) &&
        !globals4es.avoid16bits)
        gl4es_glTexImage2D(target, 0, internalformat, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    else if (((internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
               internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT)) &&
             !globals4es.avoid16bits)
        gl4es_glTexImage2D(target, 0, internalformat, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
    else if ((internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
              internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
              internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ||
              internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) &&
             !globals4es.avoid16bits)
        gl4es_glTexImage2D(target, 0, internalformat, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, NULL);
    else
        gl4es_glTexImage2D(target, 0, internalformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    int mlevel = maxlevel(width, height);
    gltexture_t* bound = gl4es_getCurrentTexture(target);
    if (levels > 1 && isDXTc(internalformat)) {
        // no mipmap will be uploaded, but they will be calculated from level 0
        bound->mipmap_need = 1;
        bound->mipmap_auto = 1;
        for (int i = 1; i <= mlevel; ++i)
            gl4es_glTexImage2D(target, i, internalformat, nlevel(width, i), nlevel(height, i), 0, bound->format,
                               bound->type, NULL);
        noerrorShim();
        return;
    }
    // no more compressed format here...
    if (mlevel > levels - 1) {
        bound->max_level = levels - 1;
        if (levels > 1 && GL4ES_AUTOMIPMAP_PLACEHOLDER != 3) bound->mipmap_need = 1;
    }

    for (int i = 1; i < levels; ++i)
        gl4es_glTexImage2D(target, i, internalformat, nlevel(width, i), nlevel(height, i), 0, bound->format,
                           bound->type, NULL);

    noerrorShim();
}

// Direct wrapper
AliasExport(void, glTexImage2D, ,
            (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border,
             GLenum format, GLenum type, const GLvoid* data));
AliasExport(void, glTexImage1D, ,
            (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type,
             const GLvoid* data));
AliasExport(void, glTexSubImage2D, ,
            (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
             GLenum type, const GLvoid* data));
AliasExport(void, glTexSubImage1D, ,
            (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data));
AliasExport(GLboolean, glIsTexture, , (GLuint texture));

// TexStorage
AliasExport(void, glTexStorage1D, , (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width));
AliasExport(void, glTexStorage2D, ,
            (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height));
