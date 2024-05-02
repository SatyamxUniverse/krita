add_subdirectory(tests)

set(libkritaconverter_LIB_SRCS
    kis_qoi_converter.cpp
    )


set(kritaqoiimport_SOURCES
    kis_qoi_import.cc
    ${libkritaconverter_LIB_SRCS}
    )

kis_add_library(kritaqoiimport MODULE ${kritaqoiimport_SOURCES})

include_directories(SYSTEM ${QOI_INCLUDE_DIR})

add_definitions(${QOI_DEFINITIONS})

target_link_libraries(kritaqoiimport kritaui ${QOI_LIBRARIES} )

install(TARGETS kritaqoiimport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

set(kritaqoiexport_SOURCES
    kis_qoi_export.cc
    ${libkritaconverter_LIB_SRCS}
)

ki18n_wrap_ui(kritaqoiexport_SOURCES kis_wdg_options_qoi.ui )

kis_add_library(kritaqoiexport MODULE ${kritaqoiexport_SOURCES})

target_link_libraries(kritaqoiexport kritaui kritaimpex ${QOI_LIBRARIES})

install(TARGETS kritaqoiexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install( PROGRAMS  krita_qoi.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
