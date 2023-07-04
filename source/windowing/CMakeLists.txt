INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
    ${STAR_GAME_INCLUDES}
    ${STAR_PLATFORM_INCLUDES}
    ${STAR_APPLICATION_INCLUDES}
    ${STAR_RENDERING_INCLUDES}
    ${STAR_WINDOWING_INCLUDES}
  )

SET (star_windowing_HEADERS
        StarButtonGroup.hpp
        StarButtonWidget.hpp
        StarCanvasWidget.hpp
        StarFlowLayout.hpp
        StarFuelWidget.hpp
        StarGuiContext.hpp
        StarGuiReader.hpp
        StarGuiTypes.hpp
        StarImageWidget.hpp
        StarImageStretchWidget.hpp
        StarItemGridWidget.hpp
        StarItemSlotWidget.hpp
        StarKeyBindings.hpp
        StarLabelWidget.hpp
        StarLargeCharPlateWidget.hpp
        StarLayout.hpp
        StarListWidget.hpp
        StarPane.hpp
        StarPaneManager.hpp
        StarPortraitWidget.hpp
        StarProgressWidget.hpp
        StarScrollArea.hpp
        StarSliderBar.hpp
        StarStackWidget.hpp
        StarTabSet.hpp
        StarTextBoxWidget.hpp
        StarVerticalLayout.hpp
        StarWidget.hpp
        StarWidgetLuaBindings.hpp
        StarWidgetParsing.hpp
    )

SET (star_windowing_SOURCES
        StarButtonGroup.cpp
        StarButtonWidget.cpp
        StarCanvasWidget.cpp
        StarFlowLayout.cpp
        StarFuelWidget.cpp
        StarGuiContext.cpp
        StarGuiReader.cpp
        StarGuiTypes.cpp
        StarImageWidget.cpp
        StarImageStretchWidget.cpp
        StarItemGridWidget.cpp
        StarItemSlotWidget.cpp
        StarKeyBindings.cpp
        StarLayout.cpp
        StarLabelWidget.cpp
        StarLargeCharPlateWidget.cpp
        StarListWidget.cpp
        StarPane.cpp
        StarPaneManager.cpp
        StarPortraitWidget.cpp
        StarProgressWidget.cpp
        StarScrollArea.cpp
        StarSliderBar.cpp
        StarStackWidget.cpp
        StarTabSet.cpp
        StarTextBoxWidget.cpp
        StarVerticalLayout.cpp
        StarWidget.cpp
        StarWidgetLuaBindings.cpp
        StarWidgetParsing.cpp
    )

ADD_LIBRARY (star_windowing OBJECT ${star_windowing_SOURCES} ${star_windowing_HEADERS})
