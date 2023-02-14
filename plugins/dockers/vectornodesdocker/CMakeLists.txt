set(KRITA_VECTORNODESDOCKER_SOURCES
    vectornodesdocker.h vectornodesdocker.cpp
    vectornodesdocker_dock.h vectornodesdocker_dock.cpp
    vectornodes_docker_widget.h vectornodes_docker_widget.cpp

    nodestyles.h nodestyles.cpp
    nodescene.h nodescene.cpp
    nodeitem.h nodeitem.cpp

    elements/connection.h elements/connection.cpp
    elements/ioelement.h elements/ioelement.cpp
    elements/nodeoutlinerectitem.h elements/nodeoutlinerectitem.cpp
    elements/editormenu.h elements/editormenu.cpp
    elements/editormenuitem.h elements/editormenuitem.cpp

    nodes/editornode.h nodes/editornode.cpp
    nodes/valuenode.h nodes/valuenode.cpp
    nodes/colornode.h nodes/colornode.cpp
    nodes/layernode.h nodes/layernode.cpp
    nodes/previewnode.h nodes/previewnode.cpp
    nodes/getshapesnode.h nodes/getshapesnode.cpp
    nodes/shapenode.h nodes/shapenode.cpp
    nodes/mathnode.h nodes/mathnode.cpp
    nodes/noisenode.h nodes/noisenode.cpp
    nodes/outputnode.h nodes/outputnode.cpp
)

ki18n_wrap_ui(KRITA_VECTORNODESDOCKER_SOURCES
    vectornodes_docker_widget.ui
)

kis_add_library(kritavectornodesdocker MODULE ${KRITA_VECTORNODESDOCKER_SOURCES})
target_link_libraries(kritavectornodesdocker kritaui)
install(TARGETS kritavectornodesdocker  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
