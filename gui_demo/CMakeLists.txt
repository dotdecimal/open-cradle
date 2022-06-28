project(gui_demo)

file(GLOB_RECURSE srcs "src/*.cpp")
file(GLOB_RECURSE headers "src/*.hpp")

file(GLOB_RECURSE preprocessed_files "src/*.hpp")
preprocess_cradle_header_files(
    "${CMAKE_CURRENT_SOURCE_DIR}" generated_srcs generated_headers
    ACCOUNT_ID decimal
    TYPE_APP_ID gui_demo
    FUNCTION_APP_ID gui_demo
    NAMESPACE gui_demo
    INDEX_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/src/gui_demo/api_index.hpp"
    INPUT_FILES ${preprocessed_files})
list(APPEND srcs ${generated_srcs})
list(APPEND headers ${generated_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/generated/src/gui_demo/api_index.hpp")
if(WIN32)
  list(APPEND srcs src/astroid.rc)
endif(WIN32)

source_group(main REGULAR_EXPRESSION "gui_demo/")
source_group(demos REGULAR_EXPRESSION "gui_demo/demos")
source_group(generated REGULAR_EXPRESSION "generated/src/")

resolve_cradle_dependencies(MY INCLUDE_WX INCLUDE_OPENGL INCLUDE_GLEW
    INCLUDE_SKIA INCLUDE_CURL INCLUDE_OPENSSL INCLUDE_DEVIL
    PROJECTS cradle)
include_directories(${MY_INCLUDE_DIRS})
include_directories(src)

add_executable(gui_demo WIN32 gui_demo.cpp ${srcs} ${headers})
target_link_libraries(gui_demo ${MY_LIBRARIES})
set_property(TARGET gui_demo PROPERTY FOLDER "other")
