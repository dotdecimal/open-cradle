project(visualization)

if (ASTROID_OVERRIDE_SOURCE_DIR)
    set(VISUALIZATION_SOURCE_DIR "/source/src/visualization")
else()
    set(VISUALIZATION_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if (WIN32)
    file(GLOB_RECURSE srcs "src/*.cpp")
    file(GLOB_RECURSE headers "src/*.hpp")
    file(GLOB_RECURSE preprocessed_files "src/*.hpp")
else()
    file(GLOB_RECURSE srcs "${VISUALIZATION_SOURCE_DIR}/src/visualization/data/*.cpp")
    file(GLOB_RECURSE headers "${VISUALIZATION_SOURCE_DIR}/src/visualization/data/*.hpp")
    file(GLOB_RECURSE preprocessed_files "${VISUALIZATION_SOURCE_DIR}/src/visualization/data/*.hpp")
endif()

preprocess_cradle_header_files(
    "${CMAKE_CURRENT_SOURCE_DIR}" generated_srcs generated_headers
    ACCOUNT_ID decimal
    TYPE_APP_ID visualization
    FUNCTION_APP_ID visualization
    NAMESPACE visualization
    INDEX_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/generated/src/visualization/api_index.hpp"
    INPUT_FILES ${preprocessed_files})
list(APPEND srcs ${generated_srcs})
list(APPEND headers ${generated_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/generated/src/visualization/api_index.hpp")

source_group(main REGULAR_EXPRESSION "visualization/")
if (WIN32)
    source_group(ui REGULAR_EXPRESSION "visualization/ui/")
    source_group(ui/rendering REGULAR_EXPRESSION "visualization/ui/rendering/")
    source_group(ui/views REGULAR_EXPRESSION "visualization/ui/views/")
endif()
source_group(data REGULAR_EXPRESSION "visualization/data/")
source_group(data/types REGULAR_EXPRESSION "visualization/data/types")
source_group(data/utilities REGULAR_EXPRESSION "visualization/data/utilities")
source_group(generated REGULAR_EXPRESSION "generated/src/")

if (WIN32)
    resolve_cradle_dependencies(MY INCLUDE_WX INCLUDE_OPENGL INCLUDE_GLEW
	INCLUDE_SKIA INCLUDE_CURL INCLUDE_OPENSSL INCLUDE_DEVIL
	PROJECTS cradle dosimetry)
    include_directories(${MY_INCLUDE_DIRS})

    add_library(visualization STATIC ${srcs} ${headers})
    target_link_libraries(visualization cradle dosimetry)
    set_property(TARGET visualization PROPERTY FOLDER "other")
else()
    resolve_cradle_dependencies(MY INCLUDE_OPENGL PROJECTS cradle dosimetry)
    include_directories(${MY_INCLUDE_DIRS})

    add_library(visualization STATIC ${srcs} ${headers})

    target_link_libraries(visualization cradle dosimetry ${MY_LIBRARIES})
endif()
