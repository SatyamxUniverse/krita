# Find SIP
# ~~~~~~~~
#
# SIP website: http://www.riverbankcomputing.co.uk/sip/index.php
#
# Find the installed version of SIP. FindSIP should be called after Python
# has been found.
#
# This file defines the following variables:
#
# SIP_VERSION - The version of SIP found expressed as a 6 digit hex number
#     suitable for comparison as a string.
#
# SIP_VERSION_STR - The version of SIP found as a human readable string.
#
# SIP_EXECUTABLE - Path and filename of the SIP command line executable.
#
# SIP_INCLUDE_DIR - Directory holding the SIP C++ header file.
#
# SIP_DEFAULT_SIP_DIR - Default directory where .sip files should be installed
#     into.

# SPDX-FileCopyrightText: 2007 Simon Edwards <simon@simonzone.com>
# SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
#
# SPDX-License-Identifier: BSD-3-Clause
#

IF(SIP_VERSION)
  # Already in cache, be silent
  SET(SIP_FOUND TRUE)
ELSE(SIP_VERSION)

  FIND_FILE(_find_sip_py FindSIP.py PATHS ${CMAKE_MODULE_PATH})

  # TODO: we don't use cmake_path(CONVERT TO_NATIVE_PATH_LIST) here, because
  #       it is unknown if it is going to work on Windows. Currently, our paths
  #       on Windows use '/' as well. We need to test if switching them into '\'
  #       will still work
  set(_path_list_separator ":")
  if (WIN32)
      set(_path_list_separator ";")
  endif()

  set(all_paths ${KRITA_PYTHONPATH_V4} ${KRITA_PYTHONPATH_V5} $ENV{PYTHONPATH})
  list(REMOVE_ITEM all_paths "")
  list(JOIN all_paths "${_path_list_separator}" _pyqt5_python_path)

  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E env
    "PYTHONPATH=${_pyqt5_python_path}"
    ${Python_EXECUTABLE} ${_find_sip_py}
    OUTPUT_VARIABLE sip_config)

  IF(sip_config)
    STRING(REGEX REPLACE "^sip_version:([^\n]+).*$" "\\1" SIP_VERSION ${sip_config})
    STRING(REGEX REPLACE ".*\nsip_version_str:([^\n]+).*$" "\\1" SIP_VERSION_STR ${sip_config})
    STRING(REGEX REPLACE ".*\nsip_bin:([^\n]+).*$" "\\1" SIP_EXECUTABLE ${sip_config})
    IF(${SIP_VERSION_STR} VERSION_LESS 5)
        IF(NOT SIP_DEFAULT_SIP_DIR)
            STRING(REGEX REPLACE ".*\ndefault_sip_dir:([^\n]+).*$" "\\1" SIP_DEFAULT_SIP_DIR ${sip_config})
        ENDIF(NOT SIP_DEFAULT_SIP_DIR)
        STRING(REGEX REPLACE ".*\nsip_inc_dir:([^\n]+).*$" "\\1" SIP_INCLUDE_DIR ${sip_config})
        FILE(TO_CMAKE_PATH ${SIP_INCLUDE_DIR} SIP_INCLUDE_DIR)
        FILE(TO_CMAKE_PATH ${SIP_DEFAULT_SIP_DIR} SIP_DEFAULT_SIP_DIR)
    ELSE(${SIP_VERSION_STR} VERSION_LESS 5)
        FIND_PROGRAM(SIP_MODULE_EXECUTABLE sip-module)
    ENDIF(${SIP_VERSION_STR} VERSION_LESS 5)
    if (WIN32 AND ${SIP_VERSION_STR} VERSION_LESS 5)
        set(SIP_EXECUTABLE ${SIP_EXECUTABLE}.exe)
    endif()
    IF(EXISTS ${SIP_EXECUTABLE})
      SET(SIP_FOUND TRUE)
    ELSE()
      MESSAGE(STATUS "Found SIP configuration but the sip executable could not be found.")
    ENDIF()
  ENDIF(sip_config)

  IF(SIP_FOUND)
    IF(NOT SIP_FIND_QUIETLY)
      MESSAGE(STATUS "Found SIP version: ${SIP_VERSION_STR}")
    ENDIF(NOT SIP_FIND_QUIETLY)
  ELSE(SIP_FOUND)
    IF(SIP_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find SIP")
    ENDIF(SIP_FIND_REQUIRED)
  ENDIF(SIP_FOUND)

ENDIF(SIP_VERSION)
