find_package(SDL3 CONFIG REQUIRED)
SET(imgui_SOURCES
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_sdl3.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
)
SET(imgui_HEADERS
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_opengl3.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_sdl3.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui.h
)
ADD_LIBRARY (imgui STATIC ${imgui_SOURCES} ${imgui_HEADERS})
target_link_directories(imgui PUBLIC SDL3::SDL3)
target_include_directories(imgui PUBLIC extern/imgui)
set(STAR_EXT_LIBS ${STAR_EXT_LIBS} imgui)
set(STAR_EXTERN_INCLUDES ${STAR_EXTERN_INCLUDES}
    ${PROJECT_SOURCE_DIR}/extern/imgui
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends
    ${PROJECT_SOURCE_DIR}/extern/imgui/misc/freetype)
