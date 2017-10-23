# find Skia

find_path(SKIA_INCLUDE_DIR core/SkPaint.hpp
  PATHS /usr/include /usr/local/include)

find_library(SKIA_CORE_LIBRARY skia_core PATHS /usr/lib /usr/local/lib C:/libs-vc14-x64/skia/lib/release)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Skia DEFAULT_MSG
    SKIA_INCLUDE_DIR
    SKIA_CORE_LIBRARY)

mark_as_advanced(SKIA_INCLUDE_DIR
    SKIA_INCLUDE_DIR
    SKIA_CORE_LIBRARY)

get_filename_component(SKIA_LIBRARY_DIR "${SKIA_CORE_LIBRARY}" DIRECTORY CACHE)
file(GLOB_RECURSE SKIA_LIBRARIES "${SKIA_LIBRARY_DIR}/*.lib")

set(SKIA_INCLUDE_DIRS
    "${SKIA_INCLUDE_DIR}/animator"
    "${SKIA_INCLUDE_DIR}/c"
    "${SKIA_INCLUDE_DIR}/codec"
    "${SKIA_INCLUDE_DIR}/config"
    "${SKIA_INCLUDE_DIR}/core"
    "${SKIA_INCLUDE_DIR}/effects"
    "${SKIA_INCLUDE_DIR}/gpu"
    "${SKIA_INCLUDE_DIR}/images"
    "${SKIA_INCLUDE_DIR}/pathops"
    "${SKIA_INCLUDE_DIR}/ports"
    "${SKIA_INCLUDE_DIR}/svg"
    "${SKIA_INCLUDE_DIR}/utils"
    "${SKIA_INCLUDE_DIR}/views"
    "${SKIA_INCLUDE_DIR}/xml")
