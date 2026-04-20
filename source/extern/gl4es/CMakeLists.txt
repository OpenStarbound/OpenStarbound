cmake_minimum_required(VERSION 3.10)

project(ng_gl4es)

#if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTOSupported OUTPUT LTOError)
    if (LTOSupported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-g0 -O3 -ffunction-sections -fdata-sections)
        add_link_options(-Wl,--gc-sections)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/O2 /GL)
    endif()
#endif()

# glslang & SPIRV-Cross

set(ENABLE_RTTI                ON  CACHE BOOL "Enables RTTI (will be took by glslang)" FORCE)
set(ENABLE_HLSL                OFF CACHE BOOL "Disable HLSL input for glslang" FORCE)
set(BUILD_EXTERNAL             OFF CACHE BOOL "Disable build external dependencies for glslang" FORCE)
set(ENABLE_GLSLANG_BINARIES    OFF CACHE BOOL "Disable glslangValidator/spirv-remap" FORCE)
set(ENABLE_OPT                 OFF CACHE BOOL "Disable SPIRV-Tools opt usage in glslang" FORCE)
set(BUILD_EXTERNAL             OFF CACHE BOOL "Don't build external deps in External/" FORCE)

set(SPIRV_CROSS_ENABLE_C_API   ON  CACHE BOOL "Enable C API" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL    ON  CACHE BOOL "Enable GLSL backend" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Disable reflect" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL    OFF CACHE BOOL "Disable HLSL backend" FORCE)
set(SPIRV_CROSS_ENABLE_MSL     OFF CACHE BOOL "Disable MSL backend" FORCE)
set(SPIRV_CROSS_ENABLE_CPP     OFF CACHE BOOL "Disable C++ API target" FORCE)
set(SPIRV_CROSS_CLI            OFF CACHE BOOL "Disable CLI binary" FORCE)
set(SPIRV_CROSS_STATIC         ON  CACHE BOOL "Prefer static libs" FORCE)
set(SPIRV_CROSS_ENABLE_UTIL    OFF  CACHE BOOL "Disable util" FORCE)

enable_language(CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DNO_GBM -DDEFAULT_ES=3 -fvisibility=hidden")

if(ANDROID)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNOX11")
endif()

include_directories(include )

set(C_SRC_FILES
    src/gl/string_utils.c
    src/gl/cJSON.c
    src/gl/config.c
    src/gl/arbconverter.c
    src/gl/arbgenerator.c
    src/gl/arbhelper.c
    src/gl/arbparser.c
    src/gl/array.c
    src/gl/blend.c
    src/gl/blit.c
    src/gl/buffers.c
    src/gl/build_info.c
    src/gl/debug.c
    src/gl/decompress.c
    src/gl/depth.c
    src/gl/directstate.c
    src/gl/drawing.c
    src/gl/enable.c
    src/gl/envvars.c
    src/gl/eval.c
    src/gl/face.c
    src/gl/fog.c
    src/gl/fpe.c
    src/gl/fpe_cache.c
    src/gl/fpe_shader.c
    src/gl/framebuffers.c
    src/gl/gl_lookup.c
    src/gl/gl4es.c
    src/gl/glstate.c
    src/gl/hint.c
    src/gl/init.c
    src/gl/light.c
    src/gl/line.c
    src/gl/list.c
    src/gl/listdraw.c
    src/gl/listrl.c
    src/gl/loader.c
    src/gl/logs.c
    src/gl/matrix.c
    src/gl/matvec.c
    src/gl/oldprogram.c
    src/gl/pixel.c
    src/gl/planes.c
    src/gl/pointsprite.c
    src/gl/preproc.c
    src/gl/program.c
    src/gl/queries.c
    src/gl/raster.c
    src/gl/render.c
    src/gl/shader.c
    src/gl/shaderconv.c
    src/gl/shader_hacks.c
    src/gl/stack.c
    src/gl/stencil.c
    src/gl/stubs.c
    src/gl/texenv.c
    src/gl/texgen.c
    src/gl/texture.c
    src/gl/texture_compressed.c
    src/gl/texture_params.c
    src/gl/texture_read.c
    src/gl/texture_3d.c
    src/gl/uniform.c
    src/gl/vertexattrib.c
    src/gl/wrap/gl4eswraps.c
    src/gl/wrap/gles.c
    src/gl/wrap/glstub.c
    src/gl/wrap/glesnative.cpp
    src/gl/math/matheval.c
    src/glx/hardext.c
    src/glx/lookup.c
    src/glx/gbm.c
    src/glx/streaming.c
    src/gl/vgpu/shaderconv.c
    src/gl/samplers.c
)

if(NOT ANDROID)
    list(APPEND C_SRC_FILES src/glx/glx.c)
endif()

set(CPP_SRC_FILES
    src/gl/glsl/glsl_for_es.cpp
    src/gl/getter.cpp
)

set(SRC_FILES ${CPP_SRC_FILES} ${C_SRC_FILES})

add_library(ng_gl4es STATIC ${SRC_FILES})

find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_c CONFIG REQUIRED)

target_link_libraries(ng_gl4es PRIVATE
    glslang::glslang
    spirv-cross-c
)

if(MACOS)
    # iOS
    target_link_libraries(ng_gl4es PUBLIC
        "-Fexternal"
        "-framework libEGL"
        "-framework libGLESv2"
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
    )
else()
if(ANDROID)
    # Android
    target_link_libraries(ng_gl4es PUBLIC
        android
        log
    )
else()
    # Nothing to link
endif()
endif()

target_include_directories(ng_gl4es PUBLIC
    include
)

set_target_properties(ng_gl4es PROPERTIES
    LINK_FLAGS "-Wl,--strip-all"
)
