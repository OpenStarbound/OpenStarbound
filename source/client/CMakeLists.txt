INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
    ${STAR_GAME_INCLUDES}
    ${STAR_PLATFORM_INCLUDES}
    ${STAR_APPLICATION_INCLUDES}
    ${STAR_RENDERING_INCLUDES}
    ${STAR_WINDOWING_INCLUDES}
    ${STAR_FRONTEND_INCLUDES}
  )

SET (star_client_HEADERS
      StarClientApplication.hpp
      StarRenderingLuaBindings.hpp
    )

SET (star_client_SOURCES
      StarClientApplication.cpp
      StarRenderingLuaBindings.cpp
    )

IF (STAR_SYSTEM_WINDOWS)
  SET (star_client_RESOURCES
      starbound.rc
    )
ENDIF ()

IF (STAR_SYSTEM_ANDROID)
  ADD_LIBRARY (starbound SHARED
    $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
    $<TARGET_OBJECTS:star_application> $<TARGET_OBJECTS:star_rendering> $<TARGET_OBJECTS:star_windowing> $<TARGET_OBJECTS:star_frontend>
    ${star_client_HEADERS} ${star_client_SOURCES} ${star_client_RESOURCES})
ELSE ()
  ADD_EXECUTABLE (starbound WIN32
    $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
    $<TARGET_OBJECTS:star_application> $<TARGET_OBJECTS:star_rendering> $<TARGET_OBJECTS:star_windowing> $<TARGET_OBJECTS:star_frontend>
    ${star_client_HEADERS} ${star_client_SOURCES} ${star_client_RESOURCES})
ENDIF()

IF (STAR_PRECOMPILED_HEADERS)
  TARGET_PRECOMPILE_HEADERS (starbound REUSE_FROM star_core)
ENDIF()

IF(STAR_SYSTEM_MACOS)
  SET_TARGET_PROPERTIES (starbound PROPERTIES LINK_FLAGS "-Wl,-rpath,'@executable_path'")
ELSEIF (UNIX)
  SET_TARGET_PROPERTIES (starbound PROPERTIES LINK_FLAGS "-Wl,--export-dynamic,-rpath,'$ORIGIN'")
ENDIF()

IF (STAR_SYSTEM_ANDROID)
  SET (STAR_ANDROID_JNI_LIB_DIR "${PROJECT_SOURCE_DIR}/../android/lib/${CMAKE_ANDROID_ARCH_ABI}")
  SET_TARGET_PROPERTIES (starbound PROPERTIES OUTPUT_NAME main)
  if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "RelWithAsserts" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    if(CMAKE_STRIP)
      ADD_CUSTOM_COMMAND(TARGET starbound POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${STAR_ANDROID_JNI_LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:starbound>" "${STAR_ANDROID_JNI_LIB_DIR}/libmain.so"
        COMMAND ${CMAKE_STRIP} --strip-unneeded "${STAR_ANDROID_JNI_LIB_DIR}/libmain.so"
        COMMENT "Copying and stripping libmain.so into Android host jniLibs"
        VERBATIM
      )
    else()
      ADD_CUSTOM_COMMAND(TARGET starbound POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${STAR_ANDROID_JNI_LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:starbound>" "${STAR_ANDROID_JNI_LIB_DIR}/libmain.so"
        COMMENT "Copying libmain.so into Android host jniLibs (strip unavailable)"
        VERBATIM
      )
    endif()
  else()
    ADD_CUSTOM_COMMAND(TARGET starbound POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${STAR_ANDROID_JNI_LIB_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:starbound>" "${STAR_ANDROID_JNI_LIB_DIR}/libmain.so"
      COMMENT "Copying libmain.so into Android host jniLibs"
      VERBATIM
    )
  endif()
ENDIF ()

IF (STAR_SYSTEM_MACOS)
  SET_TARGET_PROPERTIES (starbound PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
ENDIF()

TARGET_LINK_LIBRARIES (starbound ${STAR_EXT_LIBS} ${STAR_EXT_GUI_LIBS})
