INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
  )

FIND_PACKAGE (Qt5Core)
FIND_PACKAGE (Qt5Widgets)

SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

SET (star_mod_uploader_HEADERS
    StarStringConversion.hpp
    StarModUploader.hpp
    StarSPlainTextEdit.hpp
  )

SET (star_mod_uploader_SOURCES
    StarModUploader.cpp
    StarSPlainTextEdit.cpp
    main.cpp
  )

ADD_EXECUTABLE (mod_uploader WIN32
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base>
  ${star_mod_uploader_HEADERS} ${star_mod_uploader_SOURCES})
TARGET_LINK_LIBRARIES (mod_uploader ${STAR_EXT_LIBS} ${STEAM_API_LIBRARY} Qt5::Core Qt5::Gui Qt5::Widgets)

SET (CMAKE_AUTOMOC OFF)
SET (CMAKE_INCLUDE_CURRENT_DIR OFF)
