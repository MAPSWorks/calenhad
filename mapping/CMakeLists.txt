INCLUDE (${CMAKE_CURRENT_LIST_DIR}/projection/CMakeLists.txt)
SET(MAPPING_SOURCE_FILES
        ${PROJECTION_SOURCE_FILES}
        ${RASTER_SOURCE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/AbstractMapWidget.h
        ${CMAKE_CURRENT_LIST_DIR}/AbstractMapWidget.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CalenhadMapWidget.h
        ${CMAKE_CURRENT_LIST_DIR}/CalenhadMapWidget.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Curve.h
        ${CMAKE_CURRENT_LIST_DIR}/Curve.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CubicSpline.h
        ${CMAKE_CURRENT_LIST_DIR}/CubicSpline.cpp
        ${CMAKE_CURRENT_LIST_DIR}/TerraceCurve.h
        ${CMAKE_CURRENT_LIST_DIR}/TerraceCurve.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Graticule.h
        ${CMAKE_CURRENT_LIST_DIR}/Graticule.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Statistics.h
        ${CMAKE_CURRENT_LIST_DIR}/Statistics.cpp
        )
