SET(PREFIX_ext_meson "${EXTPREFIX}" )
ExternalProject_Add( ext_meson
    DOWNLOAD_DIR ${EXTERNALS_DOWNLOAD_DIR}
    URL https://github.com/mesonbuild/meson/releases/download/0.61.5/meson-0.61.5.tar.gz
    URL_HASH SHA256=5e9a0d65c1a51936362b9686d1c5e9e184a6fd245d57e7269750ce50c20f5d9a

    CONFIGURE_COMMAND ""

    BUILD_COMMAND ${CMAKE_COMMAND} -E env 
        "PYTHONPATH=${KRITA_PYTHONPATH}"
        ${Python_EXECUTABLE} <SOURCE_DIR>/setup.py build ${_compiler} -j ${SUBMAKE_JOBS}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env 
        "PYTHONPATH=${KRITA_PYTHONPATH}"
        ${Python_EXECUTABLE} <SOURCE_DIR>/setup.py install --skip-build --prefix=${PREFIX_ext_pyqt} --optimize=1

    BUILD_IN_SOURCE 1

    UPDATE_COMMAND ""

    DEPENDS ext_python ${NINJA_DEP}
)
