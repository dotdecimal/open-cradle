# find GLEW

find_path(GLEW_INCLUDE_DIR GL/glew.h PATHS /usr/include /usr/local/include)

find_library(GLEW_LIBRARY glew glew32 glew64 PATHS /usr/lib /usr/local/lib)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)

mark_as_advanced(GLEW_INCLUDE_DIR GLEW_LIBRARY)

set(GLEW_LIBRARIES "${GLEW_LIBRARY}")
