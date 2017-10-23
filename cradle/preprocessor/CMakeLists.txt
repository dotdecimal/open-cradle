project(preprocessor)

include(CMakeParseArguments)

find_program(OCAML_EXECUTABLE ocamlc)
find_program(OCAML_YACC_EXECUTABLE ocamlyacc)
find_program(OCAML_LEX_EXECUTABLE ocamllex)

function(add_ocaml_source prefix object_file ocaml_file dependencies)
    cmake_parse_arguments(MY "SEPARATE_INTERFACE" "" "" ${ARGN})
    set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${prefix}")
    string(REGEX REPLACE "[.]cmo" "" without_extension ${object_file})
    if (MY_SEPARATE_INTERFACE)
        set(interface_file "")
    else()
        set(interface_file ${without_extension}.cmi)
    endif()
    set(interface_dependencies)
    foreach(dependency ${dependencies})
        list(APPEND interface_dependencies "${build_dir}/${dependency}.cmi")
    endforeach()
    add_custom_command(
        OUTPUT ${object_file} ${interface_file}
        COMMAND ${OCAML_EXECUTABLE} -c ${ocaml_file}
        DEPENDS ${ocaml_file} ${interface_dependencies}
        WORKING_DIRECTORY ${build_dir}
        COMMENT "compiling OCaml file ${ocaml_file}"
        VERBATIM)
endfunction()

function(add_regular_ocaml_source prefix ocaml_file)
    set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${prefix}")
    cmake_parse_arguments(MY "" "" "DEPENDENCIES" ${ARGN})
    get_filename_component(base_name ${ocaml_file} NAME_WE)
    set(copied_file "${build_dir}/${base_name}.ml")
    set(object_file "${build_dir}/${base_name}.cmo")
    add_custom_command(
        OUTPUT ${copied_file}
        COMMAND ${CMAKE_COMMAND} -E copy ${ocaml_file} ${build_dir}
        COMMAND ${CMAKE_COMMAND} -E touch ${copied_file}
        DEPENDS ${ocaml_file}
        COMMENT "copying OCaml file ${copied_file}"
        VERBATIM)
    add_ocaml_source(${prefix} ${object_file} ${copied_file}
        "${MY_DEPENDENCIES}")
endfunction()

function(add_ocaml_yacc_source prefix yacc_file)
    set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${prefix}")
    cmake_parse_arguments(MY "" "" "DEPENDENCIES" ${ARGN})
    get_filename_component(base_name ${yacc_file} NAME_WE)
    set(copied_file "${build_dir}/${base_name}.mly")
    set(ocaml_file "${build_dir}/${base_name}.ml")
    set(interface_file "${build_dir}/${base_name}.mli")
    add_custom_command(
        OUTPUT ${ocaml_file} ${interface_file}
        COMMAND ${CMAKE_COMMAND} -E copy ${yacc_file} ${build_dir}
        COMMAND ${OCAML_YACC_EXECUTABLE} ${copied_file}
        DEPENDS ${yacc_file}
        WORKING_DIRECTORY ${build_dir}
        COMMENT "compiling OCaml Yacc file ${yacc_file}"
        VERBATIM)
    set(object_file "${build_dir}/${base_name}.cmo")
    set(interface_object_file "${build_dir}/${base_name}.cmi")
    add_ocaml_source(${prefix} ${object_file} ${ocaml_file}
        "${MY_DEPENDENCIES}" SEPARATE_INTERFACE)
    add_ocaml_source(${prefix} ${interface_object_file} ${interface_file}
        "${MY_DEPENDENCIES}" SEPARATE_INTERFACE)
endfunction()

function(add_ocaml_lex_source prefix lex_file)
    set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${prefix}")
    cmake_parse_arguments(MY "" "" "DEPENDENCIES" ${ARGN})
    get_filename_component(base_name ${lex_file} NAME_WE)
    set(copied_file "${build_dir}/${base_name}.mll")
    set(ocaml_file "${build_dir}/${base_name}.ml")
    set(object_file "${build_dir}/${base_name}.cmo")
    add_custom_command(
        OUTPUT ${ocaml_file}
        COMMAND ${CMAKE_COMMAND} -E copy ${lex_file} ${build_dir}
        COMMAND ${OCAML_LEX_EXECUTABLE} ${copied_file}
        DEPENDS ${lex_file}
        WORKING_DIRECTORY ${build_dir}
        COMMENT "compiling OCaml Lex file ${lex_file}"
        VERBATIM)
    add_ocaml_source(${prefix} ${object_file} ${ocaml_file}
        "${MY_DEPENDENCIES}")
endfunction()

function(add_ocaml_executable prefix executable)
    set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${prefix}")
    cmake_parse_arguments(MY "" "" "OBJECTS;LIBRARIES" ${ARGN})
    set(object_files)
    foreach(object ${MY_OBJECTS})
        list(APPEND object_files "${build_dir}/${object}.cmo")
    endforeach()
    set(library_files)
    foreach(library ${MY_LIBRARIES})
        list(APPEND library_files "${library}.cma")
    endforeach()
    if (WIN32)
        set(executable_file "${executable}.exe")
    else()
        set(executable_file ${executable})
    endif()
    add_custom_command(
        OUTPUT "${build_dir}/${executable_file}"
        COMMAND ${OCAML_EXECUTABLE} -o ${executable_file} ${library_files} ${object_files}
        DEPENDS ${object_files}
        WORKING_DIRECTORY ${build_dir}
        COMMENT "linking OCaml executable ${executable}"
        VERBATIM)
    add_custom_target(
        ${prefix} ALL
        DEPENDS "${build_dir}/${executable_file}")
endfunction()

# preprocessor
set(preprocessor_dir "${CMAKE_CURRENT_SOURCE_DIR}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/preprocessor")
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/types.ml")
add_ocaml_lex_source(preprocessor "${preprocessor_dir}/lexer.mll"
    DEPENDENCIES types grammar)
add_ocaml_yacc_source(preprocessor "${preprocessor_dir}/grammar.mly"
    DEPENDENCIES types)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/utilities.ml"
    DEPENDENCIES types)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/functions.ml"
    DEPENDENCIES types utilities)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/records.ml"
    DEPENDENCIES types utilities)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/structures.ml"
    DEPENDENCIES types utilities functions)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/enums.ml"
    DEPENDENCIES types utilities functions)

add_regular_ocaml_source(preprocessor "${preprocessor_dir}/dependency.ml"
    DEPENDENCIES types utilities)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/provider.ml"
    DEPENDENCIES types utilities)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/mutations.ml"
    DEPENDENCIES types utilities)

add_regular_ocaml_source(preprocessor "${preprocessor_dir}/unions.ml"
    DEPENDENCIES types utilities enums functions)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/blocks.ml"
    DEPENDENCIES types utilities grammar lexer functions mutations)
add_regular_ocaml_source(preprocessor "${preprocessor_dir}/main.ml"
    DEPENDENCIES types utilities functions structures unions enums dependency provider mutations records blocks)
add_ocaml_executable(preprocessor preprocessor
    OBJECTS types utilities lexer grammar functions structures enums unions dependency provider mutations records
        blocks main
    LIBRARIES str)
