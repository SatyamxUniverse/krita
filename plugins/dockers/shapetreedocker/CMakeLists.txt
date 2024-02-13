set(kritashapetreedocker_static_SRCS
    ShapeTreeDockerDock.cpp
    KisShapeTreeModel.cpp
)

kis_add_library(kritashapetreedocker_static STATIC ${kritashapetreedocker_static_SRCS} #[==[${kritashapetreedocker_HEADERS_MOC}]==])

target_link_libraries(kritashapetreedocker_static kritaui kritaflake)

set(kritashapetreedocker_SOURCES
    ShapeTreeDockerPlugin.cpp
)

kis_add_library(kritashapetreedocker MODULE ${kritashapetreedocker_SOURCES})

target_link_libraries(kritashapetreedocker kritashapetreedocker_static)
install(TARGETS kritashapetreedocker DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
