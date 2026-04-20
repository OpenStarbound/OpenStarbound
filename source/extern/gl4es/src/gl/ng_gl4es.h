#ifndef NG_GL4ES
#define NG_GL4ES
#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#define GL_ZERO_TO_ONE 0x935F
#define GL_TEXTURE_3D_MULTISAMPLE 0x9103
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS 0x92D9
#define GL_MAX_NAME_LENGTH 0x92F6
#define GL_ACTIVE_RESOURCES 0x92F5
#define GL_LOCATION 0x930E
#define GL_ARRAY_SIZE 0x92FB
#define GL_TYPE 0x92FA
#define GL_BLOCK_INDEX 0x92FD
#define GL_OFFSET 0x92FC
#define GL_UNIFORM 0x92E1
#define GL_NAME_LENGTH 0x92f9
#define GL_PROGRAM_BINARY_FORMAT_MESA     0x875F

#include <GL/gl.h>
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include "framebuffers.h"
#include "texture.h"
#include "wrap/gles.h"
#include "gles.h"
#include "glstate.h"
#include "const.h"
#include "gl4es.h"
#include "wrap/gl4es.h"

#endif