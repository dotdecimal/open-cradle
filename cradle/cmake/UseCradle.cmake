# UseCradle.cmake provides utilities that facilitate setting up the build
# process for projects that use CRADLE.

include(CMakeParseArguments)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cradle/cmake/")

# resolve_cradle_dependencies resolves the library dependencies that CRADLE
# projects typically depend on.
# Individual libraries can be enabled or disabled depending on the project.
# This sets various variables in the parent scope depending on what it finds.
# (All are prefixed by the specified output_prefix.)
# _INCLUDE_DIRS - The list of directories to add to the include path.
# _LIBRARIES - The list of libraries to link against.
function(resolve_cradle_dependencies output_prefix)

    set(zero_value_args INCLUDE_WX INCLUDE_OPENGL INCLUDE_SKIA
        INCLUDE_DEVIL INCLUDE_CURL INCLUDE_OPENSSL INCLUDE_DCMTK INCLUDE_GLEW
        INCLUDE_MSGPACK INCLUDE_POSTGRESQL)
    set(one_value_args )
    set(multi_value_args PROJECTS)
    cmake_parse_arguments(MY "${zero_value_args}" "${one_value_args}"
        "${multi_value_args}" ${ARGN})

    set(include_dirs )
    set(libraries )

    # Breakpad is currently only used on Windows.
    if (WIN32)
        list(APPEND include_dirs
            "$ENV{ASTROID_EXTERNAL_LIB_DIR}/breakpad/include")
        link_directories("$ENV{ASTROID_EXTERNAL_LIB_DIR}")
    endif()

    # Add the include paths for the generated header and the original headers
    # for this project. (Do the generated ones first cause they have the same
    # names as the orignals and they should take precedence.)
    list(APPEND include_dirs
        "${CMAKE_CURRENT_BINARY_DIR}/generated/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")

    # Do the same for other projects that this depends on.
    # Also add the libraries for other projects.
    foreach(p ${MY_PROJECTS})
        list(APPEND include_dirs
            "${CMAKE_BINARY_DIR}/${p}/generated/src"
            "${CMAKE_SOURCE_DIR}/${p}/src")
        list(APPEND libraries ${p})
    endforeach()

    # zlib
    find_package(ZLIB REQUIRED)
    list(APPEND include_dirs ${ZLIB_INCLUDE_DIR})
    list(APPEND libraries ${ZLIB_LIBRARIES})

    # Boost
    add_definitions(-DBOOST_ALL_NO_LIB)
    find_package(Boost
        COMPONENTS chrono thread filesystem system date_time regex
            program_options unit_test_framework REQUIRED)
    list(APPEND include_dirs ${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
    list(APPEND libraries ${Boost_LIBRARIES})

    # wxWidgets
    if(MY_INCLUDE_WX)
        find_package(wxWidgets REQUIRED core base gl)
        list(APPEND include_dirs ${wxWidgets_INCLUDE_DIRS})
        list(APPEND libraries ${wxWidgets_LIBRARIES})
    endif()

    # GLEW
    if(MY_INCLUDE_GLEW)
        find_package(GLEW REQUIRED)
        list(APPEND include_dirs ${GLEW_INCLUDE_DIR})
        list(APPEND libraries ${GLEW_LIBRARIES})
    endif()

    # GNU Linear Programming Kit
    find_package(GLPK REQUIRED)
    list(APPEND include_dirs ${GLPK_INCLUDE_DIR})
    list(APPEND libraries ${GLPK_LIBRARIES})

    # DevIL (aka OpenIL)
    if(MY_INCLUDE_DEVIL)
        find_package(DevIL REQUIRED)
        # IL_INCLUDE_DIR includes the "IL" part of the path, which we don't want.
        list(APPEND include_dirs "${IL_INCLUDE_DIR}/..")
        list(APPEND libraries ${IL_LIBRARIES} ${ILU_LIBRARIES})
    endif()

    # Skia
    if(MY_INCLUDE_SKIA)
        find_package(Skia REQUIRED)
        list(APPEND include_dirs ${SKIA_INCLUDE_DIRS})
        list(APPEND libraries ${SKIA_LIBRARIES})
    endif()

    # OpenGL
    if(MY_INCLUDE_OPENGL)
        find_package(OpenGL REQUIRED)
        list(APPEND include_dirs ${OPENGL_INCLUDE_DIR})
        list(APPEND libraries ${OPENGL_LIBRARIES})
        # OSMesa
        if(UNIX)
            find_package(OSMesa REQUIRED)
            list(APPEND include_dirs ${OSMESA_INCLUDE_DIR})
            list(APPEND libraries ${OSMESA_LIBRARY})
        endif()
    endif()

    # CURL
    if(MY_INCLUDE_CURL)
        find_package(CURL REQUIRED)
        list(APPEND include_dirs ${CURL_INCLUDE_DIR})
        list(APPEND libraries ${CURL_LIBRARIES})
    endif()

    # OpenSSL
    if(MY_INCLUDE_OPENSSL)
        find_package(OpenSSL REQUIRED)
        list(APPEND include_dirs ${OPENSSL_INCLUDE_DIR})
        list(APPEND libraries ${OPENSSL_LIBRARIES})
    endif()

    # DCMTK
    if(MY_INCLUDE_DCMTK)
        find_package(DCMTK REQUIRED)
        list(APPEND include_dirs ${DCMTK_INCLUDE_DIR})
        list(APPEND libraries ${DCMTK_LIBRARIES})
    endif()

    # MessagePack
    if(MY_INCLUDE_MSGPACK)
        find_package(MsgPack REQUIRED)
        list(APPEND include_dirs ${MSGPACK_INCLUDE_DIR})
        list(APPEND libraries ${MSGPACK_LIBRARIES})
    endif()

    # PostgreSQL
    if(MY_INCLUDE_POSTGRESQL)
        find_package(PostgreSQL REQUIRED)
        list(APPEND include_dirs ${POSTGRESQL_INCLUDE_DIR})
        list(APPEND libraries ${POSTGRESQL_LIBRARIES})
    endif()

    set("${output_prefix}_INCLUDE_DIRS" SYSTEM ${include_dirs} PARENT_SCOPE)
    set("${output_prefix}_LIBRARIES" ${libraries} PARENT_SCOPE)

endfunction()

# preprocess_cradle_header_files preprocesses a list of header files.
function(preprocess_cradle_header_files source_dir
    generated_cpp_files generated_header_files)

    set(zero_value_args )
    set(one_value_args ACCOUNT_ID TYPE_APP_ID FUNCTION_APP_ID NAMESPACE INDEX_FILE)
    set(multi_value_args INPUT_FILES)
    cmake_parse_arguments(MY "${zero_value_args}" "${one_value_args}"
        "${multi_value_args}" ${ARGN})

    set(generated_cpps)
    set(generated_headers)
    set(preprocessed_ids)

    foreach(preprocessed_file ${MY_INPUT_FILES})
        file(RELATIVE_PATH relative_path ${source_dir} ${preprocessed_file})
        get_filename_component(subdir ${relative_path} PATH)
        get_filename_component(file_name ${preprocessed_file} NAME)
        get_filename_component(file_name_we ${preprocessed_file} NAME_WE)
        set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${subdir}")
        file(MAKE_DIRECTORY ${build_dir})
        set(generated_cpp_file "${build_dir}/${file_name_we}.cpp")
        set(generated_header_file "${build_dir}/${file_name_we}.hpp")
        set(preprocessor "${CMAKE_BINARY_DIR}/cradle/preprocessor/preprocessor/preprocessor")
        if (WIN32)
            set(preprocessor "${preprocessor}.exe")
        endif()
        string(REGEX REPLACE "[:/\\\\\\.]" "_" file_id
            "${subdir}/${file_name_we}")
        add_custom_command(
            OUTPUT ${generated_cpp_file} ${generated_header_file}
            COMMAND ${preprocessor} ${preprocessed_file} ${generated_header_file} ${MY_ACCOUNT_ID} ${MY_TYPE_APP_ID} ${MY_FUNCTION_APP_ID} ${file_id} ${MY_NAMESPACE}
            DEPENDS preprocessor ${preprocessed_file}
            WORKING_DIRECTORY ${build_dir}
            COMMENT "preprocessing ${subdir}/${file_name}"
            VERBATIM)
        list(APPEND generated_headers ${generated_header_file})
        list(APPEND generated_cpps ${generated_cpp_file})
        list(APPEND preprocessed_ids ${file_id})
    endforeach()

    # Write the API index file.
    set(api_index
        "#include <cradle/api.hpp>\nnamespace ${MY_NAMESPACE} {\n")
    foreach(id ${preprocessed_ids})
        set(api_index
            "${api_index}void add_${id}_api(cradle::api_implementation& api)\;\n")
    endforeach()
    string(TOUPPER "${MY_NAMESPACE}" uppercase_namespace)
    set(api_index
        "${api_index}#define ${uppercase_namespace}_REGISTER_APIS(api)\\\n")
    foreach(id ${preprocessed_ids})
        set(api_index "${api_index}add_${id}_api(api)\;\\\n")
    endforeach()
    set(api_index "${api_index}\n}\n")
    file(WRITE "${MY_INDEX_FILE}" ${api_index})

    set(${generated_cpp_files} ${generated_cpps} PARENT_SCOPE)
    set(${generated_header_files} ${generated_headers} PARENT_SCOPE)

endfunction()

# Get the build type so we can set different flags accordingly.
string(TOLOWER "${CMAKE_BUILD_TYPE}" lowercase_build_type)

if(MSVC)
    # Disable annoying warnings/issues in Visual C++.
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_DEPRECATE)
    add_compile_options(/wd4180 /wd4355 /wd4996 /wd4503)
    # Turn on warnings as errors
    add_compile_options(/WX) #Turned off until all projects are done

    # Enable big object files
    add_compile_options(/bigobj)

    # Set some flags to make linking faster for the development build.
    if("${lowercase_build_type}" STREQUAL "release")
        add_compile_options(/Zc:inline)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Opt:NOREF /Opt:NOICF /INCREMENTAL /verbose:incr")
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    # Enable C++11 for GCC.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
	# Enable warnings as errors for GCC.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -fPIC -O0")
	# Enable code coverage options
	# Turned off until we can make these only run on the unit test projects (Sept 6,2017)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# add_cradle_target_install sets up a target to be installed to CRADLE_LIB_DIR.
macro(add_cradle_target_install target)
    install(TARGETS ${target}
        DESTINATION "${ASTROID_LIB_DIR}/lib/release"
        CONFIGURATIONS Release
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    install(TARGETS ${target}
        DESTINATION "${ASTROID_LIB_DIR}/lib/debug"
        CONFIGURATIONS Debug
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    install(TARGETS ${target}
        DESTINATION "${ASTROID_LIB_DIR}/lib/rwdi"
        CONFIGURATIONS RelWithDebInfo
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
endmacro()

# Set up the linker search path for other libraries.
if("${lowercase_build_type}" STREQUAL "debug")
    link_directories("$ENV{ASTROID_EXTERNAL_LIB_DIR}/lib/debug")
elseif("${lowercase_build_type}" STREQUAL "release")
    link_directories("$ENV{ASTROID_EXTERNAL_LIB_DIR}/lib/release")
elseif("${lowercase_build_type}" STREQUAL "relwithdebinfo")
    link_directories("$ENV{ASTROID_EXTERNAL_LIB_DIR}/lib/relwithdebinfo")
else()
    link_directories("$ENV{ASTROID_EXTERNAL_LIB_DIR}/lib/$(ConfigurationName)")
endif()
