set(PREFIX_EXT_PIL "${EXTPREFIX}")
set(PILLOW_VERSION "6.2.0")

find_program(PIP_EXECUTABLE pip)
if(NOT EXISTS ${PIP_EXECUTABLE})
  message(FATAL_ERROR "Can't find pip")
endif()

if (UNIX)
  if(NOT APPLE)
    add_custom_target(
      ext_pil
      COMMAND ${PIP_EXECUTABLE} install --target="${PREFIX_EXT_PIL}/lib/krita-python-libs" "pillow==${PILLOW_VERSION}"
      )
  endif()
endif()
