project(analysis)

file(GLOB_RECURSE srcs "src/*.cpp")
file(GLOB_RECURSE headers "src/*.hpp")

file(GLOB_RECURSE preprocessed_files "src/*.hpp")
preprocess_cradle_header_files(
    "${CMAKE_CURRENT_SOURCE_DIR}" generated_srcs generated_headers
    ACCOUNT_ID <thinknode account ID here>
    TYPE_APP_ID analysis
    FUNCTION_APP_ID analysis
    NAMESPACE analysis
    INDEX_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/src/analysis/api_index.hpp"
    INPUT_FILES ${preprocessed_files})
list(APPEND srcs ${generated_srcs})
list(APPEND headers ${generated_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/generated/src/analysis/api_index.hpp")
if(WIN32)
  list(APPEND srcs src/astroid.rc)
endif(WIN32)

source_group(main REGULAR_EXPRESSION "analysis/")
source_group(ui REGULAR_EXPRESSION "analysis/ui/")
source_group(ui/controls REGULAR_EXPRESSION "analysis/ui/controls/")
source_group(ui/state REGULAR_EXPRESSION "analysis/ui/state/")
source_group(ui/tasks REGULAR_EXPRESSION "analysis/ui/tasks/")
source_group(ui/displays REGULAR_EXPRESSION "analysis/ui/displays/")
source_group(data REGULAR_EXPRESSION "analysis/data/")
source_group(generated REGULAR_EXPRESSION "generated/src/")

resolve_cradle_dependencies(MY INCLUDE_WX INCLUDE_OPENGL INCLUDE_GLEW
    INCLUDE_SKIA INCLUDE_CURL INCLUDE_OPENSSL INCLUDE_DEVIL
    PROJECTS cradle dosimetry visualization)
include_directories(${MY_INCLUDE_DIRS})

add_executable(analysis WIN32 analysis_ui.cpp ${srcs} ${headers})
target_link_libraries(analysis ${MY_LIBRARIES})
set_property(TARGET analysis PROPERTY FOLDER "analysis")

add_executable(analysis_generate_api analysis_generate_api.cpp ${srcs} ${headers})
target_link_libraries(analysis_generate_api ${MY_LIBRARIES})
set_property(TARGET analysis_generate_api PROPERTY FOLDER "analysis")
