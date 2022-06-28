cmake_minimum_required(VERSION 3.2)
project(open-cradle)

# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ASTROID_SOURCE_DIR "${CMAKE_SOURCE_DIR}")

set(ASTROID_LIB_DIR "${CMAKE_SOURCE_DIR}/../lib")

set(CRADLE_SOURCE_DIR "${ASTROID_SOURCE_DIR}/cradle")

include("${CRADLE_SOURCE_DIR}/cmake/UseCradle.cmake")

add_subdirectory("${CRADLE_SOURCE_DIR}/preprocessor")
add_subdirectory("${CRADLE_SOURCE_DIR}")
add_subdirectory("${ASTROID_SOURCE_DIR}/visualization")
if (WIN32)
    add_subdirectory("${ASTROID_SOURCE_DIR}/gui_demo")      
endif()
