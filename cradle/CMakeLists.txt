project(cradle)

option(CRADLE_INCLUDE_GUI "Include GUI components" OFF)
option(CRADLE_INCLUDE_APP_CONFIG
    "Include platform-dependent storage of app data (requires wxWidgets)" OFF)
option(CRADLE_INCLUDE_STANDARD_IMAGE_IO
    "Include standard image file I/O (requires DevIL)" OFF)
option(CRADLE_INCLUDE_WEB_IO "Include web I/O (requires Curl)" OFF)

# C++ sources
file(GLOB_RECURSE srcs "${CRADLE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE c_srcs "${CRADLE_SOURCE_DIR}/src/*.c")
list(APPEND srcs ${c_srcs})

# C++ headers
file(GLOB_RECURSE headers "${CRADLE_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE c_headers "${CRADLE_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE impls "${CRADLE_SOURCE_DIR}/src/*.ipp")
file(GLOB_RECURSE testfiles "Unit_Tests/*.hpp")
file(GLOB_RECURSE catch_test_srcs "catch_unit_tests/*.cpp")
file(GLOB_RECURSE catch_test_files "catch_unit_tests/*.hpp")

# Collect all CRADLE headers (with a few exceptions) as preprocessed files.
# (The exceptions are excepted because they don't contain any API commands
# and because including them when they're not needed causes problems.)
file(GLOB_RECURSE preprocessed_files "${CRADLE_SOURCE_DIR}/src/cradle/*.hpp")
foreach(file ${preprocessed_files})
    if (${file} MATCHES "/external/")
        list(REMOVE_ITEM preprocessed_files ${file})
    endif()
endforeach()
list(REMOVE_ITEM preprocessed_files "${CRADLE_SOURCE_DIR}/src/cradle/test.hpp")
list(REMOVE_ITEM preprocessed_files
    "${CRADLE_SOURCE_DIR}/src/cradle/imaging/test.hpp")

# Remove sources that depend on components that aren't included.
# Including these when they're not wanted needlessly increases the
# number of external libraries required for building CRADLE.
list(REMOVE_ITEM srcs "${CRADLE_SOURCE_DIR}/src/alia/ui/backends/win32.cpp")
if (NOT CRADLE_INCLUDE_GUI)
    foreach(file ${srcs})
        if (${file} MATCHES "/gui/" OR ${file} MATCHES "/alia")
            list(REMOVE_ITEM srcs ${file})
        endif()
    endforeach()
    foreach(file ${preprocessed_files})
        if (${file} MATCHES "/gui/")
            list(REMOVE_ITEM preprocessed_files ${file})
        endif()
    endforeach()
    list(REMOVE_ITEM srcs "${CRADLE_SOURCE_DIR}/src/alia/ui/backends/wx.cpp")
    list(REMOVE_ITEM srcs "${CRADLE_SOURCE_DIR}/src/cradle/gui/app_level.cpp")
    # These are required even for non-GUI code.
    list(APPEND srcs "${CRADLE_SOURCE_DIR}/src/alia/color.cpp")
    list(APPEND headers "${CRADLE_SOURCE_DIR}/src/alia/color.hpp")
    list(APPEND srcs "${CRADLE_SOURCE_DIR}/src/alia/data_graph.cpp")
    list(APPEND headers "${CRADLE_SOURCE_DIR}/src/alia/data_graph.hpp")
    list(APPEND srcs "${CRADLE_SOURCE_DIR}/src/alia/geometry.cpp")
    list(APPEND headers "${CRADLE_SOURCE_DIR}/src/alia/geometry.hpp")
    list(APPEND srcs "${CRADLE_SOURCE_DIR}/src/alia/id.cpp")
    list(APPEND headers "${CRADLE_SOURCE_DIR}/src/alia/id.hpp")
    list(APPEND srcs "${CRADLE_SOURCE_DIR}/src/cradle/gui/types.cpp")
    list(APPEND headers "${CRADLE_SOURCE_DIR}/src/cradle/gui/types.hpp")
    list(APPEND preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/gui/types.hpp")
    list(APPEND headers
        "${CRADLE_SOURCE_DIR}/src/cradle/gui/displays/types.hpp")
    list(APPEND preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/gui/displays/types.hpp")
endif()
if (NOT CRADLE_INCLUDE_LUA)
    list(REMOVE_ITEM srcs "${CRADLE_SOURCE_DIR}/src/cradle/lua.cpp")
    list(REMOVE_ITEM preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/lua.hpp")
    list(REMOVE_ITEM headers "${CRADLE_SOURCE_DIR}/src/cradle/lua.hpp")
endif()
if (NOT CRADLE_INCLUDE_STANDARD_IMAGE_IO)
    list(REMOVE_ITEM srcs
        "${CRADLE_SOURCE_DIR}/src/cradle/io/standard_image_io.cpp")
    list(REMOVE_ITEM preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/io/standard_image_io.hpp")
    list(REMOVE_ITEM headers
        "${CRADLE_SOURCE_DIR}/src/cradle/io/standard_image_io.hpp")
endif()
if (NOT CRADLE_INCLUDE_WEB_IO)
    list(REMOVE_ITEM srcs
        "${CRADLE_SOURCE_DIR}/src/cradle/io/web_io.cpp")
    foreach(file ${srcs})
        if (${file} MATCHES "/background/")
            list(REMOVE_ITEM srcs ${file})
        endif()
    endforeach()
    foreach(file ${preprocessed_files})
        if (${file} MATCHES "/background/")
            list(REMOVE_ITEM preprocessed_files ${file})
        endif()
    endforeach()
    foreach(file ${srcs})
        if (${file} MATCHES "/io/services/")
            list(REMOVE_ITEM srcs ${file})
        endif()
    endforeach()
    foreach(file ${preprocessed_files})
        if (${file} MATCHES "/io/services/")
            list(REMOVE_ITEM preprocessed_files ${file})
        endif()
    endforeach()
endif()
# Breakpad is currently only supported under Windows.
if (NOT WIN32)
    list(REMOVE_ITEM srcs
        "${CRADLE_SOURCE_DIR}/src/cradle/breakpad.cpp")
    list(REMOVE_ITEM preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/breakpad.hpp")
    list(REMOVE_ITEM headers
        "${CRADLE_SOURCE_DIR}/src/cradle/breakpad.hpp")
endif()
# PostgreSQL is currently only supported under Windows.
if (NOT WIN32)
    list(REMOVE_ITEM headers
        "${CRADLE_SOURCE_DIR}/src/cradle/io/libpq.hpp")
    list(REMOVE_ITEM preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/io/libpq.hpp")
    list(REMOVE_ITEM srcs
        "${CRADLE_SOURCE_DIR}/src/cradle/io/postgresql.cpp")
    list(REMOVE_ITEM preprocessed_files
        "${CRADLE_SOURCE_DIR}/src/cradle/io/postgresql.hpp")
    list(REMOVE_ITEM headers
        "${CRADLE_SOURCE_DIR}/src/cradle/io/postgresql.hpp")
endif()
# Unit testing code that currently requires Windows.
if (NOT WIN32)
    foreach(file ${srcs})
        if (${file} MATCHES "/Unit_Tests/win32/")
            list(REMOVE_ITEM srcs ${file})
        endif()
    endforeach()
    foreach(file ${preprocessed_files})
        if (${file} MATCHES "/Unit_Tests/win32/")
            list(REMOVE_ITEM preprocessed_files ${file})
        endif()
    endforeach()
endif()

# Preprocess the API headers.
preprocess_cradle_header_files(
    ${CRADLE_SOURCE_DIR} generated_srcs generated_headers
    USE_INTERNAL_PREPROCESSOR
    ACCOUNT_ID <thinknode account ID>
    TYPE_APP_ID dosimetry
    FUNCTION_APP_ID dosimetry
    NAMESPACE cradle
    INDEX_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/src/cradle/api_index.hpp"
    INPUT_FILES ${preprocessed_files})
list(APPEND srcs ${generated_srcs})
list(APPEND headers ${generated_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/generated/src/cradle/api_index.hpp")

# CRADLE CMake files
file (GLOB_RECURSE cmake_files "${CRADLE_SOURCE_DIR}/cmake/*.cmake")

# Organize the files into groups.
source_group(alia REGULAR_EXPRESSION "alia/")
source_group(alia/layout REGULAR_EXPRESSION "alia/layout/")
source_group(alia/ui REGULAR_EXPRESSION "alia/ui/")
source_group(alia/ui/backends REGULAR_EXPRESSION "alia/ui/backends/")
source_group(alia/ui/utilities REGULAR_EXPRESSION "alia/ui/utilities/")
source_group(alia/ui/library REGULAR_EXPRESSION "alia/ui/library/")
source_group(external/sqlite REGULAR_EXPRESSION "sqlite3.[ch]")
source_group(external/catch REGULAR_EXPRESSION "catch.hpp")
source_group(external/clipper REGULAR_EXPRESSION "clipper.[ch]pp")
source_group(external/json REGULAR_EXPRESSION "src/json/")
source_group(general REGULAR_EXPRESSION "cradle/src/cradle/")
source_group(background REGULAR_EXPRESSION "cradle/background/")
source_group(external REGULAR_EXPRESSION "cradle/external/")
source_group(geometry REGULAR_EXPRESSION "cradle/geometry/")
source_group(gui REGULAR_EXPRESSION "cradle/gui/")
source_group(gui/app REGULAR_EXPRESSION "cradle/gui/app/")
source_group(gui/displays REGULAR_EXPRESSION "cradle/gui/displays/")
source_group(gui/displays/compositions REGULAR_EXPRESSION
    "cradle/gui/displays/compositions/")
source_group(gui/displays/views REGULAR_EXPRESSION
    "cradle/gui/displays/views/")
source_group(imaging REGULAR_EXPRESSION "cradle/imaging/")
source_group(io REGULAR_EXPRESSION "cradle/io/")
source_group(io/services REGULAR_EXPRESSION "cradle/io/services/")
source_group(math REGULAR_EXPRESSION "cradle/math/")
source_group(rt REGULAR_EXPRESSION "cradle/rt/")
source_group(unit_tests REGULAR_EXPRESSION "Unit_Tests/")
source_group(catch_unit_tests REGULAR_EXPRESSION "catch_unit_tests/")
source_group(cmake REGULAR_EXPRESSION "/cmake/")
source_group(preprocessor REGULAR_EXPRESSION "/preprocessor/")
source_group(generated REGULAR_EXPRESSION "generated/src/")

set(cradle_srcs ${srcs} ${headers} ${impls} ${cmake_files}
    ${preprocessor_files})

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )
endif()
if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif()

set(included_libs )
if(CRADLE_INCLUDE_GUI)
    list(APPEND included_libs INCLUDE_SKIA INCLUDE_GLEW INCLUDE_WX)
endif()
if(CRADLE_INCLUDE_LUA)
    list(APPEND included_libs INCLUDE_LUA)
endif()
if(CRADLE_INCLUDE_PARETO_NAVIGATION)
    list(APPEND included_libs INCLUDE_CLP)
endif()
if(CRADLE_INCLUDE_STANDARD_IMAGE_IO)
    list(APPEND included_libs INCLUDE_DEVIL)
endif()
# OpenGL is considered a core requirement for now.
list(APPEND included_libs INCLUDE_OPENGL)
list(APPEND included_libs INCLUDE_ZLIB)
if(CRADLE_INCLUDE_WEB_IO)
    list(APPEND included_libs INCLUDE_CURL INCLUDE_OPENSSL INCLUDE_MSGPACK)
endif()
if(WIN32)
	list(APPEND included_libs INCLUDE_POSTGRESQL)
endif()

resolve_cradle_dependencies(MY ${included_libs})
include_directories(${MY_INCLUDE_DIRS})
# if (WIN32)
    include_directories("Unit_Tests/")
    include_directories("catch_unit_tests/")
# endif()

add_library(cradle STATIC ${cradle_srcs})
target_link_libraries(cradle ${MY_LIBRARIES})
set_property(TARGET cradle PROPERTY FOLDER "cradle")

# This has been moved to dosimetry in the rt_types_generate_api
# add_executable(generate_api generate_api.cpp)
# target_link_libraries(generate_api cradle ${MY_LIBRARIES})

add_executable(cradle_catch_unit_tests catch_unit_tests/catch_unit_tests.cpp ${catch_test_files} ${catch_test_srcs})
target_link_libraries(cradle_catch_unit_tests cradle ${MY_LIBRARIES})
set_property(TARGET cradle_catch_unit_tests PROPERTY FOLDER "cradle")

if (WIN32)
    add_executable(cradle_unit_tests_win32 Unit_Tests/win32/Unit_Tests_win32.cpp ${testfiles})
    target_link_libraries(cradle_unit_tests_win32 cradle ${MY_LIBRARIES})
    set_property(TARGET cradle_unit_tests_win32 PROPERTY FOLDER "cradle")
endif()
