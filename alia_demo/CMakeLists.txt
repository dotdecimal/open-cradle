project(alia_demo)

file(GLOB_RECURSE srcs "src/*.cpp")
file(GLOB_RECURSE headers "src/*.hpp")

file(GLOB_RECURSE preprocessed_files "src/*.hpp")
preprocess_cradle_header_files(
    "${CMAKE_CURRENT_SOURCE_DIR}" generated_srcs generated_headers
    TYPE_APP_ID alia_demo
    FUNCTION_APP_ID dosimetry
    NAMESPACE alia_demo
    INDEX_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/src/alia_demo/api_index.hpp"
    INPUT_FILES ${preprocessed_files})
list(APPEND srcs ${generated_srcs})
list(APPEND headers ${generated_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/generated/src/alia_demo/api_index.hpp")
# if(WIN32)
#   list(APPEND srcs src/astroid.rc)
# endif(WIN32)

source_group(demos REGULAR_EXPRESSION "alia_demo/src/")
source_group(main REGULAR_EXPRESSION "alia_demo/src/(main|utilities)")

resolve_cradle_dependencies(MY INCLUDE_WX INCLUDE_OPENGL INCLUDE_SKIA INCLUDE_GLEW
    PROJECTS cradle)
include_directories(${MY_INCLUDE_DIRS})

add_executable(alia_demo WIN32 ${srcs} ${headers})
target_link_libraries(alia_demo ${MY_LIBRARIES})
